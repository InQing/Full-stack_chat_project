#include "http_client.h"
#include <brpc/channel.h>
#include <brpc/controller.h>
#include <butil/logging.h>

namespace status_server {

HttpClient::HttpClient(const std::string& server_addr)
    : _server_addr(server_addr) {
}

ServerNode HttpClient::get_optimal_server(const std::string& server_type) {
    // 创建Channel
    brpc::Channel channel;
    brpc::ChannelOptions options;
    options.protocol = "http";
    options.timeout_ms = 1000;
    options.max_retry = 3;

    if (channel.Init(_server_addr.c_str(), "", &options) != 0) {
        LOG(ERROR) << "Failed to initialize channel to " << _server_addr;
        return ServerNode();
    }

    // 创建HTTP请求
    brpc::Controller cntl;
    cntl.http_request().uri() = "/get_optimal_server?type=" + server_type;

    channel.CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
    if (cntl.Failed()) {
        LOG(ERROR) << "Request failed: " << cntl.ErrorText();
        return ServerNode();
    }

    // 检查是否被重定向到leader
    if (cntl.response_attachment().to_string().find("Not leader") != std::string::npos) {
        // 从错误消息中提取leader地址
        std::string response = cntl.response_attachment().to_string();
        size_t pos = response.find("redirect to ");
        if (pos != std::string::npos) {
            std::string leader_addr = response.substr(pos + 12);
            // 使用leader地址重新创建客户端并重试
            HttpClient leader_client(leader_addr);
            return leader_client.get_optimal_server(server_type);
        }
    }

    // 解析响应
    const std::string& response = cntl.response_attachment().to_string();
    if (response.empty()) {
        LOG(ERROR) << "Empty response";
        return ServerNode();
    }

    // 解析JSON响应
    ServerNode node;
    // TODO: 实现JSON解析逻辑
    // 这里需要根据实际的JSON格式来解析response
    // 可以使用rapidjson或其他JSON库

    return node;
}

} // namespace status_server