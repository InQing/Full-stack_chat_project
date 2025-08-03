#ifndef STATUS_SERVER_H
#define STATUS_SERVER_H

#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <braft/raft.h>
#include <braft/util.h>
#include <braft/storage.h>
#include <brpc/server.h>

namespace status_server {

// 服务器健康状态信息
struct ServerHealth {
    double cpu_usage;      // CPU使用率
    double memory_usage;   // 内存使用率
    int connection_count;  // 连接数
    int64_t last_heartbeat; // 最后一次心跳时间
};

// 服务器节点信息
struct ServerNode {
    std::string ip;
    int port;
    std::string server_type;  // 服务器类型（如ChatServer、ResourceServer）
    ServerHealth health;
};

// 状态服务器类
class StatusServer {
public:
    StatusServer() = default;
    ~StatusServer() = default;

    // 初始化服务器
    bool init(const std::string& ip, int port, const std::string& conf);

    // 启动服务器
    bool start();

    // 停止服务器
    void stop();

    // 处理服务器心跳
    bool handle_heartbeat(const std::string& server_type, const std::string& server_ip,
                         int server_port, const ServerHealth& health);

    // 获取最优服务器节点
    ServerNode get_optimal_server(const std::string& server_type);

private:
    std::string _ip;
    int _port;
    std::string _conf;  // Raft配置
    brpc::Server _server;
    std::map<std::string, std::vector<ServerNode>> _server_nodes;  // 服务器节点列表
    std::mutex _mutex;
};

} // namespace status_server

#endif // STATUS_SERVER_H