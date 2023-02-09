package vds

import (
	"encoding/binary"
	"encoding/json"
	"errors"
	"fmt"
	"math"
	"reflect"
	"strings"
	"testing"
)

func make_connection(name string) Connection {
	path := fmt.Sprintf("../../testdata/%s/%s_default.vds", name, name)
	path = fmt.Sprintf("file://%s", path)
	return NewFileConnection(path)
}

var well_known = make_connection("well_known")
var prestack = make_connection("prestack")

func toFloat32(buf []byte) (*[]float32, error) {
	fsize := 4 // sizeof(float32)
	if len(buf) % fsize != 0 {
		return nil, errors.New("Invalid buffersize")
	}

	outlen := len(buf) / 4

	out := make([]float32, outlen)
	for i := 0; i < outlen; i++ {
		offset := i * fsize
		tmp := binary.LittleEndian.Uint32(buf[offset:offset+fsize])
		out[i] = math.Float32frombits(tmp)
	}

	return &out, nil
}

func TestMetadata(t *testing.T) {
	expected := Metadata{
		Axis: []*Axis{
			{ Annotation: "Inline",    Min: 1,  Max: 5,  Samples: 3, Unit: "unitless" },
			{ Annotation: "Crossline", Min: 10, Max: 11, Samples: 2, Unit: "unitless" },
			{ Annotation: "Sample",    Min: 4,  Max: 16, Samples: 4, Unit: "ms"       },
		},
		BoundingBox: BoundingBox{
			Cdp:  [][]float64{ {5, 0}, {9, 8}, {4, 11}, {0, 3} },
			Ilxl: [][]float64{ {1, 10}, {5, 10}, {5, 11}, {1, 11}},
			Ij:   [][]float64{ {0, 0}, {2, 0}, {2, 1}, {0, 1}},
		},
		Format: "<f4",
		Crs: "utmXX",
	}

	buf, err := GetMetadata(well_known)
	if err != nil {
		t.Fatalf("Failed to retrive metadata, err %v", err)
	}

	var meta Metadata
	err = json.Unmarshal(buf, &meta)
	if err != nil {
		t.Fatalf("Failed to unmarshall response, err: %v", err)
	}

	if !reflect.DeepEqual(meta, expected) {
		t.Fatalf("Expected %v, got  %v", expected, meta)
	}
}

func TestSliceData(t *testing.T) {
	il := []float32{
		108, 109, 110, 111, // il: 3, xl: 10, samples: all
		112, 113, 114, 115, // il: 3, xl: 11, samples: all
	}

	xl := []float32{
		100, 101, 102, 103, // il: 1, xl: 10, samples: all
		108, 109, 110, 111, // il: 3, xl: 10, samples: all
		116, 117, 118, 119, // il: 5, xl: 10, samples: all
	}

	time := []float32{
		101, 105, // il: 1, xl: all, samples: 1
		109, 113, // il: 3, xl: all, samples: 1
		117, 121, // il: 5, xl: all, samples: 1
	}

	testcases := []struct{
			name      string
			lineno    int
			direction int
			expected  []float32
	} {
		{ name: "inline",    lineno: 3,  direction: AxisInline,    expected: il   },
		{ name: "i",         lineno: 1,  direction: AxisI,         expected: il   },
		{ name: "crossline", lineno: 10, direction: AxisCrossline, expected: xl   },
		{ name: "j",         lineno: 0,  direction: AxisJ,         expected: xl   },
		{ name: "time",      lineno: 8,  direction: AxisTime,      expected: time },
		{ name: "j",         lineno: 1,  direction: AxisK,         expected: time },
	}

	for _, testcase := range testcases {
		buf, err := GetSlice(well_known, testcase.lineno, testcase.direction)
		if err != nil {
			t.Errorf(
				"[case: %v] Failed to fetch slice, err: %v",
				testcase.name,
				err,
			)
		}

		slice, err := toFloat32(buf)
		if err != nil {
			t.Errorf("[case: %v] Err: %v", testcase.name, err)
		}

		if len(*slice) != len(testcase.expected) {
			msg := "[case: %v] Expected slice of len: %v, got: %v"
			t.Errorf(msg, testcase.name, len(testcase.expected), len(*slice))
		}

		for i, x := range(*slice) {
			if (x == testcase.expected[i]) {
				continue
			}

			msg := "[case: %v] Expected %v in pos %v, got: %v"
			t.Errorf(msg, testcase.name, testcase.expected[i], i, x)
		}
	}
}

