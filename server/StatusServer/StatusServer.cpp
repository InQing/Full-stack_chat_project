#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <memory>
#include <string>
#include <thread>
#include <boost/asio.hpp>
#include "const.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"
#include "AsioIOServicePool.h"
#include "StatusServiceImpl.h"
#include "Logger.h"

void RunServer() {
	auto& cfg = ConfigMgr::GetInstance();
	std::string server_address(cfg["StatusServer"]["Host"] + ":" + cfg["StatusServer"]["Port"]);
	StatusServiceImpl service;

	grpc::ServerBuilder builder;
	// 监听端口，添加服务
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);

	// 构建并启动grpc服务器
	std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
	LOGI("[StatusServer] start success! listen on : %s", server_address.c_str());

	// 创建Boost.Asio的异步调度器io_context
	boost::asio::io_context io_context;
	// 创建signal_set用于捕获SIGINT
	boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
	// 设置异步等待SIGINT信号
	signals.async_wait([&server](const boost::system::error_code& error, int signal_number) {
		if (!error) {
			LOGI("StatusServer: Shutting down server...");
			server->Shutdown();
		}
		});

	// 在单独的线程中运行io_context检测SIGINT信号
	std::thread([&io_context]() {
		io_context.run();
		}).detach();

	// 等待服务器关闭
	server->Wait();
	io_context.stop();
}

int main(int argc, char** argv) {
	try {
		RunServer();
	}
	catch (std::exception const& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return 0;
}