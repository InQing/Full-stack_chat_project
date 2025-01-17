#pragma once
#include <unordered_map>
#include <memory>
#include <mutex>
#include "Singleton.h"
	
class CSession;
class UserMgr : public Singleton<UserMgr>
{
	friend class Singleton<UserMgr>;
public:
	~UserMgr();
	std::shared_ptr<CSession> GetSession(int uid);
	void SetUserSession(int uid, std::shared_ptr<CSession> session);
	void RemoveUserSession(int uid);
private:
	UserMgr() = default;
	std::mutex mutex_;
	std::unordered_map<int, std::shared_ptr<CSession>> uid2Session_;
};

