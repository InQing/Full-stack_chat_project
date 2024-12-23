let code_prefix = "code_";

const Errors = {
    SUCCESS : 0, // 成功
    ERR_JSON : 1, // Json解析失败
    ERR_NETWORK : 2,  // 其它网络错误
    ERR_VARIFY_REPEAT : 3, // 重复请求验证码
    ERR_VARIFY_EXPIRED : 4, // 验证码过期
    ERR_VARIFY_CODE_ERR : 5, // 验证码错误
    ERR_RPC : 6, // RPC错误
    ERR_REDIS : 7, // Redis错误
    ERR_UNKOWN : 8 // 未知错误
};


module.exports = {code_prefix,Errors}