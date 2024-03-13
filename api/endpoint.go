package api

import (
	"encoding/json"
	"fmt"
	"net/http"
	"strings"

	"github.com/gin-gonic/gin"
	"github.com/gin-gonic/gin/binding"

	"github.com/equinor/vds-slice/internal/cache"
	"github.com/equinor/vds-slice/internal/core"
)

func httpStatusCode(err error) int {
	switch err.(type) {
	case *core.InvalidArgument:
		return http.StatusBadRequest
	case *core.InternalError:
		return http.StatusInternalServerError
	default:
		return http.StatusInternalServerError
	}
}

/* Call abortOnError on the context in case of an error
 *
 * This function is designed specifically for our endpoint handler functions
 * and aims at making the error handling as short and concise as possible.
 *
 * If err != nil the error will be mapped to an appropriate http status code
 * through the httpStatusCode mapper, and ctx.AbortWithError will be called
 * with this status and the error itself. It then returns true to indicate that
 * the context have been aborted.
 *
 * If err == nil the ctx is left untouched and this function returns false,
 * indicating that the context was not aborted.
 *
 * The result is a one line error handling:
 *
 *     err, _ := func()
 *     if abortOnError(ctx, err) { return }
 */
func abortOnError(ctx *gin.Context, err error) bool {
	if err == nil {
		return false
	}

	ctx.AbortWithError(httpStatusCode(err), err)

	return true
}

type Endpoint struct {
	MakeVdsConnection core.ConnectionMaker
	Cache             cache.Cache
}

func prepareRequestLogging(ctx *gin.Context, request Stringable) {
	// ignore possible errors as they should not change outcome for the user
	requestString, _ := request.toString()
	ctx.Set("request", requestString)
}

