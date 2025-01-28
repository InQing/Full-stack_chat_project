package middleware

import (
	"bytes"
	"encoding/json"
	"io"
	"log"
	"net/http"

	"github.com/gin-gonic/gin"
)

// TokenAuth 验证请求中的token
func TokenAuth() gin.HandlerFunc {
	return func(c *gin.Context) {
		log.Printf("=== 开始Token验证 ===")

		token := c.GetHeader("Authorization")
		log.Printf("收到的Token: %s", token)
		if token == "" {
			log.Printf("错误: 未提供token")
			c.JSON(http.StatusUnauthorized, gin.H{
				"code": http.StatusUnauthorized,
				"msg":  "未提供token",
			})
			c.Abort()
			return
		}

		// 读取请求体
		bodyBytes, err := io.ReadAll(c.Request.Body)
		if err != nil {
			log.Printf("错误: 读取请求体失败: %v", err)
			c.JSON(http.StatusBadRequest, gin.H{
				"code": http.StatusBadRequest,
				"msg":  "读取请求失败",
			})
			c.Abort()
			return
		}

		// 从请求体中获取uid
		var reqData struct {
			UID string `json:"uid"`
		}
		if err := json.Unmarshal(bodyBytes, &reqData); err != nil {
			log.Printf("错误: 解析请求体失败: %v", err)
			c.JSON(http.StatusBadRequest, gin.H{
				"code": http.StatusBadRequest,
				"msg":  "无效的请求参数",
			})
			c.Abort()
			return
		}
		log.Printf("收到的UID: %s", reqData.UID)

		// 从Redis获取存储的token
		storedToken, err := GetToken(reqData.UID)
		if err != nil {
			log.Printf("错误: 从Redis获取token失败: %v", err)
			c.JSON(http.StatusUnauthorized, gin.H{
				"code": http.StatusUnauthorized,
				"msg":  "token无效或已过期",
			})
			c.Abort()
			return
		}
		log.Printf("Redis中存储的Token: %s", storedToken)

		// 验证token是否匹配
		if token != storedToken {
			log.Printf("错误: token不匹配\n期望的token: %s\n实际的token: %s", storedToken, token)
			c.JSON(http.StatusUnauthorized, gin.H{
				"code": http.StatusUnauthorized,
				"msg":  "token验证失败",
			})
			c.Abort()
			return
		}

		log.Printf("Token验证成功")
		// 将uid保存到上下文中，供后续使用
		c.Set("uid", reqData.UID)
		// 重新设置请求体，供后续处理函数使用
		c.Request.Body = io.NopCloser(bytes.NewBuffer(bodyBytes))
		c.Next()
	}
}
