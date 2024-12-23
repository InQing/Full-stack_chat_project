#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "CServer.h"
#include "ConfigMgr.h"
#include "const.h"
#include "Logger.h"
#include "RedisMgr.h"

int main()
{	
	// 读取配置
	auto& cfg_mgr = ConfigMgr::GetInstance();
	std::string gate_port_str = cfg_mgr["GateServer"]["Port"];
	unsigned short gate_port = atoi(gate_port_str.c_str());

	try {
		Logger::GetInstance().SetLogFile("log.txt");
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}

	try {
		net::io_context ioc{1};
		// 注册信号ctrl-c
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
		// 启动信号异步监听ctr-c退出事件
		signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {
			if (error) {
				return;
			}
			ioc.stop();
			});

		// 启动ioc
		
		LOGI("GateServer: Server start, port is %d", gate_port);
		std::make_shared<CServer>(ioc, gate_port)->Start();
		ioc.run();

	}
	catch (std::exception const& exc) {
		LOGE("GateServer: Start failed, error is: %s", exc.what());
		return EXIT_FAILURE;
	}
}