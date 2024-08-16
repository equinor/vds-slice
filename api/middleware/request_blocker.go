package middleware

import (
	"net/http"
	"strings"

	"github.com/gin-gonic/gin"
)

func RequestBlocker(blockedIPs []string, blockedUserAgents []string) gin.HandlerFunc {
	return func(ctx *gin.Context) {
		msg := "request cannot be processed. Please contact system admin to resolve the issue"

		clientIP := ctx.ClientIP()
		for _, blockedIP := range blockedIPs {
			blockedIP = strings.TrimSpace(blockedIP)
			if blockedIP == clientIP {
				ctx.AbortWithStatusJSON(http.StatusForbidden, &ErrorResponse{Error: msg})
				return
			}
		}

		userAgent := ctx.Request.UserAgent()
		for _, blockedAgent := range blockedUserAgents {
			blockedAgent = strings.TrimSpace(blockedAgent)
			if strings.Contains(userAgent, blockedAgent) {
				ctx.AbortWithStatusJSON(http.StatusForbidden, &ErrorResponse{Error: msg})
				return
			}
		}
		ctx.Next()
	}
}
