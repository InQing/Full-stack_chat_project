#include "UserMgr.h"

UserMgr::~UserMgr() {
	uid2Session_.clear();
}

std::shared_ptr<CSession> UserMgr::GetSession(int uid)
{
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = uid2Session_.find(uid);
	if (it == uid2Session_.end()) {
		return nullptr;
	}
	return it->second;
}

void UserMgr::SetUserSession(int uid, std::shared_ptr<CSession> session)
{
	std::lock_guard<std::mutex> lock(mutex_);
	uid2Session_[uid] = session;
}

void UserMgr::RemoveUserSession(int uid)
{
	std::lock_guard<std::mutex> lock(mutex_);
	uid2Session_.erase(uid);
}



	

