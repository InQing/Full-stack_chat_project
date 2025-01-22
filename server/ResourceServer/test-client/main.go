package main

import (
	"bytes"
	"crypto/md5"
	"encoding/hex"
	"flag"
	"fmt"
	"io"
	"mime/multipart"
	"net/http"
	"os"
	"path/filepath"
)

const (
	ChunkSize = 1024 * 1024 // 1MB
	ServerURL = "http://localhost:8080"
)

type UploadRequest struct {
	FileID      string `json:"file_id"`
	ChunkNumber int    `json:"chunk_number"`
	TotalChunks int    `json:"total_chunks"`
	MD5         string `json:"md5"`
}

func main() {
	// 命令行参数
	action := flag.String("action", "", "upload or download")
	filePath := flag.String("file", "", "file path")
	fileID := flag.String("id", "", "file id for download")
	flag.Parse()

	if *action == "" {
		fmt.Println("请指定操作类型: -action=upload 或 -action=download")
		return
	}

	switch *action {
	case "upload":
		if *filePath == "" {
			fmt.Println("请指定上传文件路径: -file=<path>")
			return
		}
		if err := uploadFile(*filePath); err != nil {
			fmt.Printf("上传失败: %v\n", err)
		}
	case "download":
		if *fileID == "" {
			fmt.Println("请指定文件ID: -id=<file_id>")
			return
		}
		if *filePath == "" {
			*filePath = *fileID // 使用fileID作为保存的文件名
		}
		if err := downloadFile(*fileID, *filePath); err != nil {
			fmt.Printf("下载失败: %v\n", err)
		}
	default:
		fmt.Println("不支持的操作类型")
	}
}

func uploadFile(filePath string) error {
	file, err := os.Open(filePath)
	if err != nil {
		return fmt.Errorf("打开文件失败: %v", err)
	}
	defer file.Close()

	// 获取文件大小
	fileInfo, err := file.Stat()
	if err != nil {
		return fmt.Errorf("获取文件信息失败: %v", err)
	}

	// 计算总分片数
	totalChunks := (fileInfo.Size() + ChunkSize - 1) / ChunkSize
	fileID := filepath.Base(filePath)

	fmt.Printf("开始上传文件 %s，总大小：%d 字节，分片数：%d\n", fileID, fileInfo.Size(), totalChunks)

	// 分片上传
	for i := int64(1); i <= totalChunks; i++ {
		// 读取分片数据
		chunk := make([]byte, ChunkSize)
		n, err := file.Read(chunk)
		if err != nil && err != io.EOF {
			return fmt.Errorf("读取分片失败: %v", err)
		}
		chunk = chunk[:n]

		// 计算分片MD5
		hash := md5.New()
		hash.Write(chunk)
		md5Value := hex.EncodeToString(hash.Sum(nil))

		// 准备上传请求
		req := UploadRequest{
			FileID:      fileID,
			ChunkNumber: int(i),
			TotalChunks: int(totalChunks),
			MD5:         md5Value,
		}

		// 发送请求
		if err := uploadChunk(req, chunk); err != nil {
			return fmt.Errorf("上传分片 %d 失败: %v", i, err)
		}

		fmt.Printf("分片 %d/%d 上传完成\n", i, totalChunks)
	}

	fmt.Println("文件上传完成")
	return nil
}

func uploadChunk(req UploadRequest, chunk []byte) error {
	// 创建multipart请求
	body := &bytes.Buffer{}
	writer := multipart.NewWriter(body)

	// 添加文件数据
	part, err := writer.CreateFormFile("chunk", req.FileID)
	if err != nil {
		return err
	}
	part.Write(chunk)

	// 添加其他字段
	writer.WriteField("file_id", req.FileID)
	writer.WriteField("chunk_number", fmt.Sprintf("%d", req.ChunkNumber))
	writer.WriteField("total_chunks", fmt.Sprintf("%d", req.TotalChunks))
	writer.WriteField("md5", req.MD5)

	writer.Close()

	// 发送请求
	resp, err := http.Post(ServerURL+"/upload", writer.FormDataContentType(), body)
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		respBody, _ := io.ReadAll(resp.Body)
		return fmt.Errorf("服务器返回错误: %s", string(respBody))
	}

	return nil
}

func downloadFile(fileID, savePath string) error {
	fmt.Printf("开始下载文件 %s\n", fileID)

	// 发送下载请求
	resp, err := http.Get(fmt.Sprintf("%s/download/%s", ServerURL, fileID))
	if err != nil {
		return fmt.Errorf("发送下载请求失败: %v", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		body, _ := io.ReadAll(resp.Body)
		return fmt.Errorf("服务器返回错误: %s", string(body))
	}

	// 创建保存文件
	out, err := os.Create(savePath)
	if err != nil {
		return fmt.Errorf("创建文件失败: %v", err)
	}
	defer out.Close()

	// 写入文件
	written, err := io.Copy(out, resp.Body)
	if err != nil {
		return fmt.Errorf("保存文件失败: %v", err)
	}

	fmt.Printf("文件下载完成，大小：%d 字节\n", written)
	return nil
}
