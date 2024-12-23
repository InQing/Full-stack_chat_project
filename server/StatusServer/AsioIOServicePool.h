#pragma once
#include <boost/asio.hpp>
#include "Singleton.h"
class AsioIOServicePool : public Singleton<AsioIOServicePool>
{
	friend class Singleton<AsioIOServicePool>;
public:
	using IOService = net::io_context;
	using Work = net::io_context::work;
	using WorkPtr = std::unique_ptr<Work>;

	~AsioIOServicePool();
	// 用轮询方式得到下一个IOService
	IOService& GetIOService();
	void Stop();
private:
	// 初始化IOService的个数，默认为CPU核心数
	AsioIOServicePool(std::size_t size = std::thread::hardware_concurrency());
	std::vector<IOService> ioservices_;
	std::vector<WorkPtr> works_;
	std::vector<std::thread> threads_;
	std::size_t next_ioservice_;
};

