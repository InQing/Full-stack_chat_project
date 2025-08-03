#include "grpc_client.h"
#include <grpcpp/grpcpp.h>
#include <butil/logging.h>
#include <thread>
#include <chrono>
#include "proto/status_service.grpc.pb.h"

namespace status_server {

GrpcClient::GrpcClient(const std::string& server_addr, const std::string& server_type,
                       const std::string& server_ip, int server_port)
    : _server_addr(server_addr)
    , _server_type(server_type)
    , _server_ip(server_ip)
    , _server_port(server_port)
    , _is_running(false) {

    // 初始化gRPC通道
    grpc::ChannelArguments args;
    args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 1000);
    args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 3000);
    _channel = grpc::CreateCustomChannel(_server_addr, grpc::InsecureChannelCredentials(), args);
    _stub = StatusService::NewStub(_channel);
    if (!_channel) {
        LOG(ERROR) << "Failed to initialize channel to " << _server_addr;
    }
}

bool GrpcClient::send_heartbeat(const ServerHealth& health) {
    if (!_channel) {
        LOG(ERROR) << "Channel is not initialized";
        return false;
    }

    // 创建请求
    HealthStatus request;
    request.set_server_type(_server_type);
    request.set_server_ip(_server_ip);
    request.set_server_port(_server_port);
    request.set_cpu_usage(health.cpu_usage);
    request.set_memory_usage(health.memory_usage);
    request.set_connection_count(health.connection_count);

    // 发送请求
    grpc::ClientContext context;
    HeartbeatResponse response;
    grpc::Status status = _stub->SendHeartbeat(&context, request, &response);

    if (!status.ok()) {
        // 检查是否需要重定向到leader
        if (response.has_leader_addr()) {
            // 更新通道地址并重试
            _server_addr = response.leader_addr();
            grpc::ChannelArguments args;
            args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 1000);
            args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 3000);
            _channel = grpc::CreateCustomChannel(_server_addr, grpc::InsecureChannelCredentials(), args);
            _stub = StatusService::NewStub(_channel);
            return send_heartbeat(health);  // 递归重试
        }
        LOG(ERROR) << "Failed to send heartbeat: " << status.error_message();
        return false;
    }

    return true;
}

bool GrpcClient::start_heartbeat_service(int interval_ms) {
    if (_is_running) {
        return true;  // 已经在运行
    }

    _is_running = true;
    std::thread([this, interval_ms]() {
        while (_is_running) {
            // 收集当前服务器的健康状态
            ServerHealth health;
            // TODO: 实现收集系统状态的逻辑
            // 可以使用系统API获取CPU、内存使用率等信息

            // 发送心跳
            if (!send_heartbeat(health)) {
                LOG(WARNING) << "Failed to send heartbeat";
            }

            // 等待下一次心跳
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
        }
    }).detach();

    return true;
}

void GrpcClient::stop_heartbeat_service() {
    _is_running = false;
}

} // namespace status_server