package api

import (
	"bytes"
	"encoding/json"
	"io"
	"mime"
	"mime/multipart"
	"net/http"
	"net/http/httptest"
	"net/url"
	"reflect"
	"strings"
	"testing"

	"github.com/gin-gonic/gin"
)

const well_known = "../testdata/wellknown/well_known_default.vds"

type sliceTest struct {
	name           string
	method         string
	slice          testSliceQuery
	sliceQuery     string
	expectedStatus int
	expectedError  string
}

// define own help types to assure separation between production and test code
type testErrorResponse struct {
	Error string `json:"error" binding:"required"`
}

type testSliceQuery struct {
	Vds       string `json:"vds"`
	Direction string `json:"direction"`
	Lineno    int    `json:"lineno"`
	Sas       string `json:"sas"`
}

type testSliceAxis struct {
	Annotation string  `json:"annotation" binding:"required"`
	Max        float32 `json:"max"        binding:"required"`
	Min        float32 `json:"min"        binding:"required"`
	Samples    int     `json:"samples"    binding:"required"`
	Unit       string  `json:"unit"       binding:"required"`
}

type testSliceMetadata struct {
	Axis   []testSliceAxis `json:"axis"   binding:"required"`
	Format int             `json:"format" binding:"required"`
}

func TestSliceHappyHTTPResponse(t *testing.T) {
	testcases := []sliceTest{
		{
			name:   "Valid GET Query",
			method: http.MethodGet,
			slice: testSliceQuery{
				Vds:       well_known,
				Direction: "i",
				Lineno:    0, //side-effect assurance that 0 is accepted
				Sas:       "n/a",
			},
			expectedStatus: http.StatusOK,
		},
		{
			name:   "Valid json POST Query",
			method: http.MethodPost,
			slice: testSliceQuery{
				Vds:       well_known,
				Direction: "crossline",
				Lineno:    10,
				Sas:       "n/a",
			},
			expectedStatus: http.StatusOK,
		},
	}

	for _, testcase := range testcases {
		w := httptest.NewRecorder()
		ctx, r := gin.CreateTestContext(w)
		setupTestServer(r)

		prepareSliceRequest(ctx, t, testcase)
		r.ServeHTTP(w, ctx.Request)

		if w.Result().StatusCode != testcase.expectedStatus {
			msg := "Got status %v; want %d %s in case '%s'"
			t.Errorf(
				msg,
				w.Result().Status,
				testcase.expectedStatus,
				http.StatusText(testcase.expectedStatus),
				testcase.name,
			)
		}
		parts := readMultipartData(t, w)
		if len(parts) != 2 {
			msg := "Got %d parts in reply; want it to always contain 2 in case '%s'"
			t.Errorf(
				msg,
				len(parts),
				testcase.name,
			)
		}

		inlineAxis := testSliceAxis{
			Annotation: "Inline", Max: 5.0, Min: 1.0, Samples: 3, Unit: "unitless",
		}
		crosslineAxis := testSliceAxis{
			Annotation: "Crossline", Max: 11.0, Min: 10.0, Samples: 2, Unit: "unitless",
		}
		sampleAxis := testSliceAxis{
			Annotation: "Sample", Max: 16.0, Min: 4.0, Samples: 4, Unit: "ms",
		}
		expectedFormat := 3

		var expectedMetadata *testSliceMetadata
		switch testcase.slice.Direction {
		case "i":
			expectedMetadata = &testSliceMetadata{
				Axis:   []testSliceAxis{crosslineAxis, sampleAxis},
				Format: expectedFormat}
		case "crossline":
			expectedMetadata = &testSliceMetadata{
				Axis:   []testSliceAxis{inlineAxis, sampleAxis},
				Format: expectedFormat}
		default:
			t.Fatalf("Unhandled direction %s in case %s", testcase.slice.Direction, testcase.name)
		}

		metadata := &testSliceMetadata{}
		err := json.Unmarshal(parts[0], metadata)

		if err != nil {
			msg := "Failed json metadata extraction in case '%s'"
			t.Fatalf(
				msg,
				testcase.name,
			)
		}

		if !reflect.DeepEqual(metadata, expectedMetadata) {
			msg := "Got %v as metadata; want it to be %v in case '%s'"
			t.Fatalf(
				msg,
				metadata,
				expectedMetadata,
				testcase.name,
			)
		}

		expectedDataLength := expectedMetadata.Axis[0].Samples *
			expectedMetadata.Axis[1].Samples * 4 //4 bytes each
		if len(parts[1]) != expectedDataLength {
			msg := "Got %d bytes in data reply; want it to be %d in case '%s'"
			t.Errorf(
				msg,
				len(parts[2]),
				expectedDataLength,
				testcase.name,
			)
		}
	}
}

