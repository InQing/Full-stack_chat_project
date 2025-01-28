package middleware

import (
	"fmt"
	"log"
	"os"
	"time"

	"github.com/gin-gonic/gin"
)

var (
	infoLogger  *log.Logger
	errorLogger *log.Logger
)

func init() {
	infoLogger = log.New(os.Stdout, "[INFO] ", log.Ldate|log.Ltime)
	errorLogger = log.New(os.Stderr, "[ERROR] ", log.Ldate|log.Ltime)
}

// Info 记录信息日志
func Info(format string, v ...interface{}) {
	msg := fmt.Sprintf(format, v...)
	infoLogger.Println(msg)
}

// Error 记录错误日志
func Error(format string, v ...interface{}) {
	msg := fmt.Sprintf(format, v...)
	errorLogger.Println(msg)
}

// LogMiddleware 日志中间件
func LogMiddleware() gin.HandlerFunc {
	return func(c *gin.Context) {
		// 开始时间
		start := time.Now()

		// 处理请求
		c.Next()

		// 结束时间
		end := time.Now()
		latency := end.Sub(start)

		// 记录请求信息
		Info("Request: %s %s | Status: %d | Latency: %v",
			c.Request.Method,
			c.Request.URL.Path,
			c.Writer.Status(),
			latency,
		)
	}
}
