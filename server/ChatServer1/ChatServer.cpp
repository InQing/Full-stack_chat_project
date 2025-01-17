#include <csignal>
#include <thread>
#include <mutex>
#include "CServer.h"
#include "ConfigMgr.h"
#include "AsioIOServicePool.h"
#include "RedisMgr.h"
#include "const.h"
#include "ChatServiceImpl.h"
#include "Logger.h"

int main() {
	try {
		auto &cfg = ConfigMgr::GetInstance();
		auto pool = AsioIOServicePool::GetInstance();

		auto server_name = cfg["SelfServer"]["Name"];
		// ChatServer启动，将redis中该服务器的用户登录数量设置为0
		RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, "0");	
		
		// 启动Grpc服务器
		std::string server_addr(cfg["SelfServer"]["Host"] + ":" + cfg["SelfServer"]["RPCPort"]);
		ChatServiceImpl service;
		grpc::ServerBuilder builder;
		builder.AddListeningPort(server_addr, grpc::InsecureServerCredentials());
		builder.RegisterService(&service);
		std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
		LOGI("[%s] GRPC server started at %s", server_name.c_str(), server_addr.c_str());

		// 单独启动一个线程，处理GRPC服务
		std::thread grpc_thread([&]() {
			server->Wait();
			});
		grpc_thread.detach();

		// 监听退出信号
		boost::asio::io_context io_context;
		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([&io_context, pool, &server](auto, auto){
			LOGI("Received exit signal, shutting down...");
			io_context.stop();
			pool->Stop();
			server->Shutdown();
			});

		// 启动聊天服务器
		auto port_str = cfg["SelfServer"]["Port"];
		CServer s(io_context, atoi(port_str.c_str()));
		io_context.run();

		// 服务器退出，将redis中该服务器的用户登录数量清空
		RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
		RedisMgr::GetInstance()->Close();
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}