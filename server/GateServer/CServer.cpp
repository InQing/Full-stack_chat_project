#include "CServer.h"
#include "HttpConnection.h"
#include "AsioIOServicePool.h"
#include "const.h"
#include "Logger.h"

// 含引用类型，必须用初始化列表构造
CServer::CServer(net::io_context& ioc, unsigned short& port) : _ioc(ioc),
_acceptor(ioc, tcp::endpoint(tcp::v4(), port)){}

// 监听新连接
void CServer::Start()
{	
	LOGI("CServer: Starts to listen!");
	auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
	std::shared_ptr<HttpConnection> new_con = std::make_shared<HttpConnection>(io_context);
	auto self = shared_from_this();
	// 异步接收客户端连接，并用lambda回调处理
	_acceptor.async_accept(new_con->GetSocket(), [self, new_con](beast::error_code ec) {
		try {
			// 出错则放弃这个连接，继续监听
			if (ec) {
				LOGW("CServer: Connection failed!");
				self->Start();
				return;
			}
			else {
				LOGI("Cserver: New connection!");
				new_con->Start();
				self->Start();
			}
		}
		catch (std::exception& exc) {
			LOGW("Cserver: Listen failed, error is %s", exc.what());
			self->Start(); // 继续监听
		}
	});
}
