#include "LogicSystem.h"
#include "StatusGrpcClient.h"
#include "MysqlMgr.h"
#include "CSession.h"
#include "Logger.h"
#include "const.h"
#include "RedisMgr.h"
#include "ConfigMgr.h"
#include "UserMgr.h"
#include "ChatGrpcClient.h"

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
	func_callbacks_[MSG_IDS::ID_SEARCH_USER_REQ] = std::bind(&LogicSystem::SearchInfoApply, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	// 注册好友申请回调
	func_callbacks_[ID_ADD_FRIEND_REQ] = std::bind(&LogicSystem::AddFriendApply, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	// 注册认证好友回调
	func_callbacks_[ID_AUTH_FRIEND_REQ] = std::bind(&LogicSystem::AuthFriendApply, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	// 注册文本消息回调
	func_callbacks_[ID_TEXT_CHAT_MSG_REQ] = std::bind(&LogicSystem::DealChatTextMsg, this,
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
	rt_value["nick"] = user_info->nick;
	rt_value["desc"] = user_info->desc;
	rt_value["sex"] = user_info->sex;
	rt_value["icon"] = user_info->icon;

	// 从Mysql中获取好友申请列表
	std::vector<std::shared_ptr<ApplyInfo>> apply_list;
	auto b_apply = GetFriendApplyInfo(uid, apply_list);
	if (b_apply) {
		for (auto& apply : apply_list) {
			Json::Value obj;
			obj["name"] = apply->_name;
			obj["uid"] = apply->_uid;
			obj["icon"] = apply->_icon;
			obj["nick"] = apply->_nick;
			obj["sex"] = apply->_sex;
			obj["desc"] = apply->_desc;
			obj["status"] = apply->_status;
			rt_value["apply_list"].append(obj);
		}
	}
	// 从Mysql中获取好友列表
	std::vector<std::shared_ptr<UserInfo>> friend_list;
	bool b_friend_list = GetFriendList(uid, friend_list);
	for (auto& friend_ele : friend_list) {
		Json::Value obj;
		obj["name"] = friend_ele->name;
		obj["uid"] = friend_ele->uid;
		obj["icon"] = friend_ele->icon;
		obj["nick"] = friend_ele->nick;
		obj["sex"] = friend_ele->sex;
		obj["desc"] = friend_ele->desc;
		obj["back"] = friend_ele->back;
		rt_value["friend_list"].append(obj);
	}


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

void LogicSystem::SearchInfoApply(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid_str = root["uid"].asString();
	LOGI("LogicSystem::SearchInfoHandler: user SearchInfo, uid is %s", uid_str.c_str());

	Json::Value rt_value;

	Defer deder([this, &rt_value, session]() {
		std::string return_str = rt_value.toStyledString();
		session->Send(return_str, ID_SEARCH_USER_RSP);
		});

	auto user_info = std::make_shared<UserInfo>();
	bool success = GetUserInfo(std::stoi(uid_str), user_info);
	if (!success) {
		rt_value["error"] = ErrorCodes::ERR_UID_INVALID;
		return;
	}

	rt_value["error"] = ErrorCodes::SUCCESS;
	rt_value["uid"] = user_info->uid;
	rt_value["name"] = user_info->name;
	rt_value["email"] = user_info->email;
	rt_value["nick"] = user_info->nick;
	rt_value["desc"] = user_info->desc;
	rt_value["sex"] = user_info->sex;
	rt_value["icon"] = user_info->icon;
}

void LogicSystem::AddFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto applyname = root["applyname"].asString();
	auto bakname = root["bakname"].asString();
	auto touid = root["touid"].asInt();
	LOGI("LogicSystem::AddFriendApply: user AddFriendApply, uid is %d, applyname is %s, bakname is %s, touid is %d", 
		uid, applyname.c_str(), bakname.c_str(), touid);

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::SUCCESS;
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_ADD_FRIEND_RSP);
		});

	// 更新数据库
	MysqlMgr::GetInstance()->AddFriendApply(uid, touid);

	// 在redis查找touid对应的server ip
	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPREFIX + to_str;
	std::string to_ip_value = "";
	bool is_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
	if (!is_ip) {
		return;
	}

	auto& cfg = ConfigMgr::GetInstance();
	auto self_name = cfg["SelfServer"]["Name"];

	auto apply_info = std::make_shared<UserInfo>();
	bool is_info = GetUserInfo(uid, apply_info);

	// 在同一个服务器中，直接通知对方有申请消息
	if (to_ip_value == self_name) {
		auto session = UserMgr::GetInstance()->GetSession(touid);
		if (session) {
			//在内存中则直接发送通知对方
			Json::Value  notify;
			notify["error"] = ErrorCodes::SUCCESS;
			notify["applyuid"] = uid;
			notify["name"] = applyname;
			if (is_info) {
				notify["icon"] = apply_info->icon;
				notify["sex"] = apply_info->sex;
				notify["nick"] = apply_info->nick;
				notify["desc"] = apply_info->desc;
			}
			std::string return_str = notify.toStyledString();
			session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);
		}

		return;
	}

	// 不同服务器中，GRPC通知对方有申请消息
	AddFriendReq add_req;
	add_req.set_applyuid(uid);
	add_req.set_touid(touid);
	add_req.set_name(applyname);
	add_req.set_desc(apply_info->desc);
	if (is_info) {
		add_req.set_icon(apply_info->icon);
		add_req.set_sex(apply_info->sex);
		add_req.set_nick(apply_info->nick);
	}

	//发送通知
	ChatGrpcClient::GetInstance()->NotifyAddFriend(to_ip_value, add_req);
}

