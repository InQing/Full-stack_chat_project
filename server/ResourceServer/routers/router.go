package routers

import (
	"github.com/gin-gonic/gin"
)

// SetupRouter 设置路由
func SetupRouter(r *gin.Engine) {
	// 文件上传路由
	r.POST("/upload", func(c *gin.Context) {
		// TODO: 实现文件上传处理
		c.JSON(200, gin.H{"message": "upload endpoint"})
	})

	// 文件下载路由
	r.GET("/download/:fileId", func(c *gin.Context) {
		// TODO: 实现文件下载处理
		c.JSON(200, gin.H{"message": "download endpoint"})
	})
}
