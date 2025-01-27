package api

import (
	"resource-server/middleware"
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

	// 上传相关路由组
	uploadGroup := r.Group("/upload")
	{
		// 初始化上传接口 - 使用token验证
		uploadGroup.POST("/init", middleware.TokenAuth(), uploadHandler.InitUpload)

		// 分片上传接口 - 使用uploadID验证
		uploadGroup.POST("/chunk", middleware.UploadIDAuth(uploadService), uploadHandler.Upload)
	}

	// 下载相关路由
	r.GET("/download/:fileId", middleware.DownloadTokenAuth(), downloadHandler.Download)
}
