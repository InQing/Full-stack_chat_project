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

std::shared_ptr<UserInfo> MysqlMgr::GetUser(int uid)
{
	return dao_.GetUser(uid);
}

bool MysqlMgr::AddFriendApply(const int& from, const int& to) {
	return dao_.AddFriendApply(from, to);
}