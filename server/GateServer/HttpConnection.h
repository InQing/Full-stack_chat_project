#pragma once
#include "const.h"

class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
	friend class LogicSystem;
public:
	HttpConnection(net::io_context& ioc);
	void Start();
	tcp::socket& GetSocket() {
		return _socket;
	}
private:
	tcp::socket _socket;
	beast::flat_buffer _buffer{1024 * 8}; // 接收数据的缓存区
	http::request<http::dynamic_body> _request; // 完整的HTTP请求
	http::response<http::dynamic_body> _response; // 完整的HTTP回复
	net::steady_timer _deadline{
		_socket.get_executor(), std::chrono::seconds(60) }; // 超时定时器，检测服务端HTTP回复是否超时

	std::string _get_url;
	std::unordered_map<std::string, std::string> _get_params;

	void HandleReq(); // 处理Http请求
	void CheckDeadline(); // 检测连接是否超时
	void WriteResponse(); // 回复客户端

	unsigned char ToHex(unsigned char x); // char转16进制
	unsigned char FromHex(unsigned char x); // 16进制转char
	std::string UrlEncode(const std::string& str); // url编码
	std::string UrlDecode(const std::string& str); // url解码
	void PreParseGetParam(); // 解析url（带参）
};

