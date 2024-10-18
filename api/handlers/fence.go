package handlers

import (
	"fmt"
	"strings"

	"github.com/equinor/oneseismic-api/internal/cache"
	"github.com/equinor/oneseismic-api/internal/core"

	"github.com/gin-gonic/gin"
)

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

type FenceRequest struct {
	RequestedResource
	// Coordinate system for the requested fence
	// Supported options are:
	// ilxl : inline, crossline pairs
	// ij   : Coordinates are given as in 0-indexed system, where the first
	//        line in each direction is 0 and the last is number-of-lines - 1.
	// cdp  : Coordinates are given as cdpx/cdpy pairs. In the original SEGY
	//        this would correspond to the cdpx and cdpy fields in the
	//        trace-headers after applying the scaling factor.
	CoordinateSystem string `json:"coordinateSystem" binding:"required" example:"cdp"`

	// A list of (x, y) points in the coordinate system specified in
	// coordinateSystem, for example [[2000.5, 100.5], [2050, 200], [10, 20]].
	Coordinates [][]float32 `json:"coordinates" binding:"required"`

	// Interpolation method
	// Supported options are: nearest, linear, cubic, angular and triangular.
	// Defaults to nearest.
	// This field is passed on to OpenVDS, which does the actual interpolation.
	// Note: For nearest interpolation result will snap to the nearest point
	// as per "half up" rounding. This is different from openvds logic.
	Interpolation string `json:"interpolation" example:"linear"`

	// Providing a FillValue is optional and will be used for the sample points
	// that lie outside the seismic cube.
	// Note: In case the FillValue is not set, and any of the provided coordinates
	// fall outside the seismic cube, the request will be rejected with an error.
	FillValue *float32 `json:"fillValue"`
} //@name FenceRequest

func (f FenceRequest) toString() (string, error) {
	coordinates := func() string {
		var length = len(f.Coordinates)
		const halfPrintLength = 5
		const printLength = halfPrintLength * 2
		if length > printLength {
			return fmt.Sprintf("%v, ...[%d element(s) skipped]..., %v",
				f.Coordinates[0:halfPrintLength],
				length-printLength,
				f.Coordinates[length-halfPrintLength:length])
		} else {
			return fmt.Sprintf("%v", f.Coordinates)
		}
	}()

	fillValue := "None"
	if f.FillValue != nil {
		fillValue = fmt.Sprintf("%.2f", *f.FillValue)
	}

	msg := "{%s, coordinate system: %s, coordinates: %s, " +
		"interpolation (optional): %s, fill value (optional): %s}"

	return fmt.Sprintf(
		msg,
		f.RequestedResource.toString(),
		f.CoordinateSystem,
		coordinates,
		f.Interpolation,
		fillValue,
	), nil
}

// gob library is used to serialize data
// however, library does not distinguish between FillValue pointing to 0 or to nil
// so additional wrapper type is created to avoid same hash for different requests
type HashableFenceRequest struct {
	FenceRequest
	IsFillValueSupplied bool
}

/** Compute a hash of the request that uniquely identifies the requested fence
 *
 * The hash is computed based on all fields that contribute toward a unique response.
 * I.e. every field except the sas token and with additional fill value information
 */
func (f FenceRequest) hash() (string, error) {
	// Strip the sas tokens before computing hash
	f.Sas = nil

	r := HashableFenceRequest{FenceRequest: f}
	if r.FillValue != nil {
		r.IsFillValueSupplied = true
	}
	return cache.Hash(r)
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