func TestSliceOutOfBounds(t *testing.T) {
	testcases := []struct{
			name      string
			lineno    int
			direction int
	} {
		{ name: "Inline too low",     lineno: 0,  direction: AxisInline    },
		{ name: "Inline too high",    lineno: 6,  direction: AxisInline    },
		{ name: "Crossline too low",  lineno: 9,  direction: AxisCrossline },
		{ name: "Crossline too high", lineno: 12, direction: AxisCrossline },
		{ name: "Time too low",       lineno: 3,  direction: AxisTime      },
		{ name: "Time too high",      lineno: 17, direction: AxisTime      },
		{ name: "I too low",          lineno: -1, direction: AxisI         },
		{ name: "I too high",         lineno: 3,  direction: AxisI         },
		{ name: "J too low",          lineno: -1, direction: AxisJ         },
		{ name: "J too high",         lineno: 2,  direction: AxisJ         },
		{ name: "K too low",          lineno: -1, direction: AxisK         },
		{ name: "K too high",         lineno: 4,  direction: AxisK         },
	}

	for _, testcase := range testcases {
		_, err := GetSlice(well_known, testcase.lineno, testcase.direction)
		if err == nil {
			t.Errorf(
				"[case: %v] Expected slice to fail",
				testcase.name,
			)
		}

		if !strings.Contains(err.Error(), "Invalid lineno") {
			t.Errorf(
				"[case: %v] Expected error to contain 'Invalid lineno', was: %v",
				testcase.name,
				err,
			)
		}
	}

}

func TestSliceStridedLineno(t *testing.T) {
	testcases := []struct{
			name      string
			lineno    int
			direction int
	} {
		{ name: "inline", lineno: 2, direction: AxisInline },
		{ name: "time",   lineno: 5, direction: AxisTime   },
	}

	for _, testcase := range testcases {
		_, err := GetSlice(well_known, testcase.lineno, testcase.direction)
		if err == nil {
			t.Errorf(
				"[case: %v] Expected slice to fail",
				testcase.name,
			)
		}

		if !strings.Contains(err.Error(), "Invalid lineno") {
			t.Errorf(
				"[case: %v] Expected error to contain 'Invalid lineno', was: %v",
				testcase.name,
				err,
			)
		}
	}
}

func TestSliceInvalidAxis(t *testing.T) {
	testcases := []struct{
			name      string
			direction int
	} {
		{ name: "Too small", direction: -1 },
		{ name: "Too large", direction: 8  },
	}

	for _, testcase := range testcases {
		_, err := GetSlice(well_known, 0, testcase.direction)

		if err == nil {
			t.Errorf(
				"[case: %v] Expected slice to fail",
				testcase.name,
			)
		}

		if !strings.Contains(err.Error(), "Unhandled axis") {
			t.Errorf(
				"[case: %s] Expected error to contain 'Unhandled axis', was: %v",
				testcase.name,
				err,
			)
		}
	}
}

func TestDepthAxis(t *testing.T) {
	testcases := []struct{
			name      string
			direction int
			err       string
	} {
		{
			name: "Depth",
			direction: AxisDepth,
			err: "Unable to use Depth on cube with depth units: ms",
		},
		{
			name: "Time",
			direction: AxisSample,
			err: "Unable to use Sample on cube with depth units: ms",
		},
	}

	for _, testcase := range testcases {
		_, err := GetSlice(well_known, 0, testcase.direction)

		if err == nil {
			t.Errorf(
				"[case: %v] Expected slice to fail",
				testcase.name,
			)
		}

		if !strings.Contains(err.Error(), testcase.err) {
			t.Errorf(
				"[case: %s] Expected error to contain '%s', was: %v",
				testcase.name,
				testcase.err,
				err,
			)
		}
	}
}

