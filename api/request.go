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
	// Provided VDS A, VDS B and the binary_operator-key "subtraction" the request returns data from data set (A - B) in the intersection (A ∩ B).
	// Valid options are: "addition", "subtraction", "multiplication", "division" and empty string ("").
	//
	// Note that there are some restrictions when applying a binary operation on two cubes.
	// The axes must be in the same order and have matching stepsize and units.
	// CRS, annotated origin and inline/crossline spacing should be identical.
	// Inside the intersection both cubes should have data at the same lines and
	// there should be at least two samples in each dimension.
	BinaryOperator string `json:"binary_operator,omitempty" example:"subtraction"`
}

func (r RequestedResource) credentials() ([]string, []string, string) {
	return r.Vds, r.Sas, r.BinaryOperator
}

func (f RequestedResource) toString() string {
	allVds := "[" + strings.Join(f.Vds, ", ") + "]"
	return fmt.Sprintf("vds: %s, binary_operator: %s", allVds, f.BinaryOperator)
}

func (r RequestedResource) getRequestedResource() RequestedResource {
	return r
}

type DataRequest interface {
	toString() (string, error)
	hash() (string, error)
	credentials() ([]string, []string, string)
	execute(handle core.DSHandle) (data [][]byte, metadata []byte, err error)
	getRequestedResource() RequestedResource
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
