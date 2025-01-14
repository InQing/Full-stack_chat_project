const config_module = require('./config')
const Redis = require("ioredis");

// 重连配置
const RECONNECT_INTERVAL = 5000; // 重连间隔时间，5秒
const MAX_RECONNECT_ATTEMPTS = 0; // 0表示无限重试

let RedisCli = null;

// 创建Redis客户端的函数
function createRedisClient() {
    const client = new Redis({
        host: config_module.redis_host,
        port: config_module.redis_port,
        password: config_module.redis_passwd,
        retryStrategy(times) {
            // 重试策略
            if (MAX_RECONNECT_ATTEMPTS > 0 && times > MAX_RECONNECT_ATTEMPTS) {
                console.log('Max reconnection attempts reached, giving up');
                return null;
            }
            console.log(`Retry attempt ${times}, reconnecting in ${RECONNECT_INTERVAL}ms`);
            return RECONNECT_INTERVAL;
        },
        reconnectOnError(err) {
            // 决定是否在发生错误时重新连接
            const targetError = 'READONLY';
            if (err.message.includes(targetError)) {
                return true;
            }
            return false;
        }
    });

    // 监听连接事件
    client.on('connect', () => {
        console.log('Redis connected successfully');
    });

    client.on('ready', () => {
        console.log('Redis client ready');
    });

    client.on('error', (err) => {
        console.error('Redis error:', err);
    });

    client.on('close', () => {
        console.log('Redis connection closed');
    });

    client.on('reconnecting', (delay) => {
        console.log(`Redis reconnecting in ${delay}ms`);
    });

    client.on('end', () => {
        console.log('Redis connection ended');
    });

    return client;
}

// 初始化Redis客户端
RedisCli = createRedisClient();

// 发送心跳机制，确保连接不被断开
const HEARTBEAT_INTERVAL = 30 * 1000;
const heartbeatTimer = setInterval(async () => {
    try {
        await RedisCli.ping();
        // console.log("Redis heartbeat sent successfully");
    } catch (err) {
        console.error("Redis heartbeat failed", err);
    }
}, HEARTBEAT_INTERVAL);

/**
 * 确保Redis连接的包装函数
 */
async function ensureConnection(operation) {
    try {
        if (!RedisCli || !['connecting', 'connect', 'ready'].includes(RedisCli.status)) {
          console.log('Redis not connected or in an invalid state, reconnecting...');
          RedisCli = createRedisClient();
      }

        return await operation();
    } catch (error) {
        console.error('Redis operation failed:', error);
        throw error;
    }
}

/**
 * 根据key获取value
 */
async function GetRedis(key) {
    return ensureConnection(async () => {
        try {
            const result = await RedisCli.get(key);
            if (result === null) {
                console.log('result:', '<' + result + '>', 'This key cannot be found...');
                return null;
            }
            console.log('Result:', '<' + result + '>', 'Get key success!...');
            return result;
        } catch (error) {
            console.log('GetRedis error is', error);
            return null;
        }
    });
}

/**
 * 根据key查询redis中是否存在key
 */
async function QueryRedis(key) {
    return ensureConnection(async () => {
        try {
            const result = await RedisCli.exists(key);
            if (result === 0) {
                console.log('result:', '<' + result + '>', 'This key is null...');
                return null;
            }
            console.log('Result:', '<' + result + '>', 'With this value!...');
            return result;
        } catch (error) {
            console.log('QueryRedis error is', error);
            return null;
        }
    });
}

/**
 * 设置key和value，并过期时间
 */
async function SetRedisExpire(key, value, exptime) {
    return ensureConnection(async () => {
        try {
            await RedisCli.set(key, value);
            await RedisCli.expire(key, exptime);
            return true;
        } catch (error) {
            console.log('SetRedisExpire error is', error);
            return false;
        }
    });
}

/**
 * 优雅退出函数
 */
function Quit() {
    clearInterval(heartbeatTimer); // 清除心跳定时器
    if (RedisCli) {
        RedisCli.quit().then(() => {
            console.log('Redis connection closed gracefully');
        }).catch((err) => {
            console.error('Error closing Redis connection:', err);
        });
    }
}

module.exports = { GetRedis, QueryRedis, Quit, SetRedisExpire };