func TestSliceMetadataAxisOrdering(t *testing.T) {
	testcases := []struct{
			name          string
			direction     int
			lineno        int
			expectedAxis []string
	} {
		{
			name: "Inline",
			direction: AxisInline,
			lineno: 3,
			expectedAxis: []string{"Sample", "Crossline"},
		},
		{
			name: "I",
			lineno: 0,
			direction: AxisI,
			expectedAxis: []string{"Sample", "Crossline"},
		},
		{
			name: "Crossline",
			direction: AxisCrossline,
			lineno: 10,
			expectedAxis: []string{"Sample", "Inline"},
		},
		{
			name: "J",
			lineno: 0,
			direction: AxisJ,
			expectedAxis: []string{"Sample", "Inline"},
		},
		{
			name: "Time",
			direction: AxisTime,
			lineno: 4,
			expectedAxis: []string{"Crossline", "Inline"},
		},
		{
			name: "K",
			lineno: 0,
			direction: AxisK,
			expectedAxis: []string{"Crossline", "Inline"},
		},
	}

	for _, testcase := range testcases {
		buf, err := GetSliceMetadata(well_known, testcase.direction)
		if err != nil {
			t.Fatalf(
				"[case: %v] Failed to get slice metadata, err: %v",
				testcase.name,
				err,
			)
		}

		var meta SliceMetadata
		err = json.Unmarshal(buf, &meta)
		if err != nil {
			t.Fatalf(
				"[case: %v] Failed to unmarshall response, err: %v",
				testcase.name,
				err,
			)
		}

		axis := []string{ meta.X.Annotation, meta.Y.Annotation }
		for i, ax := range(testcase.expectedAxis) {
			if ax != axis[i] {
				t.Fatalf(
					"[case: %v] Expected axis to be %v, got %v",
					testcase.name,
					testcase.expectedAxis,
					axis,
				)
			}
		}
	}
}

func TestFence(t *testing.T) {
	expected := []float32{
		108, 109, 110, 111, // il: 3, xl: 10, samples: all
		112, 113, 114, 115, // il: 3, xl: 11, samples: all
		100, 101, 102, 103, // il: 1, xl: 10, samples: all
		108, 109, 110, 111, // il: 3, xl: 10, samples: all
		116, 117, 118, 119, // il: 5, xl: 10, samples: all
	}

	testcases := []struct{
		coordinate_system int
		coordinates       [][]float32
	} {
		{
			coordinate_system: CoordinateSystemIndex,
			coordinates:
				[][]float32{{1, 0}, {1, 1}, {0, 0}, {1, 0}, {2, 0}},
		},
		{
			coordinate_system: CoordinateSystemAnnotation,
			coordinates:
				[][]float32{{3, 10}, {3, 11}, {1, 10}, {3, 10}, {5, 10}},
		},
		{
			coordinate_system: CoordinateSystemCdp,
			coordinates:
				[][]float32{{7, 4}, {2, 7}, {5, 0}, {7, 4}, {9, 8}},
		},
	}

	interpolationMethod, _ := GetInterpolationMethod("nearest")

	for _, testcase := range testcases {
		buf, err := GetFence(
			well_known,
			testcase.coordinate_system,
			testcase.coordinates,
			interpolationMethod,
		)
		if err != nil {
			t.Errorf(
				"[coordinate_system: %v] Failed to fetch fence, err: %v",
				testcase.coordinate_system,
				err,
			)
		}

		fence, err := toFloat32(buf)
		if err != nil {
			t.Errorf(
				"[coordinate_system: %v] Err: %v",
				testcase.coordinate_system,
				err,
			)
		}

		if len(*fence) != len(expected) {
			msg := "[coordinate_system: %v] Expected fence of len: %v, got: %v"
			t.Errorf(
				msg,
				testcase.coordinate_system,
				len(expected),
				len(*fence),
			)
		}

		for i, x := range(*fence) {
			if (x == expected[i]) {
				continue
			}

			msg := "[coordinate_system: %v] Expected %v in pos %v, got: %v"
			t.Errorf(
				msg,
				testcase.coordinate_system,
				expected[i],
				i,
				x,
			)
		}
	}
}

