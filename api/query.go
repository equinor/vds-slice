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

// Query for slice endpoints
// @Description Query payload for slice endpoints /slice and /slice/metadata.
type SliceQuery struct {
	// The blob path to a vds on form: container/subpath
	Vds string `form:"vds" json:"vds" binding:"required"`

	// Direction can be specified in two domains
	// - Annotation. Valid options: Inline, Crossline and Depth/Time/Sample
	// - Index. Valid options: i, j and k.
	//
	// Only one of Depth, Time and Sample is valid for a given VDS. Which one
	// depends on the depth units. E.g. Depth is a valid option for a VDS with
	// depth unit "m" or "ft", but not if the units are "ms", "s".
	// Sample is valid when the depth is unitless.
	//
	// i, j, k are zero-indexed and correspond to Inline, Crossline,
	// Depth/Time/Sample, respectably.
	//
	// All options are case-insensitive.
	Direction string `form:"direction" json:"direction" binding:"required"`

	// Linenumber of the slice
	Lineno *int `form:"lineno" json:"lineno" binding:"required"`

	// A valid sas-token with read access to the container specified in Vds
	Sas string `form:"sas" json:"sas" binding:"required"`
} //@name SliceQuery

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
// @Router   /slice  [get]
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
// @Router   /slice  [post]
func (e *Endpoint) SlicePost(ctx *gin.Context) {
	query, err := sliceParsePostReq(ctx)
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}
	e.slice(ctx, *query)
}
