#include "LogicSystem.h"
#include "StatusGrpcClient.h"
#include "MysqlMgr.h"
#include "CSession.h"
#include "Logger.h"
#include "const.h"
#include "RedisMgr.h"
#include "ConfigMgr.h"
#include "UserMgr.h"

LogicSystem::LogicSystem() :is_stop_(false) {
	RegisterCallBacks();
	worker_thread_ = std::thread(&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem() {
	is_stop_ = true;
	consume_.notify_one();
	worker_thread_.join();
}

void LogicSystem::PostMsgToQue(std::shared_ptr<LogicNode> msg) {
	std::unique_lock<std::mutex> unique_lk(mutex_);
	msg_que_.push(msg);
	// 由0变为1则发送通知信号, 处理数据
	if (msg_que_.size() == 1) {
		unique_lk.unlock();
		consume_.notify_one();
	}
}

void LogicSystem::DealMsg() {
	while (true) {
		std::unique_lock<std::mutex> unique_lk(mutex_);
		// 判断队列为空则用条件变量阻塞等待，并释放锁
		while (msg_que_.empty() && !is_stop_) {
			consume_.wait(unique_lk);
		}

		// 若为关闭状态，则把所有逻辑执行完后退出循环
		if (is_stop_) {
			while (!msg_que_.empty()) {
				auto msg_node = msg_que_.front();
				auto call_back_iter = func_callbacks_.find(msg_node->_recvnode->_msg_id);
				if (call_back_iter == func_callbacks_.end()) {
					LOGW("LogicSystem: msg id [%d] handler not found!", msg_node->_recvnode->_msg_id);
					msg_que_.pop();
					continue;
				}
				call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
					std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
			}
			break;
		}

		// 若服务器没有停止，则照常处理队列中消息
		auto msg_node = msg_que_.front();
		auto call_back_iter = func_callbacks_.find(msg_node->_recvnode->_msg_id);
		if (call_back_iter == func_callbacks_.end()) {
			LOGW("LogicSystem: msg id [%d] handler not found!", msg_node->_recvnode->_msg_id);
			msg_que_.pop();
			continue;
		}
		call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
			std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
		msg_que_.pop();
	}
}

void LogicSystem::RegisterCallBacks() {
	// 注册登录回调
	func_callbacks_[MSG_IDS::ID_CHAT_LOGIN] = std::bind(&LogicSystem::LoginHandler, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	// 注册搜索用户回调
func_callbacks_[MSG_IDS::ID_SEARCH_USER_REQ] = std::bind(&LogicSystem::SearchInfoHandler, this,
	std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void LogicSystem::LoginHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto token = root["token"].asString();
	LOGI("LoginSystem::LoginHandler: user login, uid is %d, user token is %s", uid, token.c_str());

	Json::Value rt_value; // 回包数据
	Defer defer([this, &rt_value, session]() {
		std::string return_str = rt_value.toStyledString();
		session->Send(return_str, MSG_IDS::ID_CHAT_LOGIN_RSP);
	});

	// 从Redis中查询用户的token是否匹配
	auto uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	std::string token_value = "";
	bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
	if (!success) {
		rt_value["error"] = ErrorCodes::ERR_UID_INVALID;
		return;
	}
	if (token_value != token) {
		rt_value["error"] = ErrorCodes::ERR_TOKEN_INVALID;
		return;
	}

	rt_value["error"] = ErrorCodes::SUCCESS;

	auto user_info = std::make_shared<UserInfo>();
	success = GetUserInfo(uid, user_info);
	if (!success) {
		rt_value["error"] = ErrorCodes::ERR_UID_INVALID;
		return;
	}
	rt_value["uid"] = uid;
	rt_value["pwd"] = user_info->pwd;
	rt_value["name"] = user_info->name;
	rt_value["email"] = user_info->email;
	//rtvalue["nick"] = user_info->nick;
	//rtvalue["desc"] = user_info->desc;
	//rtvalue["sex"] = user_info->sex;
	//rtvalue["icon"] = user_info->icon;

	// 获取申请列表

	// 获取好友列表

	// 将Redis中本服务器的用户数量+1
	auto server_name = ConfigMgr::GetInstance()["SelfServer"]["Name"];
	auto rd_res = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server_name);
	int count = 0;
	if (!rd_res.empty()) {
		count = std::stoi(rd_res);
	}
	count++;
	auto count_str = std::to_string(count);
	RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, count_str);

	// Session绑定用户uid
	session->SetUserId(uid);
	// 设置用户登录的server_name
	std::string  ipkey = USERIPPREFIX + uid_str;
	RedisMgr::GetInstance()->Set(ipkey, server_name);

	//uid和session绑定管理,方便以后踢人操作
	UserMgr::GetInstance()->SetUserSession(uid, session);
}

void LogicSystem::SearchInfoHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid_str = root["info"].asString();
	LOGI("LogicSystem::SearchInfoHandler: user SearchInfo, uid is %s", uid_str.c_str());

	Json::Value rt_value;

	Defer deder([this, &rt_value, session]() {
		std::string return_str = rt_value.toStyledString();
		session->Send(return_str, ID_SEARCH_USER_RSP);
		});

	std::shared_ptr<UserInfo> user_info;
	bool success = GetUserInfo(std::stoi(uid_str), user_info);
	if (!success) {
		rt_value["error"] = ErrorCodes::ERR_UID_INVALID;
		return;
	}

	rt_value["error"] = ErrorCodes::SUCCESS;
	rt_value["uid"] = user_info->uid;
	rt_value["name"] = user_info->name;
	rt_value["email"] = user_info->email;
	//rtvalue["nick"] = user_info->nick;
	//rtvalue["desc"] = user_info->desc;
	//rtvalue["sex"] = user_info->sex;
	//rtvalue["icon"] = user_info->icon;
}

bool LogicSystem::GetUserInfo(int uid, std::shared_ptr<UserInfo>& userinfo)
{
	// 优先从redis中查询用户信息
	std::string base_key = USER_BASE_INFO + std::to_string(uid);
	std::string info_str = "";
	bool success = RedisMgr::GetInstance()->Get(base_key, info_str);
	if (success) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		userinfo->uid = root["uid"].asInt();
		userinfo->name = root["name"].asString();
		userinfo->pwd = root["pwd"].asString();
		userinfo->email = root["email"].asString();
		// userinfo->nick = root["nick"].asString();
		// userinfo->desc = root["desc"].asString();
		// userinfo->sex = root["sex"].asInt();
		// userinfo->icon = root["icon"].asString();
	}
	else {
		//redis中没有则查询mysql
		std::shared_ptr<UserInfo> user_info = nullptr;
		user_info = MysqlMgr::GetInstance()->GetUser(uid);
		if (user_info == nullptr) {
			return false;
		}

		userinfo = user_info;

		//将数据库内容写入redis缓存
		Json::Value redis_root;
		redis_root["uid"] = uid;
		redis_root["pwd"] = userinfo->pwd;
		redis_root["name"] = userinfo->name;
		redis_root["email"] = userinfo->email;
		//redis_root["nick"] = userinfo->nick;
		//redis_root["desc"] = userinfo->desc;
		//redis_root["sex"] = userinfo->sex;
		//redis_root["icon"] = userinfo->icon;
		RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
	}

	return true;
}