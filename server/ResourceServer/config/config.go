package config

import (
	"os"

	"gopkg.in/yaml.v3"
)

// Config 配置结构体
type Config struct {
	Server ServerConfig `yaml:"server"`
	Log    LogConfig    `yaml:"log"`
	Qiniu  QiniuConfig  `yaml:"qiniu"`
}

// ServerConfig 服务器配置
type ServerConfig struct {
	Port           int    `yaml:"port"`
	Host           string `yaml:"host"`
	UploadDir      string `yaml:"upload_dir"`       // 上传文件临时存储目录
	ChunkSize      int64  `yaml:"chunk_size"`       // 分片大小（字节）
	MaxFileSize    int64  `yaml:"max_file_size"`    // 最大文件大小（字节）
	AllowedFileExt string `yaml:"allowed_file_ext"` // 允许的文件扩展名
	StorageType    string `yaml:"storage_type"`     // 存储类型：local或qiniu
}

// LogConfig 日志配置
type LogConfig struct {
	Path       string `yaml:"path"`
	Level      string `yaml:"level"`
	MaxSize    int    `yaml:"max_size"`
	MaxBackups int    `yaml:"max_backups"`
	MaxAge     int    `yaml:"max_age"`
	Compress   bool   `yaml:"compress"`
}

// QiniuConfig 七牛云配置
type QiniuConfig struct {
	AccessKey string `yaml:"access_key"`
	SecretKey string `yaml:"secret_key"`
	Bucket    string `yaml:"bucket"`    // 存储空间名称
	Domain    string `yaml:"domain"`    // 访问域名
	Zone      string `yaml:"zone"`      // 存储区域
	UseHTTPS  bool   `yaml:"use_https"` // 是否使用HTTPS
}

var globalConfig Config

// Init 初始化配置
func Init(configFile string) error {
	// 读取配置文件
	data, err := os.ReadFile(configFile)
	if err != nil {
		return err
	}

	// 解析配置文件
	err = yaml.Unmarshal(data, &globalConfig)
	if err != nil {
		return err
	}

	// 创建上传目录
	if err := os.MkdirAll(globalConfig.Server.UploadDir, 0755); err != nil {
		return err
	}

	return nil
}

// Get 获取全局配置
func Get() *Config {
	return &globalConfig
}

// GetServer 获取服务器配置
func GetServer() *ServerConfig {
	return &globalConfig.Server
}

// GetQiniu 获取七牛云配置
func GetQiniu() *QiniuConfig {
	return &globalConfig.Qiniu
}

// GetLog 获取日志配置
func GetLog() *LogConfig {
	return &globalConfig.Log
}
