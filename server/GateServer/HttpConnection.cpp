#include "HttpConnection.h"
#include "LogicSystem.h"
#include "const.h"
#include "Logger.h"

HttpConnection::HttpConnection(net::io_context& ioc) : _socket(ioc) {}

void HttpConnection::Start()
{
	auto self = shared_from_this();
	// 异步地读取客户端发来的HTTP数据包，并回调处理
	http::async_read(_socket, _buffer, _request, [self](beast::error_code ec,
		std::size_t bytes_transferred) {
			try {
				if (ec) {
					LOGE("HttpConnection: Receive failed, error is %s", ec.what());
					return;
				}

				LOGI("HttpConnection: Received request");
				boost::ignore_unused(bytes_transferred); // 忽略bytes_transferred未使用的警告
				self->HandleReq(); // 处理请求
				self->CheckDeadline(); // 检测服务端Http回复是否超时
			}
			catch (std::exception& exc) {
				std::cout << "exception is" << exc.what() << std::endl;
			}
		});
}

void HttpConnection::HandleReq()
{
	// 设置HTTP版本
	_response.version(_request.version());
	// 设置HTTP为短连接，回复后断开TCP连接
	_response.keep_alive(false);

	// 处理GET请求
	if (_request.method() == http::verb::get) {
		PreParseGetParam();
		bool success = LogicSystem::GetInstance()->HandleGet(_get_url, shared_from_this());
		if (!success) {
			LOGW("HttpConnection: Handle get error! url is '% s'", _get_url);
			_response.result(http::status::not_found); // 状态码：404
			_response.set(http::field::content_type, "text/plain"); // 回复类型：plain
			beast::ostream(_response.body()) << "url not found\r\n"; // 请求体
			WriteResponse();
			return;
		}

		LOGI("HttpConnection: Handle get succeed, url is '% s'", _get_url);
		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");
		WriteResponse();
		return;
	}

	// 处理POST请求
	if (_request.method() == http::verb::post) {
		std::string post_url = _request.target();
		bool success = LogicSystem::GetInstance()->HandlePost(_request.target(), shared_from_this());
		if (!success) {
			LOGW("HttpConnection: Handle post error! url is '%s'", post_url);
			_response.result(http::status::not_found);
			_response.set(http::field::content_type, "text/plain");
			beast::ostream(_response.body()) << "url not found\r\n";
			WriteResponse();
			return;
		}

		LOGI("HttpConnection: Handle post succeed, url is '%s'", post_url);
		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");
		WriteResponse();
		return;
	}
}

void HttpConnection::CheckDeadline()
{
	auto self = shared_from_this();

	_deadline.async_wait([self](beast::error_code ec) {
		if (!ec) {
			// HTTP连接超时，关闭socket
			self->_socket.close(ec);
			LOGW("HttpConnection: Http connection time out!");
		}
	});
}

void HttpConnection::WriteResponse()
{
	auto self = shared_from_this();
	_response.content_length(_response.body().size());
	// 异步回复给客户端
	http::async_write(_socket, _response, [self](beast::error_code ec, std::size_t) {
		// HTTP是短连接，回复完后关闭连接
		self->_socket.shutdown(tcp::socket::shutdown_send, ec);
		// 关闭超时定时器
		self->_deadline.cancel();
		});
}

unsigned char HttpConnection::ToHex(unsigned char x)
{
	return  x > 9 ? x + 55 : x + 48;
}

unsigned char HttpConnection::FromHex(unsigned char x)
{
	unsigned char y;
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else if (x >= '0' && x <= '9') y = x - '0';
	else assert(0);
	return y;
}

std::string HttpConnection::UrlEncode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//判断是否仅有数字和字母构成
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ') //为空字符
			strTemp += "+";
		else
		{
			//其他字符需要提前加%并且高四位和低四位分别转为16进制
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);
			strTemp += ToHex((unsigned char)str[i] & 0x0F);
		}
	}
	return strTemp;
}

std::string HttpConnection::UrlDecode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//还原+为空
		if (str[i] == '+') strTemp += ' ';
		//遇到%将后面的两个字符从16进制转为char再拼接
		else if (str[i] == '%')
		{
			assert(i + 2 < length);
			unsigned char high = FromHex((unsigned char)str[++i]);
			unsigned char low = FromHex((unsigned char)str[++i]);
			strTemp += high * 16 + low;
		}
		else strTemp += str[i];
	}
	return strTemp;
}

void HttpConnection::PreParseGetParam()
{
	// 提取 URI  
	auto uri = _request.target();
	// 查找查询字符串的开始位置（即 '?' 的位置）  
	auto query_pos = uri.find('?');
	if (query_pos == std::string::npos) {
		_get_url = uri;
		return;
	}

	_get_url = uri.substr(0, query_pos);
	std::string query_string = uri.substr(query_pos + 1);
	std::string key;
	std::string value;
	size_t pos = 0;
	while ((pos = query_string.find('&')) != std::string::npos) {
		auto pair = query_string.substr(0, pos);
		size_t eq_pos = pair.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(pair.substr(0, eq_pos)); // 假设有 url_decode 函数来处理URL解码  
			value = UrlDecode(pair.substr(eq_pos + 1));
			_get_params[key] = value;
		}
		query_string.erase(0, pos + 1);
	}
	// 处理最后一个参数对（如果没有 & 分隔符）  
	if (!query_string.empty()) {
		size_t eq_pos = query_string.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(query_string.substr(0, eq_pos));
			value = UrlDecode(query_string.substr(eq_pos + 1));
			_get_params[key] = value;
		}
	}
}
