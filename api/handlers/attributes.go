package handlers

import (
	"fmt"

	"github.com/gin-gonic/gin"

	"github.com/equinor/oneseismic-api/internal/cache"
	"github.com/equinor/oneseismic-api/internal/core"
)

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

// Query for Attribute endpoints
// @Description Query payload for attribute endpoint.
type AttributeRequest struct {
	RequestedResource

	// Horizontal interpolation method
	// Supported options are: nearest, linear, cubic, angular and triangular.
	// Defaults to nearest.
	// This field is passed on to OpenVDS, which does the actual interpolation.
	//
	// This only applies to the horizontal plane. Traces are always
	// interpolated with cubic interpolation (algorithm: modified makima)
	// Note: For nearest interpolation result will snap to the nearest point
	// as per "half up" rounding. This is different from openvds logic.
	Interpolation string `json:"interpolation" example:"linear"`

	// Stepsize for samples within the window defined by above below
	//
	// Samples within the vertical window will be re-sampled to 'stepsize'
	// using cubic interpolation (modified makima) before the attributes are
	// calculated.
	//
	// This value should be given in the vertical domain of the traces. E.g.
	// 0.1 implies re-sample samples at an interval of 0.1 meter (if it's a
	// depth cube) or 0.1 ms (if it's a time cube with vertical units in
	// milliseconds).
	//
	// Setting this to zero, or omitting it will default it to the vertical
	// stepsize in the VDS volume.
	Stepsize float32 `json:"stepsize" example:"1.0"`

	// Requested attributes. Multiple attributes can be calculated by the same
	// request. This is considerably faster than doing one request per
	// attribute.
	Attributes []string `json:"attributes" binding:"required" swaggertype:"array,string" example:"min,max"`
} //@name AttributeRequest

// Query for Attribute along the surface endpoints
// @Description Query payload for attribute "along" endpoint.
type AttributeAlongSurfaceRequest struct {
	AttributeRequest

	// Surface along which data must be retrieved
	Surface core.RegularSurface `json:"surface" binding:"required"`

	// Samples interval above the horizon to include in attribute calculation.
	// This value should be given in the VDS's vertical domain. E.g. if the
	// vertical domain is 'ms' then a value of 22 means to include samples up
	// to 22 ms above the horizon definition. The value is rounded down to the
	// nearest whole sample. I.e. if the cube is sampled at 4ms, the attribute
	// calculation will include samples 4, 8, 12, 16 and 20ms above the
	// horizon, while the sample at 24ms is excluded.
	//
	// For each point of the surface (sample_value - above) should be in range
	// of vertical axis.
	//
	// Defaults to zero
	Above float32 `json:"above" example:"20.0"`

	// Samples interval below the horizon to include in attribute calculation.
	// Implements the same behavior as 'above'.
	//
	// For each point of the surface (sample_value + below) should be in range
	// of vertical axis.
	//
	// Defaults to zero
	Below float32 `json:"below" example:"20.0"`
} //@name AttributeAlongSurfaceRequest

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

/** Compute a hash of the request that uniquely identifies the requested attributes
 *
 * The hash is computed based on all fields that contribute toward a unique response.
 * I.e. every field except the sas token.
 */
func (h AttributeAlongSurfaceRequest) hash() (string, error) {
	// Strip the sas tokens before computing hash
	h.Sas = nil
	return cache.Hash(h)
}

func (h AttributeAlongSurfaceRequest) toString() (string, error) {
	msg := "{%s, Horizon: %s " +
		"interpolation: %s, Above: %.2f, Below: %.2f, Stepsize: %.2f, " +
		"Attributes: %v}"
	return fmt.Sprintf(
		msg,
		h.RequestedResource.toString(),
		h.Surface.ToString(),
		h.Interpolation,
		h.Above,
		h.Below,
		h.Stepsize,
		h.Attributes,
	), nil
}

// Query for Attribute between surfaces endpoints
// @Description Query payload for attribute "between" endpoint.
type AttributeBetweenSurfacesRequest struct {
	AttributeRequest

	// One of the two surfaces between which data will be retrieved. This value
	// should be given in the VDS's vertical domain, Annotation (for example,
	// Depth or Time). Surface will be used as reference for any sampling
	// operation, i.e. points that are on this surface will be present in the
	// final calculations and could be retrieved through samplevalue. At the
	// surface points where no data exists fillvalue will be set in the result
	// buffer.
	PrimarySurface core.RegularSurface `json:"primarySurface" binding:"required"`

	// One of the two surfaces between which data will be retrieved. This value
	// should be given in the VDS's vertical domain, Annotation (for example,
	// Depth or Time). Surface will be used to define data boundaries. For every
	// point on the primary surface the closest point on the secondary surface
	// would be found and its value set as the request boundary. It might not be
	// included in final calculations. If the closest value is fillvalue,
	// fillvalue will be set in the result buffer. It is not required for
	// surfaces to have the same plane (origin, rotation, step). If surfaces
	// intersect, exception will be thrown. If any of the values of the surface
	// is outside of data boundaries, exception will be raised.
	SecondarySurface core.RegularSurface `json:"secondarySurface" binding:"required"`
} //@name AttributeBetweenSurfacesRequest

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

/** Compute a hash of the request that uniquely identifies the requested attributes
 *
 * The hash is computed based on all fields that contribute toward a unique response.
 * I.e. every field except the sas token.
 */
func (h AttributeBetweenSurfacesRequest) hash() (string, error) {
	// Strip the sas tokens before computing hash
	h.Sas = nil
	return cache.Hash(h)
}

func (h AttributeBetweenSurfacesRequest) toString() (string, error) {
	msg := "{vds: %s, " +
		"Primary surface: %s" +
		"Secondary surface: %s" +
		"Interpolation: %s, Stepsize: %.2f, Attributes: %v}"
	return fmt.Sprintf(
		msg,
		h.RequestedResource.toString(),
		h.PrimarySurface.ToString(),
		h.SecondarySurface.ToString(),
		h.Interpolation,
		h.Stepsize,
		h.Attributes,
	), nil
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
