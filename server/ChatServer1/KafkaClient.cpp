#include "KafkaClient.h"
#include "ConfigMgr.h"
#include <iostream>
#include <thread>
#include <chrono>

KafkaClient* KafkaClient::instance_ = nullptr;

void KafkaEventCallback::event_cb(RdKafka::Event& event)
{
    switch (event.type())
    {
    case RdKafka::Event::EVENT_ERROR:
        std::cout << "Kafka错误: " << event.str() << std::endl;
        break;
    default:
        std::cout << "Kafka事件: " << event.type() << " " << event.str() << std::endl;
        break;
    }
}

KafkaClient::KafkaClient()
{
    auto& cfg = ConfigMgr::GetInstance();

    // 从配置文件读取Kafka配置
    auto host = cfg["Kafka"]["Host"];
    auto port = cfg["Kafka"]["Port"];
    auto initial_size = std::thread::hardware_concurrency();
    auto max_size = initial_size * 3;

    // 构造broker连接字符串
    std::string brokers = host + ":" + port;

    // 初始化连接池
    if (!InitPool(brokers, initial_size, max_size))
    {
        std::cout << "Kafka连接池初始化失败" << std::endl;
    }
}

bool KafkaClient::InitPool(const std::string& brokers, size_t initial_size, size_t max_size)
{
    brokers_ = brokers;
    max_pool_size_ = max_size;

    // 预创建生产者连接
    for (size_t i = 0; i < initial_size; ++i)
    {
        auto conn = CreateProducerConnection();
        if (conn)
        {
            producer_pool_.push(conn);
        }
    }

    // 启动定期清理空闲连接的线程
    std::thread cleanup_thread([this]()
        {
            while (running_)
            {
                std::this_thread::sleep_for(std::chrono::minutes(5));
                CleanIdleConnections();
            } });
    cleanup_thread.detach();

    return !producer_pool_.empty();
}

KafkaClient::~KafkaClient()
{
    running_ = false;

    std::lock_guard<std::mutex> producer_lock(producer_mutex_);
    while (!producer_pool_.empty())
    {
        auto conn = producer_pool_.front();
        producer_pool_.pop();
        if (conn->producer)
        {
            conn->producer->flush(10000);
            delete conn->producer;
        }
        if (conn->conf)
        {
            delete conn->conf;
        }
    }

    std::lock_guard<std::mutex> consumer_lock(consumer_mutex_);
    for (auto& pair : consumer_pool_)
    {
        auto conn = pair.second;
        if (conn->consumer)
        {
            conn->consumer->close();
            delete conn->consumer;
        }
        if (conn->conf)
        {
            delete conn->conf;
        }
    }
    consumer_pool_.clear();
}

std::shared_ptr<KafkaConnection> KafkaClient::CreateProducerConnection()
{
    auto conn = std::make_shared<KafkaConnection>();
    std::string errstr;

    conn->conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    if (conn->conf->set("bootstrap.servers", brokers_, errstr) != RdKafka::Conf::CONF_OK)
    {
        std::cout << "生产者配置错误: " << errstr << std::endl;
        delete conn->conf;
        return nullptr;
    }

    conn->producer = RdKafka::Producer::create(conn->conf, errstr);
    if (!conn->producer)
    {
        std::cout << "创建生产者失败: " << errstr << std::endl;
        delete conn->conf;
        return nullptr;
    }

    return conn;
}

std::shared_ptr<KafkaConnection> KafkaClient::CreateConsumerConnection(const std::string& group_id)
{
    auto conn = std::make_shared<KafkaConnection>();
    std::string errstr;

    conn->conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    if (conn->conf->set("bootstrap.servers", brokers_, errstr) != RdKafka::Conf::CONF_OK ||
        conn->conf->set("group.id", group_id, errstr) != RdKafka::Conf::CONF_OK ||
        conn->conf->set("enable.auto.commit", "true", errstr) != RdKafka::Conf::CONF_OK ||
        conn->conf->set("auto.offset.reset", "earliest", errstr) != RdKafka::Conf::CONF_OK)
    {
        std::cout << "消费者配置错误: " << errstr << std::endl;
        delete conn->conf;
        return nullptr;
    }

    conn->conf->set("event_cb", &event_cb_, errstr);
    conn->consumer = RdKafka::KafkaConsumer::create(conn->conf, errstr);
    if (!conn->consumer)
    {
        std::cout << "创建消费者失败: " << errstr << std::endl;
        delete conn->conf;
        return nullptr;
    }

    conn->group_id = group_id;
    return conn;
}

std::shared_ptr<KafkaConnection> KafkaClient::GetProducerConnection()
{
    std::lock_guard<std::mutex> lock(producer_mutex_);

    if (!producer_pool_.empty())
    {
        auto conn = producer_pool_.front();
        producer_pool_.pop();
        conn->in_use = true;
        conn->last_used = std::chrono::system_clock::now();
        return conn;
    }

    if (producer_pool_.size() < max_pool_size_)
    {
        auto conn = CreateProducerConnection();
        if (conn)
        {
            conn->in_use = true;
            conn->last_used = std::chrono::system_clock::now();
        }
        return conn;
    }

    return nullptr;
}

std::shared_ptr<KafkaConnection> KafkaClient::GetConsumerConnection(const std::string& group_id)
{
    std::lock_guard<std::mutex> lock(consumer_mutex_);

    auto it = consumer_pool_.find(group_id);
    if (it != consumer_pool_.end())
    {
        return it->second;
    }

    if (consumer_pool_.size() < max_pool_size_)
    {
        auto conn = CreateConsumerConnection(group_id);
        if (conn)
        {
            conn->in_use = true;
            conn->last_used = std::chrono::system_clock::now();
            consumer_pool_[group_id] = conn;
            return conn;
        }
    }

    return nullptr;
}

