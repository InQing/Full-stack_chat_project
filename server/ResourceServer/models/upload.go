package models

// UploadInitRequest 文件上传初始化请求
type UploadInitRequest struct {
	Filename string `json:"filename"`
	FileID   string `json:"file_id"`
	UID      string `json:"uid"` // 添加用户ID字段
}

// UploadInitResponse 文件上传初始化响应
type UploadInitResponse struct {
	Code    int    `json:"code"`
	Message string `json:"message"`
	Data    struct {
		UploadID string `json:"upload_id"` // 上传会话ID
		FileID   string `json:"file_id"`   // 文件ID
	} `json:"data"`
}
