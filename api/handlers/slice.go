package handlers

import (
	"fmt"
	"strings"

	"github.com/gin-gonic/gin"

	"github.com/equinor/vds-slice/internal/cache"
	"github.com/equinor/vds-slice/internal/core"
)

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

// Query for slice endpoints
// @Description Query payload for slice endpoint /slice.
type SliceRequest struct {
	RequestedResource

	// Direction can be specified in two domains
	// - Annotation. Valid options: Inline, Crossline and Depth/Time/Sample
	// - Index. Valid options: i, j and k.
	//
	// When requesting z-slices using annotation it's recommended to use Depth
	// or Time as they include validation against the VDS itself. I.e. the
	// server returns an error if you request a time-slice from a depth cube
	// and visa-versa. Use Sample if you don't care for such guarantees or as a
	// fallback if the VDS file is wonky.
	//
	// i, j, k are zero-indexed and correspond to Inline, Crossline,
	// Depth/Time/Sample, respectively.
	//
	// All options are case-insensitive.
	Direction string `json:"direction" binding:"required" example:"inline"`

	// Line number of the slice
	Lineno *int `json:"lineno" binding:"required" example:"10000"`

	// Restrict the slice in the other dimensions (sub-slicing)
	//
	// Bounds can be used to retrieve sub-slices. For example: when requesting
	// a time slice you can set inline and/or crossline bounds to restrict the
	// area of the slice.
	//
	// Bounds are optional. Not specifying any bounds will produce the requested
	// slice through the entire volume.
	//
	// Any bounds in the same direction as the slice itself are ignored.
	//
	// Bounds are applied one after the other. If there are multiple bounds in the
	// same direction, the last one takes precedence.
	//
	// Bounds will throw out-of-bounds error if their range is outside the
	// cubes dimensions.
	//
	// Bounds can be set using both annotation and index. You are free to mix
	// and match as you see fit.
	Bounds []core.Bound `json:"bounds" binding:"dive"`
} //@name SliceRequest

/** Compute a hash of the request that uniquely identifies the requested slice
 *
 * The hash is computed based on all fields that contribute toward a unique response.
 * I.e. every field except the sas token.
 */
func (s SliceRequest) hash() (string, error) {
	// Strip the sas tokens before computing hash
	s.Sas = nil
	return cache.Hash(s)
}

func (s SliceRequest) toString() (string, error) {

	bounds := func() string {
		var allBounds []string
		for _, bound := range s.Bounds {
			allBounds = append(allBounds,
				fmt.Sprintf("%s: [%d, %d]", *bound.Direction, *bound.Lower, *bound.Upper))
		}
		return strings.Join(allBounds, ", ")
	}()

	return fmt.Sprintf("{%s, direction: %s, lineno: %d, bounds: %s}",
		s.RequestedResource.toString(),
		s.Direction,
		*s.Lineno,
		bounds), nil
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