void KafkaClient::ReleaseConnection(std::shared_ptr<KafkaConnection> conn)
{
    if (!conn)
        return;

    conn->in_use = false;
    conn->last_used = std::chrono::system_clock::now();

    if (conn->producer)
    {
        std::lock_guard<std::mutex> lock(producer_mutex_);
        producer_pool_.push(conn);
    }
}

bool KafkaClient::StoreOfflineMessage(const std::string& topic, const std::string& message)
{
    auto conn = GetProducerConnection();
    if (!conn || !conn->producer)
    {
        std::cout << "无法获取生产者连接" << std::endl;
        return false;
    }

    RdKafka::ErrorCode err = conn->producer->produce(
        topic,
        RdKafka::Topic::PARTITION_UA,
        RdKafka::Producer::RK_MSG_COPY,
        const_cast<char*>(message.c_str()),
        message.size(),
        nullptr, 0,
        0,
        nullptr);

    if (err != RdKafka::ERR_NO_ERROR)
    {
        std::cout << "存储离线消息失败: " << RdKafka::err2str(err) << std::endl;
        ReleaseConnection(conn);
        return false;
    }

    conn->producer->poll(0);
    ReleaseConnection(conn);
    return true;
}

void KafkaClient::ConsumeOfflineMessages(const std::string& topic,
    const std::string& group_id,
    MessageCallback msg_callback,
    CompletionCallback completion_callback)
{
    auto conn = GetConsumerConnection(group_id);
    if (!conn || !conn->consumer)
    {
        std::cout << "无法获取消费者连接" << std::endl;
        return;
    }

    std::vector<std::string> topics = { topic };
    RdKafka::ErrorCode err = conn->consumer->subscribe(topics);
    if (err != RdKafka::ERR_NO_ERROR)
    {
        std::cout << "订阅主题失败: " << RdKafka::err2str(err) << std::endl;
        return;
    }

    int empty_count = 0;           // 用于记录连续空消息的次数
    const int MAX_EMPTY_COUNT = 5; // 连续5次没有消息则认为消费完成

    while (running_)
    {
        RdKafka::Message* msg = conn->consumer->consume(1000);

        switch (msg->err())
        {
        case RdKafka::ERR_NO_ERROR:
        {
            empty_count = 0; // 重置计数器
            std::string payload(static_cast<const char*>(msg->payload()), msg->len());
            msg_callback(payload);
        }
        break;

        case RdKafka::ERR__TIMED_OUT:
            empty_count++;
            if (empty_count >= MAX_EMPTY_COUNT)
            {
                // 连续多次没有新消息，认为消费完成
                delete msg;

                // 调用完成回调
                if (completion_callback)
                {
                    completion_callback();
                }

                // 停止消费并回收连接
                StopConsume(group_id);
                return;
            }
            break;

        case RdKafka::ERR__PARTITION_EOF:
            empty_count++;
            std::cout << "到达分区末尾" << std::endl;
            break;

        default:
            std::cout << "消费错误: " << msg->errstr() << std::endl;
            break;
        }

        delete msg;
    }
}

void KafkaClient::StopConsume(const std::string& group_id)
{
    std::lock_guard<std::mutex> lock(consumer_mutex_);
    auto it = consumer_pool_.find(group_id);
    if (it != consumer_pool_.end())
    {
        if (it->second->consumer)
        {
            it->second->consumer->close();
            delete it->second->consumer;
        }
        if (it->second->conf)
        {
            delete it->second->conf;
        }
        consumer_pool_.erase(it);
    }
}

void KafkaClient::CleanIdleConnections(int idle_seconds)
{
    std::lock_guard<std::mutex> cleanup_lock(cleanup_mutex_);
    auto now = std::chrono::system_clock::now();

    // 清理空闲的生产者连接
    {
        std::lock_guard<std::mutex> producer_lock(producer_mutex_);
        std::queue<std::shared_ptr<KafkaConnection>> temp_queue;
        while (!producer_pool_.empty())
        {
            auto conn = producer_pool_.front();
            producer_pool_.pop();

            auto idle_duration = std::chrono::duration_cast<std::chrono::seconds>(
                now - conn->last_used)
                .count();

            if (!conn->in_use && idle_duration > idle_seconds)
            {
                if (conn->producer)
                {
                    conn->producer->flush(1000);
                    delete conn->producer;
                }
                if (conn->conf)
                {
                    delete conn->conf;
                }
            }
            else
            {
                temp_queue.push(conn);
            }
        }
        producer_pool_ = std::move(temp_queue);
    }

    // 清理空闲的消费者连接
    {
        std::lock_guard<std::mutex> consumer_lock(consumer_mutex_);
        std::vector<std::string> to_remove;
        for (const auto& pair : consumer_pool_)
        {
            auto idle_duration = std::chrono::duration_cast<std::chrono::seconds>(
                now - pair.second->last_used)
                .count();

            if (!pair.second->in_use && idle_duration > idle_seconds)
            {
                to_remove.push_back(pair.first);
            }
        }

        for (const auto& group_id : to_remove)
        {
            StopConsume(group_id);
        }
    }
}

KafkaClient* KafkaClient::GetInstance()
{
    static KafkaClient instance; // 使用局部静态变量确保线程安全的初始化
    return &instance;
}