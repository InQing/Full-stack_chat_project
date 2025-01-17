#pragma once
#include "Singleton.h"
#include "const.h"
#include <functional>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <unordered_map>

class CSession;
class LogicNode;

typedef std::function<void(std::shared_ptr<CSession>, const short& msg_id, const std::string& msg_data)> FuncCallBack;
class LogicSystem : public Singleton<LogicSystem> {
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem();
	void PostMsgToQue(std::shared_ptr<LogicNode> msg);
private:
	LogicSystem();
	void DealMsg();
	void RegisterCallBacks();
	void LoginHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
	bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
	std::thread worker_thread_;
	std::queue<std::shared_ptr<LogicNode>> msg_que_;
	std::mutex mutex_;
	std::condition_variable consume_;
	bool is_stop_;
	std::unordered_map<short, FuncCallBack> func_callbacks_;
	std::unordered_map<int, std::shared_ptr<UserInfo>> users_;
};