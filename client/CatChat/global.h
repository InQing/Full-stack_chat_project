#ifndef GLOBAL_H
#define GLOBAL_H

#include <QWidget>
#include <qstyle.h>
#include <QRegularExpression>
#include <QString>
#include <QDir>
#include <QSettings>
#include <QDebug>

// 网关服务器的url前缀
extern QString gate_url_prefix;
// 资源服务器的url前缀
extern QString resource_url_prefix;

// 刷新qss
extern std::function<void(QWidget*)> repolish;

// 密码加密函数
extern std::function<QString(QString)> xorString;

// 功能模块
enum Modules{
    MOD_REGISTER = 0, // 注册
    MOD_RESETMOD = 1, // 重置密码
    MOD_LOGIN = 2, // 登录
};

// 请求类型
enum ReqId{
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
    ID_FILE_CHAT_MSG_REQ = 1020, //文件聊天信息请求
    ID_FILE_CHAT_MSG_RSP = 1021, //文件聊天信息回复
    ID_NOTIFY_FILE_CHAT_MSG_REQ = 1022, //通知用户文件聊天信息
};

// 错误码
enum ErrorCodes
{
    SUCCESS = 0,              // 成功
    ERR_JSON = 1,             // Json解析失败
    ERR_NETWORK = 2,          // 其它网络错误
    ERR_VARIFY_REPEAT = 3,    // 重复请求验证码
    ERR_VARIFY_EXPIRED = 4,   // 验证码过期
    ERR_VARIFY_CODE_ERR = 5,  // 验证码错误
    ERR_RPC = 6,              // RPC错误
    ERR_REDIS = 7,            // Redis错误
    ERR_USER_EXIT = 8,        // 用户名已存在
    ERR_UNKOWN = 9,           // 未知错误
    ERR_EMAIL_NOT_MATCH = 10, // 用户名与邮箱不匹配
    ERR_MYSQL = 11,           // MySql错误
    ERR_PWD_INVALID = 12,     // 密码错误
    ERR_UID_INVALID = 13,     // uid错误
    ERR_TOKEN_INVALID = 14,   // token错误
    ERR_SERVER = 15,          // 服务器错误
    ERR_UPLOAD_INIT = 16,     // 上传初始化错误
    ERR_UPLOAD_CHUNK = 17,     // 上传分片错误
};

enum TipErr{
    TIP_SUCCESS = 0,
    TIP_EMAIL_ERR = 1,
    TIP_PWD_ERR = 2,
    TIP_CONFIRM_ERR = 3,
    TIP_PWD_CONFIRM = 4,
    TIP_VARIFY_ERR = 5,
    TIP_USER_ERR = 6
};

enum ClickLbState{
    Normal = 0,
    Selected = 1
};

struct ServerInfo{
    QString Host;
    QString Port;
    QString Token;
    int Uid;
};

enum class ChatRole
{
    Self,
    Other
};

struct MsgInfo{
    QString msgFlag;//"text,image,file"
    QString content;//表示文件和图像的url,文本信息
    QPixmap pixmap;//文件和图片的缩略图
};

//聊天界面几种模式
enum ChatUIMode{
    SearchMode, //搜索模式
    ChatMode, //聊天模式
    ContactMode, //联系模式
};

//自定义QListWidgetItem的几种类型
enum ListItemType{
    CHAT_USER_ITEM, //聊天用户
    CONTACT_USER_ITEM, //联系人用户
    SEARCH_USER_ITEM, //搜索到的用户
    ADD_USER_TIP_ITEM, //提示添加用户
    INVALID_ITEM,  //不可点击条目
    GROUP_TIP_ITEM, //分组提示条目
    LINE_ITEM,  //分割线
    APPLY_FRIEND_ITEM, //好友申请
};

//申请好友标签输入框最低长度
const int MIN_APPLY_LABEL_ED_LEN = 40;

const QString add_prefix = "添加标签 ";

const int  tip_offset = 5;


const std::vector<QString>  strs ={"hello world !",
                                   "nice to meet u",
                                   "New year，new life",
                                   "You have to love yourself",
                                   "My love is written in the wind ever since the whole world is you"};

const std::vector<QString> heads = {
    ":/res/head_1.jpg",
    ":/res/head_2.jpg",
    ":/res/head_3.jpg",
    ":/res/head_4.jpg",
    ":/res/head_5.jpg"
};

const std::vector<QString> names = {
    "HanMeiMei",
    "Lily",
    "Ben",
    "Androw",
    "Max",
    "Summer",
    "Candy",
    "Hunter"
};

const int CHAT_COUNT_PER_PAGE = 13;

#endif // GLOBAL_H
