package api

import (
	"encoding/json"
	"log"
	"net/http"
	"net/url"
	"strings"

	"github.com/gin-gonic/gin"
	"github.com/gin-gonic/gin/binding"

	"github.com/equinor/vds-slice/internal/vds"
)


type SliceQuery struct {
	Vds       string  `form:"vds"       json:"vds"       binding:"required"`
	Direction string  `form:"direction" json:"direction" binding:"required"`
	Lineno    *int    `form:"lineno"    json:"lineno"    binding:"required"`
	Sas       string  `form:"sas"       json:"sas"       binding:"required"`
}

type Endpoint struct {
	StorageURL string
	Protocol   string
}

func (e *Endpoint) sliceMetadata(ctx *gin.Context, query SliceQuery) {
	conn, err := vds.MakeConnection(e.Protocol,  e.StorageURL, query.Vds, query.Sas)
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	axis, err := vds.GetAxis(strings.ToLower(query.Direction))
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	buffer, err := vds.SliceMetadata(*conn, *query.Lineno, axis)
	if err != nil {
		log.Println(err)
		ctx.AbortWithStatus(http.StatusInternalServerError)
		return
	}

	ctx.Data(http.StatusOK, "application/json", buffer)
}

func (e *Endpoint) slice(ctx *gin.Context, query SliceQuery) {
	conn, err := vds.MakeConnection(e.Protocol, e.StorageURL, query.Vds, query.Sas)
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	axis, err := vds.GetAxis(strings.ToLower(query.Direction))
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	buffer, err := vds.Slice(*conn, *query.Lineno, axis)
	if err != nil {
		log.Println(err)
		ctx.AbortWithStatus(http.StatusInternalServerError)
		return
	}

	ctx.Data(http.StatusOK, "application/octet-stream", buffer)
}

func sliceParseGetReq(ctx *gin.Context) (*SliceQuery, error) {
	var query SliceQuery

	q, err := url.QueryUnescape(ctx.Query("query"))
	if err != nil {
		return nil, err
	}

	err = json.Unmarshal([]byte(q), &query)
	if err != nil {
		return nil, err
	}

	if err = binding.Validator.ValidateStruct(&query); err != nil {
		return nil, err
	}

	return &query, nil
}

func sliceParsePostReq(ctx *gin.Context) (*SliceQuery, error) {
	var query SliceQuery
	if err := ctx.ShouldBind(&query); err != nil {
		return nil, err
	}
	return &query, nil
}

func (e *Endpoint) Health(ctx *gin.Context) {
	ctx.String(http.StatusOK, "I am up and running")
}

// SliceMetadata godoc
// @Summary  Fetch metadata related to a single slice
// @Param    query  query  string  True  "Urlencoded/escaped SliceQuery"
// @Produce  application/json
// @Success  200  {object}  vds.Metadata
// @Router   /slice/metadata  [get]
func (e *Endpoint) SliceMetadataGet(ctx *gin.Context) {
	query, err := sliceParseGetReq(ctx)
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}
	e.sliceMetadata(ctx, *query)
}

// SliceMetadata godoc
// @Summary  Fetch metadata related to a single slice
// @Param    body  body  SliceQuery  True  "Query Parameters"
// @Accept   application/json
// @Produce  application/json
// @Success  200  {object}  vds.Metadata
// @Router   /slice/metadata  [post]
func (e *Endpoint) SliceMetadataPost(ctx *gin.Context) {
	query, err := sliceParsePostReq(ctx)
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}
	e.sliceMetadata(ctx, *query)
}

// Slice godoc
// @Summary  Fetch a single slice in any cube direction
// @Param    query  query  string  True  "Urlencoded/escaped SliceQuery"
// @Produce  application/octet-stream
// @Success  200
// @Router   /slice/metadata  [get]
func (e *Endpoint) SliceGet(ctx *gin.Context) {
	query, err := sliceParseGetReq(ctx)
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}
	e.slice(ctx, *query)
}

// Slice godoc
// @Summary  Fetch a single slice in any cube direction
// @Param    body  body  SliceQuery  True  "Query Parameters"
// @Accept   application/json
// @Produce  application/octet-stream
// @Success  200
// @Router   /slice/metadata  [post]
func (e *Endpoint) SlicePost(ctx *gin.Context) {
	query, err := sliceParsePostReq(ctx)
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}
	e.slice(ctx, *query)
}
