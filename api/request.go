package api

import (
	"fmt"
	"net/url"
	"strings"

	"github.com/equinor/vds-slice/internal/cache"
	"github.com/equinor/vds-slice/internal/core"
)

type RequestedResource struct {
	// The blob URL can be sent in either format:
	// - https://account.blob.core.windows.net/container/blob
	//    In the above case the user must provide a sas-token as a separate key.
	//
	// - https://account.blob.core.windows.net/container/blob?sp=r&st=2022-09-12T09:44:17Z&se=2022-09-12T17:44:17Z&spr=https&sv=2021-06-08&sr=c&sig=...
	//	  Instead of passing the sas-token explicitly in the sas field, you can
	//	  pass an sign url. If the sas-token is provided in both fields, the
	//	  sas-token in the sas field is prioritized.
	//
	// Note that your whole query string will be passed further down to
	// openvds. We expect query parameters to contain sas-token and sas-token
	// only and give no guarantee that Openvds/Azure returns you if you provide
	// any additional arguments.
	//
	// Warning: We do not expect storage accounts to have snapshots. If your
	// storage account has any, please contact System Admin, as due to caching
	// you might end up with incorrect data.
	Vds []string `json:"vds" binding:"required" example:"https://account.blob.core.windows.net/container/blob"`

	// A valid sas-token with read access to the container specified in Vds
	Sas []string `json:"sas,omitempty" example:"sp=r&st=2022-09-12T09:44:17Z&se=2022-09-12T17:44:17Z&spr=https&sv=2021-06-08&sr=c&sig=..."`

	// When two blob URL´s are provided the *binary_operator* defines how data
	// from these sources are combined. Provided blob A and blob B and the
	// operation **subtraction** any request returns data from (A - B).
	// Valid options are: **addition**, **subtraction**, **multiplication** and **division**.
	Binary_operator string `json:"binary_operator,omitempty" example:"subtraction"`
}

func (r RequestedResource) credentials() ([]string, []string) {
	return r.Vds, r.Sas
}

type DataRequest interface {
	toString() (string, error)
	hash() (string, error)
	credentials() ([]string, []string)
	execute(handle core.DSHandle) (data [][]byte, metadata []byte, err error)
	cubeFunction() string
}

type Stringable interface {
	toString() (string, error)
}
type Normalizable interface {
	NormalizeConnection() error
}

func (r *RequestedResource) NormalizeConnection() error {

	for i, url_req := range r.Vds {
		url_object, err := url.Parse(url_req)
		if err != nil {
			return core.NewInvalidArgument(err.Error())
		}
		if strings.TrimSpace(r.Sas[i]) == "" {
			if url_object.RawQuery == "" {
				return core.NewInvalidArgument("No valid Sas token is found in the request")
			}
			r.Sas[i] = url_object.RawQuery
		}
		url_object.RawQuery = ""
		url_object.Host = url_object.Hostname()
		r.Vds[i] = url_object.String()
	}
	return nil
}

type MetadataRequest struct {
	RequestedResource
} //@name MetadataRequest

func (m MetadataRequest) toString() (string, error) {
	return m.RequestedResource.toString(), nil
}

