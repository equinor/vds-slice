package api

import (
	"bytes"
	"errors"
	"github.com/gin-gonic/gin"
	"log"
	"mime/multipart"
	"net/http"
	"net/textproto"
	"strings"
)

type errorResponse struct {
	Error string `json:"error"`
}

func writeResponse(ctx *gin.Context, metadata []byte, data []byte) {
	response := &bytes.Buffer{}
	writer := multipart.NewWriter(response)

	err := writeData(ctx, writer, "application/json", metadata)
	if err != nil {
		ctx.AbortWithError(http.StatusInternalServerError, err)
		return
	}
	err = writeData(ctx, writer, "application/octet-stream", data)
	if err != nil {
		ctx.AbortWithError(http.StatusInternalServerError, err)
		return
	}
	err = writer.Close()
	if err != nil {
		log.Println(err)
		ctx.AbortWithError(http.StatusInternalServerError,
			errors.New("unexpected internal error when writing "+
				"Response Data (close). Please retry and contact "+
				"the system admin if the problem persists"))
		return
	}
	ctx.Data(http.StatusOK, "multipart/mixed; boundary="+writer.Boundary(), response.Bytes())
}

func writeData(ctx *gin.Context, writer *multipart.Writer, contentType string, data []byte) error {
	dataPart, err := writer.CreatePart(textproto.MIMEHeader{"Content-Type": {contentType}})
	if err != nil {
		log.Println(err)
		return errors.New("unexpected internal error when writing " +
			"Response Data (create part). Please retry and contact " +
			"the system admin if the problem persists")
	}
	_, err = dataPart.Write(data)
	if err != nil {
		log.Println(err)
		return errors.New("unexpected internal error when writing " +
			"Response Data (write part). Please retry and contact " +
			"the system admin if the problem persists")
	}
	return nil
}

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

	ctx.JSON(status, &errorResponse{Error: error})
}
