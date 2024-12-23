#pragma once

#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "const.h"
#include "Singleton.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;
using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService;

class VarifyConPool
{
public:
	VarifyConPool(std::size_t size, std::string host, std::string port) :
		pool_size_(size), host_(host), port_(port), is_stop_(false)
	{
		for (std::size_t i = 0; i < pool_size_; ++i) {
			std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
				grpc::InsecureChannelCredentials());

			stubs_.push(VarifyService::NewStub(channel));
		}
	}

	~VarifyConPool()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		Close();
		while (!stubs_.empty()) {
			stubs_.pop();
		}
	}

	std::unique_ptr<VarifyService::Stub> GetConnection()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this]() {
			if (is_stop_) {
				return true;
			}
			return !stubs_.empty();
			});
		if (is_stop_) {
			return nullptr;
		}
		auto con = std::move(stubs_.front());
		stubs_.pop();
		return con;
	}

	void ReturnConnection(std::unique_ptr<VarifyService::Stub> con)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (is_stop_) {
			return;
		}
		stubs_.push(std::move(con));
		cond_.notify_one();
	}

	void Close()
	{
		is_stop_ = true;
		cond_.notify_all();
	}
private:
	std::size_t pool_size_;
	std::string host_;
	std::string port_;
	std::atomic<bool> is_stop_;
	std::queue<std::unique_ptr<VarifyService::Stub>> stubs_;
	std::mutex mutex_;
	std::condition_variable cond_;
};




class VarifyGrpcClient : public Singleton<VarifyGrpcClient>
{
	friend class Singleton<VarifyGrpcClient>;
public:
	GetVarifyRsp GetVarifyCode(std::string email);

private:
	VarifyGrpcClient();

	std::unique_ptr<VarifyConPool> con_pool_;
};