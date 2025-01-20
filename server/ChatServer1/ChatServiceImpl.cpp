#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "ChatServiceImpl.h"
#include "UserMgr.h"
#include "MysqlMgr.h"
#include "RedisMgr.h"
#include "CSession.h"
#include "data.h"
#include "Logger.h"


Status ChatServiceImpl::NotifyAddFriend(ServerContext* context, const AddFriendReq* request,
    AddFriendRsp* reply) {
    //查找用户是否在本服务器
    auto touid = request->touid();
    auto session = UserMgr::GetInstance()->GetSession(touid);
	LOGI("ChatServiceImpl::NotifyAddFriend, touid:%d", touid);

    Defer defer([request, reply]() {
        reply->set_applyuid(request->applyuid());
        reply->set_touid(request->touid());
        });

    //用户不在内存中则直接返回
    if (session == nullptr) {
        return Status::OK;
    }

    //在内存中则直接发送通知对方
    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::SUCCESS;
    rtvalue["applyuid"] = request->applyuid();
    rtvalue["name"] = request->name();
    rtvalue["desc"] = request->desc();
    rtvalue["icon"] = request->icon();
    rtvalue["sex"] = request->sex();
    rtvalue["nick"] = request->nick();

    std::string return_str = rtvalue.toStyledString();

    session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);

    return Status::OK;
}

Status ChatServiceImpl::NotifyAuthFriend(ServerContext* context, const AuthFriendReq* request,
	AuthFriendRsp* reply) {
	//查找用户是否在本服务器
	auto touid = request->touid();
	auto fromuid = request->fromuid();
	auto session = UserMgr::GetInstance()->GetSession(touid);
	LOGI("ChatServiceImpl::NotifyAuthFriend, touid:%d, fromuid:%d", touid, fromuid);

	Defer defer([request, reply]() {
		reply->set_error(ErrorCodes::SUCCESS);
		reply->set_fromuid(request->fromuid());
		reply->set_touid(request->touid());
		});

	//用户不在内存中则直接返回
	if (session == nullptr) {
		return Status::OK;
	}

	//在内存中则直接发送通知对方
	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::SUCCESS;
	rtvalue["fromuid"] = request->fromuid();
	rtvalue["touid"] = request->touid();

	auto user_info = std::make_shared<UserInfo>();
	bool is_info = GetUserInfo(fromuid, user_info);
	if (is_info) {
		rtvalue["name"] = user_info->name;
		rtvalue["nick"] = user_info->nick;
		rtvalue["icon"] = user_info->icon;
		rtvalue["sex"] = user_info->sex;
	}
	else {
		rtvalue["error"] = ErrorCodes::ERR_UID_INVALID;;
	}

	std::string return_str = rtvalue.toStyledString();

	session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
	return Status::OK;
}

Status ChatServiceImpl::NotifyTextChatMsg(::grpc::ServerContext* context,
	const TextChatMsgReq* request, TextChatMsgRsp* reply) {
	// 查找用户是否在本服务器
	auto touid = request->touid();
	auto session = UserMgr::GetInstance()->GetSession(touid);
	LOGI("ChatServiceImpl::NotifyTextChatMsg, touid:%d", touid);
	reply->set_error(ErrorCodes::SUCCESS);

	// 用户不在内存中则直接返回
	// TODO.. 用户不在线则存入Redis离线消息队列
	if (session == nullptr) {
		return Status::OK;
	}

	// 在内存中则直接发送通知对方
	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::SUCCESS;
	rtvalue["fromuid"] = request->fromuid();
	rtvalue["touid"] = request->touid();

	// 将聊天数据组织为数组
	Json::Value text_array;
	for (auto& msg : request->textmsgs()) {
		Json::Value element;
		element["content"] = msg.msgcontent();
		element["msgid"] = msg.msgid();
		text_array.append(element);
	}
	rtvalue["text_array"] = text_array;

	std::string return_str = rtvalue.toStyledString();

	session->Send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
	return Status::OK;
}


bool ChatServiceImpl::GetUserInfo(int uid, std::shared_ptr<UserInfo>& userinfo) {
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
