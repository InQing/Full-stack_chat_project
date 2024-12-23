#include "AsioIOServicePool.h"

AsioIOServicePool::~AsioIOServicePool()
{
	Stop();
}

net::io_context& AsioIOServicePool::GetIOService()
{
	auto& service = ioservices_[next_ioservice_++];
	if (next_ioservice_ == ioservices_.size()) {
		next_ioservice_ = 0;
	}
	return service;
}

void AsioIOServicePool::Stop()
{
	//因为仅仅执行work.reset并不能让iocontext从run的状态中退出
	//当iocontext已经绑定了读或写的监听事件后，还需要手动stop该服务。
	for (auto& work : works_) {
		work->get_io_context().stop();
		work.reset();
	}

	// 回收线程
	for (auto& t : threads_) {
		t.join();
	}
}

AsioIOServicePool::AsioIOServicePool(std::size_t size) :
	ioservices_(size), works_(size), next_ioservice_(0) {
	for (std::size_t i = 0; i < size; ++i) {
		works_[i] = std::make_unique<Work>(ioservices_[i]);
	}
	//创建size个线程，每个线程内部启动ioservice
	for (std::size_t i = 0; i < size; ++i) {
		threads_.emplace_back([this, i]() {
			ioservices_[i].run();
		});
	}
}