package handlers

import (
	"fmt"
	"net/http"

	"github.com/gin-gonic/gin"

	"github.com/equinor/oneseismic-api/internal/core"
)

// MetadataGet godoc
// @Summary  Return volumetric metadata about the VDS
// @description.markdown metadata
// @Tags     metadata
// @Param    query  query  string  True  "Urlencoded/escaped MetadataRequest"
// @Produce  json
// @Success  200 {object} core.Metadata
// @Failure  400 {object} ErrorResponse "Request is invalid"
// @Failure  500 {object} ErrorResponse "openvds failed to process the request"
// @Router   /metadata  [get]
func (e *Endpoint) MetadataGet(ctx *gin.Context) {
	var request MetadataRequest
	err := parseGetRequest(ctx, &request)
	if abortOnError(ctx, err) {
		return
	}

	e.metadata(ctx, request)
}

// MetadataPost godoc
// @Summary  Return volumetric metadata about the VDS
// @description.markdown metadata
// @Tags     metadata
// @Param    body  body  MetadataRequest  True  "Request parameters"
// @Produce  json
// @Success  200 {object} core.Metadata
// @Failure  400 {object} ErrorResponse "Request is invalid"
// @Failure  500 {object} ErrorResponse "openvds failed to process the request"
// @Router   /metadata  [post]
func (e *Endpoint) MetadataPost(ctx *gin.Context) {
	var request MetadataRequest
	err := parsePostRequest(ctx, &request)
	if abortOnError(ctx, err) {
		return
	}

	e.metadata(ctx, request)
}

func (e *Endpoint) metadata(ctx *gin.Context, request MetadataRequest) {
	prepareRequestLogging(ctx, request)
	prepareMetricsLogging(ctx, request.RequestedResource)

	connections, binaryOperator, err := e.readConnectionParameters(ctx, request.RequestedResource)

	if err != nil {
		return
	}

	handle, err := core.CreateDSHandle(connections, binaryOperator)
	if abortOnError(ctx, err) {
		return
	}
	defer handle.Close()

	buffer, err := handle.GetMetadata()
	if abortOnError(ctx, err) {
		return
	}

	ctx.Data(http.StatusOK, "application/json", buffer)
}

type MetadataRequest struct {
	RequestedResource
} //@name MetadataRequest

func (m MetadataRequest) toString() (string, error) {
	return fmt.Sprintf("{%s}",
		m.RequestedResource.toString(),
	), nil
}
