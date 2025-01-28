package qiniu

import (
	"context"
	"crypto/tls"
	"fmt"
	"io"
	"net/http"
	"time"

	"github.com/qiniu/go-sdk/v7/auth/qbox"
	"github.com/qiniu/go-sdk/v7/storage"
)

type QiniuStorage struct {
	accessKey string
	secretKey string
	bucket    string
	domain    string
}

func NewQiniuStorage(accessKey, secretKey, bucket, domain string) *QiniuStorage {
	return &QiniuStorage{
		accessKey: accessKey,
		secretKey: secretKey,
		bucket:    bucket,
		domain:    domain,
	}
}

// Upload 上传文件到七牛云
func (q *QiniuStorage) Upload(key string, reader io.Reader) error {
	mac := qbox.NewMac(q.accessKey, q.secretKey)
	putPolicy := storage.PutPolicy{
		Scope: q.bucket,
	}
	upToken := putPolicy.UploadToken(mac)

	cfg := storage.Config{
		Zone:          &storage.ZoneHuadong,
		UseCdnDomains: false,
		UseHTTPS:      true,
	}

	formUploader := storage.NewFormUploader(&cfg)
	ret := storage.PutRet{}

	err := formUploader.Put(context.Background(), &ret, upToken, key, reader, -1, nil)
	if err != nil {
		return err
	}

	return nil
}

// GetPublicURL 获取文件的公开访问URL
func (q *QiniuStorage) GetPublicURL(key string) string {
	return storage.MakePublicURL(q.domain, key)
}

// GetUploadToken 获取上传凭证
func (q *QiniuStorage) GetUploadToken() string {
	mac := qbox.NewMac(q.accessKey, q.secretKey)
	putPolicy := storage.PutPolicy{
		Scope: q.bucket,
	}
	return putPolicy.UploadToken(mac)
}

// GetPrivateURL 获取私有空间文件的下载链接
func (q *QiniuStorage) GetPrivateURL(key string) string {
	mac := qbox.NewMac(q.accessKey, q.secretKey)
	deadline := time.Now().Add(time.Hour).Unix() // 1小时有效期
	privateURL := storage.MakePrivateURL(mac, q.domain, key, deadline)
	return privateURL
}

// GetFile 从七牛云下载文件
func (q *QiniuStorage) GetFile(key string) (*http.Response, error) {
	mac := qbox.NewMac(q.accessKey, q.secretKey)

	// 创建下载管理器
	downloadManager := storage.NewBucketManager(mac, &storage.Config{
		Zone:     &storage.ZoneHuadong,
		UseHTTPS: true,
	})

	// 获取文件信息
	fileInfo, err := downloadManager.Stat(q.bucket, key)
	if err != nil {
		return nil, fmt.Errorf("获取文件信息失败: %v", err)
	}

	// 生成私有下载链接
	deadline := time.Now().Add(time.Hour).Unix()
	privateURL := storage.MakePrivateURL(mac, q.domain, key, deadline)

	fmt.Printf("Private URL: %s\n", privateURL)

	// 创建HTTP客户端，由于七牛云CDN证书问题，这里跳过证书验证
	client := &http.Client{
		Transport: &http.Transport{
			TLSClientConfig: &tls.Config{
				InsecureSkipVerify: true, // 跳过证书验证
			},
		},
	}

	// 发起下载请求
	req, err := http.NewRequest("GET", privateURL, nil)
	if err != nil {
		return nil, fmt.Errorf("创建请求失败: %v", err)
	}

	// 发送请求
	resp, err := client.Do(req)
	if err != nil {
		return nil, fmt.Errorf("下载文件失败: %v", err)
	}

	if resp.StatusCode != http.StatusOK {
		resp.Body.Close()
		return nil, fmt.Errorf("下载文件失败，状态码：%d", resp.StatusCode)
	}

	// 设置响应头
	resp.ContentLength = fileInfo.Fsize

	return resp, nil
}

// Download 实现Storage接口的Download方法
func (q *QiniuStorage) Download(key string) (io.ReadCloser, int64, error) {
	resp, err := q.GetFile(key)
	if err != nil {
		return nil, 0, err
	}
	return resp.Body, resp.ContentLength, nil
}

// Delete 实现Storage接口的Delete方法
func (q *QiniuStorage) Delete(key string) error {
	mac := qbox.NewMac(q.accessKey, q.secretKey)
	cfg := storage.Config{
		Zone: &storage.ZoneHuadong,
	}
	bucketManager := storage.NewBucketManager(mac, &cfg)
	return bucketManager.Delete(q.bucket, key)
}

// Exists 实现Storage接口的Exists方法
func (q *QiniuStorage) Exists(key string) bool {
	mac := qbox.NewMac(q.accessKey, q.secretKey)
	cfg := storage.Config{
		Zone: &storage.ZoneHuadong,
	}
	bucketManager := storage.NewBucketManager(mac, &cfg)
	_, err := bucketManager.Stat(q.bucket, key)
	return err == nil
}
