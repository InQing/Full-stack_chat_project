#include "CServer.h"
#include "AsioIOServicePool.h"
#include "Logger.h"
#include "CSession.h"

CServer::CServer(boost::asio::io_context& io_context, short port) :_io_context(io_context), _port(port),
_acceptor(io_context, tcp::endpoint(tcp::v4(), port))
{
	LOGI("[ChatServer2] start success! listen on : %d", _port);
	StartAccept();
}

void CServer::HandleAccept(std::shared_ptr<CSession> new_session, const boost::system::error_code& error) {
	if (!error) {
		new_session->Start();
		std::lock_guard<std::mutex> lock(_mutex);
		_sessions.insert(make_pair(new_session->GetSessionId(), new_session));
	}
	else {
		LOGW("session accept failed, error is %s", error.what());
	}

	StartAccept();
}

void CServer::StartAccept() {
	auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
	std::shared_ptr<CSession> new_session = std::make_shared<CSession>(io_context, this);
	// 异步监听TCP连接，绑定HandleAccept作为可调用对象
	_acceptor.async_accept(new_session->GetSocket(), std::bind(&CServer::HandleAccept, this, new_session, std::placeholders::_1));
}

void CServer::ClearSession(const std::string& session_id) {
	std::lock_guard<std::mutex> lock(_mutex);
	_sessions.erase(session_id);
}