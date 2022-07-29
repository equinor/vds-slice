package api

import (
	"bytes"
	"encoding/json"
	"mime/multipart"
	"net/http"
	"net/http/httptest"
	"net/url"
	"strconv"
	"strings"
	"testing"

	"github.com/gin-gonic/gin"
)

const well_known = "../testdata/wellknown/well_known_default.vds"

type SliceTest struct {
	name                string
	method              string
	contentType         string
	slice               SliceQuery
	sliceQuery          string
	expectedStatus      int
	expectedContent     []string
}

type FenceTest struct {
	name            string
	method          string
	fence           FenceQuery
	fenceQuery      string
	expectedStatus  int
	expectedContent []string
}

func TestSliceHTTPResponse(t *testing.T) {
	testcases := []SliceTest{
		{
			name:   "Valid GET Query",
			method: http.MethodGet,
			slice: SliceQuery{
				Vds:       well_known,
				Direction: "i",
				Lineno:    1,
				Sas:       "n/a",
			},
			expectedStatus: http.StatusOK,
			expectedContent: []string{
				"\"HasError\":false",
				"Annotation",
				"Content-Type: application/octet-stream",
			},
		},
		{
			name:        "Valid json POST Query",
			method:      http.MethodPost,
			contentType: "application/json",
			slice: SliceQuery{
				Vds:       well_known,
				Direction: "crossline",
				Lineno:    10,
				Sas:       "n/a",
			},
			expectedStatus: http.StatusOK,
			expectedContent: []string{
				"\"HasError\":false",
				"Annotation",
				"Content-Type: application/octet-stream",
			},
		},
		{
			name:        "Valid application/x-www-form-urlencoded POST Query",
			method:      http.MethodPost,
			contentType: "application/x-www-form-urlencoded",
			slice: SliceQuery{
				Vds:       well_known,
				Direction: "time",
				Lineno:    4,
				Sas:       "n/a",
			},
			expectedStatus: http.StatusOK,
			expectedContent: []string{
				"\"HasError\":false",
				"Annotation",
				"Content-Type: application/octet-stream",
			},
		},
		{
			name:        "Valid multipart/form-data POST Query",
			method:      http.MethodPost,
			contentType: "multipart/form-data",
			slice: SliceQuery{
				Vds:       well_known,
				Direction: "inline",
				Lineno:    1,
				Sas:       "n/a",
			},
			expectedStatus: http.StatusOK,
			expectedContent: []string{
				"\"HasError\":false",
				"Annotation",
				"Content-Type: application/octet-stream",
			},
		},
		{
			name:           "Invalid json GET query",
			method:         http.MethodGet,
			sliceQuery:     "help I am a duck",
			expectedStatus: http.StatusBadRequest,
			expectedContent: []string{
				"\"HasError\":true",
				"invalid character",
			},
		},
		{
			name:           "Invalid json POST query",
			method:         http.MethodPost,
			contentType:    "application/json",
			sliceQuery:     "help I am a duck",
			expectedStatus: http.StatusBadRequest,
			expectedContent: []string{
				"\"HasError\":true",
				"invalid character",
			},
		},
		{
			name:   "Missing parameters GET query",
			method: http.MethodGet,
			slice: SliceQuery{
				Vds:       well_known,
				Direction: "i",
				Sas:       "n/a",
			},
			expectedStatus: http.StatusBadRequest,
			expectedContent: []string{
				"\"HasError\":true",
				"Error:Field validation for 'Lineno'",
			},
		},
		{
			name:        "Missing parameters POST Query",
			method:      http.MethodPost,
			contentType: "application/json",
			slice: SliceQuery{
				Vds:    well_known,
				Lineno: 1,
				Sas:    "n/a",
			},
			expectedStatus: http.StatusBadRequest,
			expectedContent: []string{
				"\"HasError\":true",
				"Error:Field validation for 'Direction'",
			},
		},
		{
			name:        "Query with unknown axis",
			method:      http.MethodPost,
			contentType: "application/json",
			slice: SliceQuery{
				Vds:       well_known,
				Direction: "unknown",
				Lineno:    1,
				Sas:       "n/a",
			},
			expectedStatus: http.StatusBadRequest,
			expectedContent: []string{
				"\"HasError\":true",
				"Invalid direction 'unknown', valid options are",
			},
		},
		{
			name:        "Query which passed all input checks but still should fail",
			method:      http.MethodPost,
			contentType: "application/json",
			slice: SliceQuery{
				Vds:       well_known,
				Direction: "i",
				Lineno:    10,
				Sas:       "n/a",
			},
			expectedStatus: http.StatusOK,
			expectedContent: []string{
				"\"HasError\":true",
				"Invalid lineno: 10, valid range: [0:2:1]",
			},
		},
	}

	for _, testcase := range testcases {
		w := httptest.NewRecorder()
		ctx, r := gin.CreateTestContext(w)
		r.Use(gin.Logger())
		r.Use(ResponseHandler)

		endpoint := Endpoint{StorageURL: "", Protocol: "file://"}

		r.POST("/slice", endpoint.SlicePost)
		r.GET("/slice", endpoint.SliceGet)

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
		expectedContentType := "multipart/mixed; boundary="
		if len(w.Result().Header.Values("Content-Type")) != 1 ||
			!strings.Contains(w.Result().Header.Get("Content-Type"), expectedContentType) {
			msg := "Got content types %v; want it to contain '%s' in case '%s'"
			t.Errorf(
				msg,
				w.Result().Header.Values("Content-Type"),
				expectedContentType,
				testcase.name,
			)
		}
		for _, content := range testcase.expectedContent {

			if !strings.Contains(w.Body.String(), content) {
				msg := "Got body %s; want it to contain '%s' in case '%s'"
				t.Errorf(
					msg,
					w.Body.String(),
					content,
					testcase.name,
				)
			}
		}
	}
}

