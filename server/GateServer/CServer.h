#pragma once
#include "const.h"
class CServer : public std::enable_shared_from_this<CServer>
{
public:
	CServer(net::io_context& ioc, unsigned short& port);
	void Start(); // 启动服务器
private:
	net::io_context& _ioc; // 管理和调度异步任务
	tcp::acceptor _acceptor; // 异步监听并接收TCP连接
};

