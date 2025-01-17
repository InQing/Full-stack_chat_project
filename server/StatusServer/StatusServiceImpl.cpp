#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include "logger.h"
#include "RedisMgr.h"

// 生成token
std::string generate_unique_string() {
	boost::uuids::uuid uuid = boost::uuids::random_generator()();
	std::string unique_string = to_string(uuid);

	return unique_string;
}

Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply)
{
	LOGI("StatusServiceImpl: GetChatServer is called");
	const auto& server = getChatServer();
	reply->set_host(server.host);
	reply->set_port(server.port);
	reply->set_error(ErrorCodes::SUCCESS);
	reply->set_token(generate_unique_string());
	insertToken(request->uid(), reply->token());
	return Status::OK;
}

StatusServiceImpl::StatusServiceImpl()
{
	auto& cfg = ConfigMgr::GetInstance();
	auto server_list = cfg["ChatServers"]["Name"];

	std::vector<std::string> servers;

	std::stringstream ss(server_list);
	std::string server;

	while (std::getline(ss, server, ',')) {
		servers.push_back(server);
	}

	for (auto& server : servers) {
		if (cfg[server]["Name"].empty()) {
			continue;
		}

		ChatServer chat_server;
		chat_server.port = cfg[server]["Port"];
		chat_server.host = cfg[server]["Host"];
		chat_server.name = cfg[server]["Name"];
		_servers[chat_server.name] = chat_server;
	}

}

ChatServer StatusServiceImpl::getChatServer() {
	std::lock_guard<std::mutex> guard(_server_mtx);
	auto minServer = _servers.begin()->second;
	auto count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, minServer.name);
	if (count_str.empty()) {
		// 不存在则默认设置为最大
		minServer.con_count = INT_MAX;
	}
	else {
		minServer.con_count = std::stoi(count_str);
	}

	// 负载均衡，选择连接数最少的服务器
	for (auto& server : _servers) {
		if (server.second.name == minServer.name) {
			continue;
		}

		auto count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server.second.name);
		if (count_str.empty()) {
			server.second.con_count = INT_MAX;
		}
		else {
			server.second.con_count = std::stoi(count_str);
		}

		if (server.second.con_count < minServer.con_count) {
			minServer = server.second;
		}
	}

	return minServer;
}

Status StatusServiceImpl::Login(ServerContext* context, const LoginReq* request, LoginRsp* reply)
{	
	LOGI("StatusServiceImpl: Login is called");
	auto uid = request->uid();
	auto token = request->token();
	
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	std::string token_value = "";
	bool success = RedisMgr::GetInstance()->Get(token_key, token_value);

	if (!success) {
		reply->set_error(ErrorCodes::ERR_UID_INVALID);
		return Status::OK;
	}
	if (token_value != token) {
		reply->set_error(ErrorCodes::ERR_TOKEN_INVALID);
		return Status::OK;
	}
	reply->set_error(ErrorCodes::SUCCESS);
	reply->set_uid(uid);
	reply->set_token(token);
	return Status::OK;
}

void StatusServiceImpl::insertToken(int uid, std::string token)
{
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	RedisMgr::GetInstance()->Set(token_key, token);
}
