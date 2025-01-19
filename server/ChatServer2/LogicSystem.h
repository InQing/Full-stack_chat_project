#pragma once
#include <functional>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <unordered_map>
#include "Singleton.h"
#include "const.h"
#include "data.h"

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
	void SearchInfoApply(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
	void AddFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
	void AuthFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
	bool GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& list);
	bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_list);
	bool GetUserInfo(int uid, std::shared_ptr<UserInfo>& userinfo);

	std::thread worker_thread_;
	std::queue<std::shared_ptr<LogicNode>> msg_que_;
	std::mutex mutex_;
	std::condition_variable consume_;
	bool is_stop_;
	std::unordered_map<short, FuncCallBack> func_callbacks_;
	std::unordered_map<int, std::shared_ptr<UserInfo>> users_;
};