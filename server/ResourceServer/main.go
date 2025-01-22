package main

import (
	"fmt"
	"log"

	"resource-server/api"
	"resource-server/config"
	"resource-server/middleware"
	"resource-server/storage"

	"github.com/gin-gonic/gin"
)

func main() {
	// 加载配置文件
	if err := config.Init("config/config.yaml"); err != nil {
		log.Fatalf("Failed to load config: %v", err)
	}

	// 创建 Gin 引擎
	r := gin.Default()

	// 添加中间件
	r.Use(middleware.LogMiddleware()) // 添加日志中间件

	// 创建存储实例
	cfg := config.Get()
	storage, err := storage.NewStorage(cfg)
	if err != nil {
		log.Fatalf("Failed to create storage: %v", err)
	}

	// 注册路由
	api.RegisterRoutes(r, cfg.Server.UploadDir, storage)

	// 启动服务器
	serverConfig := config.GetServer()
	addr := fmt.Sprintf("%s:%d", serverConfig.Host, serverConfig.Port)
	middleware.Info("Server starting at %s", addr)
	if err := r.Run(addr); err != nil {
		middleware.Error("Server failed to start: %v", err)
		log.Fatalf("Server failed to start: %v", err)
	}
}
