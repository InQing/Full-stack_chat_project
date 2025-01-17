#pragma once
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "const.h"
#include "MsgNode.h"

class CServer;

class CSession : public std::enable_shared_from_this<CSession>
{
public:
	CSession(boost::asio::io_context& io_context, CServer* server);
	~CSession() = default;
	tcp::socket& GetSocket();
	std::string& GetSessionId();
	void SetUserId(int uid);
	int GetUserId();
	void Start();
	void Send(char* msg, short max_length, short msgid);
	void Send(const std::string& msg, short msgid);
	void Close();
	void AsyncReadBody(int length);
	void AsyncReadHead(int total_len);
private:
	std::shared_ptr<CSession> SharedSelf();
	// 读取指定字节数，异步非阻塞，能保证读完整指定字节
	void AsyncReadFull(std::size_t maxLength, std::function<void(const boost::system::error_code&, std::size_t)> handler);
	void AsyncReadLen(std::size_t  read_len, std::size_t total_len,
		std::function<void(const boost::system::error_code&, std::size_t)> handler);
	void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self);
	
	tcp::socket socket_;
	std::string session_id_;
	int user_id_;
	char data_[MAX_LENGTH];
	CServer* server_;
	bool is_close_;
	std::queue<std::shared_ptr<SendNode>> send_que_;
	std::mutex send_lock_;
	std::shared_ptr<RecvNode> recv_msg_node_; // 收到的消息结构
	bool is_head_parse_;
	std::shared_ptr<MsgNode> recv_head_node_; //收到的头部结构
};

class LogicNode {
	friend class LogicSystem;
public:
	LogicNode(std::shared_ptr<CSession> session,
		std::shared_ptr<RecvNode> recv_node) : _session(session), _recvnode(recv_node) {}
private:
	std::shared_ptr<CSession> _session;
	std::shared_ptr<RecvNode> _recvnode;
};