func TestSliceErrorHTTPResponse(t *testing.T) {
	testcases := []sliceTest{
		{
			name:           "Invalid json GET query",
			method:         http.MethodGet,
			sliceQuery:     "help I am a duck",
			expectedStatus: http.StatusBadRequest,
			expectedError:  "invalid character",
		},
		{
			name:           "Invalid json POST query",
			method:         http.MethodPost,
			sliceQuery:     "help I am a duck",
			expectedStatus: http.StatusBadRequest,
			expectedError:  "invalid character",
		},
		{
			name:   "Missing parameters GET query",
			method: http.MethodGet,
			sliceQuery: "{\"vds\":\"" + well_known +
				"\", \"direction\":\"i\", \"sas\": \"n/a\"}",
			expectedStatus: http.StatusBadRequest,
			expectedError:  "Error:Field validation for 'Lineno'",
		},
		{
			name:   "Missing parameters POST Query",
			method: http.MethodPost,
			sliceQuery: "{\"vds\":\"" + well_known +
				"\", \"lineno\":1, \"sas\": \"n/a\"}",
			expectedStatus: http.StatusBadRequest,
			expectedError:  "Error:Field validation for 'Direction'",
		},
		{
			name:   "Query with unknown axis",
			method: http.MethodPost,
			slice: testSliceQuery{
				Vds:       well_known,
				Direction: "unknown",
				Lineno:    1,
				Sas:       "n/a",
			},
			expectedStatus: http.StatusBadRequest,
			expectedError:  "Invalid direction 'unknown', valid options are",
		},
		{
			name:   "Query which passed all input checks but still should fail",
			method: http.MethodPost,
			slice: testSliceQuery{
				Vds:       well_known,
				Direction: "i",
				Lineno:    10,
				Sas:       "n/a",
			},
			expectedStatus: http.StatusInternalServerError,
			expectedError:  "Invalid lineno: 10, valid range: [0:2:1]",
		},
	}

	for _, testcase := range testcases {
		w := httptest.NewRecorder()
		ctx, r := gin.CreateTestContext(w)
		setupTestServer(r)

		prepareSliceRequest(ctx, t, testcase)
		r.ServeHTTP(w, ctx.Request)

		if w.Result().StatusCode != testcase.expectedStatus {
			msg := "Got status %v; want %d %s in case '%s'"
			t.Errorf(
				msg,
				w.Result().Status,
				testcase.expectedStatus,
				http.StatusText(testcase.expectedStatus),
				testcase.name,
			)
		}
		assertError(w, t, testcase.name, testcase.expectedError)
	}
}

func setupTestServer(r *gin.Engine) {
	r.Use(gin.Logger())
	r.Use(ErrorHandler)

	endpoint := Endpoint{StorageURL: "", Protocol: "file://"}

	r.POST("/slice", endpoint.SlicePost)
	r.GET("/slice", endpoint.SliceGet)

	r.POST("/fence", endpoint.FencePost)
	r.GET("/fence", endpoint.FenceGet)
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
	method string, query interface{}, jsonQuery string) {
	jsonRequest := jsonQuery
	if jsonQuery == "" {
		req, err := json.Marshal(query)
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
	prepareRequest(ctx, t, "/slice", testcase.method, testcase.slice, testcase.sliceQuery)
}
