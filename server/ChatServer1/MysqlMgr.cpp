#include "MysqlMgr.h"

MysqlMgr::~MysqlMgr() {

}

int MysqlMgr::RegUser(const std::string& user, const std::string& email, const std::string& pwd)
{
    return dao_.RegUser(user, email, pwd);
}

bool MysqlMgr::CheckEmail(const std::string& name, const std::string& email) {
	return dao_.CheckEmail(name, email);
}

bool MysqlMgr::UpdatePwd(const std::string& name, const std::string& pwd) {
	return dao_.UpdatePwd(name, pwd);
}

bool MysqlMgr::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
	return dao_.CheckPwd(email, pwd, userInfo);
}

std::shared_ptr<UserInfo> MysqlMgr::GetUserInfo(int uid)
{
	return dao_.GetUserInfo(uid);
}

bool MysqlMgr::AddFriendApply(const int& from, const int& to) {
	return dao_.AddFriendApply(from, to);
}

bool MysqlMgr::AuthFriendApply(const int& from, const int& to) {
	return dao_.AuthFriendApply(from, to);
}

bool MysqlMgr::AddFriend(const int& from, const int& to, std::string back_name) {
	return dao_.AddFriend(from, to, back_name);
}

bool MysqlMgr::GetApplyList(int touid,
	std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit) {

	return dao_.GetApplyList(touid, applyList, begin, limit);
}

bool MysqlMgr::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo> >& user_info) {
	return dao_.GetFriendList(self_id, user_info);
}