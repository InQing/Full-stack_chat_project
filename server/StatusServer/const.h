#pragma once

// Boost.Asio处理网络编程中的异步I/O操作，支持TCP、UDP、定时器
// Beast处理HTTP

#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include <unordered_map>
#include <string>
#include <queue>
#include <atomic>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

enum ErrorCodes {
    SUCCESS = 0, // 成功
    ERR_JSON = 1, // Json解析失败
    ERR_NETWORK = 2,  // 其它网络错误
    ERR_VARIFY_REPEAT = 3, // 重复请求验证码
    ERR_VARIFY_EXPIRED = 4, // 验证码过期
    ERR_VARIFY_CODE_ERR = 5, // 验证码错误
    ERR_RPC = 6, // RPC错误
    ERR_REDIS = 7, // Redis错误
    ERR_USER_EXIT = 8, // 用户名已存在
    ERR_UNKOWN = 9,// 未知错误
    ERR_EMAIL_NOT_MATCH = 10, // 用户名与邮箱不匹配
    ERR_MYSQL = 11, // MySql错误
    ERR_PWD_INVALID = 12, // 密码错误
    ERR_UID_INVALID = 13, // uid错误
    ERR_TOKEN_INVALID = 14, // token错误
};

// Defer类，在生命周期结束时执行func操作
class Defer {
public:
    Defer(std::function<void()> func) : func_(func) {};

    ~Defer() {
        func_();
    }
private:
    std::function<void()> func_;
};

// 用户信息
struct UserInfo {
    std::string name;
    std::string pwd;
    int uid;
    std::string email;
};

#define CODE_PREFIX "code_"
