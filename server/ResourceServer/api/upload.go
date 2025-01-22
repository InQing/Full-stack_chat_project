package api

import (
	"io"
	"net/http"
	"resource-server/service"

	"resource-server/models"

	"strconv"

	"github.com/gin-gonic/gin"
)

// UploadRequest 上传请求的结构体
type UploadRequest struct {
	FileID      string `json:"file_id"`
	Chunk       string `json:"chunk"`
	ChunkNumber int    `json:"chunk_number"`
	TotalChunks int    `json:"total_chunks"`
	MD5         string `json:"md5"`
}

type UploadHandler struct {
	uploadService *service.UploadService
}

func NewUploadHandler(uploadService *service.UploadService) *UploadHandler {
	return &UploadHandler{
		uploadService: uploadService,
	}
}

// Upload 处理文件分片上传
func (h *UploadHandler) Upload(c *gin.Context) {
	var req UploadRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	// 创建ChunkInfo
	chunkInfo := &models.ChunkInfo{
		FileID:      req.FileID,
		ChunkNumber: req.ChunkNumber,
		TotalChunks: req.TotalChunks,
		MD5:         req.MD5,
	}

	// 处理上传
	err := h.uploadService.HandleChunkUpload(chunkInfo, []byte(req.Chunk))
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
		return
	}

	c.JSON(http.StatusOK, gin.H{
		"status":  "success",
		"message": "分片上传成功",
	})
}

// HandleChunkUpload 处理分片上传请求
func (h *UploadHandler) HandleChunkUpload(c *gin.Context) {
	// 获取表单字段
	fileID := c.PostForm("file_id")
	chunkNumber := c.PostForm("chunk_number")
	totalChunks := c.PostForm("total_chunks")
	md5Value := c.PostForm("md5")

	// 转换数字字段
	chunkNum, err := strconv.Atoi(chunkNumber)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "无效的分片编号"})
		return
	}
	totalNum, err := strconv.Atoi(totalChunks)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "无效的总分片数"})
		return
	}

	// 创建ChunkInfo
	chunkInfo := &models.ChunkInfo{
		FileID:      fileID,
		ChunkNumber: chunkNum,
		TotalChunks: totalNum,
		MD5:         md5Value,
	}

	// 获取文件数据
	file, err := c.FormFile("chunk")
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"error": "获取文件数据失败",
		})
		return
	}

	// 读取文件数据
	f, err := file.Open()
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"error": "读取文件数据失败",
		})
		return
	}
	defer f.Close()

	data, err := io.ReadAll(f)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"error": "读取文件数据失败",
		})
		return
	}

	// 处理分片上传
	err = h.uploadService.HandleChunkUpload(chunkInfo, data)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"error": err.Error(),
		})
		return
	}

	c.JSON(http.StatusOK, gin.H{
		"status":  "success",
		"message": "分片上传成功",
	})
}
