package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"mime"
	"mime/multipart"
	"net/http"
	"net/http/httptest"
	"net/url"
	"testing"

	"github.com/gin-gonic/gin"
	"github.com/stretchr/testify/require"

	"github.com/equinor/vds-slice/api"
	"github.com/equinor/vds-slice/internal/cache"
	"github.com/equinor/vds-slice/internal/core"
)

const well_known = "../../testdata/well_known/well_known_default.vds"
const samples10 = "../../testdata/10_samples/10_samples_default.vds"

type baseTest struct {
	name           string
	method         string
	jsonRequest    string
	expectedStatus int
	expectedError  string
}

type endpointTest interface {
	base() baseTest
	endpoint() string
	requestAsJSON() (string, error)
}

type sliceTest struct {
	baseTest
	slice testSliceRequest
}

func (s sliceTest) endpoint() string {
	return "/slice"
}

func (s sliceTest) base() baseTest {
	return s.baseTest
}

func (s sliceTest) requestAsJSON() (string, error) {
	req, err := json.Marshal(s.slice)
	if err != nil {
		return "", fmt.Errorf("cannot marshal slice request %v", s.slice)
	}
	return string(req), nil
}

type fenceTest struct {
	baseTest
	fence testFenceRequest
}

func (f fenceTest) endpoint() string {
	return "/fence"
}

func (f fenceTest) base() baseTest {
	return f.baseTest
}

func (f fenceTest) requestAsJSON() (string, error) {
	req, err := json.Marshal(f.fence)
	if err != nil {
		return "", fmt.Errorf("cannot marshal fence request %v", f.fence)
	}
	return string(req), nil
}

type metadataTest struct {
	baseTest
	metadata testMetadataRequest
}

func (m metadataTest) endpoint() string {
	return "/metadata"
}

func (m metadataTest) base() baseTest {
	return m.baseTest
}

func (m metadataTest) requestAsJSON() (string, error) {
	req, err := json.Marshal(m.metadata)
	if err != nil {
		return "", fmt.Errorf("cannot marshal metadata request %v", m.metadata)
	}
	return string(req), nil
}

type attributeEndpointTest interface {
	endpointTest
	nrows() int
	ncols() int
}

type attributeAlongSurfaceTest struct {
	baseTest
	attribute testAttributeAlongSurfaceRequest
}

type attributeBetweenSurfacesTest struct {
	baseTest
	attribute testAttributeBetweenSurfacesRequest
}

func (h attributeAlongSurfaceTest) endpoint() string {
	return "/attributes/surface/along"
}

func (h attributeAlongSurfaceTest) base() baseTest {
	return h.baseTest
}

func (h attributeAlongSurfaceTest) nrows() int {
	return len(h.attribute.Values)
}

func (h attributeAlongSurfaceTest) ncols() int {
	return len(h.attribute.Values[0])
}

func (h attributeAlongSurfaceTest) requestAsJSON() (string, error) {
	out := map[string]interface{}{}

	out["vds"] = h.attribute.Vds
	out["sas"] = h.attribute.Sas
	surface := map[string]interface{}{}
	surface["values"] = h.attribute.Values
	surface["rotation"] = 33.69
	surface["xinc"] = 7.2111
	surface["yinc"] = 3.6056
	surface["xori"] = 2
	surface["yori"] = 0
	surface["fillValue"] = 666.66
	out["surface"] = surface
	out["above"] = h.attribute.Above
	out["below"] = h.attribute.Below
	out["stepsize"] = h.attribute.StepSize
	out["Attributes"] = h.attribute.Attributes
	if h.attribute.Interpolation != "" {
		out["interpolation"] = h.attribute.Interpolation
	} else {
		out["interpolation"] = "cubic"
	}

	req, err := json.Marshal(out)
	if err != nil {
		return "", fmt.Errorf("cannot marshal attribute request %v", h.attribute)
	}
	return string(req), nil
}

func (h attributeBetweenSurfacesTest) endpoint() string {
	return "/attributes/surface/between"
}

func (h attributeBetweenSurfacesTest) base() baseTest {
	return h.baseTest
}

func (h attributeBetweenSurfacesTest) nrows() int {
	return len(h.attribute.ValuesPrimary)
}

func (h attributeBetweenSurfacesTest) ncols() int {
	return len(h.attribute.ValuesPrimary[0])
}

func (h attributeBetweenSurfacesTest) requestAsJSON() (string, error) {
	out := map[string]interface{}{}

	out["vds"] = h.attribute.Vds
	out["sas"] = h.attribute.Sas

	primary := map[string]interface{}{}
	primary["values"] = h.attribute.ValuesPrimary
	primary["rotation"] = 33.69
	primary["xinc"] = 7.2111
	primary["yinc"] = 3.6056
	primary["xori"] = 2
	primary["yori"] = 0
	primary["fillValue"] = 666.66
	out["primarySurface"] = primary

	secondary := map[string]interface{}{}
	for k, v := range primary {
		secondary[k] = v
	}
	secondary["values"] = h.attribute.ValuesSecondary
	out["secondarySurface"] = secondary

	out["stepsize"] = h.attribute.StepSize
	out["attributes"] = h.attribute.Attributes
	if h.attribute.Interpolation != "" {
		out["interpolation"] = h.attribute.Interpolation
	} else {
		out["interpolation"] = "cubic"
	}

	req, err := json.Marshal(out)
	if err != nil {
		return "", fmt.Errorf("cannot marshal attribute request %v", h.attribute)
	}
	return string(req), nil
}

