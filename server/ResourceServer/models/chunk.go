package models

// ChunkInfo 表示文件分片的信息
type ChunkInfo struct {
	FileID      string `json:"file_id"`      // 文件唯一标识
	ChunkNumber int    `json:"chunk_number"` // 当前分片编号
	TotalChunks int    `json:"total_chunks"` // 总分片数
	MD5         string `json:"md5"`          // 分片的MD5值
}

// UploadStatus 表示文件上传的状态
type UploadStatus struct {
	FileID         string         // 文件唯一标识
	UploadID       string         // 上传会话ID
	Filename       string         // 文件名
	TotalChunks    int            // 总分片数
	UploadedChunks map[int]string // 已上传分片的编号和对应的MD5值
	TempDir        string         // 临时存储目录
	Completed      bool           // 是否已完成上传
	FinalMD5       string         // 完整文件的MD5值
}