void LogicSystem::AuthFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data) {

	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);

	auto uid = root["fromuid"].asInt();
	auto touid = root["touid"].asInt();
	auto back_name = root["back"].asString();
	LOGI("LogicSystem::AuthFriendApply: user AuthFriendApply, fromuid is %d, touid is %d, back_name is %s",
		uid, touid, back_name.c_str());

	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::SUCCESS;
	auto user_info = std::make_shared<UserInfo>();

	bool is_info = GetUserInfo(touid, user_info);
	if (is_info) {
		rtvalue["name"] = user_info->name;
		rtvalue["nick"] = user_info->nick;
		rtvalue["icon"] = user_info->icon;
		rtvalue["sex"] = user_info->sex;
		rtvalue["uid"] = touid;
	}
	else {
		rtvalue["error"] = ErrorCodes::ERR_UID_INVALID;
	}

	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_AUTH_FRIEND_RSP);
		});

	//先更新数据库
	MysqlMgr::GetInstance()->AuthFriendApply(uid, touid);

	//更新数据库添加好友
	MysqlMgr::GetInstance()->AddFriend(uid, touid, back_name);

	//查询redis 查找touid对应的server ip
	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPREFIX + to_str;
	std::string to_ip_value = "";
	bool is_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
	if (!is_ip) {
		return;
	}

	auto& cfg = ConfigMgr::GetInstance();
	auto self_name = cfg["SelfServer"]["Name"];
	// 在同一个服务器中，直接通知对方有认证通过消息
	if (to_ip_value == self_name) {
		auto session = UserMgr::GetInstance()->GetSession(touid);
		if (session) {
			//在内存中则直接发送通知对方
			Json::Value  notify;
			notify["error"] = ErrorCodes::SUCCESS;
			notify["fromuid"] = uid;
			notify["touid"] = touid;
			auto user_info = std::make_shared<UserInfo>();
			bool is_info = GetUserInfo(uid, user_info);
			if (is_info) {
				notify["name"] = user_info->name;
				notify["nick"] = user_info->nick;
				notify["icon"] = user_info->icon;
				notify["sex"] = user_info->sex;
			}
			else {
				notify["error"] = ErrorCodes::ERR_UID_INVALID;
			}

			std::string return_str = notify.toStyledString();
			session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
		}
		return;
	}


	AuthFriendReq auth_req;
	auth_req.set_fromuid(uid);
	auth_req.set_touid(touid);

	// 不在同一个服务器中，GRPC发送通知
	ChatGrpcClient::GetInstance()->NotifyAuthFriend(to_ip_value, auth_req);
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
		userinfo->nick = root["nick"].asString();
		userinfo->desc = root["desc"].asString();
		userinfo->sex = root["sex"].asInt();
		userinfo->icon = root["icon"].asString();
	}
	else {
		//redis中没有则查询mysql
		std::shared_ptr<UserInfo> user_info = nullptr;
		user_info = MysqlMgr::GetInstance()->GetUserInfo(uid);
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
		redis_root["nick"] = userinfo->nick;
		redis_root["desc"] = userinfo->desc;
		redis_root["sex"] = userinfo->sex;
		redis_root["icon"] = userinfo->icon;
		RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
	}

	return true;
}

bool LogicSystem::GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& list) {
	//从mysql获取好友申请列表
	return MysqlMgr::GetInstance()->GetApplyList(to_uid, list, 0, 10);
}

bool LogicSystem::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_list) {
	//从mysql获取好友列表
	return MysqlMgr::GetInstance()->GetFriendList(self_id, user_list);
}

void LogicSystem::DealChatTextMsg(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);

	auto uid = root["fromuid"].asInt();
	auto touid = root["touid"].asInt();
	LOGI("LogicSystem::DealChatTextMsg: user DealChatTextMsg, fromuid is %d, touid is %d, msg_data is %s",
		uid, touid, msg_data.c_str());

	const Json::Value  arrays = root["text_array"];

	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::SUCCESS;
	rtvalue["text_array"] = arrays;
	rtvalue["fromuid"] = uid;
	rtvalue["touid"] = touid;

	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_TEXT_CHAT_MSG_RSP);
		});


	//查询redis 查找touid对应的server ip
	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPREFIX + to_str;
	std::string to_ip_value = "";
	bool is_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
	// 对方不在线，不推送消息
	// TODO... 将消息存入redis，待对方上线后再推送
	if (!is_ip) {
		return;
	}

	auto& cfg = ConfigMgr::GetInstance();
	auto self_name = cfg["SelfServer"]["Name"];
	// 对方位于同一个服务器中，直接通知对方有新消息
	if (to_ip_value == self_name) {
		auto session = UserMgr::GetInstance()->GetSession(touid);
		if (session) {
			//在内存中则直接发送通知对方
			std::string return_str = rtvalue.toStyledString();
			session->Send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
		}

		return;
	}

	// 不同服务器中，GRPC通知对方有新消息
	TextChatMsgReq text_msg_req;
	text_msg_req.set_fromuid(uid);
	text_msg_req.set_touid(touid);
	for (const auto& txt_obj : arrays) {
		auto content = txt_obj["content"].asString();
		auto msgid = txt_obj["msgid"].asString();
		auto* text_msg = text_msg_req.add_textmsgs();
		text_msg->set_msgid(msgid);
		text_msg->set_msgcontent(content);
	}

	ChatGrpcClient::GetInstance()->NotifyTextChatMsg(to_ip_value, text_msg_req, rtvalue);
}
