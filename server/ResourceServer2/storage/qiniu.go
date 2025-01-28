package storage

import (
	"context"
	"crypto/md5"
	"encoding/base64"
	"encoding/hex"
	"errors"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"sync"
	"time"

	"resource-server/config"
	"resource-server/middleware"

	"github.com/qiniu/go-sdk/v7/auth/qbox"
	"github.com/qiniu/go-sdk/v7/storage"
)

// UploadInfo 上传信息
type UploadInfo struct {
	FileID      string       // 文件唯一标识
	TotalSize   int64        // 文件总大小
	ChunkSize   int64        // 分片大小
	TotalChunks int          // 总分片数
	Chunks      map[int]bool // 已上传的分片
	TempFile    string       // 临时文件路径
	Lock        sync.RWMutex // 并发控制
}

// QiniuStorage 七牛云存储实现
type QiniuStorage struct {
	mac           *qbox.Mac
	bucket        string
	domain        string
	useHTTPS      bool
	bucketManager *storage.BucketManager
	uploadDir     string
	uploads       sync.Map // map[string]*UploadInfo
}

// NewQiniuStorage 创建七牛云存储实例
func NewQiniuStorage() *QiniuStorage {
	qiniuConfig := config.GetQiniu()
	serverConfig := config.GetServer()
	mac := qbox.NewMac(qiniuConfig.AccessKey, qiniuConfig.SecretKey)
	cfg := storage.Config{
		Zone:          selectZone(qiniuConfig.Zone),
		UseHTTPS:      qiniuConfig.UseHTTPS,
		UseCdnDomains: false,
	}

	s := &QiniuStorage{
		mac:           mac,
		bucket:        qiniuConfig.Bucket,
		domain:        qiniuConfig.Domain,
		useHTTPS:      qiniuConfig.UseHTTPS,
		bucketManager: storage.NewBucketManager(mac, &cfg),
		uploadDir:     serverConfig.UploadDir,
	}

	middleware.Info("QiniuStorage initialized with bucket: %s", qiniuConfig.Bucket)
	return s
}

// InitUpload 初始化上传任务
func (s *QiniuStorage) InitUpload(fileID string, totalSize int64) (*UploadInfo, error) {
	chunkSize := config.GetServer().ChunkSize
	totalChunks := int((totalSize + chunkSize - 1) / chunkSize)
	tempFile := filepath.Join(s.uploadDir, fileID)

	info := &UploadInfo{
		FileID:      fileID,
		TotalSize:   totalSize,
		ChunkSize:   chunkSize,
		TotalChunks: totalChunks,
		Chunks:      make(map[int]bool),
		TempFile:    tempFile,
	}

	s.uploads.Store(fileID, info)
	middleware.Info("Upload initialized - FileID: %s, TotalChunks: %d", fileID, totalChunks)
	return info, nil
}

// SaveChunk 保存分片
func (s *QiniuStorage) SaveChunk(fileID string, chunkNumber int, chunk string, md5 string) error {
	value, ok := s.uploads.Load(fileID)
	if !ok {
		return errors.New("upload not initialized")
	}
	info := value.(*UploadInfo)

	// 加锁保护并发写入
	info.Lock.Lock()
	defer info.Lock.Unlock()

	// 验证分片MD5
	chunkData, err := base64.StdEncoding.DecodeString(chunk)
	if err != nil {
		return err
	}
	if actualMD5 := calculateMD5(chunkData); actualMD5 != md5 {
		return errors.New("md5 mismatch")
	}

	// 写入分片
	f, err := os.OpenFile(info.TempFile, os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		return err
	}
	defer f.Close()

	offset := (int64(chunkNumber) - 1) * info.ChunkSize
	if _, err := f.WriteAt(chunkData, offset); err != nil {
		return err
	}

	// 记录分片上传状态
	info.Chunks[chunkNumber] = true
	middleware.Info("Chunk saved - FileID: %s, ChunkNumber: %d", fileID, chunkNumber)
	return nil
}

// CompleteUpload 完成上传并上传到七牛云
func (s *QiniuStorage) CompleteUpload(fileID string) error {
	value, ok := s.uploads.Load(fileID)
	if !ok {
		return errors.New("upload not initialized")
	}
	info := value.(*UploadInfo)

	// 检查所有分片是否都已上传
	if len(info.Chunks) != info.TotalChunks {
		return errors.New("not all chunks uploaded")
	}

	// 使用七牛云的断点续传上传文件
	cfg := storage.Config{
		Zone:          selectZone(config.GetQiniu().Zone),
		UseHTTPS:      s.useHTTPS,
		UseCdnDomains: false,
	}
	resumeUploader := storage.NewResumeUploaderV2(&cfg)

	putPolicy := storage.PutPolicy{
		Scope: s.bucket + ":" + fileID,
	}
	upToken := putPolicy.UploadToken(s.mac)

	// 配置上传参数
	recorder, err := storage.NewFileRecorder(s.uploadDir)
	if err != nil {
		return err
	}
	upConfig := storage.RputV2Extra{
		Recorder: recorder,
	}

	// 上传到七牛云
	err = resumeUploader.PutFile(context.Background(), nil, upToken, fileID, info.TempFile, &upConfig)
	if err != nil {
		middleware.Error("Failed to upload to Qiniu: %v", err)
		return err
	}

	// 清理临时文件和上传信息
	os.Remove(info.TempFile)
	s.uploads.Delete(fileID)

	middleware.Info("Upload completed - FileID: %s", fileID)
	return nil
}

// GetUploadProgress 获取上传进度
func (s *QiniuStorage) GetUploadProgress(fileID string) (int, error) {
	value, ok := s.uploads.Load(fileID)
	if !ok {
		return 0, errors.New("upload not found")
	}
	info := value.(*UploadInfo)

	info.Lock.RLock()
	defer info.Lock.RUnlock()

	return len(info.Chunks), nil
}

// calculateMD5 计算数据的MD5值
func calculateMD5(data []byte) string {
	hash := md5.New()
	hash.Write(data)
	return hex.EncodeToString(hash.Sum(nil))
}

// GetFileStream 获取文件流（从七牛云下载）
func (s *QiniuStorage) GetFileStream(fileID string) (io.ReadCloser, error) {
	deadline := time.Now().Add(time.Hour).Unix()
	privateURL := storage.MakePrivateURL(s.mac, s.domain, fileID, deadline)

	resp, err := http.Get(privateURL)
	if err != nil {
		return nil, err
	}

	if resp.StatusCode != 200 {
		resp.Body.Close()
		return nil, errors.New("file not found")
	}

	return resp.Body, nil
}

// selectZone 根据配置选择存储区域
func selectZone(zone string) *storage.Zone {
	switch zone {
	case "z0":
		return &storage.ZoneHuadong
	case "z1":
		return &storage.ZoneHuabei
	case "z2":
		return &storage.ZoneHuanan
	default:
		return &storage.ZoneHuadong
	}
}
