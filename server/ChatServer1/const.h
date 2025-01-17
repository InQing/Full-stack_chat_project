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
#include <mutex>

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
    ERR_UNKOWN = 9 ,// 未知错误
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

enum MSG_IDS {
    ID_GET_VARIFY_CODE = 1001, // 获取验证码
    ID_REG_USER = 1002, // 注册用户
    ID_RESET_PWD = 1003, // 重置密码
    ID_LOGIN_USER = 1004, // 登录
    ID_CHAT_LOGIN = 1005, // 登录聊天服务器
    ID_CHAT_LOGIN_RSP = 1006, //用户登陆回包
    ID_SEARCH_USER_REQ = 1007, //用户搜索请求
    ID_SEARCH_USER_RSP = 1008, //搜索用户回包
    ID_ADD_FRIEND_REQ = 1009, //申请添加好友请求
    ID_ADD_FRIEND_RSP = 1010, //申请添加好友回复
    ID_NOTIFY_ADD_FRIEND_REQ = 1011,  //通知用户添加好友申请
    ID_AUTH_FRIEND_REQ = 1013,  //认证好友请求
    ID_AUTH_FRIEND_RSP = 1014,  //认证好友回复
    ID_NOTIFY_AUTH_FRIEND_REQ = 1015, //通知用户认证好友申请
    ID_TEXT_CHAT_MSG_REQ = 1017, //文本聊天信息请求
    ID_TEXT_CHAT_MSG_RSP = 1018, //文本聊天信息回复
    ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019, //通知用户文本聊天信息
};

#define USERIPPREFIX  "uip_"
#define USERTOKENPREFIX  "utoken_"
#define IPCOUNTPREFIX  "ipcount_"
#define USER_BASE_INFO "ubaseinfo_"
#define LOGIN_COUNT  "logincount"
#define NAME_INFO  "nameinfo_"
