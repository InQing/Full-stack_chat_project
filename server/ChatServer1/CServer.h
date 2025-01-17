#pragma once
#include <memory>
#include <unordered_map>
#include <mutex>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class CSession;

class CServer
{
public:
	CServer(boost::asio::io_context& io_context, short port);
	~CServer() = default;
	void ClearSession(const std::string& session_id);
private:
	void HandleAccept(std::shared_ptr<CSession>, const boost::system::error_code& error);
	void StartAccept();
	boost::asio::io_context& _io_context;
	short _port;
	tcp::acceptor _acceptor; // 接受客户端tcp连接
	std::unordered_map<std::string, std::shared_ptr<CSession>> _sessions;
	std::mutex _mutex;
};

