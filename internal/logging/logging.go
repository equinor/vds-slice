package logging

import (
	"fmt"
	"time"
	"strings"

	"github.com/gin-gonic/gin"
)

func FormattedLogger() gin.HandlerFunc {
	return gin.LoggerWithFormatter(func (param gin.LogFormatterParams) string {
		var statusColor, methodColor, resetColor string
		if param.IsOutputColor() {
			statusColor = param.StatusCodeColor()
			methodColor = param.MethodColor()
			resetColor  = param.ResetColor()
		}

		if param.Latency > time.Minute {
			param.Latency = param.Latency.Truncate(time.Second)
		}

		/*
		 * Strip off the query parameters from the path. In particular we don't want
		 * the clients sas token to be logged on our server.
		 */
		path := strings.Split(param.Path, "?")[0]

		request := param.Keys["request"]
		if request == nil {
			request = ""
		} else {
			request = fmt.Sprintf("%s\n", request)
		}

		return fmt.Sprintf("[GIN] %v |%s %3d %s| %13v | %15s |%s %-7s %s %#v\nUser-Agent: %s\n%s%s",
			param.TimeStamp.Format(time.RFC1123),
			statusColor, param.StatusCode, resetColor,
			param.Latency,
			param.ClientIP,
			methodColor, param.Method, resetColor,
			path,
			param.Request.UserAgent(),
			request,
			param.ErrorMessage,
		)
	})
}
