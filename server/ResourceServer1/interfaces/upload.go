package interfaces

// UploadService 上传服务接口
type UploadService interface {
	GetUploadStatus(fileId string) (interface{}, bool)
}
