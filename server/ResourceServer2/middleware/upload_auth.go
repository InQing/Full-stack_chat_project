package middleware

import (
	"bytes"
	"io"
	"log"
	"net/http"
	"resource-server/interfaces"
	"resource-server/models"

	"github.com/gin-gonic/gin"
)

// UploadIDAuth 验证上传请求中的uploadID
func UploadIDAuth(uploadService interfaces.UploadService) gin.HandlerFunc {
	return func(c *gin.Context) {
		log.Printf("=== 开始验证Upload-ID ===")

		uploadID := c.GetHeader("Upload-ID")
		log.Printf("收到的Upload-ID: %s", uploadID)
		if uploadID == "" {
			log.Printf("错误: 未提供Upload-ID")
			c.JSON(http.StatusUnauthorized, gin.H{
				"code": http.StatusUnauthorized,
				"msg":  "未提供Upload-ID",
			})
			c.Abort()
			return
		}

		// 读取请求体
		bodyBytes, err := io.ReadAll(c.Request.Body)
		if err != nil {
			log.Printf("错误: 读取请求体失败: %v", err)
			c.JSON(http.StatusBadRequest, gin.H{
				"code": http.StatusBadRequest,
				"msg":  "读取请求失败",
			})
			c.Abort()
			return
		}

		// 重新设置请求体，供后续使用
		c.Request.Body = io.NopCloser(bytes.NewBuffer(bodyBytes))

		// 从请求体中获取fileId
		var reqData struct {
			FileID string `json:"file_id"`
		}
		if err := c.ShouldBindJSON(&reqData); err != nil {
			log.Printf("错误: 解析请求体失败: %v", err)
			c.JSON(http.StatusBadRequest, gin.H{
				"code": http.StatusBadRequest,
				"msg":  "无效的请求参数",
			})
			c.Abort()
			return
		}

		// 再次重新设置请求体，供后续处理函数使用
		c.Request.Body = io.NopCloser(bytes.NewBuffer(bodyBytes))

		// 从uploadStatusMap中获取状态
		statusInterface, ok := uploadService.GetUploadStatus(reqData.FileID)
		if !ok {
			log.Printf("错误: 未找到上传状态")
			c.JSON(http.StatusUnauthorized, gin.H{
				"code": http.StatusUnauthorized,
				"msg":  "无效的上传状态",
			})
			c.Abort()
			return
		}

		status := statusInterface.(*models.UploadStatus)
		if status.UploadID != uploadID {
			log.Printf("错误: Upload-ID不匹配，期望: %s，实际: %s", status.UploadID, uploadID)
			c.JSON(http.StatusUnauthorized, gin.H{
				"code": http.StatusUnauthorized,
				"msg":  "无效的Upload-ID",
			})
			c.Abort()
			return
		}

		log.Printf("Upload-ID验证成功")
		c.Next()
	}
}