func TestFenceHTTPResponse(t *testing.T) {
	testcases := []FenceTest{
		{
			name:   "Valid GET Query",
			method: http.MethodGet,
			fence: FenceQuery{
				Vds:              well_known,
				CoordinateSystem: "ilxl",
				Fence:            [][]float32{{3, 11}, {2, 10}},
				Sas:              "n/a",
			},
			expectedStatus: http.StatusOK,
			expectedContent: []string{
				"\"HasError\":false",
				"Content-Type: application/octet-stream",
			},
		},
		{
			name:   "Valid json POST Query",
			method: http.MethodPost,
			fence: FenceQuery{
				Vds:              well_known,
				CoordinateSystem: "ij",
				Fence:            [][]float32{{0, 1}, {1, 1}, {1, 0}},
				Sas:              "n/a",
			},
			expectedStatus: http.StatusOK,
			expectedContent: []string{
				"\"HasError\":false",
				"Content-Type: application/octet-stream",
			},
		},
		{
			name:           "Invalid json GET query",
			method:         http.MethodGet,
			fenceQuery:     "help I am a duck",
			expectedStatus: http.StatusBadRequest,
			expectedContent: []string{
				"\"HasError\":true",
				"invalid character",
			},
		},
		{
			name:           "Invalid json POST query",
			method:         http.MethodPost,
			fenceQuery:     "help I am a duck",
			expectedStatus: http.StatusBadRequest,
			expectedContent: []string{
				"\"HasError\":true",
				"invalid character",
			},
		},
		{
			name:   "Missing parameters GET query",
			method: http.MethodGet,
			fence: FenceQuery{
				Vds:              well_known,
				CoordinateSystem: "ij",
				Sas:              "n/a",
			},
			expectedStatus: http.StatusBadRequest,
			expectedContent: []string{
				"\"HasError\":true",
				"Error:Field validation for 'Fence'",
			},
		},
		{
			name:   "Missing parameters POST Query",
			method: http.MethodPost,
			fence: FenceQuery{
				Vds:              well_known,
				CoordinateSystem: "ilxl",
				Sas:              "n/a",
			},
			expectedStatus: http.StatusBadRequest,
			expectedContent: []string{
				"\"HasError\":true",
				"Error:Field validation for 'Fence'",
			},
		},
		{
			name:   "Query which passed all input checks but still should fail",
			method: http.MethodPost,
			fence: FenceQuery{
				Vds:              "unknown",
				CoordinateSystem: "ilxl",
				Fence:            [][]float32{{3, 12}, {2, 10}},
				Sas:              "n/a",
			},
			expectedStatus: http.StatusOK,
			expectedContent: []string{
				"\"HasError\":true",
				"Could not open VDS",
			},
		},
	}

	for _, testcase := range testcases {
		w := httptest.NewRecorder()
		ctx, r := gin.CreateTestContext(w)
		r.Use(gin.Logger())
		r.Use(ResponseHandler)

		endpoint := Endpoint{StorageURL: "", Protocol: "file://"}

		r.POST("/fence", endpoint.FencePost)
		r.GET("/fence", endpoint.FenceGet)

		prepareFenceRequest(ctx, t, testcase)
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
		expectedContentType := "multipart/mixed; boundary="
		if len(w.Result().Header.Values("Content-Type")) != 1 ||
			!strings.Contains(w.Result().Header.Get("Content-Type"), expectedContentType) {
			msg := "Got content types %v; want it to contain '%s' in case '%s'"
			t.Errorf(
				msg,
				w.Result().Header.Values("Content-Type"),
				expectedContentType,
				testcase.name,
			)
		}
		for _, content := range testcase.expectedContent {

			if !strings.Contains(w.Body.String(), content) {
				msg := "Got body %s; want it to contain '%s' in case '%s'"
				t.Errorf(
					msg,
					w.Body.String(),
					content,
					testcase.name,
				)
			}
		}
	}
}

