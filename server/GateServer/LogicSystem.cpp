#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VarifyGrpcClient.h"
#include "const.h"
#include "RedisMgr.h"
#include "Logger.h"
#include "MysqlMgr.h"
#include "StatusGrpcClient.h"

bool LogicSystem::HandleGet(std::string url, std::shared_ptr<HttpConnection> connection)
{
	if (_get_handlers.find(url) == _get_handlers.end()) {
		return false;
	}

	_get_handlers[url](connection);
	return true;
}

void LogicSystem::RegGet(std::string url, HttpHandler handler)
{
	_get_handlers.insert({ url, handler });
}

bool LogicSystem::HandlePost(std::string url, std::shared_ptr<HttpConnection> connection)
{
	if (_post_handlers.find(url) == _post_handlers.end()) {
		return false;
	}

	_post_handlers[url](connection);
	return true;
}

void LogicSystem::RegPost(std::string url, HttpHandler handler)
{
	_post_handlers.insert({ url, handler });
}

LogicSystem::LogicSystem() {
	// 测试
	RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
		beast::ostream(connection->_response.body()) << "receive get_test request" << std::endl;
		int i = 0;
		for (auto elem : connection->_get_params) {
			i++;
			beast::ostream(connection->_response.body()) << "param" << i << ":" << "key is "
				<< elem.first << ", value is " << elem.second << std::endl;
		}
	});

	// 获取验证码
	RegPost("/get_Varifycode", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = beast::buffers_to_string(connection->_request.body().data());
		LOGI("LogicSystem: receive get_Varifycode post, body is %s", body_str.c_str());
		connection->_response.set(http::field::content_type, "text/json");
		// json解析
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			LOGW("LogicSystem: Failed to parse JSON data!");
			root["error"] = ErrorCodes::ERR_JSON;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		LOGI("LogicSystem: User's email is %s", email.c_str());

		// grpc发送验证服务
		GetVarifyRsp rsp =  VarifyGrpcClient::GetInstance()->GetVarifyCode(email);

		root["error"] = rsp.error();
		root["email"] = src_root["email"];
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
	});

	// 注册用户
	RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = beast::buffers_to_string(connection->_request.body().data());
		LOGI("LogicSystem: receive user_register post, body is %s", body_str.c_str());
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			LOGW("LogicSystem: Failed to parse JSON data!");
			root["error"] = ErrorCodes::ERR_JSON;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		// 检查验证码是否过期（或是否存在）
		std::string  varify_code;
		bool is_get_varify = RedisMgr::GetInstance()->Get(CODE_PREFIX + src_root["email"].asString(), varify_code);
		if (!is_get_varify) {
			LOGW("LogicSystem: Varify code expired");
			root["error"] = ErrorCodes::ERR_VARIFY_EXPIRED;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		// 检查验证码是否正确
		if (varify_code != src_root["varifycode"].asString()) {
			LOGW("LogicSystem: Varify code error");
			root["error"] = ErrorCodes::ERR_VARIFY_CODE_ERR;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		// 获取账户信息
		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();
		auto confirm = src_root["confirm"].asString();
	
		//查找数据库判断用户是否存在
		int uid = MysqlMgr::GetInstance()->RegUser(name, email, pwd);
		if (uid == -1) {
			LOGW("LogicSystem: user or email exist!");
			root["error"] = ErrorCodes::ERR_USER_EXIT;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		root["error"] = ErrorCodes::SUCCESS;
		root["uid"] = uid;
		root["email"] = email;
		root["user"] = name;
		root["passwd"] = pwd;
		root["confirm"] = confirm;
		root["varifycode"] = src_root["varifycode"].asString();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
	});

	// 重置密码
	RegPost("/reset_pwd", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		LOGI("LogicSystem: receive reset_pwd post, body is %s", body_str.c_str());
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			LOGE("LogicSystem: Failed to parse JSON data!");
			root["error"] = ErrorCodes::ERR_JSON;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();

		//先查找redis中email对应的验证码是否合理
		std::string varify_code;
		bool b_get_varify = RedisMgr::GetInstance()->Get(CODE_PREFIX + src_root["email"].asString(), varify_code);
		if (!b_get_varify) {
			LOGE("LogicSystem: varify code expired!");
			root["error"] = ErrorCodes::ERR_VARIFY_EXPIRED;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (varify_code != src_root["varifycode"].asString()) {
			LOGE("LogicSystem: varify code error!");
			root["error"] = ErrorCodes::ERR_VARIFY_CODE_ERR;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//查询数据库判断用户名和邮箱是否匹配
		bool email_valid = MysqlMgr::GetInstance()->CheckEmail(name, email);
		if (!email_valid) {
			LOGE("LogicSystem: user and email not match!");
			root["error"] = ErrorCodes::ERR_EMAIL_NOT_MATCH;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//更新密码为最新密码
		bool b_up = MysqlMgr::GetInstance()->UpdatePwd(name, pwd);
		if (!b_up) {
			LOGE("LogicSystem: update pwd failed!");
			root["error"] = ErrorCodes::ERR_MYSQL;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		LOGI("LogicSystem: succeed to update password!");
		root["error"] = 0;
		root["email"] = email;
		root["user"] = name;
		root["passwd"] = pwd;
		root["varifycode"] = src_root["varifycode"].asString();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	// 登录用户
	RegPost("/user_login", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		LOGI("LogicSystem: receive user_login post, body is %s", body_str.c_str());
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			LOGE("LogicSystem: Failed to parse JSON data!");
			root["error"] = ErrorCodes::ERR_JSON;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		auto pwd = src_root["passwd"].asString();
		UserInfo userInfo;
		//查询数据库判断邮箱和密码是否匹配
		bool pwd_valid = MysqlMgr::GetInstance()->CheckPwd(email, pwd, userInfo);
		if (!pwd_valid) {
			LOGE("LogicSystem: user pwd not match!");
			root["error"] = ErrorCodes::ERR_PWD_INVALID;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//查询StatusServer找到合适的连接
		auto reply = StatusGrpcClient::GetInstance()->GetChatServer(userInfo.uid);
		if (reply.error()) {
			LOGE("LogicSystem: grpc get chat server failed, error is %s", reply.error());
			root["error"] = ErrorCodes::ERR_RPC;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		LOGI("LogicSystem: succeed to load userinfo, uid is %d", userInfo.uid);
		root["error"] = ErrorCodes::SUCCESS;
		root["email"] = email;
		root["uid"] = userInfo.uid;
		root["token"] = reply.token();
		root["host"] = reply.host();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});
}
