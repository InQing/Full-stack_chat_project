#include "status_node.h"
#include <braft/util.h>
#include <brpc/controller.h>
#include <butil/logging.h>

namespace status_server
{

    StatusNode::StatusNode(StatusServer *status_server)
        : _status_server(status_server)
    {
    }

    bool StatusNode::init(const std::string &group_id, const std::string &conf)
    {
        _group_id = group_id;

        // 解析配置
        braft::NodeOptions node_options;
        if (node_options.initial_conf.parse_from(conf) != 0)
        {
            LOG(ERROR) << "Failed to parse configuration " << conf;
            return false;
        }

        // 设置选项
        node_options.election_timeout_ms = 5000;
        node_options.fsm = this;
        node_options.node_owns_fsm = false;
        node_options.snapshot_interval_s = 30;
        std::string prefix = "local://./status_server_" + group_id;
        node_options.log_uri = prefix + "/log";
        node_options.raft_meta_uri = prefix + "/raft_meta";
        node_options.snapshot_uri = prefix + "/snapshot";

        // 创建Node实例
        braft::Node *node = new braft::Node(_group_id, node_options);
        if (node == nullptr)
        {
            LOG(ERROR) << "Failed to create raft node";
            return false;
        }
        _node = node;

        // 启动节点
        if (node->init(node_options) != 0)
        {
            LOG(ERROR) << "Failed to init raft node";
            delete node;
            return false;
        }

        return true;
    }

    void StatusNode::shutdown()
    {
        if (_node)
        {
            _node->shutdown(nullptr);
        }
    }

    void StatusNode::handle_client_write_request(
        const std::string &server_type,
        const std::string &server_ip,
        int server_port,
        const ServerHealth &health,
        braft::Closure *done)
    {

        // 如果不是leader，转发到leader
        if (!_node->is_leader())
        {
            braft::PeerId leader = _node->leader_id();
            if (!leader.is_empty())
            {
                done->status().set_error(EPERM, "Not leader, redirect to " + leader.to_string());
            }
            else
            {
                done->status().set_error(EPERM, "Not leader, and leader is unknown");
            }
            done->Run();
            return;
        }

        // 构造请求数据
        std::string request_data = server_type + "\n" +
                                   server_ip + "\n" +
                                   std::to_string(server_port) + "\n" +
                                   std::to_string(health.cpu_usage) + "\n" +
                                   std::to_string(health.memory_usage) + "\n" +
                                   std::to_string(health.connection_count);

        // 提交到Raft组
        braft::Task task;
        task.data = &request_data;
        task.done = done;
        _node->apply(task);
    }

    void StatusNode::on_apply(braft::Iterator &iter)
    {
        // 遍历所有请求
        for (; iter.valid(); iter.next())
        {
            // 解析请求数据
            std::string request_data = iter.data().to_string();
            std::istringstream iss(request_data);
            std::string server_type, server_ip;
            int server_port;
            ServerHealth health;

            std::getline(iss, server_type);
            std::getline(iss, server_ip);
            iss >> server_port;
            iss >> health.cpu_usage;
            iss >> health.memory_usage;
            iss >> health.connection_count;

            // 更新服务器状态
            _status_server->handle_heartbeat(server_type, server_ip, server_port, health);

            // 如果有done，处理它
            if (iter.done())
            {
                iter.done()->status().set_error(0, "Success");
                iter.done()->Run();
            }
        }
    }

    void StatusNode::on_snapshot_save(braft::SnapshotWriter *writer, braft::Closure *done)
    {
        // 实现快照保存逻辑
        done->status().set_error(0, "Success");
        done->Run();
    }

    int StatusNode::on_snapshot_load(braft::SnapshotReader *reader)
    {
        // 实现快照加载逻辑
        return 0;
    }

    void StatusNode::on_leader_start(int64_t term)
    {
        LOG(INFO) << "Node becomes leader, term: " << term;
    }

    void StatusNode::on_leader_stop(const butil::Status &status)
    {
        LOG(INFO) << "Node steps down: " << status.error_str();
    }

    void StatusNode::on_error(const ::braft::Error &e)
    {
        LOG(ERROR) << "Raft error: " << e.status().error_str();
    }

    void StatusNode::on_configuration_committed(const ::braft::Configuration &conf)
    {
        LOG(INFO) << "Configuration change committed: " << conf.to_string();
    }

    void StatusNode::on_stop_following(const ::braft::LeaderChangeContext &ctx)
    {
        LOG(INFO) << "Node stops following " << ctx.leader_id();
    }

    void StatusNode::on_start_following(const ::braft::LeaderChangeContext &ctx)
    {
        LOG(INFO) << "Node starts following " << ctx.leader_id();
    }

} // namespace status_server