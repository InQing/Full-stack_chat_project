#include "ChatGrpcClient.h"
#include "ConfigMgr.h"
#include "Logger.h"

ChatGrpcClient::ChatGrpcClient() {
	auto& cfg = ConfigMgr::GetInstance();
	auto server_list = cfg["PeerServers"]["Servers"];
	std::vector<std::string> servers;

	std::stringstream ss(server_list);
	std::string word;
	while (std::getline(ss, word, ',')) {
		servers.push_back(word);
	}

	for (auto& server : servers) {
		if (cfg[server]["Name"].empty()){
			continue;
		}
		pools_[cfg[server]["Name"]] = std::make_unique<ChatConPool>(std::thread::hardware_concurrency(), cfg[server]["Host"], cfg[server]["RPCPort"]);
	}
}

AddFriendRsp ChatGrpcClient::NotifyAddFriend(std::string server_ip, const AddFriendReq& req) {
    LOGI("ChatGrpcClient::NotifyAddFriend");
	AddFriendRsp rsp;
    Defer defer([&rsp, &req]() {
        rsp.set_applyuid(req.applyuid());
        rsp.set_touid(req.touid());
        });

    auto find_iter = pools_.find(server_ip);
    if (find_iter == pools_.end()) {
        rsp.set_error(ErrorCodes::ERR_SERVER);
        return rsp;
    }

    auto& pool = find_iter->second;
    ClientContext context;
    auto stub = pool->GetConnection();
    Status status = stub->NotifyAddFriend(&context, req, &rsp);
    Defer defercon([&stub, this, &pool]() {
        pool->ReturnConnection(std::move(stub));
        });

    if (!status.ok()) {
        rsp.set_error(ErrorCodes::ERR_RPC);
        return rsp;
    }

    return rsp;
}

AuthFriendRsp ChatGrpcClient::NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req) {
	LOGI("ChatGrpcClient::NotifyAuthFriend");
	AuthFriendRsp rsp;
	rsp.set_error(ErrorCodes::SUCCESS);

	Defer defer([&rsp, &req]() {
		rsp.set_fromuid(req.fromuid());
		rsp.set_touid(req.touid());
		});

	auto find_iter = pools_.find(server_ip);
	if (find_iter == pools_.end()) {
		return rsp;
	}

	auto& pool = find_iter->second;
	ClientContext context;
	auto stub = pool->GetConnection();
	Status status = stub->NotifyAuthFriend(&context, req, &rsp);
	Defer defercon([&stub, this, &pool]() {
		pool->ReturnConnection(std::move(stub));
		});

	if (!status.ok()) {
		rsp.set_error(ErrorCodes::ERR_RPC);
		return rsp;
	}

	return rsp;
}

bool ChatGrpcClient::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo) {
	return true;
}

TextChatMsgRsp ChatGrpcClient::NotifyTextChatMsg(std::string server_ip,
	const TextChatMsgReq& req, const Json::Value& rtvalue) {
	LOGI("ChatGrpcClient::NotifyTextChatMsg");
	TextChatMsgRsp rsp;
	rsp.set_error(ErrorCodes::SUCCESS);

	Defer defer([&rsp, &req]() {
		rsp.set_fromuid(req.fromuid());
		rsp.set_touid(req.touid());
		for (const auto& text_data : req.textmsgs()) {
			TextChatData* new_msg = rsp.add_textmsgs();
			new_msg->set_msgid(text_data.msgid());
			new_msg->set_msgcontent(text_data.msgcontent());
		}

		});

	auto find_iter = pools_.find(server_ip);
	if (find_iter == pools_.end()) {
		return rsp;
	}

	auto& pool = find_iter->second;
	ClientContext context;
	auto stub = pool->GetConnection();
	Status status = stub->NotifyTextChatMsg(&context, req, &rsp);
	Defer defercon([&stub, this, &pool]() {
		pool->ReturnConnection(std::move(stub));
		});

	if (!status.ok()) {
		rsp.set_error(ErrorCodes::ERR_RPC);
		return rsp;
	}

	return rsp;
}