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
	"strings"
	"testing"

	"github.com/gin-gonic/gin"

	"github.com/equinor/vds-slice/api"
	"github.com/equinor/vds-slice/internal/cache"
	"github.com/equinor/vds-slice/internal/vds"
)

const well_known = "../../testdata/well_known/well_known_default.vds"

type sliceTest struct {
	name           string
	method         string
	slice          testSliceRequest
	sliceRequest   string
	expectedStatus int
	expectedError  string
}

type fenceTest struct {
	name           string
	method         string
	fence          testFenceRequest
	fenceRequest   string
	expectedStatus int
	expectedError  string
}

type metadataTest struct {
	name            string
	method          string
	metadata        testMetadataRequest
	metadataRequest string
	expectedStatus  int
	expectedError   string
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

func setupTestServer(r *gin.Engine) {
	endpoint := api.Endpoint{
		MakeVdsConnection: MakeFileConnection(),
		Cache:             cache.NewNoCache(),
	}

	setupApp(r, &endpoint, nil)
}

func assertError(w *httptest.ResponseRecorder, t *testing.T, test_name string,
	expected_error string) {

	testErrorInfo := &testErrorResponse{}
	err := json.Unmarshal(w.Body.Bytes(), testErrorInfo)
	if err != nil {
		t.Fatalf("Couldn't unmarshal data")
	}

	if !strings.Contains(testErrorInfo.Error, expected_error) {
		msg := "Got error string %s; want it to contain '%s' in case '%s'"
		t.Errorf(
			msg,
			testErrorInfo.Error,
			expected_error,
			test_name,
		)
	}
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

func prepareRequest(ctx *gin.Context, t *testing.T, endpoint string,
	method string, request interface{}, jsonReq string) {
	jsonRequest := jsonReq
	if jsonRequest == "" {
		req, err := json.Marshal(request)
		if err != nil {
			t.Fatalf("Cannot marshal request")
		}
		jsonRequest = string(req)
	}

	switch method := method; method {
	case http.MethodGet:
		ctx.Request, _ = http.NewRequest(
			http.MethodGet,
			endpoint,
			nil,
		)

		q := url.Values{}
		q.Add("query", jsonRequest)
		ctx.Request.URL.RawQuery = q.Encode()

	case http.MethodPost:
		ctx.Request, _ = http.NewRequest(
			http.MethodPost,
			endpoint,
			bytes.NewBuffer([]byte(jsonRequest)),
		)
		ctx.Request.Header.Set("Content-Type", "application/json")
	default:
		t.Fatalf("Unknown method")
	}
}

func prepareSliceRequest(ctx *gin.Context, t *testing.T, testcase sliceTest) {
	prepareRequest(
		ctx,
		t,
		"/slice",
		testcase.method,
		testcase.slice,
		testcase.sliceRequest,
	)
}

func prepareFenceRequest(ctx *gin.Context, t *testing.T, testcase fenceTest) {
	prepareRequest(
		ctx,
		t,
		"/fence",
		testcase.method,
		testcase.fence,
		testcase.fenceRequest,
	)
}

func prepareMetadataRequest(ctx *gin.Context, t *testing.T, testcase metadataTest) {
	prepareRequest(
		ctx,
		t,
		"/metadata",
		testcase.method,
		testcase.metadata,
		testcase.metadataRequest,
	)
}
