package vds

import (
	"encoding/binary"
	"encoding/json"
	"errors"
	"math"
	"strings"
	"testing"
)

const (
	well_known = "file://../../testdata/wellknown/well_known_default.vds"
)

type Axis struct {
	Annotation string  `json:"annotation"`
	Min        float64 `json:"min"`
	Max        float64 `json:"max"`
	Sample     float64 `json:"sample"`
	Unit       string  `json:"unit"`
}

type Metadata struct {
	Format int     `json:"format"`
	Axis   []*Axis `json:"axis"`
}

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
		buf, err := Slice(well_known, "", testcase.lineno, testcase.direction)
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
		_, err := Slice(well_known, "", testcase.lineno, testcase.direction)
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
		_, err := Slice(well_known, "", testcase.lineno, testcase.direction)
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
		_, err := Slice(well_known, "", 0, testcase.direction)

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
		_, err := Slice(well_known, "", 0, testcase.direction)

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
			expectedAxis: []string{"Crossline", "Sample"},
		},
		{
			name: "I",
			lineno: 0,
			direction: AxisI,
			expectedAxis: []string{"Crossline", "Sample"},
		},
		{
			name: "Crossline",
			direction: AxisCrossline,
			lineno: 10,
			expectedAxis: []string{"Inline", "Sample"},
		},
		{
			name: "J",
			lineno: 0,
			direction: AxisJ,
			expectedAxis: []string{"Inline", "Sample"},
		},
		{
			name: "Time",
			direction: AxisTime,
			lineno: 4,
			expectedAxis: []string{"Inline", "Crossline"},
		},
		{
			name: "K",
			lineno: 0,
			direction: AxisK,
			expectedAxis: []string{"Inline", "Crossline"},
		},
	}

	for _, testcase := range testcases {
		buf, err := SliceMetadata(well_known, "", testcase.lineno, testcase.direction)

		var meta Metadata
		err = json.Unmarshal(buf, &meta)
		if err != nil {
			t.Fatalf(
				"[case: %v] Failed to unmarshall response, err: %v",
				testcase.name,
				err,
			)
		}

		for i, axis := range(meta.Axis) {
			if testcase.expectedAxis[i] != axis.Annotation {
				t.Fatalf(
					"[case: %v] Expected axis %v to be %v, got %v",
					testcase.name,
					i,
					testcase.expectedAxis[i],
					axis.Annotation,
				)
			}
		}
	}
}