func TestFenceBorders(t *testing.T) {
	testcases := []struct {
		name              string
		coordinate_system int
		coordinates       [][]float32
		interpolation     string
		error             string
	}{
		{
			name:              "coordinate 2 is just-out-of-upper-bound in direction 0",
			coordinate_system: CoordinateSystemAnnotation,
			coordinates:       [][]float32{{5, 9.5}, {6, 11.25}},
			error:             "is out of boundaries in dimension 0.",
		},
		{
			name:              "coordinate 1 is just-out-of-upper-bound in direction 1",
			coordinate_system: CoordinateSystemAnnotation,
			coordinates:       [][]float32{{5.5, 11.5}, {3, 10}},
			error:             "is out of boundaries in dimension 1.",
		},
		{
			name:              "coordinate is long way out of upper-bound in both directions",
			coordinate_system: CoordinateSystemCdp,
			coordinates:       [][]float32{{700, 1200}},
			error:             "is out of boundaries in dimension 0.",
		},
		{
			name:              "coordinate 2 is just-out-of-lower-bound in direction 1",
			coordinate_system: CoordinateSystemAnnotation,
			coordinates:       [][]float32{{0, 11}, {5.9999, 10}, {0.0001, 9.4999}},
			error:             "is out of boundaries in dimension 1.",
		},
		{
			name:              "negative coordinate 1 is out-of-lower-bound in direction 0",
			coordinate_system: CoordinateSystemIndex,
			coordinates:       [][]float32{{-1, 0}, {-3, 0}},
			error:             "is out of boundaries in dimension 0.",
		},
	}

	for _, testcase := range testcases {
		interpolationMethod, _ := GetInterpolationMethod("linear")
		_, err := GetFence(well_known, testcase.coordinate_system, testcase.coordinates, interpolationMethod)

		if err == nil {
			msg := "in testcase \"%s\" expected to fail given fence is out of bounds %v"
			t.Errorf(msg, testcase.name, testcase.coordinates)
		} else {
			if !strings.Contains(err.Error(), testcase.error) {
				msg := "Unexpected error message in testcase \"%s\", expected \"%s\", was \"%s\""
				t.Errorf(msg, testcase.name, testcase.error, err.Error())
			}
		}
	}
}

func TestFenceNearestInterpolationSnap(t *testing.T) {
	testcases := []struct {
		coordinate_system int
		coordinates       [][]float32
		expected          []float32
	}{
		{
			coordinate_system: CoordinateSystemAnnotation,
			coordinates:       [][]float32{{3, 10}},
			expected:          []float32{108, 109, 110, 111},
		},
		{
			coordinate_system: CoordinateSystemAnnotation,
			coordinates:       [][]float32{{3.5, 10.25}},
			expected:          []float32{108, 109, 110, 111},
		},
		{
			coordinate_system: CoordinateSystemAnnotation,
			coordinates:       [][]float32{{3.9999, 10.4999}},
			expected:          []float32{108, 109, 110, 111},
		},
		{
			coordinate_system: CoordinateSystemAnnotation,
			coordinates:       [][]float32{{4, 10.5}},
			expected:          []float32{120, 121, 122, 123},
		},
		{
			coordinate_system: CoordinateSystemAnnotation,
			coordinates:       [][]float32{{4.5, 10.75}},
			expected:          []float32{120, 121, 122, 123},
		},
		{
			coordinate_system: CoordinateSystemAnnotation,
			coordinates:       [][]float32{{5, 11}},
			expected:          []float32{120, 121, 122, 123},
		},
		{
			coordinate_system: CoordinateSystemAnnotation,
			coordinates:       [][]float32{{5.5, 11.25}},
			expected:          []float32{120, 121, 122, 123},
		},
		{
			coordinate_system: CoordinateSystemIndex,
			coordinates:       [][]float32{{-0.5, -0.5}},
			expected:          []float32{100, 101, 102, 103},
		},
		{
			coordinate_system: CoordinateSystemCdp,
			coordinates:       [][]float32{{8, 5}},
			expected:          []float32{108, 109, 110, 111},
		},
		{
			coordinate_system: CoordinateSystemCdp,
			coordinates:       [][]float32{{5.5, 7.5}},
			expected:          []float32{120, 121, 122, 123},
		},
	}

	interpolationMethod, _ := GetInterpolationMethod("nearest")

	for _, testcase := range testcases {
		buf, err := GetFence(
			well_known,
			testcase.coordinate_system,
			testcase.coordinates,
			interpolationMethod,
		)
		if err != nil {
			t.Errorf(
				"[coordinate_system: %v] Failed to fetch fence, err: %v",
				testcase.coordinate_system,
				err,
			)
		}

		fence, err := toFloat32(buf)
		if err != nil {
			t.Errorf(
				"[coordinate_system: %v] Err: %v",
				testcase.coordinate_system,
				err,
			)
		}

		if len(*fence) != len(testcase.expected) {
			msg := "[coordinate_system: %v] Expected fence of len: %v, got: %v"
			t.Errorf(
				msg,
				testcase.coordinate_system,
				len(testcase.expected),
				len(*fence),
			)
		}

		for i, x := range *fence {
			if x == testcase.expected[i] {
				continue
			}

			msg := "[coordinate_system: %v] Expected %v in pos %v, got: %v"
			t.Errorf(
				msg,
				testcase.coordinate_system,
				testcase.expected[i],
				i,
				x,
			)
		}
	}
}

