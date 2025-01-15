#include <iostream>
#include <sstream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "CSession.h"
#include "CServer.h"
#include "Logger.h"
#include "LogicSystem.h"

CSession::CSession(boost::asio::io_context& io_context, CServer* server) :
	socket_(io_context), server_(server), is_close_(false), is_head_parse_(false) {
	boost::uuids::uuid uuid_temp = boost::uuids::random_generator()();
	session_id_ = boost::uuids::to_string(uuid_temp);
	recv_head_node_ = std::make_shared<MsgNode>(HEAD_TOTAL_LEN);
}

tcp::socket& CSession::GetSocket() {
	return socket_;
}

std::string& CSession::GetSessionId() {
	return session_id_;
}

void CSession::SetUserId(int uid)
{
	user_id_ = uid;
}

int CSession::GetUserId()
{
	return user_id_;
}

void CSession::Start() {
	AsyncReadHead(HEAD_TOTAL_LEN);
}

void CSession::Send(const std::string& msg, short msgid) {
	std::lock_guard<std::mutex> lock(send_lock_);
	int send_que_size = send_que_.size();
	if (send_que_size > MAX_SENDQUE) {
		LOGW("CSession: session: %s, send que fulled, size is %d", session_id_, send_que_size);
		return;
	}

	send_que_.push(std::make_shared<SendNode>(msg.c_str(), msg.length(), msgid));
	// 还有其它数据需要发送，先返回
	if (send_que_size > 0) {
		return;
	}
	// 发送队列头的数据,并设置发送后的回调
	auto& msg_node = send_que_.front();
	boost::asio::async_write(socket_, boost::asio::buffer(msg_node->_data, msg_node->_total_len),
		std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}

void CSession::Send(char* msg, short max_length, short msgid) {
	std::lock_guard<std::mutex> lock(send_lock_);
	int send_que_size = send_que_.size();
	if (send_que_size > MAX_SENDQUE) {
		LOGW("CSession: session: %s, send que fulled, size is %d", session_id_, send_que_size);
		return;
	}

	send_que_.push(std::make_shared<SendNode>(msg, max_length, msgid));
	if (send_que_size > 0) {
		return;
	}
	auto& msg_node = send_que_.front();
	boost::asio::async_write(socket_, boost::asio::buffer(msg_node->_data, msg_node->_total_len),
		std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}

void CSession::Close() {
	socket_.close();
	is_close_ = true;
}

std::shared_ptr<CSession> CSession::SharedSelf() {
	return shared_from_this();
}

void CSession::AsyncReadHead(int total_len) {
	auto self = shared_from_this();
	AsyncReadFull(HEAD_TOTAL_LEN, [self, this](const boost::system::error_code& ec, std::size_t bytes_transfered) {
		try {
			if (ec) {
				LOGW("CSession: read head failed! error is %s", ec.what());
				Close();
				server_->ClearSession(session_id_);
				return;
			}

			if (bytes_transfered < HEAD_TOTAL_LEN) {
				LOGW("CSession: read head length not match! read %d, total is %d", bytes_transfered, HEAD_TOTAL_LEN);
				Close();
				server_->ClearSession(session_id_);
				return;
			}

			recv_head_node_->Clear();
			memcpy(recv_head_node_->_data, data_, HEAD_TOTAL_LEN);
			short msg_id = 0;
			memcpy(&msg_id, recv_head_node_->_data, HEAD_ID_LEN);
			// 大端转小端
			msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
			
			// id非法
			if (msg_id > MAX_LENGTH) {
				LOGW("CSession: invalid msg_id is &d", msg_id);
				server_->ClearSession(session_id_);
				return;
			}
			short msg_len = 0;
			memcpy(&msg_len, recv_head_node_->_data + HEAD_ID_LEN, HEAD_DATA_LEN);
			// 网络字节序转化为本地字节序
			msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);

			// len非法
			if (msg_len > MAX_LENGTH) {
				LOGW("CSession: invalid data length is &d", msg_len);
				server_->ClearSession(session_id_);
				return;
			}

			recv_msg_node_ = std::make_shared<RecvNode>(msg_len, msg_id);
			AsyncReadBody(msg_len);

		}
		catch (std::exception& e) {
			std::cerr << "Exception code is " << e.what() << std::endl;
		}
	});
}

void CSession::AsyncReadBody(int total_len)
{
	auto self = shared_from_this();
	AsyncReadFull(total_len, [self, this, total_len](const boost::system::error_code& ec, std::size_t bytes_transfered) {
		try {
			if (ec) {
				LOGW("CSession: read body failed, error is &s", ec.what());
				Close();
				server_->ClearSession(session_id_);
				return;
			}

			if (bytes_transfered < total_len) {
				LOGW("CSession: read body length not match! read %d, total is %d", bytes_transfered, total_len);
				Close();
				server_->ClearSession(session_id_);
				return;
			}

			memcpy(recv_msg_node_->_data, data_, total_len);
			recv_msg_node_->_cur_len += total_len;
			recv_msg_node_->_data[recv_msg_node_->_total_len] = '\0';
			//此处将消息投递到逻辑队列中
			LogicSystem::GetInstance()->PostMsgToQue(std::make_shared<LogicNode>(shared_from_this(), recv_msg_node_));
			//继续监听头部接受事件
			AsyncReadHead(HEAD_TOTAL_LEN);
		}
		catch (std::exception& e) {
			std::cerr << "Exception code is " << e.what() << std::endl;
		}
		});
}

// 完整地读取指定字节
void CSession::AsyncReadFull(std::size_t maxLength, std::function<void(const boost::system::error_code&, std::size_t)> handler)
{
	memset(data_, 0, MAX_LENGTH);
	AsyncReadLen(0, maxLength, handler);
}

// 递归读取指定字节数，异步非阻塞，能保证读完整指定字节
void CSession::AsyncReadLen(std::size_t read_len, std::size_t total_len,
	std::function<void(const boost::system::error_code&, std::size_t)> handler) {
	auto self = shared_from_this();
	// async_read_some 只是尝试读取数据，但实际读取的字节数并不确定，可能会少于预期的长度
	socket_.async_read_some(boost::asio::buffer(data_ + read_len, total_len - read_len),
		[read_len, total_len, handler, self](const boost::system::error_code& ec, std::size_t  bytesTransfered) {
			// bytesTransfered为本次读取到的字节数
			// 出现错误，调用回调函数
			if (ec) {
				handler(ec, read_len + bytesTransfered);
				return;
			}
			// 读取到指定字节数
			if (read_len + bytesTransfered >= total_len) {
				handler(ec, read_len + bytesTransfered);
				return;
			}

			// 未读取到指定字节数，继续读取
			self->AsyncReadLen(read_len + bytesTransfered, total_len, handler);
		});
}

void CSession::HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self) {
	try {
		if (!error) {
			std::lock_guard<std::mutex> lock(send_lock_);
			send_que_.pop();
			if (!send_que_.empty()) {
				auto msg_node = send_que_.front();
				boost::asio::async_write(socket_, boost::asio::buffer(msg_node->_data, msg_node->_total_len),
					std::bind(&CSession::HandleWrite, this, std::placeholders::_1, shared_self));
			}
		}
		else {
			LOGW("handle write failed, error is %s", error.what());
			Close();
			server_->ClearSession(session_id_);
		}
	}
	catch (std::exception e) {
		std::cerr << "Exception code : " << e.what() << std::endl;
	}
}