func prepareSliceRequest(ctx *gin.Context, t *testing.T, testcase SliceTest) {
	jsonRequest := testcase.sliceQuery
	if testcase.sliceQuery == "" {
		req, err := json.Marshal(testcase.slice)
		if err != nil {
			t.Fatalf("Cannot marshal request")
		}
		jsonRequest = string(req)
	}

	switch method := testcase.method; method {
	case http.MethodGet:
		ctx.Request, _ = http.NewRequest(
			http.MethodGet,
			"/slice",
			nil,
		)

		q := url.Values{}
		q.Add("query", jsonRequest)
		ctx.Request.URL.RawQuery = q.Encode()

	case http.MethodPost:
		switch contentType := testcase.contentType; contentType {
		case "application/json":
			ctx.Request, _ = http.NewRequest(
				http.MethodPost,
				"/slice",
				bytes.NewBuffer([]byte(jsonRequest)),
			)
			ctx.Request.Header.Set("Content-Type", testcase.contentType)

		case "application/x-www-form-urlencoded":
			q := url.Values{}
			q.Add("vds", testcase.slice.Vds)
			q.Add("direction", testcase.slice.Direction)
			q.Add("lineno", strconv.Itoa(testcase.slice.Lineno))
			q.Add("sas", testcase.slice.Sas)
			urlEncodedRequest := q.Encode()

			ctx.Request, _ = http.NewRequest(
				http.MethodPost,
				"/slice",
				bytes.NewBuffer([]byte(urlEncodedRequest)),
			)
			ctx.Request.Header.Set("Content-Type", testcase.contentType)

		case "multipart/form-data":
			payload := &bytes.Buffer{}
			writer := multipart.NewWriter(payload)
			_ = writer.WriteField("vds", testcase.slice.Vds)
			_ = writer.WriteField("direction", testcase.slice.Direction)
			_ = writer.WriteField("lineno", strconv.Itoa(testcase.slice.Lineno))
			_ = writer.WriteField("sas", testcase.slice.Sas)
			writer.Close()

			ctx.Request, _ = http.NewRequest(
				http.MethodPost,
				"/slice",
				payload,
			)
			ctx.Request.Header.Set("Content-Type", writer.FormDataContentType())

		default:
			t.Fatalf("Unknown content type")
		}
	default:
		t.Fatalf("Unknown method")
	}
}

func prepareFenceRequest(ctx *gin.Context, t *testing.T, testcase FenceTest) {
	jsonRequest := testcase.fenceQuery
	if testcase.fenceQuery == "" {
		req, err := json.Marshal(testcase.fence)
		if err != nil {
			t.Fatalf("Cannot marshal request")
		}
		jsonRequest = string(req)
	}

	switch method := testcase.method; method {
	case http.MethodGet:
		ctx.Request, _ = http.NewRequest(
			http.MethodGet,
			"/fence",
			nil,
		)

		q := url.Values{}
		q.Add("query", jsonRequest)
		ctx.Request.URL.RawQuery = q.Encode()

	case http.MethodPost:
		ctx.Request, _ = http.NewRequest(
			http.MethodPost,
			"/fence",
			bytes.NewBuffer([]byte(jsonRequest)),
		)
		ctx.Request.Header.Set("Content-Type", "application/json")
	default:
		t.Fatalf("Unknown method")
	}
}
