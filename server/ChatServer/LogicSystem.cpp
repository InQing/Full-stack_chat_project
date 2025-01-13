#include "LogicSystem.h"
#include "StatusGrpcClient.h"
#include "MysqlMgr.h"
#include "CSession.h"
#include "Logger.h"
#include "const.h"

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
	func_callbacks_[MSG_IDS::MSG_CHAT_LOGIN] = std::bind(&LogicSystem::LoginHandler, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void LogicSystem::LoginHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto token = root["token"].asString();
	LOGI("LoginSystem::LoginHandler: user login uid is %d, user token is %s", uid, token.c_str());
	// 从状态服务器获取token匹配是否准确
	auto rsp = StatusGrpcClient::GetInstance()->Login(uid, token);
	Json::Value rt_value; // 回包数据
	Defer defer([this, &rt_value, session]() {
		std::string return_str = rt_value.toStyledString();
		session->Send(return_str, MSG_IDS::MSG_CHAT_LOGIN_RSP);
	});

	rt_value["error"] = rsp.error();
	if (rsp.error() != ErrorCodes::SUCCESS) {
		return;
	}

	// 内存中查询用户信息
	std::shared_ptr<UserInfo> user_info = nullptr;
	auto find_iter = users_.find(uid);
	if (find_iter == users_.end()) {
		user_info = MysqlMgr::GetInstance()->GetUser(uid);
		if (user_info == nullptr) {
			rt_value["error"] = ErrorCodes::ERR_UID_INVALID;
			return;
		}

		users_[uid] = user_info;
	}
	else {
		user_info = find_iter->second;
	}

	rt_value["uid"] = uid;
	rt_value["token"] = rsp.token();
	rt_value["name"] = user_info->name;
}