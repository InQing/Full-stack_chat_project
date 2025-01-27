package middleware

import (
	"log"
	"net/http"

	"github.com/gin-gonic/gin"
)

// DownloadTokenAuth 验证下载请求中的token
func DownloadTokenAuth() gin.HandlerFunc {
	return func(c *gin.Context) {
		log.Printf("=== 开始下载Token验证 ===")

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

		// 从URL参数中获取uid
		uid := c.Query("uid")
		if uid == "" {
			log.Printf("错误: 未提供uid")
			c.JSON(http.StatusBadRequest, gin.H{
				"code": http.StatusBadRequest,
				"msg":  "未提供uid参数",
			})
			c.Abort()
			return
		}
		log.Printf("收到的UID: %s", uid)

		// 从Redis获取存储的token
		storedToken, err := GetToken(uid)
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
		c.Set("uid", uid)
		c.Next()
	}
}
