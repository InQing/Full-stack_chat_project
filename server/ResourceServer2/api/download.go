package api

import (
	"fmt"
	"io"
	"log"
	"net/http"
	"resource-server/service"

	"github.com/gin-gonic/gin"
)

type DownloadHandler struct {
	downloadService *service.DownloadService
}

func NewDownloadHandler(downloadService *service.DownloadService) *DownloadHandler {
	return &DownloadHandler{
		downloadService: downloadService,
	}
}

// Download 处理文件下载请求
func (h *DownloadHandler) Download(c *gin.Context) {
	fileID := c.Param("fileId")
	if fileID == "" {
		log.Printf("错误: 文件ID为空")
		c.JSON(http.StatusBadRequest, gin.H{"error": "文件ID不能为空"})
		return
	}
	log.Printf("开始下载文件: %s", fileID)

	// 获取文件流
	reader, contentLength, err := h.downloadService.GetFileStream(fileID)
	if err != nil {
		log.Printf("获取文件流失败: %v", err)
		c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
		return
	}
	defer reader.Close()

	// 设置响应头
	c.Header("Content-Description", "File Transfer")
	c.Header("Content-Transfer-Encoding", "binary")
	c.Header("Content-Disposition", fmt.Sprintf(`attachment; filename="%s"`, fileID))
	c.Header("Content-Type", "application/octet-stream")
	if contentLength > 0 {
		c.Header("Content-Length", fmt.Sprintf("%d", contentLength))
	}

	// 流式传输文件
	c.Stream(func(w io.Writer) bool {
		_, err := io.Copy(w, reader)
		if err != nil {
			log.Printf("文件传输失败: %v", err)
		}
		return err == nil
	})
}
