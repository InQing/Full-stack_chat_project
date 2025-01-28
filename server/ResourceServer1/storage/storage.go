package storage

import (
	"io"
)

// Storage 存储接口
type Storage interface {
	// Upload 上传文件
	Upload(key string, reader io.Reader) error
	// Download 下载文件
	Download(key string) (io.ReadCloser, int64, error)
	// Delete 删除文件
	Delete(key string) error
	// Exists 检查文件是否存在
	Exists(key string) bool
}
