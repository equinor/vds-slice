package api

import (
	"bytes"
	"encoding/json"
	"github.com/gin-gonic/gin"
	"log"
	"mime/multipart"
	"net/http"
	"net/textproto"
)

type ResponseMetadata struct {
	HasError bool
	Errors   []string
	Format   string
}

func createCommonMetadata(ctx *gin.Context, writer *multipart.Writer, format string) ([]byte, error) {
	hasError := len(ctx.Errors) != 0 || ctx.Writer.Status() != http.StatusOK
	errors := []string{}

	for _, err := range ctx.Errors {
		errors = append(errors, err.Error())
	}

	commonMetadata := &ResponseMetadata{
		HasError: hasError,
		Errors:   errors,
		Format:   format,
	}

	return json.Marshal(commonMetadata)
}

func writeData(ctx *gin.Context, writer *multipart.Writer, contentType string, data []byte) {
	dataPart, err := writer.CreatePart(textproto.MIMEHeader{"Content-Type": {contentType}})
	if err != nil {
		log.Println(err)
		ctx.AbortWithStatus(http.StatusInternalServerError)
		return
	}
	_, err = dataPart.Write(data)
	if err != nil {
		log.Println(err)
		ctx.AbortWithStatus(http.StatusInternalServerError)
		return
	}
}

func ResponseHandler(ctx *gin.Context) {

	response := &bytes.Buffer{}
	writer := multipart.NewWriter(response)
	// gin seems to override content-type in some cases, so it must be set explicitly
	ctx.Writer.Header().Set("Content-Type", "multipart/mixed; boundary="+writer.Boundary())
	ctx.Next()

	format := ctx.GetString("format")

	metadata, exists := ctx.Get("metadata")
	if !exists {
		metadata = []byte{}
	}

	data, exists := ctx.Get("data")
	if !exists {
		data = []byte{}
	}

	commonMetadataJSON, err := createCommonMetadata(ctx, writer, format)
	if err != nil {
		log.Println(err)
		ctx.AbortWithStatus(http.StatusInternalServerError)
		return
	}
	writeData(ctx, writer, "application/json", commonMetadataJSON)
	writeData(ctx, writer, "application/json", metadata.([]byte))
	writeData(ctx, writer, "application/octet-stream", data.([]byte))
	err = writer.Close()
	if err != nil {
		log.Println(err)
		ctx.AbortWithStatus(http.StatusInternalServerError)
		return
	}
	ctx.Data(-1, "multipart/mixed; boundary="+writer.Boundary(), response.Bytes())
}
