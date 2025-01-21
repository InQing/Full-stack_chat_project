package main

import (
	"log"

	"github.com/gin-gonic/gin"
)

func main() {
	// 初始化 Gin 引擎
	r := gin.Default()

	// TODO: 初始化配置

	// TODO: 注册路由

	// TODO: 注册中间件

	// 启动服务器
	if err := r.Run(":8080"); err != nil {
		log.Fatalf("服务器启动失败: %v", err)
	}
}