// define own help types to assure separation between production and test code
type testErrorResponse struct {
	Error string `json:"error" binding:"required"`
}

type testBound struct {
	Direction string  `json:"direction"`
	Lower     float32 `json:"lower"`
	Upper     float32 `json:"upper"`
}

type testSliceRequest struct {
	Vds       []string    `json:"vds"`
	Direction string      `json:"direction"`
	Lineno    int         `json:"lineno"`
	Sas       []string    `json:"sas"`
	Bounds    []testBound `json:"bounds"`
}

type testFenceRequest struct {
	Vds              []string    `json:"vds"`
	CoordinateSystem string      `json:"coordinateSystem"`
	Coordinates      [][]float32 `json:"coordinates"`
	FillValue        float32     `json:"fillValue"`
	Sas              []string    `json:"sas"`
}

type testMetadataRequest struct {
	Vds []string `json:"vds"`
	Sas []string `json:"sas"`
}

type testAttributeAlongSurfaceRequest struct {
	Vds           []string
	Sas           []string
	Values        [][]float32
	Interpolation string
	Above         float32
	Below         float32
	StepSize      float32
	Attributes    []string
}

type testAttributeBetweenSurfacesRequest struct {
	Vds             []string
	Sas             []string
	ValuesPrimary   [][]float32
	ValuesSecondary [][]float32
	Interpolation   string
	StepSize        float32
	Attributes      []string
}

type testSliceAxis struct {
	Annotation string  `json:"annotation" binding:"required"`
	Max        float32 `json:"max"        binding:"required"`
	Min        float32 `json:"min"        binding:"required"`
	Samples    int     `json:"samples"    binding:"required"`
	StepSize   float32 `json:"stepsize"   binding:"required"`
	Unit       string  `json:"unit"       binding:"required"`
}

type testSliceMetadata struct {
	X      testSliceAxis `json:"x"      binding:"required"`
	Y      testSliceAxis `json:"y"      binding:"required"`
	Format string        `json:"format" binding:"required"`
}

func MakeFileConnection() core.ConnectionMaker {
	return func(path, sas string) (core.Connection, error) {
		path = fmt.Sprintf("file://%s", path)
		return core.NewFileConnection(path), nil
	}
}

func setupTest(t *testing.T, testcase endpointTest) *httptest.ResponseRecorder {
	w := httptest.NewRecorder()
	ctx, r := gin.CreateTestContext(w)

	endpoint := api.Endpoint{
		MakeVdsConnection: MakeFileConnection(),
		Cache:             cache.NewNoCache(),
	}

	setupApp(r, &endpoint, nil)

	prepareRequest(ctx, t, testcase)
	r.ServeHTTP(w, ctx.Request)

	return w
}

func readMultipartData(t *testing.T, w *httptest.ResponseRecorder) [][]byte {
	_, params, err := mime.ParseMediaType(w.Result().Header.Get("Content-Type"))
	require.NoErrorf(t, err, "Cannot parse Content Type")
	mr := multipart.NewReader(w.Body, params["boundary"])

	parts := [][]byte{}
	for {
		p, err := mr.NextPart()
		if err == io.EOF {
			return parts
		}
		require.NoErrorf(t, err, "Couldn't process part")
		data, err := io.ReadAll(p)
		require.NoErrorf(t, err, "Couldn't read part")
		parts = append(parts, data)
	}
}

func prepareRequest(ctx *gin.Context, t *testing.T, testcase endpointTest) {
	jsonRequest := testcase.base().jsonRequest
	if jsonRequest == "" {
		var err error
		jsonRequest, err = testcase.requestAsJSON()
		require.NoError(t, err)
	}

	switch method := testcase.base().method; method {
	case http.MethodGet:
		ctx.Request, _ = http.NewRequest(
			http.MethodGet,
			testcase.endpoint(),
			nil,
		)

		q := url.Values{}

		if jsonRequest != "MODIFIER: skip query parameter" {
			q.Add("query", jsonRequest)
		}
		ctx.Request.URL.RawQuery = q.Encode()

	case http.MethodPost:
		ctx.Request, _ = http.NewRequest(
			http.MethodPost,
			testcase.endpoint(),
			bytes.NewBuffer([]byte(jsonRequest)),
		)
		ctx.Request.Header.Set("Content-Type", "application/json")
	default:
		t.Fatalf("Unknown method")
	}
}

func requireStatus(t *testing.T, testcase endpointTest, w *httptest.ResponseRecorder) {
	require.Equalf(t, testcase.base().expectedStatus, w.Result().StatusCode,
		"Test '%v'. Wrong response status. Body: %v", testcase.base().name, w.Body.String())
}
