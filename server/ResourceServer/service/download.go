package service

import (
	"fmt"
	"io"
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
	if !s.storage.Exists(fileID) {
		return nil, 0, fmt.Errorf("文件不存在")
	}
	return s.storage.Download(fileID)
}
