#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <string>
#include "status_server.h"

namespace status_server {

class HttpClient {
public:
    HttpClient(const std::string& server_addr);
    ~HttpClient() = default;

    // 获取指定类型的最优服务器节点
    ServerNode get_optimal_server(const std::string& server_type);

private:
    std::string _server_addr;  // 状态服务器地址
};

} // namespace status_server

#endif // HTTP_CLIENT_H