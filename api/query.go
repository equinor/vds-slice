package api

import (
	"encoding/json"
	"net/http"
	"strings"

	"github.com/gin-gonic/gin"
	"github.com/gin-gonic/gin/binding"

	"github.com/equinor/vds-slice/internal/vds"
)

type Endpoint struct {
	MakeVdsConnection vds.ConnectionMaker
}

func (e *Endpoint) metadata(ctx *gin.Context, query MetadataRequest) {
	conn, err := e.MakeVdsConnection(query.Vds, query.Sas)
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	buffer, err := vds.GetMetadata(*conn)
	if err != nil {
		ctx.AbortWithError(http.StatusInternalServerError, err)
		return
	}

	ctx.Data(http.StatusOK, "application/json", buffer)
}

func (e *Endpoint) slice(ctx *gin.Context, query SliceRequest) {
	conn, err := e.MakeVdsConnection(query.Vds, query.Sas)
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	axis, err := vds.GetAxis(strings.ToLower(query.Direction))
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	metadata, err := vds.GetSliceMetadata(*conn, *query.Lineno, axis)
	if err != nil {
		ctx.AbortWithError(http.StatusInternalServerError, err)
		return
	}

	data, err := vds.GetSlice(*conn, *query.Lineno, axis)
	if err != nil {
		ctx.AbortWithError(http.StatusInternalServerError, err)
		return
	}
	writeResponse(ctx, metadata, data)
}

func (e *Endpoint) fence(ctx *gin.Context, query FenceRequest) {
	conn, err := e.MakeVdsConnection(query.Vds, query.Sas)
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	coordinateSystem, err := vds.GetCoordinateSystem(strings.ToLower(query.CoordinateSystem))
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	interpolation, err := vds.GetInterpolationMethod(query.Interpolation)
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	metadata, err := vds.GetFenceMetadata(*conn, query.Coordinates)
	if err != nil {
		ctx.AbortWithError(http.StatusInternalServerError, err)
		return
	}

	data, err := vds.GetFence(
		*conn,
		coordinateSystem,
		query.Coordinates,
		interpolation,
	)

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

// MetadataGet godoc
// @Summary  Return volumetric metadata about the VDS
// @Tags     metadata
// @Param    query  query  string  True  "Urlencoded/escaped MetadataRequest"
// @Produce  json
// @Success  200 {object} vds.Metadata
// @Failure  400 {object} ErrorResponse "Request is invalid"
// @Failure  500 {object} ErrorResponse "openvds failed to process the request"
// @Router   /metadata  [get]
func (e *Endpoint) MetadataGet(ctx *gin.Context) {
	var query MetadataRequest
	if err := parseGetRequest(ctx, &query); err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}
	e.metadata(ctx, query)
}

// MetadataPost godoc
// @Summary  Return volumetric metadata about the VDS
// @Tags     metadata
// @Param    body  body  MetadataRequest  True  "Request parameters"
// @Produce  json
// @Success  200 {object} vds.Metadata
// @Failure  400 {object} ErrorResponse "Request is invalid"
// @Failure  500 {object} ErrorResponse "openvds failed to process the request"
// @Router   /metadata  [post]
func (e *Endpoint) MetadataPost(ctx *gin.Context) {
	var query MetadataRequest
	if err := ctx.ShouldBind(&query); err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}
	e.metadata(ctx, query)
}

// SliceGet godoc
// @Summary  Fetch a slice from a VDS
// @description.markdown slice
// @Tags     slice
// @Param    query  query  string  True  "Urlencoded/escaped SliceRequest"
// @Produce  multipart/mixed
// @Success  200 {object} vds.SliceMetadata "(Example below only for metadata part)"
// @Failure  400 {object} ErrorResponse "Request is invalid"
// @Failure  500 {object} ErrorResponse "openvds failed to process the request"
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
// @Summary  Fetch a slice from a VDS
// @description.markdown slice
// @Tags     slice
// @Param    body  body  SliceRequest  True  "Query Parameters"
// @Accept   application/json
// @Produce  multipart/mixed
// @Success  200 {object} vds.SliceMetadata "(Example below only for metadata part)"
// @Failure  400 {object} ErrorResponse "Request is invalid"
// @Failure  500 {object} ErrorResponse "openvds failed to process the request"
// @Router   /slice  [post]
func (e *Endpoint) SlicePost(ctx *gin.Context) {
	var query SliceRequest
	if err := ctx.ShouldBind(&query); err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}
	e.slice(ctx, query)
}

// FenceGet godoc
// @Summary  Returns traces along an arbitrary path, such as a well-path
// @description.markdown fence
// @Tags     fence
// @Param    query  query  string  True  "Urlencoded/escaped FenceResponse"
// @Accept   application/json
// @Produce  multipart/mixed
// @Success  200 {object} vds.FenceMetadata "(Example below only for metadata part)"
// @Failure  400 {object} ErrorResponse "Request is invalid"
// @Failure  500 {object} ErrorResponse "openvds failed to process the request"
// @Router   /fence  [get]
func (e *Endpoint) FenceGet(ctx *gin.Context) {
	var query FenceRequest
	if err := parseGetRequest(ctx, &query); err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}
	e.fence(ctx, query)
}

// FencePost godoc
// @Summary  Returns traces along an arbitrary path, such as a well-path
// @description.markdown fence
// @Tags     fence
// @Param    body  body  FenceRequest  True  "Request Parameters"
// @Accept   application/json
// @Produce  multipart/mixed
// @Success  200 {object} vds.FenceMetadata "(Example below only for metadata part)"
// @Failure  400 {object} ErrorResponse "Request is invalid"
// @Failure  500 {object} ErrorResponse "openvds failed to process the request"
// @Router   /fence  [post]
func (e *Endpoint) FencePost(ctx *gin.Context) {
	var query FenceRequest
	if err := ctx.ShouldBind(&query); err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}
	e.fence(ctx, query)
}
