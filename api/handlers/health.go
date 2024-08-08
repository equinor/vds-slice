package handlers

import (
	"net/http"
	"github.com/gin-gonic/gin"
)

func (e *Endpoint) Health(ctx *gin.Context) {

	ctx.HTML(http.StatusOK, "index.html", gin.H{})
}
