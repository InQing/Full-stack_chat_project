package config

import (
	"os"

	"gopkg.in/yaml.v3"
)

// Config 服务器配置结构
type Config struct {
	Server ServerConfig `yaml:"server"`
	Qiniu  QiniuConfig  `yaml:"qiniu"`
}

// ServerConfig 服务器相关配置
type ServerConfig struct {
	Port int `yaml:"port"`
}

// QiniuConfig 七牛云配置
type QiniuConfig struct {
	AccessKey string `yaml:"access_key"`
	SecretKey string `yaml:"secret_key"`
	Bucket    string `yaml:"bucket"`
	Domain    string `yaml:"domain"`
}

var globalConfig *Config

// Init 初始化配置
func Init() error {
	// 读取配置文件
	data, err := os.ReadFile("config/config.yaml")
	if err != nil {
		return err
	}

	// 解析配置文件
	globalConfig = &Config{}
	if err := yaml.Unmarshal(data, globalConfig); err != nil {
		return err
	}

	return nil
}

// GetConfig 获取全局配置
func GetConfig() *Config {
	return globalConfig
}