func prepareMetricsLogging(ctx *gin.Context, request interface {
	credentials() ([]string, []string, string)
}) {
	vds, _, _ := request.credentials()
	ctx.Set("vds", vds)
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

func (e *Endpoint) makeDataRequest(
	ctx *gin.Context,
	request DataRequest,
) {
	prepareRequestLogging(ctx, request)
	prepareMetricsLogging(ctx, request)

	connections, binaryOperator, err := e.readConnectionParameters(ctx, request.getRequestedResource())
	if err != nil {
		return
	}

	cacheKey, err := request.hash()
	if abortOnError(ctx, err) {
		return
	}

	cacheEntry, hit := e.Cache.Get(cacheKey)
	if hit {
		var isAuthorizedToRead = true
		for i := 0; i < len(connections); i++ {
			if !connections[i].IsAuthorizedToRead() {
				isAuthorizedToRead = false
			}
		}
		if isAuthorizedToRead {
			ctx.Set("cache-hit", true)
			writeResponse(ctx, cacheEntry.Metadata(), cacheEntry.Data())
			return
		}
	}

	handle, err := core.CreateDSHandle(connections, binaryOperator)
	if abortOnError(ctx, err) {
		return
	}
	defer handle.Close()

	data, metadata, err := request.execute(handle)
	if abortOnError(ctx, err) {
		return
	}

	e.Cache.Set(cacheKey, cache.NewCacheEntry(data, metadata))

	writeResponse(ctx, metadata, data)
}

func (e *Endpoint) readConnectionParameters(
	ctx *gin.Context,
	request RequestedResource,
) ([]core.Connection, uint32, error) {

	vdsUrls, sasTokens, binaryOperatorString := request.credentials()

	binaryOperator, err := core.GetBinaryOperator(binaryOperatorString)
	if abortOnError(ctx, err) {
		return nil, 100, err
	}

	var connections []core.Connection

	if len(vdsUrls) == 1 && binaryOperator != core.BinaryOperatorNoOperator {
		err := core.NewInvalidArgument("Binary operator must be empty when a single VDS url is provided")
		if abortOnError(ctx, err) {
			return nil, 100, err
		}
	} else if len(vdsUrls) == 2 && binaryOperator == core.BinaryOperatorNoOperator {
		err := core.NewInvalidArgument("Binary operator must be provided when two VDS urls are provided")
		if abortOnError(ctx, err) {
			return nil, 100, err
		}
	} else if len(vdsUrls) > 2 {
		err := core.NewInvalidArgument("No endpoint accepts more than two vds urls.")
		if abortOnError(ctx, err) {
			return nil, 100, err
		}
	}

	for i := 0; i < len(vdsUrls); i++ {
		vdsConn, err := e.MakeVdsConnection(vdsUrls[i], sasTokens[i])
		if abortOnError(ctx, err) {
			return nil, 100, err
		}
		connections = append(connections, vdsConn)
	}

	return connections, binaryOperator, nil
}

func (request SliceRequest) execute(
	handle core.DSHandle,
) (data [][]byte, metadata []byte, err error) {
	axis, err := core.GetAxis(strings.ToLower(request.Direction))
	if err != nil {
		return
	}

	metadata, err = handle.GetSliceMetadata(
		*request.Lineno,
		axis,
		request.Bounds,
	)
	if err != nil {
		return
	}

	res, err := handle.GetSlice(*request.Lineno, axis, request.Bounds)
	if err != nil {
		return
	}
	data = [][]byte{res}

	return data, metadata, nil
}

func (request FenceRequest) execute(
	handle core.DSHandle,
) (data [][]byte, metadata []byte, err error) {
	coordinateSystem, err := core.GetCoordinateSystem(
		strings.ToLower(request.CoordinateSystem),
	)
	if err != nil {
		return
	}

	interpolation, err := core.GetInterpolationMethod(request.Interpolation)
	if err != nil {
		return
	}

	metadata, err = handle.GetFenceMetadata(request.Coordinates)
	if err != nil {
		return
	}

	res, err := handle.GetFence(
		coordinateSystem,
		request.Coordinates,
		interpolation,
		request.FillValue,
	)
	if err != nil {
		return
	}
	data = [][]byte{res}

	return data, metadata, nil
}

func validateVerticalWindow(above float32, below float32, stepSize float32) error {
	const lowerBound = 0
	const upperBound = 250

	if above < lowerBound || above >= upperBound {
		return core.NewInvalidArgument(fmt.Sprintf(
			"'above' out of range! Must be within [%d, %d], was %f",
			lowerBound,
			upperBound,
			above,
		))
	}

	if below < lowerBound || below >= upperBound {
		return core.NewInvalidArgument(fmt.Sprintf(
			"'below' out of range! Must be within [%d, %d], was %f",
			lowerBound,
			upperBound,
			below,
		))
	}

	if stepSize < lowerBound {
		return core.NewInvalidArgument(fmt.Sprintf(
			"'stepsize' out of range! Must be bigger than %d, was %f",
			lowerBound,
			stepSize,
		))
	}

	return nil
}

func (request AttributeAlongSurfaceRequest) execute(
	handle core.DSHandle,
) (data [][]byte, metadata []byte, err error) {
	err = validateVerticalWindow(request.Above, request.Below, request.Stepsize)
	if err != nil {
		return
	}

	interpolation, err := core.GetInterpolationMethod(request.Interpolation)
	if err != nil {
		return
	}

	metadata, err = handle.GetAttributeMetadata(request.Surface.Values)
	if err != nil {
		return
	}

	data, err = handle.GetAttributesAlongSurface(
		request.Surface,
		request.Above,
		request.Below,
		request.Stepsize,
		request.Attributes,
		interpolation,
	)
	if err != nil {
		return
	}

	return data, metadata, nil
}

func (request AttributeBetweenSurfacesRequest) execute(
	handle core.DSHandle,
) (data [][]byte, metadata []byte, err error) {
	interpolation, err := core.GetInterpolationMethod(request.Interpolation)
	if err != nil {
		return
	}

	metadata, err = handle.GetAttributeMetadata(request.PrimarySurface.Values)
	if err != nil {
		return
	}

	data, err = handle.GetAttributesBetweenSurfaces(
		request.PrimarySurface,
		request.SecondarySurface,
		request.Stepsize,
		request.Attributes,
		interpolation,
	)
	if err != nil {
		return
	}

	return data, metadata, nil
}

func parseGetRequest(ctx *gin.Context, v Normalizable) error {
	query, status := ctx.GetQuery("query")
	if !status {
		return core.NewInvalidArgument(
			"GET request to specified endpoint requires a 'query' parameter",
		)
	}
	if err := json.Unmarshal([]byte(query), v); err != nil {
		msg := "Please ensure that the supplied query is valid " +
			"and conforms to the expected swagger Request specification: %v"
		return core.NewInvalidArgument(
			fmt.Sprintf(msg, err.Error()))
	}

	if err := binding.Validator.ValidateStruct(v); err != nil {
		return core.NewInvalidArgument(err.Error())
	}

	return v.NormalizeConnection()
}

func parsePostRequest(ctx *gin.Context, v Normalizable) error {
	if err := ctx.ShouldBind(v); err != nil {
		return core.NewInvalidArgument(err.Error())
	}
	return v.NormalizeConnection()
}

func (e *Endpoint) Health(ctx *gin.Context) {

	ctx.HTML(http.StatusOK, "index.html", gin.H{})
}

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

// SliceGet godoc
// @Summary  Fetch a slice from a VDS
// @description.markdown slice
// @Tags     slice
// @Param    query  query  string  True  "Urlencoded/escaped SliceRequest"
// @Produce  multipart/mixed
// @Success  200 {object} core.SliceMetadata "(Example below only for metadata part)"
// @Failure  400 {object} ErrorResponse "Request is invalid"
// @Failure  500 {object} ErrorResponse "openvds failed to process the request"
// @Router   /slice  [get]
func (e *Endpoint) SliceGet(ctx *gin.Context) {
	var request SliceRequest
	err := parseGetRequest(ctx, &request)
	if abortOnError(ctx, err) {
		return
	}

	e.makeDataRequest(ctx, request)
}

// SlicePost godoc
// @Summary  Fetch a slice from a VDS
// @description.markdown slice
// @Tags     slice
// @Param    body  body  SliceRequest  True  "Query Parameters"
// @Accept   application/json
// @Produce  multipart/mixed
// @Success  200 {object} core.SliceMetadata "(Example below only for metadata part)"
// @Failure  400 {object} ErrorResponse "Request is invalid"
// @Failure  500 {object} ErrorResponse "openvds failed to process the request"
// @Router   /slice  [post]
func (e *Endpoint) SlicePost(ctx *gin.Context) {
	var request SliceRequest
	err := parsePostRequest(ctx, &request)
	if abortOnError(ctx, err) {
		return
	}

	e.makeDataRequest(ctx, request)
}

// FenceGet godoc
// @Summary  Returns traces along an arbitrary path, such as a well-path
// @description.markdown fence
// @Tags     fence
// @Param    query  query  string  True  "Urlencoded/escaped FenceResponse"
// @Accept   application/json
// @Produce  multipart/mixed
// @Success  200 {object} core.FenceMetadata "(Example below only for metadata part)"
// @Failure  400 {object} ErrorResponse "Request is invalid"
// @Failure  500 {object} ErrorResponse "openvds failed to process the request"
// @Router   /fence  [get]
func (e *Endpoint) FenceGet(ctx *gin.Context) {
	var request FenceRequest
	err := parseGetRequest(ctx, &request)
	if abortOnError(ctx, err) {
		return
	}

	e.makeDataRequest(ctx, request)
}

// FencePost godoc
// @Summary  Returns traces along an arbitrary path, such as a well-path
// @description.markdown fence
// @Tags     fence
// @Param    body  body  FenceRequest  True  "Request Parameters"
// @Accept   application/json
// @Produce  multipart/mixed
// @Success  200 {object} core.FenceMetadata "(Example below only for metadata part)"
// @Failure  400 {object} ErrorResponse "Request is invalid"
// @Failure  500 {object} ErrorResponse "openvds failed to process the request"
// @Router   /fence  [post]
func (e *Endpoint) FencePost(ctx *gin.Context) {
	var request FenceRequest
	err := parsePostRequest(ctx, &request)
	if abortOnError(ctx, err) {
		return
	}

	e.makeDataRequest(ctx, request)
}

// AttributesAlongSurfacePost godoc
// @Summary  Returns horizon attributes along the surface
// @description.markdown attribute_along
// @Tags     attributes
// @Param    body  body  AttributeAlongSurfaceRequest  True  "Request Parameters"
// @Accept   application/json
// @Produce  multipart/mixed
// @Success  200 {object} core.AttributeMetadata "(Example below only for metadata part)"
// @Failure  400 {object} ErrorResponse "Request is invalid"
// @Failure  500 {object} ErrorResponse "openvds failed to process the request"
// @Router   /attributes/surface/along  [post]
func (e *Endpoint) AttributesAlongSurfacePost(ctx *gin.Context) {
	var request AttributeAlongSurfaceRequest
	err := parsePostRequest(ctx, &request)
	if abortOnError(ctx, err) {
		return
	}

	e.makeDataRequest(ctx, request)
}

// AttributesBetweenSurfacesPost godoc
// @Summary  Returns horizon attributes between provided surfaces
// @description.markdown attribute_between
// @Tags     attributes
// @Param    body  body  AttributeBetweenSurfacesRequest  True  "Request Parameters"
// @Accept   application/json
// @Produce  multipart/mixed
// @Success  200 {object} core.AttributeMetadata "(Example below only for metadata part)"
// @Failure  400 {object} ErrorResponse "Request is invalid"
// @Failure  500 {object} ErrorResponse "openvds failed to process the request"
// @Router   /attributes/surface/between  [post]
func (e *Endpoint) AttributesBetweenSurfacesPost(ctx *gin.Context) {
	var request AttributeBetweenSurfacesRequest
	err := parsePostRequest(ctx, &request)
	if abortOnError(ctx, err) {
		return
	}

	e.makeDataRequest(ctx, request)
}
