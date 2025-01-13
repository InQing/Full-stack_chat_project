#pragma once
#include "Singleton.h"
#include "Logger.h"
#include <hiredis/hiredis.h>

class RedisConPool
{
public:
    RedisConPool(std::size_t size, std::string host, std::string port, std::string pwd) :
        pool_size_(size), host_(host.c_str()), port_(atoi(port.c_str())), pwd_(pwd), is_stop_(false)
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

        // 启动心跳检测线程
        check_thread_ = std::thread([this]() {
            while (!is_stop_) {
                CheckConnection();
                std::this_thread::sleep_for(std::chrono::seconds(45));
            }
            });

        // 设置线程为分离状态
        check_thread_.detach();
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
    std::string pwd_;

    std::atomic<bool> is_stop_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::queue<std::unique_ptr<redisContext>> connections_;
    std::thread check_thread_;

    void CheckConnection() {
        std::vector<std::unique_ptr<redisContext>> conns;
        size_t conn_count = 0;

        // 获取当前所有连接
        {
            std::lock_guard<std::mutex> lock(mutex_);
            conn_count = connections_.size();
        }

        // 逐个检查连接
        for (size_t i = 0; i < conn_count; ++i) {
            // 获取一个连接
            auto conn = GetConnection();
            if (!conn) continue;

            // 发送PING命令
            auto reply = (redisReply*)redisCommand(conn.get(), "PING");
            if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
                // 连接失败，释放旧连接
                if (reply) {
                    freeReplyObject(reply);
                }

                // 创建新连接
                auto* context = redisConnect(host_, port_);
                if (context != nullptr && context->err == 0) {
                    auto auth_reply = (redisReply*)redisCommand(context, "AUTH %s", pwd_.c_str());
                    if (auth_reply && auth_reply->type != REDIS_REPLY_ERROR) {
                        // 释放旧连接并设置新连接
                        conn.reset(context);
                        freeReplyObject(auth_reply);
                    }
                    else {
                        if (auth_reply) {
                            freeReplyObject(auth_reply);
                        }
                        redisFree(context);
                    }
                }
            }
            else {
                freeReplyObject(reply);
            }

            // 归还连接
            if (conn) {
                ReturnConnection(std::move(conn));
            }
        }
    }
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

