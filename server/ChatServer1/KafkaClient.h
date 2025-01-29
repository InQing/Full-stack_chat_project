#pragma once
#include <librdkafka/rdkafkacpp.h>
#include <string>
#include <memory>
#include <functional>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <chrono>
#include <atomic>

// Kafka事件回调类
class KafkaEventCallback : public RdKafka::EventCb
{
public:
    void event_cb(RdKafka::Event& event) override;
};

// Kafka连接对象
struct KafkaConnection
{
    RdKafka::Producer* producer{nullptr};
    RdKafka::KafkaConsumer* consumer{nullptr};
    RdKafka::Conf* conf{nullptr};
    std::string group_id;
    bool in_use{ false };
    std::chrono::system_clock::time_point last_used;
};

// 在KafkaClient.h中添加回调函数类型定义
using MessageCallback = std::function<void(const std::string&)>;
using CompletionCallback = std::function<void()>;

class KafkaClient
{
public:
    static KafkaClient* GetInstance();

    // 存储离线消息
    bool StoreOfflineMessage(const std::string& topic, const std::string& message);

    // 消费离线消息
    void ConsumeOfflineMessages(const std::string& topic,
        const std::string& group_id,
        MessageCallback msg_callback,
        CompletionCallback completion_callback = nullptr);

    // 停止消费
    void StopConsume(const std::string& group_id);

    // 清理空闲连接
    void CleanIdleConnections(int idle_seconds = 300);

private:
    KafkaClient();
    ~KafkaClient();

    // 初始化连接池
    bool InitPool(const std::string& brokers, size_t initial_size, size_t max_size);

    // 创建生产者连接
    std::shared_ptr<KafkaConnection> CreateProducerConnection();
    // 创建消费者连接
    std::shared_ptr<KafkaConnection> CreateConsumerConnection(const std::string& group_id);

    // 从连接池获取生产者连接
    std::shared_ptr<KafkaConnection> GetProducerConnection();
    // 从连接池获取消费者连接
    std::shared_ptr<KafkaConnection> GetConsumerConnection(const std::string& group_id);

    // 释放连接回连接池
    void ReleaseConnection(std::shared_ptr<KafkaConnection> conn);

private:
    static KafkaClient* instance_;
    std::string brokers_;
    size_t max_pool_size_{ 20 };

    std::mutex producer_mutex_;
    std::mutex consumer_mutex_;
    std::mutex cleanup_mutex_;

    // 生产者连接池
    std::queue<std::shared_ptr<KafkaConnection>> producer_pool_;
    // 消费者连接池 (group_id -> connection)
    std::unordered_map<std::string, std::shared_ptr<KafkaConnection>> consumer_pool_;

    KafkaEventCallback event_cb_;
    std::atomic<bool> running_{true};
};