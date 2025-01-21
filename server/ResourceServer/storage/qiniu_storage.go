package storage

import (
	"github.com/qiniu/go-sdk/v7/auth/qbox"
	"github.com/qiniu/go-sdk/v7/storage"
)

// QiniuStorage 七牛云存储服务
type QiniuStorage struct {
	accessKey string
	secretKey string
	bucket    string
	domain    string
	mac       *qbox.Mac
}

// NewQiniuStorage 创建七牛云存储实例
func NewQiniuStorage(accessKey, secretKey, bucket, domain string) *QiniuStorage {
	return &QiniuStorage{
		accessKey: accessKey,
		secretKey: secretKey,
		bucket:    bucket,
		domain:    domain,
		mac:       qbox.NewMac(accessKey, secretKey),
	}
}

// GetUploadToken 获取上传凭证
func (q *QiniuStorage) GetUploadToken() string {
	putPolicy := storage.PutPolicy{
		Scope: q.bucket,
	}
	return putPolicy.UploadToken(q.mac)
}

// GetDownloadURL 获取文件下载链接
func (q *QiniuStorage) GetDownloadURL(key string) string {
	domain := q.domain
	deadline := int64(3600) // 1小时有效期

	return storage.MakePrivateURL(q.mac, domain, key, deadline)
}
