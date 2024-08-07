package middleware

import (
	"github.com/gin-gonic/gin"
	"net/http"
	"strings"
)

// @Description Error response description
type ErrorResponse struct {
	// Textual description of encountered error
	Error string `json:"error" example:"message"`
} // @name ErrorResponse

func ErrorHandler(ctx *gin.Context) {
	ctx.Next()

	// no errors + error status can happen for example for paths handled by gin
	// (accessing /nonexistent)
	hasError := len(ctx.Errors) != 0 || ctx.Writer.Status() != http.StatusOK
	if !hasError {
		return
	}

	status := -1
	if ctx.Writer.Status() == http.StatusOK {
		status = http.StatusInternalServerError
	}

	errors := []string{}
	for _, err := range ctx.Errors {
		errors = append(errors, err.Error())
	}
	error := strings.Join(errors[:], ",")

	ctx.JSON(status, &ErrorResponse{Error: error})
}
