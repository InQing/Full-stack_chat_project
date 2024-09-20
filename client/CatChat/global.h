#ifndef GLOBAL_H
#define GLOBAL_H

#include <QWidget>
#include <qstyle.h>
#include <QRegularExpression>
#include <memory>

// 刷新qss
extern std::function<void(QWidget*)> repolish;

// HTTP请求类型
enum ReqId{
    ID_GET_VARIFY_CODE = 1001, // 获取验证码
    ID_REG_USER = 1002, // 注册用户
};

// 错误码
enum ErrorCodes{
    SUCCESS = 0, // 成功
    ERR_JSON = 1, // Json解析失败
    ERR_NETWORK = 2,  // 其它网络错误
};

// 功能模块
enum Modules{
    MOD_REGISTER = 0, // 注册
};

#endif // GLOBAL_H
