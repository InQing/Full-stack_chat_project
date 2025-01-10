#include "usermgr.h"

UserMgr::UserMgr(){}
UserMgr::~UserMgr(){}

void UserMgr::SetName(QString name)
{
    name_ = name;
}

void UserMgr::SetUid(int uid)
{
    uid_ = uid;
}

void UserMgr::SetToken(QString token)
{
    token_ = token;
}
