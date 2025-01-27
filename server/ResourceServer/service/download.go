package service

import (
	"fmt"
	"io"
	"log"
	"resource-server/storage"
)

type DownloadService struct {
	storage storage.Storage
}

func NewDownloadService(storage storage.Storage) *DownloadService {
	return &DownloadService{
		storage: storage,
	}
}

// GetFileStream 从存储获取文件流
func (s *DownloadService) GetFileStream(fileID string) (io.ReadCloser, int64, error) {
	log.Printf("检查文件是否存在: %s", fileID)
	if !s.storage.Exists(fileID) {
		log.Printf("文件不存在: %s", fileID)
		return nil, 0, fmt.Errorf("文件不存在: %s", fileID)
	}
	log.Printf("开始从存储下载文件: %s", fileID)
	reader, size, err := s.storage.Download(fileID)
	if err != nil {
		log.Printf("从存储下载文件失败: %v", err)
		return nil, 0, fmt.Errorf("下载文件失败: %v", err)
	}
	log.Printf("文件下载成功，大小: %d bytes", size)
	return reader, size, nil
}
