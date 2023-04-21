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
	"github.com/equinor/vds-slice/internal/vds"
)

const well_known = "../../testdata/well_known/well_known_default.vds"

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

type horizonTest struct {
	baseTest
	horizon testHorizonRequest
}

func (h horizonTest) endpoint() string {
	return "/horizon"
}

func (h horizonTest) base() baseTest {
	return h.baseTest
}

func (h horizonTest) requestAsJSON() (string, error) {
	out := map[string]interface{}{}

	out["vds"] = h.horizon.Vds
	out["sas"] = h.horizon.Sas
	out["horizon"] = h.horizon.Horizon
	out["rotation"] = 33.69
	out["xinc"] = 7.2111
	out["yinc"] = 3.6056
	out["xori"] = 2
	out["yori"] = 0
	out["fillValue"] = 666.66
	if h.horizon.Interpolation != "" {
		out["interpolation"] = h.horizon.Interpolation
	} else {
		out["interpolation"] = "cubic"
	}

	req, err := json.Marshal(out)
	if err != nil {
		return "", fmt.Errorf("cannot marshal horizon request %v", h.horizon)
	}
	return string(req), nil
}

// define own help types to assure separation between production and test code
type testErrorResponse struct {
	Error string `json:"error" binding:"required"`
}

type testSliceRequest struct {
	Vds       string `json:"vds"`
	Direction string `json:"direction"`
	Lineno    int    `json:"lineno"`
	Sas       string `json:"sas"`
}

type testFenceRequest struct {
	Vds              string      `json:"vds"`
	CoordinateSystem string      `json:"coordinateSystem"`
	Coordinates      [][]float32 `json:"coordinates"`
	Sas              string      `json:"sas"`
}

type testMetadataRequest struct {
	Vds string `json:"vds"`
	Sas string `json:"sas"`
}

type testHorizonRequest struct {
	Vds           string
	Sas           string
	Horizon       [][]float32
	Interpolation string
}

type testSliceAxis struct {
	Annotation string  `json:"annotation" binding:"required"`
	Max        float32 `json:"max"        binding:"required"`
	Min        float32 `json:"min"        binding:"required"`
	Samples    int     `json:"samples"    binding:"required"`
	Unit       string  `json:"unit"       binding:"required"`
}

type testSliceMetadata struct {
	X      testSliceAxis `json:"x"      binding:"required"`
	Y      testSliceAxis `json:"y"      binding:"required"`
	Format string        `json:"format" binding:"required"`
}

func MakeFileConnection() vds.ConnectionMaker {
	return func(path, sas string) (vds.Connection, error) {
		path = fmt.Sprintf("file://%s", path)
		return vds.NewFileConnection(path), nil
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
	if err != nil {
		t.Fatalf("Cannot parse Content Type")
	}
	mr := multipart.NewReader(w.Body, params["boundary"])

	parts := [][]byte{}
	for {
		p, err := mr.NextPart()
		if err == io.EOF {
			return parts
		}
		if err != nil {
			t.Fatalf("Couldn't process part")
		}
		data, err := io.ReadAll(p)
		if err != nil {
			t.Fatalf("Couldn't read part")
		}
		parts = append(parts, data)
	}
}

func prepareRequest(ctx *gin.Context, t *testing.T, testcase endpointTest) {
	jsonRequest := testcase.base().jsonRequest
	if jsonRequest == "" {
		var err error
		jsonRequest, err = testcase.requestAsJSON()
		if err != nil {
			t.Fatal(err)
		}
	}

	switch method := testcase.base().method; method {
	case http.MethodGet:
		ctx.Request, _ = http.NewRequest(
			http.MethodGet,
			testcase.endpoint(),
			nil,
		)

		q := url.Values{}
		q.Add("query", jsonRequest)
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