func TestInvalidFence(t *testing.T) {
	fence := [][]float32{{1, 0}, {1, 1, 0}, {0, 0}, {1, 0}, {2, 0}}

	interpolationMethod, _ := GetInterpolationMethod("nearest")

	_, err := GetFence(well_known, CoordinateSystemIndex, fence, interpolationMethod)

	if err == nil {
		msg := "Expected to fail given invalid fence %v"
		t.Errorf(msg, fence)
	} else {
		expected := "invalid coordinate [1 1 0] at position 1, expected [x y] pair"
		if err.Error() != expected {
			msg := "Unexpected error message, expected \"%s\", was \"%s\""
			t.Errorf(msg, expected, err.Error())
		}
	}
}

/*
We don't know the specifics of how OpenVDS implements the different
interpolation algorithms, so we don't know what values to expect in
return. As a substitute we pass in different interpolation values
and check they yield different results.
*/
func TestFenceInterpolation(t *testing.T) {
	fence := [][]float32{{3.2, 3}, {3.2, 6.3}, {1, 3}, {3.2, 3}, {5.4, 3}}
	interpolationMethods := []string{"nearest", "linear", "cubic", "triangular", "angular"}
	for i, v1 := range interpolationMethods {
		interpolationMethod, _ := GetInterpolationMethod(v1)
		buf1, _ := GetFence(well_known, CoordinateSystemCdp, fence, interpolationMethod)
		for _, v2 := range interpolationMethods[i+1:] {
			interpolationMethod, _ := GetInterpolationMethod(v2)
			buf2, _ := GetFence(well_known, CoordinateSystemCdp, fence, interpolationMethod)
			different := false
			for k := range buf1 {
				if buf1[k] != buf2[k] {
					different = true
					break
				}
			}
			if !different {
				msg := "[fence_interpolation]Expected %v interpolation and %v interpolation to be different"
				t.Errorf(msg, v1, v2)
			}
		}
	}
}

func TestFenceInterpolationDefaultIsNearest(t *testing.T) {
	defaultInterpolation, _ := GetInterpolationMethod("")
	nearestInterpolation, _ := GetInterpolationMethod("nearest")
	if defaultInterpolation != nearestInterpolation {
		msg := "[fence_interpolation]Default interpolation is not nearest"
		t.Error(msg)
	}
}

func TestInvalidInterpolationMethod(t *testing.T) {
	interpolation := "sand"
	_, err := GetInterpolationMethod(interpolation)
	if err == nil {
		msg := "[fence_interpolation]Expected fail given invalid interpolation method: %v"
		t.Errorf(msg, interpolation)
	} else {
		options := "nearest, linear, cubic, angular or triangular"
		expected := fmt.Sprintf("invalid interpolation method 'sand', valid options are: %s", options)
		if err.Error() != expected {
			msg := "Unexpected error message, expected \"%s\", was \"%s\""
			t.Errorf(msg, expected, err.Error())
		}
	}
}

func TestFenceInterpolationCaseInsensitive(t *testing.T) {
	expectedInterpolation, _ := GetInterpolationMethod("cubic")
	interpolation, _ := GetInterpolationMethod("CuBiC")
	if interpolation != expectedInterpolation {
		msg := "[fence_interpolation]Fence interpolation is not case insensitive"
		t.Error(msg)
	}
}

func TestOnly3DSupported(t *testing.T) {
	testcases := []struct {
		name     string
		function func() ([]byte, error)
	}{
		{
			name:     "Slice",
			function: func() ([]byte, error) { return GetSlice(prestack, 0, 0) },
		},
		{
			name:     "SliceMetadata",
			function: func() ([]byte, error) { return GetSliceMetadata(prestack, 0) },
		},
		{
			name:     "Fence",
			function: func() ([]byte, error) { return GetFence(prestack, 0, [][]float32{{0, 0}}, 0) },
		},
		{
			name:     "FenceMetadata",
			function: func() ([]byte, error) { return GetFenceMetadata(prestack, [][]float32{{0, 0}}) },
		},
		{
			name:     "Metadata",
			function: func() ([]byte, error) { return GetMetadata(prestack) },
		},
	}

	for _, testcase := range testcases {
		_, err := testcase.function()

		if err == nil {
			t.Errorf(
				"[case: %v] Expected slice to fail",
				testcase.name,
			)
		}

		if !strings.Contains(err.Error(), "3 dimensions, got 4") {
			t.Errorf(
				"[case: %s] Expected error to contain '3 dimensions, got 4', was: %v",
				testcase.name,
				err,
			)
		}
	}
}
