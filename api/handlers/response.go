package handlers

import (
	"bytes"
	"errors"
	"github.com/gin-gonic/gin"
	"log"
	"mime/multipart"
	"net/http"
	"net/textproto"
)

func writeResponse(ctx *gin.Context, metadata []byte, data [][]byte) {
	response := &bytes.Buffer{}
	writer := multipart.NewWriter(response)

	err := writeData(ctx, writer, "application/json", metadata)
	if err != nil {
		ctx.AbortWithError(http.StatusInternalServerError, err)
		return
	}

	for _, part := range data {
		err = writeData(ctx, writer, "application/octet-stream", part)
		if err != nil {
			ctx.AbortWithError(http.StatusInternalServerError, err)
			return
		}
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
