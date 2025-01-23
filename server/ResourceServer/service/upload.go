package service

import (
	"crypto/md5"
	"encoding/hex"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"strings"
	"sync"

	"resource-server/models"
	"resource-server/storage"
)

type UploadService struct {
	uploadStatusMap sync.Map
	tempDir         string
	storage         storage.Storage
}

func NewUploadService(tempDir string, storage storage.Storage) *UploadService {
	return &UploadService{
		tempDir: tempDir,
		storage: storage,
	}
}

// 从FileId中提取原始文件名
// FileId格式: yyyyMMddHHmmss_filename
func extractFileName(fileId string) string {
	parts := strings.SplitN(fileId, "_", 2)
	if len(parts) != 2 {
		return fileId // 如果格式不对，返回原始fileId
	}
	return parts[1]
}

// HandleChunkUpload 处理分片上传
func (s *UploadService) HandleChunkUpload(chunk *models.ChunkInfo, data []byte) error {
	// 获取或创建上传状态
	statusInterface, _ := s.uploadStatusMap.LoadOrStore(chunk.FileID, &models.UploadStatus{
		FileID:         chunk.FileID,
		TotalChunks:    chunk.TotalChunks,
		UploadedChunks: make(map[int]string),
		TempDir:        filepath.Join(s.tempDir, chunk.FileID),
	})
	status := statusInterface.(*models.UploadStatus)

	// 创建临时目录
	if err := os.MkdirAll(status.TempDir, 0755); err != nil {
		return fmt.Errorf("创建临时目录失败: %v", err)
	}

	// 保存分片
	chunkPath := filepath.Join(status.TempDir, fmt.Sprintf("chunk_%d", chunk.ChunkNumber))
	if err := os.WriteFile(chunkPath, data, 0644); err != nil {
		return fmt.Errorf("保存分片失败: %v", err)
	}

	// 验证分片MD5
	calculatedMD5 := calculateMD5(data)
	if calculatedMD5 != chunk.MD5 {
		os.Remove(chunkPath)
		return fmt.Errorf("分片MD5校验失败")
	}

	// 记录已上传的分片
	status.UploadedChunks[chunk.ChunkNumber] = chunk.MD5

	// 检查是否所有分片都已上传
	if len(status.UploadedChunks) == status.TotalChunks {
		return s.mergeAndUpload(status)
	}

	return nil
}

// mergeAndUpload 合并文件并上传到存储
func (s *UploadService) mergeAndUpload(status *models.UploadStatus) error {
	// 确保在函数返回时清理临时文件和状态
	defer func() {
		os.RemoveAll(status.TempDir)
		s.uploadStatusMap.Delete(status.FileID)
	}()

	// 从FileId中提取原始文件名
	fileName := extractFileName(status.FileID)
	
	// 1. 合并文件
	mergedFilePath := filepath.Join(status.TempDir, fileName)
	if err := s.mergeChunks(status, mergedFilePath); err != nil {
		return err
	}

	// 2. 上传到存储
	file, err := os.Open(mergedFilePath)
	if err != nil {
		return fmt.Errorf("打开合并文件失败: %v", err)
	}
	defer file.Close()

	if err := s.storage.Upload(fileName, file); err != nil {
		return fmt.Errorf("上传文件失败: %v", err)
	}

	status.Completed = true
	return nil
}

// mergeChunks 合并文件分片
func (s *UploadService) mergeChunks(status *models.UploadStatus, mergedFilePath string) error {
	mergedFile, err := os.Create(mergedFilePath)
	if err != nil {
		return fmt.Errorf("创建合并文件失败: %v", err)
	}
	defer mergedFile.Close()

	// 按顺序合并分片
	for i := 1; i <= status.TotalChunks; i++ {
		chunkPath := filepath.Join(status.TempDir, fmt.Sprintf("chunk_%d", i))
		chunkData, err := os.ReadFile(chunkPath)
		if err != nil {
			return fmt.Errorf("读取分片失败: %v", err)
		}
		if _, err := mergedFile.Write(chunkData); err != nil {
			return fmt.Errorf("写入合并文件失败: %v", err)
		}
		// 删除已合并的分片
		os.Remove(chunkPath)
	}

	// 计算合并后文件的MD5
	mergedFile.Seek(0, 0)
	hash := md5.New()
	if _, err := io.Copy(hash, mergedFile); err != nil {
		return fmt.Errorf("计算合并文件MD5失败: %v", err)
	}
	status.FinalMD5 = hex.EncodeToString(hash.Sum(nil))

	return nil
}

func calculateMD5(data []byte) string {
	hash := md5.New()
	hash.Write(data)
	return hex.EncodeToString(hash.Sum(nil))
}
