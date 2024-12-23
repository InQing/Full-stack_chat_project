#pragma once
#include "Singleton.h"
#include "Logger.h"
#include <hiredis/hiredis.h>

class RedisConPool
{
public:
    RedisConPool(std::size_t size, std::string host, std::string port, std::string pwd) :
        pool_size_(size), host_(host.c_str()), port_(atoi(port.c_str())), is_stop_(false)
    {
        for (std::size_t i = 0; i < pool_size_; ++i) {
            auto* context = redisConnect(host_, port_);
            if (context == nullptr || context->err != 0) {
                if (context != nullptr) {
                    redisFree(context);
                }
                continue;
            }

            auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd.c_str());
            if (reply->type == REDIS_REPLY_ERROR) {
                //执行成功 释放redisCommand执行后返回的redisReply所占用的内存
                freeReplyObject(reply);
                continue;
            }

            //执行成功 释放redisCommand执行后返回的redisReply所占用的内存
            freeReplyObject(reply);
            connections_.push(std::unique_ptr<redisContext>(context));
        }

        if (connections_.size() == pool_size_) {
            LOGI("RedisConPool: Redis connect succeed!");
        }
        else {
            LOGW("RedisConPool: Redis connect failed!");
        }
    }

    ~RedisConPool()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        while (connections_.size()) {
            connections_.pop();
        }
    }

    std::unique_ptr<redisContext> GetConnection()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this]() {
            return is_stop_ || !connections_.empty();
            });
        if (is_stop_) {
            return nullptr;
        }
        auto con = std::move(connections_.front());
        connections_.pop();
        return con;
    }

    void ReturnConnection(std::unique_ptr<redisContext> con)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (is_stop_) {
            return;
        }
        connections_.push(std::move(con));
        cond_.notify_one();
    }

    void Close()
    {
        is_stop_ = true;
        cond_.notify_all();
    }
private:
    std::size_t pool_size_;
    const char* host_;
    int port_;

    std::atomic<bool> is_stop_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::queue<std::unique_ptr<redisContext>> connections_;
};



class RedisMgr : public Singleton<RedisMgr>, public std::enable_shared_from_this<RedisMgr>
{
	friend class Singleton<RedisMgr>;
public:
    ~RedisMgr();
    bool Get(const std::string& key, std::string& value);
    bool Set(const std::string& key, const std::string& value);
    bool LPush(const std::string& key, const std::string& value);
    bool LPop(const std::string& key, std::string& value);
    bool RPush(const std::string& key, const std::string& value);
    bool RPop(const std::string& key, std::string& value);
    bool HSet(const std::string& key, const std::string& hkey, const std::string& value);
    bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
    std::string HGet(const std::string& key, const std::string& hkey);
    bool Del(const std::string& key);
    bool ExistsKey(const std::string& key);
    void Close();
private:
    RedisMgr();
    std::unique_ptr<RedisConPool> con_pool_;
};

