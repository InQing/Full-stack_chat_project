#pragma once
#include "const.h"
#include "data.h"
#include "Singleton.h"
#include "ConfigMgr.h" 
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <grpcpp/grpcpp.h>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::AddFriendReq;
using message::AddFriendRsp;

using message::AuthFriendReq;
using message::AuthFriendRsp;

using message::GetChatServerRsp;
using message::LoginRsp;
using message::LoginReq;
using message::ChatService;

using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;

using message::FileChatMsgReq;
using message::FileChatMsgRsp;
using message::FileChatData;

class ChatConPool {
public:
	ChatConPool(size_t pool_size, const std::string host, const std::string port)
	: pool_size_(pool_size), host_(host), port_(port), is_stop_(false) {
		for (size_t i = 0; i < pool_size_; ++i) {
			std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials());
			connections_.push(ChatService::NewStub(channel));
		}
	}

	~ChatConPool() {
		std::lock_guard<std::mutex> lock(mutex_);
		Close();
		while (!connections_.empty()) {
			connections_.pop();
		}
	}

	std::unique_ptr<ChatService::Stub> GetConnection() {
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this]() { 
			return is_stop_ || !connections_.empty();
			});
		if (is_stop_) {
			return nullptr;
		}
		auto connection = std::move(connections_.front());
		connections_.pop();
		return connection;
	}

	void ReturnConnection(std::unique_ptr<ChatService::Stub> connection) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (is_stop_) {
			return;
		}
		connections_.push(std::move(connection));
		cond_.notify_one();
	}
	
	void Close() {
		is_stop_ = true;
		cond_.notify_all();
	}
private:
	size_t pool_size_;
	std::string host_;
	std::string port_;
	std::queue<std::unique_ptr<ChatService::Stub>> connections_;
	std::mutex mutex_;
	std::condition_variable cond_;
	bool is_stop_;

};

class ChatGrpcClient : public Singleton<ChatGrpcClient>
{
	friend class Singleton<ChatGrpcClient>;
public:
	~ChatGrpcClient() {}
	AddFriendRsp NotifyAddFriend(std::string server_ip, const AddFriendReq& req);
	AuthFriendRsp NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req);
	bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
	TextChatMsgRsp NotifyTextChatMsg(std::string server_ip, const TextChatMsgReq& req, const Json::Value& rtvalue);
	FileChatMsgRsp NotifyFileChatMsg(std::string server_ip, const FileChatMsgReq& req, const Json::Value& rtvalue);
private:
	ChatGrpcClient();
	std::unordered_map <std::string, std::unique_ptr<ChatConPool>> pools_;
};

