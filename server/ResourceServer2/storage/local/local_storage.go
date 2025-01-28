package local

import (
	"io"
	"os"
	"path/filepath"
)

type LocalStorage struct {
	rootDir string
}

func NewLocalStorage(rootDir string) *LocalStorage {
	return &LocalStorage{
		rootDir: rootDir,
	}
}

func (s *LocalStorage) Upload(key string, reader io.Reader) error {
	filePath := filepath.Join(s.rootDir, key)

	// 确保目录存在
	if err := os.MkdirAll(filepath.Dir(filePath), 0755); err != nil {
		return err
	}

	// 创建文件
	file, err := os.Create(filePath)
	if err != nil {
		return err
	}
	defer file.Close()

	// 写入文件
	_, err = io.Copy(file, reader)
	return err
}

func (s *LocalStorage) Download(key string) (io.ReadCloser, int64, error) {
	filePath := filepath.Join(s.rootDir, key)

	// 打开文件
	file, err := os.Open(filePath)
	if err != nil {
		return nil, 0, err
	}

	// 获取文件信息
	info, err := file.Stat()
	if err != nil {
		file.Close()
		return nil, 0, err
	}

	return file, info.Size(), nil
}

func (s *LocalStorage) Delete(key string) error {
	return os.Remove(filepath.Join(s.rootDir, key))
}

func (s *LocalStorage) Exists(key string) bool {
	_, err := os.Stat(filepath.Join(s.rootDir, key))
	return err == nil
}
