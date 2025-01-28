package middleware

import (
	"context"
	"fmt"
	"log"
	"resource-server/config"

	"github.com/redis/go-redis/v9"
)

var RedisClient *redis.Client

// InitRedis 初始化Redis连接
func InitRedis(cfg *config.Config) error {
	log.Printf("=== 初始化Redis连接 ===")
	log.Printf("Redis配置: %s:%d", cfg.Redis.Host, cfg.Redis.Port)

	RedisClient = redis.NewClient(&redis.Options{
		Addr:     fmt.Sprintf("%s:%d", cfg.Redis.Host, cfg.Redis.Port),
		Password: cfg.Redis.Password,
		DB:       cfg.Redis.DB,
	})

	// 测试连接
	ctx := context.Background()
	if _, err := RedisClient.Ping(ctx).Result(); err != nil {
		log.Printf("错误: Redis连接失败: %v", err)
		return err
	}

	log.Printf("Redis连接成功")
	return nil
}

// GetToken 从Redis获取用户token
func GetToken(uid string) (string, error) {
	log.Printf("=== 从Redis获取Token ===")
	log.Printf("用户ID: %s", uid)

	ctx := context.Background()
	redisKey := fmt.Sprintf("utoken_%s", uid)
	log.Printf("Redis键: %s", redisKey)

	token, err := RedisClient.Get(ctx, redisKey).Result()
	if err != nil {
		log.Printf("错误: 获取Token失败: %v", err)
		return "", err
	}

	log.Printf("获取Token成功")
	return token, nil
}
