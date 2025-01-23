package api

import (
	"resource-server/service"
	"resource-server/storage"

	"github.com/gin-gonic/gin"
)

// RegisterRoutes 注册所有路由
func RegisterRoutes(r *gin.Engine, tempDir string, storage storage.Storage) {
	// 初始化上传处理器
	uploadService := service.NewUploadService(tempDir, storage)
	uploadHandler := NewUploadHandler(uploadService)

	// 初始化下载处理器
	downloadService := service.NewDownloadService(storage)
	downloadHandler := NewDownloadHandler(downloadService)

	// 上传相关路由
	r.POST("/upload", uploadHandler.Upload)  // 使用Upload处理JSON格式的请求

	// 下载相关路由
	r.GET("/download/:fileId", downloadHandler.Download)
}

// TODO: 实现具体的处理函数
