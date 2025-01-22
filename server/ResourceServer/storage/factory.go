package storage

import (
	"fmt"
	"resource-server/config"
	"resource-server/storage/local"
	"resource-server/storage/qiniu"
)

// NewStorage 根据配置创建存储实例
func NewStorage(cfg *config.Config) (Storage, error) {
	switch cfg.Server.StorageType {
	case "local":
		return local.NewLocalStorage(cfg.Server.UploadDir), nil
	case "qiniu":
		return qiniu.NewQiniuStorage(
			cfg.Qiniu.AccessKey,
			cfg.Qiniu.SecretKey,
			cfg.Qiniu.Bucket,
			cfg.Qiniu.Domain,
		), nil
	default:
		return nil, fmt.Errorf("不支持的存储类型: %s", cfg.Server.StorageType)
	}
}