type GenericFenceRequest struct {
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

func (f GenericFenceRequest) toString() string {
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

	return fmt.Sprintf("{coordinate system: %s, coordinates: %s, interpolation (optional): %s}",
		f.CoordinateSystem,
		coordinates,
		f.Interpolation,
	)
}

/** Compute a hash of the request that uniquely identifies the requested fence
 *
 * The hash is computed based on all fields that contribute toward a unique response.
 * I.e. every field except the sas token.
 */
func (f FenceRequest) hash() (string, error) {
	// Strip the sas token before computing hash
	f.Sas = nil
	return cache.Hash(f)
}

// Query for slice endpoints
// @Description Query payload for slice endpoint /slice.
type GenericSliceRequest struct {
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
} //@name GenericSliceRequest

/** Compute a hash of the request that uniquely identifies the requested slice
 *
 * The hash is computed based on all fields that contribute toward a unique response.
 * I.e. every field except the sas token.
 */
func (s SliceRequest) hash() (string, error) {
	// Strip the sas token before computing hash
	s.Sas = nil
	return cache.Hash(s)
}

func (s GenericSliceRequest) toString() string {

	bounds := func() string {
		var bounds_all = ""
		for _, bound := range s.Bounds {
			if len(bounds_all) > 1 {
				bounds_all += ", "
			}
			bounds_all += fmt.Sprintf("[%s, %d, %d]", *bound.Direction, *bound.Lower, *bound.Upper)
		}
		return bounds_all
	}()

	return fmt.Sprintf("direction: %s, lineno: %d, bounds: %s",
		s.Direction,
		*s.Lineno,
		bounds)
}

// Query for Attribute endpoints
// @Description Query payload for attribute endpoint.
type AttributeRequest struct {

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
type AttributeAlongRequest struct {
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
	// Defaults to zero
	Above float32 `json:"above" example:"20.0"`

	// Samples interval below the horizon to include in attribute calculation.
	// Implements the same behaviour as 'above'.
	//
	// Defaults to zero
	Below float32 `json:"below" example:"20.0"`
} //@name AttributeAlongSurfaceRequest

/** Compute a hash of the request that uniquely identifies the requested attributes
 *
 * The hash is computed based on all fields that contribute toward a unique response.
 * I.e. every field except the sas token.
 */
func (h AttributeAlongSurfaceRequest) hash() (string, error) {
	// Strip the sas token before computing hash
	h.Sas = nil
	return cache.Hash(h)
}

func (h AttributeAlongSurfaceRequest) toString() (string, error) {
	return fmt.Sprintf("{%s, %s}", h.RequestedResource.toString(), h.AttributeAlongRequest.toString()), nil
}

type AttributeBetweenRequest struct {
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

/** Compute a hash of the request that uniquely identifies the requested attributes
 *
 * The hash is computed based on all fields that contribute toward a unique response.
 * I.e. every field except the sas token.
 */
func (h AttributeBetweenSurfacesRequest) hash() (string, error) {
	// Strip the sas token before computing hash
	h.Sas = nil
	return cache.Hash(h)
}

func (h AttributeBetweenSurfacesRequest) toString() (string, error) {
	return fmt.Sprintf("{%s, %s}", h.RequestedResource.toString(), h.AttributeBetweenRequest.toString()), nil
}

func (f RequestedResource) toString() string {
	var vds_all = "["
	for _, vds := range f.Vds {
		if len(vds_all) > 1 {
			vds_all += ", "
		}
		vds_all += vds
	}
	vds_all += "]"
	return fmt.Sprintf("vds: %s, binary_operator: %s", vds_all, f.Binary_operator)
}

func (s RequestedResource) cubeFunction() string {
	return s.Binary_operator
}

type SliceRequest struct {
	RequestedResource
	GenericSliceRequest
} //@name SliceRequest

func (s SliceRequest) toString() (string, error) {
	return fmt.Sprintf("{%s, %s}", s.RequestedResource.toString(), s.GenericSliceRequest.toString()), nil
}

type FenceRequest struct {
	RequestedResource
	GenericFenceRequest
} //@name FenceRequest

func (f FenceRequest) toString() (string, error) {
	return fmt.Sprintf("{%s, %s}", f.RequestedResource.toString(), f.GenericFenceRequest.toString()), nil
}

func (f AttributeRequest) toString() string {
	return fmt.Sprintf("interpolation: %s, stepsize: %f, attributes: %v",
		f.Interpolation,
		f.Stepsize,
		f.Attributes)
}

func RegularSurfaceToString(s core.RegularSurface) string {
	return fmt.Sprintf("values: (ncols: %d, nrows: %d), rotation: %.2f, origin: [%.2f, %.2f], increment: [%.2f, %.2f], fillvalue: %.2f",
		len(s.Values[0]),
		len(s.Values),
		*s.Rotation,
		*s.Xori,
		*s.Yori,
		s.Xinc,
		s.Yinc,
		*s.FillValue)
}

func (f AttributeAlongRequest) toString() string {

	return fmt.Sprintf("%s, surface: %s, above: %f, below: %f",
		f.AttributeRequest.toString(),
		RegularSurfaceToString(f.Surface),
		f.Above,
		f.Below)
}

func (f AttributeBetweenRequest) toString() string {

	return fmt.Sprintf("%s, primarySurface: %s, secondarySurface: %s",
		f.AttributeRequest.toString(),
		RegularSurfaceToString(f.PrimarySurface),
		RegularSurfaceToString(f.SecondarySurface))
}

type AttributeAlongSurfaceRequest struct {
	RequestedResource
	AttributeAlongRequest
} //@name AttributeAlongSurfaceRequest

type AttributeBetweenSurfacesRequest struct {
	RequestedResource
	AttributeBetweenRequest
} //@name AttributeBetweenSurfacesRequest
