#include "status_server.h"
#include <brpc/channel.h>
#include <butil/time.h>
#include <gflags/gflags.h>
#include <algorithm>
#include <grpcpp/grpcpp.h>
#include "proto/status_service.grpc.pb.h"

namespace status_server
{

    // 初始化服务器
    class StatusServiceImpl final : public StatusService::Service
    {
    public:
        StatusServiceImpl(StatusServer *server) : _server(server) {}

        grpc::Status SendHeartbeat(grpc::ServerContext *context, const HealthStatus *request,
                                   HeartbeatResponse *response) override
        {
            // 检查是否是leader
            if (!_server->is_leader())
            {
                response->set_success(false);
                response->set_message("Not leader");
                std::string leader_addr = _server->get_leader_address();
                if (!leader_addr.empty())
                {
                    response->set_leader_addr(leader_addr);
                }
                return grpc::Status::OK;
            }

            // 构造健康信息
            ServerHealth health;
            health.cpu_usage = request->cpu_usage();
            health.memory_usage = request->memory_usage();
            health.connection_count = request->connection_count();

            // 通过Raft协议处理心跳
            braft::Closure *done = new braft::Closure;
            _server->get_node()->handle_client_write_request(
                request->server_type(),
                request->server_ip(),
                request->server_port(),
                health,
                done);

            response->set_success(true);
            response->set_message("Success");
            return grpc::Status::OK;
        }

    private:
        StatusServer *_server;
    };

    bool StatusServer::init(const std::string &ip, int port, const std::string &conf)
    {
        _ip = ip;
        _port = port;
        _conf = conf;

        // 初始化brpc服务器
        brpc::ServerOptions brpc_options;
        brpc_options.idle_timeout_sec = -1;
        if (_server.Start(port, &brpc_options) != 0)
        {
            LOG(ERROR) << "Failed to start brpc server on port " << port;
            return false;
        }

        // 初始化gRPC服务器
        std::string server_address = ip + ":" + std::to_string(port + 1);
        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

        // 注册服务
        _grpc_service_impl = std::make_unique<StatusServiceImpl>(this);
        builder.RegisterService(_grpc_service_impl.get());

        // 启动gRPC服务器
        _grpc_server = builder.BuildAndStart();
        if (!_grpc_server)
        {
            LOG(ERROR) << "Failed to start grpc server on " << server_address;
            return false;
        }

        return true;
    }

    // 启动服务器
    bool StatusServer::start()
    {
        return true;
    }

    // 停止服务器
    void StatusServer::stop()
    {
        _server.Stop(0);
        _server.Join();
    }

    // 处理服务器心跳
    bool StatusServer::handle_heartbeat(const std::string &server_type,
                                        const std::string &server_ip,
                                        int server_port,
                                        const ServerHealth &health)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        // 更新服务器节点信息
        auto &nodes = _server_nodes[server_type];
        auto it = std::find_if(nodes.begin(), nodes.end(),
                               [&](const ServerNode &node)
                               {
                                   return node.ip == server_ip && node.port == server_port;
                               });

        if (it != nodes.end())
        {
            // 更新已存在节点的健康信息
            it->health = health;
            it->health.last_heartbeat = butil::gettimeofday_ms();
        }
        else
        {
            // 添加新节点
            ServerNode node;
            node.ip = server_ip;
            node.port = server_port;
            node.server_type = server_type;
            node.health = health;
            node.health.last_heartbeat = butil::gettimeofday_ms();
            nodes.push_back(node);
        }

        return true;
    }

    // 获取最优服务器节点
    ServerNode StatusServer::get_optimal_server(const std::string &server_type)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        auto it = _server_nodes.find(server_type);
        if (it == _server_nodes.end() || it->second.empty())
        {
            return ServerNode(); // 返回空节点
        }

        // 移除超时的节点（30秒无心跳）
        auto &nodes = it->second;
        int64_t now = butil::gettimeofday_ms();
        nodes.erase(
            std::remove_if(nodes.begin(), nodes.end(),
                           [now](const ServerNode &node)
                           {
                               return now - node.health.last_heartbeat > 30000;
                           }),
            nodes.end());

        if (nodes.empty())
        {
            return ServerNode();
        }

        // 使用加权评分选择最优节点
        auto best_node = std::min_element(nodes.begin(), nodes.end(),
                                          [](const ServerNode &a, const ServerNode &b)
                                          {
                                              // 计算综合得分（越低越好）
                                              double score_a = a.health.cpu_usage * 0.4 +
                                                               a.health.memory_usage * 0.3 +
                                                               (a.health.connection_count / 100.0) * 0.3;
                                              double score_b = b.health.cpu_usage * 0.4 +
                                                               b.health.memory_usage * 0.3 +
                                                               (b.health.connection_count / 100.0) * 0.3;
                                              return score_a < score_b;
                                          });

        return *best_node;
    }

} // namespace status_server