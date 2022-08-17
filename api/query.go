package api

import (
	"encoding/json"
	"net/http"
	"strings"

	"github.com/gin-gonic/gin"
	"github.com/gin-gonic/gin/binding"

	"github.com/equinor/vds-slice/internal/vds"
)

type FenceRequest struct {
	Vds              string      `json:"vds"               binding:"required"`
	CoordinateSystem string      `json:"coordinate_system" binding:"required"`
	Fence            [][]float32 `json:"coordinates"       binding:"required"`
	Interpolation    string      `json:"interpolation"`
	Sas              string      `json:"sas"               binding:"required"`
}

// Query for slice endpoints
// @Description Query payload for slice endpoint /slice.
type SliceRequest struct {
	// The blob path to a vds in form: container/subpath
	Vds string `json:"vds" binding:"required"`

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
	// Depth/Time/Sample, respectively.
	//
	// All options are case-insensitive.
	Direction string `json:"direction" binding:"required"`

	// Line number of the slice
	Lineno *int `json:"lineno" binding:"required"`

	// A valid sas-token with read access to the container specified in Vds
	Sas string `json:"sas" binding:"required"`
} //@name SliceRequest

type Endpoint struct {
	StorageURL string
	Protocol   string
}

func (e *Endpoint) slice(ctx *gin.Context, query SliceRequest) {
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

	metadata, err := vds.SliceMetadata(*conn, *query.Lineno, axis)
	if err != nil {
		ctx.AbortWithError(http.StatusInternalServerError, err)
		return
	}

	data, err := vds.Slice(*conn, *query.Lineno, axis)
	if err != nil {
		ctx.AbortWithError(http.StatusInternalServerError, err)
		return
	}
	writeResponse(ctx, metadata, data)
}

func parseGetRequest(ctx *gin.Context, v interface{}) error {
	if err := json.Unmarshal([]byte(ctx.Query("query")), v); err != nil {
		return err
	}

	return binding.Validator.ValidateStruct(v)
}

func (e *Endpoint) Health(ctx *gin.Context) {
	ctx.String(http.StatusOK, "I am up and running")
}

// SliceGet godoc
// @Summary  Fetch a slice from a VDS
// @description.markdown slice
// @Param    query  query  string  True  "Urlencoded/escaped SliceRequest"
// @Produce  multipart/mixed
// @Success  200 {object} vds.Metadata "(Example below only for metadata part)"
// @Router   /slice  [get]
func (e *Endpoint) SliceGet(ctx *gin.Context) {
	var query SliceRequest
	if err := parseGetRequest(ctx, &query); err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}
	e.slice(ctx, query)
}

// SlicePost godoc
// @Summary  Fetch metadata related to a single slice
// @description.markdown slice
// @Param    body  body  SliceRequest  True  "Query Parameters"
// @Accept   application/json
// @Produce  multipart/mixed
// @Success  200 {object} vds.Metadata "(Example below only for metadata part)"
// @Router   /slice  [post]
func (e *Endpoint) SlicePost(ctx *gin.Context) {
	var query SliceRequest
	if err := ctx.ShouldBind(&query); err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}
	e.slice(ctx, query)
}

func (e *Endpoint) FenceGet(ctx *gin.Context) {
	var query FenceRequest
	if err := parseGetRequest(ctx, &query); err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}
	e.fence(ctx, query)
}

func (e *Endpoint) FencePost(ctx *gin.Context) {
	var query FenceRequest
	if err := ctx.ShouldBind(&query); err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}
	e.fence(ctx, query)
}

func (e *Endpoint) fence(ctx *gin.Context, query FenceRequest) {
	conn, err := vds.MakeConnection(e.Protocol, e.StorageURL, query.Vds, query.Sas)
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	interpolation, err := vds.GetInterpolationMethod(query.Interpolation)
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	data, err := vds.Fence(*conn, query.CoordinateSystem, query.Fence, interpolation)

	if err != nil {
		ctx.AbortWithError(http.StatusInternalServerError, err)
		return
	}
	writeResponse(ctx, []byte{}, data)
}
