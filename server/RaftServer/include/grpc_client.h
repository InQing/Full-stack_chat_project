#ifndef GRPC_CLIENT_H
#define GRPC_CLIENT_H

#include <string>
#include <memory>
#include <grpcpp/grpcpp.h>
#include "status_server.h"
#include "proto/status_service.grpc.pb.h"

namespace status_server {

class GrpcClient {
public:
    GrpcClient(const std::string& server_addr, const std::string& server_type,
               const std::string& server_ip, int server_port);
    ~GrpcClient() = default;

    // 发送心跳信息
    bool send_heartbeat(const ServerHealth& health);

    // 启动心跳服务
    bool start_heartbeat_service(int interval_ms = 5000);

    // 停止心跳服务
    void stop_heartbeat_service();

private:
    std::string _server_addr;  // 状态服务器地址
    std::string _server_type;  // 服务器类型
    std::string _server_ip;    // 服务器IP
    int _server_port;          // 服务器端口
    bool _is_running;          // 心跳服务运行状态
    std::shared_ptr<grpc::Channel> _channel;  // gRPC通道
    std::unique_ptr<StatusService::Stub> _stub;  // gRPC存根
};

} // namespace status_server

#endif // GRPC_CLIENT_H