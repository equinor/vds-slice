package api

import (
	"fmt"
	"log"
	"net/http"
	"strings"

	"github.com/gin-gonic/gin"

	"github.com/equinor/vds-slice/internal/vds"
)

type SliceQuery struct {
	Vds       string `form:"vds"       json:"vds"       binding:"required"`
	Direction string `form:"direction" json:"direction" binding:"required"`
	Lineno    *int   `form:"lineno"    json:"lineno"    binding:"required"`
}

type Endpoint struct {
	StorageURL string
}

func (e *Endpoint) Health(ctx *gin.Context) {
	ctx.String(http.StatusOK, "I am up and running")
}

func (e *Endpoint) sliceMetadata(ctx *gin.Context) {
	var query SliceQuery

	if err := ctx.ShouldBind(&query); err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	querystr := ctx.Request.URL.Query()

	delete(querystr, "vds")
	delete(querystr, "direction")
	delete(querystr, "lineno")

	url := fmt.Sprintf("azure://%v", query.Vds)

	/*
	 * This is super buggy as assumes that no other query-paramters are
	 * present.
	 */
	cred := fmt.Sprintf(
		"BlobEndpoint=%v;SharedAccessSignature=?%v",
		e.StorageURL,
		querystr.Encode(),
	)

	axis, err := vds.GetAxis(strings.ToLower(query.Direction))

	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	buffer, err := vds.SliceMetadata(url, cred, *query.Lineno, axis)
	if err != nil {
		log.Println(err)
		ctx.AbortWithStatus(http.StatusInternalServerError)
		return
	}

	ctx.Data(http.StatusOK, "application/json", buffer)
}

func (e *Endpoint) slice(ctx *gin.Context) {
	var query SliceQuery

	if err := ctx.ShouldBind(&query); err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	querystr := ctx.Request.URL.Query()

	delete(querystr, "vds")
	delete(querystr, "direction")
	delete(querystr, "lineno")

	url := fmt.Sprintf("azure://%v", query.Vds)

	/*
	 * This is super buggy as assumes that no other query-paramters are
	 * present.
	 */
	cred := fmt.Sprintf(
		"BlobEndpoint=%v;SharedAccessSignature=?%v",
		e.StorageURL,
		querystr.Encode(),
	)

	axis, err := vds.GetAxis(strings.ToLower(query.Direction))

	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	buffer, err := vds.Slice(url, cred, *query.Lineno, axis)
	if err != nil {
		log.Println(err)
		ctx.AbortWithStatus(http.StatusInternalServerError)
		return
	}

	ctx.Data(http.StatusOK, "application/octet-stream", buffer)
}

func (e *Endpoint) SliceMetadataGet(ctx *gin.Context) {
	e.sliceMetadata(ctx)
}

func (e *Endpoint) SliceMetadataPost(ctx *gin.Context) {
	e.sliceMetadata(ctx)
}

func (e *Endpoint) SliceGet(ctx *gin.Context) {
	e.slice(ctx)
}

func (e *Endpoint) SlicePost(ctx *gin.Context) {
	e.slice(ctx)
}
