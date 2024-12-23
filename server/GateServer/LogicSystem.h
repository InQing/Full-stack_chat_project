#pragma once
#include "Singleton.h"
#include "const.h"
#include <functional>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

class HttpConnection;
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;

class LogicSystem : public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem() {};
	bool HandleGet(std::string url, std::shared_ptr<HttpConnection> connection); // 处理get请求
	void RegGet(std::string url, HttpHandler handler); // 注册get请求
	bool HandlePost(std::string url, std::shared_ptr<HttpConnection> connection); // 处理post请求
	void RegPost(std::string url, HttpHandler handler); // 注册post请求

private:
	LogicSystem();
	std::unordered_map<std::string, HttpHandler> _post_handlers;
	std::unordered_map<std::string, HttpHandler> _get_handlers;
};

