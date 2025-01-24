package api

import (
	"bytes"
	"encoding/base64"
	"io"
	"log"
	"net/http"
	"resource-server/service"

	"resource-server/models"

	"github.com/gin-gonic/gin"
)

// UploadRequest 上传请求的结构体
type UploadRequest struct {
	FileID      string `json:"file_id"`
	UploadID    string `json:"upload_id"`
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
	log.Printf("=== 开始处理分片上传请求 ===")

	// 读取并打印原始请求数据
	rawData, err := io.ReadAll(c.Request.Body)
	if err != nil {
		log.Printf("读取请求体失败: %v", err)
		c.JSON(http.StatusBadRequest, gin.H{"error": "读取请求失败"})
		return
	}
	// 因为已经读取了body，需要重新设置
	c.Request.Body = io.NopCloser(bytes.NewBuffer(rawData))

	var req UploadRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		log.Printf("解析JSON请求失败: %v", err)
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	log.Printf("请求数据: FileID=%s, ChunkNumber=%d, TotalChunks=%d, MD5=%s, ChunkSize=%d字节",
		req.FileID, req.ChunkNumber, req.TotalChunks, req.MD5, len(req.Chunk))

	// Base64解码分片数据
	chunkData, err := base64.StdEncoding.DecodeString(req.Chunk)
	if err != nil {
		log.Printf("Base64解码失败: %v", err)
		c.JSON(http.StatusBadRequest, gin.H{"error": "无效的分片数据"})
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
	err = h.uploadService.HandleChunkUpload(chunkInfo, chunkData)
	if err != nil {
		log.Printf("保存分片失败: %v", err)
		c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
		return
	}

	log.Printf("分片 %d/%d 上传成功", req.ChunkNumber, req.TotalChunks)

	c.JSON(http.StatusOK, gin.H{
		"status":  "success",
		"message": "分片上传成功",
	})

	log.Printf("=== 分片上传请求处理完成 ===\n")
}

// InitUpload 处理文件上传初始化请求
func (h *UploadHandler) InitUpload(c *gin.Context) {
	var req models.UploadInitRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"code":    http.StatusBadRequest,
			"message": "请求参数错误",
		})
		return
	}

	// 调用service层处理初始化
	uploadID, err := h.uploadService.InitUpload(req.Filename, req.FileID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"code":    http.StatusInternalServerError,
			"message": "初始化上传失败",
		})
		return
	}

	// 返回成功响应
	response := models.UploadInitResponse{
		Code:    http.StatusOK,
		Message: "初始化成功",
	}
	response.Data.UploadID = uploadID
	response.Data.FileID = req.FileID

	c.JSON(http.StatusOK, response)
}
