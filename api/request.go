package api

import (
	"encoding/json"
	"fmt"
	"net/url"
	"strings"

	"github.com/equinor/vds-slice/internal/cache"
	"github.com/equinor/vds-slice/internal/core"
)

type stringOrSlice []string

func (s *stringOrSlice) UnmarshalJSON(data []byte) error {
	var str string
	err := json.Unmarshal(data, &str)
	if err == nil {
		*s = []string{str}
		return nil
	}
	var slice []string
	err = json.Unmarshal(data, &slice)
	if err == nil {
		*s = slice
		return nil
	}
	return err
}

type RequestedResource struct {
	// The vds-key can either be provided as a string (single blob URL) or a list of strings (one or more blob URLs).
	//
	// The blob URL can be provided in signed or unsigned form. Only requests where all the blob URLs
	// are of the same form will be accepted. I.e. all signed or all unsigned.
	// - Unsigned form:
	//    example:"https://account.blob.core.windows.net/container/blob"
	//    If the unsigned form is used the sas-token must be provided in a separate sas-key.
	//
	// - Signed form:
	//    example:"https://account.blob.core.windows.net/container/blob?sp=r&st=2022-09-12T09:44:17Z&se=2022-09-12T17:44:17Z&spr=https&sv=2021-06-08&sr=c&sig=..."
	//    In the signed form the blob URL and the sas-token are separated by "?" and passed as a single string.
	//    The sas-key can not be used when blob URLs are provided in signed form,
	//    i.e. the user can choose to leave the sas-key unassigned or to send the empty string ("").
	//
	// Note: The whole query string will be passed further down to openvds.
	// We expect query parameters to contain sas-token and sas-token
	// only and give no guarantee for what Openvds/Azure returns
	// if any additional arguments are provided.
	//
	// Warning: We do not expect storage accounts to have snapshots. If your
	// storage account has any, please contact System Admin, as due to caching
	// you might end up with incorrect data.
	Vds stringOrSlice `json:"vds" binding:"required" example:"https://account.blob.core.windows.net/container/blob"`

	// A sas-token is a string containing a key that must have read access to the corresponding blob URL.
	// If blob URLs are provided in the signed form the sas-key must be undefined or set to the empty string ("").
	// When blob URLs are provided in unsigned form the sas-key must contain tokens corresponding to the provided URLs.
	// If the vds-key is provided as a string then the sas-key must be provided as a string.
	// When using string list representation the vds-key and sas-key must contain the same number of elements and
	// element number "i" in the sas-key is the token for the URL in element "i" in the vds-key.
	Sas stringOrSlice `json:"sas,omitempty" example:"sp=r&st=2022-09-12T09:44:17Z&se=2022-09-12T17:44:17Z&spr=https&sv=2021-06-08&sr=c&sig=..."`

	// When a singe blob URL and sas token pair is provided the binary_operator-key must be undefined or be the empty string("").
	// If two pairs are provided the binary_operator-key defines how the two data sets are combined into a new virtual data set.
	// Provided VDS A, VDS B and the binary_operator-key "subtraction" the request returns data from data set (A - B).
	// Valid options are: "addition", "subtraction", "multiplication", "division" and empty string ("").
	//
	// Note that there are some restrictions when applying a binary operation on two cubes.
	// Each of the axes inline, crossline and depth/time must be identical.
	// I.e, same start value, same stepsize and same number of values.
	// Further more the axis must be of the same type, for instance it is not possible to take the
	// difference between a time cube and a depth cube.
	BinaryOperator string `json:"binary_operator,omitempty" example:"subtraction"`
}

func (r RequestedResource) credentials() ([]string, []string, string) {
	return r.Vds, r.Sas, r.BinaryOperator
}

func (f RequestedResource) toString() string {
	allVds := "[" + strings.Join(f.Vds, ", ") + "]"
	return fmt.Sprintf("vds: %s, binary_operator: %s", allVds, f.BinaryOperator)
}

type DataRequest interface {
	toString() (string, error)
	hash() (string, error)
	credentials() ([]string, []string, string)
	execute(handle core.DSHandle) (data [][]byte, metadata []byte, err error)
}

type Stringable interface {
	toString() (string, error)
}
type Normalizable interface {
	NormalizeConnection() error
}

func (r *RequestedResource) NormalizeConnection() error {

	if len(r.Vds) == 0 {
		return core.NewInvalidArgument("No VDS url provided")
	}

	for i, urlReq := range r.Vds {
		if urlReq == "" {
			return core.NewInvalidArgument(fmt.Sprintf(
				"VDS url cannot be the empty string. VDS url %d is empty", i+1))
		}
	}

	signedUrls := false
	if len(r.Sas) == 0 || (len(r.Sas) == 1 && r.Sas[0] == "") {
		// All vds urls are signed
		r.Sas = []string{}
		signedUrls = true
	}

	for i, urlReq := range r.Vds {
		urlObject, err := url.Parse(urlReq)
		if err != nil {
			return core.NewInvalidArgument(err.Error())
		}

		if signedUrls {
			if urlObject.RawQuery == "" {
				return core.NewInvalidArgument(fmt.Sprintf(
					"No valid Sas token found at the end of vds url nr %d", i+1))
			}
			r.Sas = append(r.Sas, urlObject.RawQuery)
		} else {
			if urlObject.RawQuery != "" {
				return core.NewInvalidArgument(fmt.Sprintf(
					"Signed urls are not accepted when providing sas-tokens. Vds url nr %d is signed", i+1))
			}
		}
		urlObject.RawQuery = ""
		urlObject.Host = urlObject.Hostname()
		r.Vds[i] = urlObject.String()
	}

	if len(r.Vds) != len(r.Sas) {
		return core.NewInvalidArgument(fmt.Sprintf(
			"Number of VDS urls and sas tokens do not match. Vds: %d, Sas: %d", len(r.Vds), len(r.Sas)))
	}

	return nil
}

type MetadataRequest struct {
	RequestedResource
} //@name MetadataRequest

func (m MetadataRequest) toString() (string, error) {
	return fmt.Sprintf("{%s}",
		m.RequestedResource.toString(),
	), nil
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

	return fmt.Sprintf("{%s, coordinate system: %s, coordinates: %s, interpolation (optional): %s}",
		f.RequestedResource.toString(),
		f.CoordinateSystem,
		coordinates,
		f.Interpolation,
	), nil
}

/** Compute a hash of the request that uniquely identifies the requested fence
 *
 * The hash is computed based on all fields that contribute toward a unique response.
 * I.e. every field except the sas token.
 */
func (f FenceRequest) hash() (string, error) {
	// Strip the sas tokens before computing hash
	f.Sas = nil
	return cache.Hash(f)
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
