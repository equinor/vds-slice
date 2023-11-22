package core

import (
	"bytes"
	"encoding/json"
	"strings"
	"testing"

	"github.com/stretchr/testify/require"
)

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

	testcases := []struct {
		name      string
		lineno    int
		direction int
		expected  []float32
	}{
		{name: "inline", lineno: 3, direction: AxisInline, expected: il},
		{name: "i", lineno: 1, direction: AxisI, expected: il},
		{name: "crossline", lineno: 10, direction: AxisCrossline, expected: xl},
		{name: "j", lineno: 0, direction: AxisJ, expected: xl},
		{name: "time", lineno: 8, direction: AxisTime, expected: time},
		{name: "j", lineno: 1, direction: AxisK, expected: time},
	}

	for _, testcase := range testcases {
		handle, _ := NewDSHandle(well_known, "")
		defer handle.Close()
		buf, err := handle.GetSlice(
			testcase.lineno,
			testcase.direction,
			[]Bound{},
		)
		require.NoErrorf(t, err,
			"[case: %v] Failed to fetch slice, err: %v",
			testcase.name,
			err,
		)

		slice, err := toFloat32(buf)
		require.NoErrorf(t, err, "[case: %v] Err: %v", testcase.name, err)

		require.Equalf(t, testcase.expected, *slice, "[case: %v]", testcase.name)
	}
}

func TestSliceOutOfBounds(t *testing.T) {
	testcases := []struct {
		name      string
		lineno    int
		direction int
	}{
		{name: "Inline too low", lineno: 0, direction: AxisInline},
		{name: "Inline too high", lineno: 6, direction: AxisInline},
		{name: "Crossline too low", lineno: 9, direction: AxisCrossline},
		{name: "Crossline too high", lineno: 12, direction: AxisCrossline},
		{name: "Time too low", lineno: 3, direction: AxisTime},
		{name: "Time too high", lineno: 17, direction: AxisTime},
		{name: "I too low", lineno: -1, direction: AxisI},
		{name: "I too high", lineno: 3, direction: AxisI},
		{name: "J too low", lineno: -1, direction: AxisJ},
		{name: "J too high", lineno: 2, direction: AxisJ},
		{name: "K too low", lineno: -1, direction: AxisK},
		{name: "K too high", lineno: 4, direction: AxisK},
	}

	for _, testcase := range testcases {
		handle, _ := NewDSHandle(well_known, "")
		defer handle.Close()
		_, err := handle.GetSlice(
			testcase.lineno,
			testcase.direction,
			[]Bound{},
		)

		require.ErrorContains(t, err, "Invalid lineno")
	}

}

func TestSliceStepSizedLineno(t *testing.T) {
	testcases := []struct {
		name      string
		lineno    int
		direction int
	}{
		{name: "inline", lineno: 2, direction: AxisInline},
		{name: "time", lineno: 5, direction: AxisTime},
	}

	for _, testcase := range testcases {
		handle, _ := NewDSHandle(well_known, "")
		defer handle.Close()
		_, err := handle.GetSlice(
			testcase.lineno,
			testcase.direction,
			[]Bound{},
		)

		require.ErrorContains(t, err, "Invalid lineno")
	}
}

func TestSliceInvalidAxis(t *testing.T) {
	testcases := []struct {
		name      string
		direction int
	}{
		{name: "Too small", direction: -1},
		{name: "Too large", direction: 8},
	}

	for _, testcase := range testcases {
		handle, _ := NewDSHandle(well_known, "")
		defer handle.Close()
		_, err := handle.GetSlice(0, testcase.direction, []Bound{})

		require.ErrorContains(t, err, "Unhandled axis")
	}
}

func TestSliceBounds(t *testing.T) {
	newBound := func(direction string, lower, upper int) Bound {
		return Bound{Direction: &direction, Lower: &lower, Upper: &upper}
	}
	newAxis := func(
		annotation string,
		min float64,
		max float64,
		samples int,
		stepsize float64,
	) Axis {
		unit := "ms"
		anno := strings.ToLower(annotation)
		if anno == "inline" || anno == "crossline" {
			unit = "unitless"
		}

		return Axis{
			Annotation: annotation,
			Min:        min,
			Max:        max,
			Samples:    samples,
			StepSize:   stepsize,
			Unit:       unit,
		}
	}

	testCases := []struct {
		name          string
		direction     string
		lineno        int
		bounds        []Bound
		expectedSlice []float32
		expectedErr   error
		expectedShape []int
		expectedXAxis Axis
		expectedYAxis Axis
		expectedGeo   [][]float64
	}{
		{
			name:      "Constraint in slice dir is ignored - same coordinate system",
			direction: "i",
			lineno:    1,
			bounds: []Bound{
				newBound("i", 0, 1),
			},
			expectedSlice: []float32{
				108, 109, 110, 111,
				112, 113, 114, 115,
			},
			expectedShape: []int{2, 4},
			expectedXAxis: newAxis("Sample", 4, 16, 4, 4),
			expectedYAxis: newAxis("Crossline", 10, 11, 2, 1),
			expectedGeo:   [][]float64{{8, 4}, {6, 7}},
		},
		{
			name:      "Constraint in slice dir is ignored - different coordinate system",
			direction: "crossline",
			lineno:    10,
			bounds: []Bound{
				newBound("j", 0, 1),
			},
			expectedSlice: []float32{
				100, 101, 102, 103,
				108, 109, 110, 111,
				116, 117, 118, 119,
			},
			expectedShape: []int{3, 4},
			expectedXAxis: newAxis("Sample", 4, 16, 4, 4),
			expectedYAxis: newAxis("Inline", 1, 5, 3, 2),
			expectedGeo:   [][]float64{{2, 0}, {14, 8}},
		},
		{
			name:      "Multiple constraints in slice dir are all ignored",
			direction: "time",
			lineno:    4,
			bounds: []Bound{
				newBound("time", 8, 12),
				newBound("k", 0, 1),
			},
			expectedSlice: []float32{
				100, 104,
				108, 112,
				116, 120,
			},
			expectedShape: []int{3, 2},
			expectedXAxis: newAxis("Crossline", 10, 11, 2, 1),
			expectedYAxis: newAxis("Inline", 1, 5, 3, 2),
			expectedGeo:   [][]float64{{2, 0}, {14, 8}, {12, 11}, {0, 3}},
		},
		{
			name:      "Single constraint - same coordinate system",
			direction: "inline",
			lineno:    3,
			bounds: []Bound{
				newBound("crossline", 10, 10),
			},
			expectedSlice: []float32{
				108, 109, 110, 111,
			},
			expectedShape: []int{1, 4},
			expectedXAxis: newAxis("Sample", 4, 16, 4, 4),
			expectedYAxis: newAxis("Crossline", 10, 10, 1, 1),
			expectedGeo:   [][]float64{{8, 4}, {8, 4}},
		},
		{
			name:      "Single constraint - different coordinate system",
			direction: "sample",
			lineno:    4,
			bounds: []Bound{
				newBound("i", 0, 1),
			},
			expectedSlice: []float32{
				100, 104,
				108, 112,
			},
			expectedShape: []int{2, 2},
			expectedXAxis: newAxis("Crossline", 10, 11, 2, 1),
			expectedYAxis: newAxis("Inline", 1, 3, 2, 2),
			expectedGeo:   [][]float64{{2, 0}, {8, 4}, {6, 7}, {0, 3}},
		},
		{
			name:      "Two constraints - same coordinate system",
			direction: "k",
			lineno:    0,
			bounds: []Bound{
				newBound("i", 0, 1),
				newBound("j", 0, 0),
			},
			expectedSlice: []float32{
				100,
				108,
			},
			expectedShape: []int{2, 1},
			expectedXAxis: newAxis("Crossline", 10, 10, 1, 1),
			expectedYAxis: newAxis("Inline", 1, 3, 2, 2),
			expectedGeo:   [][]float64{{2, 0}, {8, 4}, {8, 4}, {2, 0}},
		},
		{
			name:      "Two constraints - different coordinate system",
			direction: "j",
			lineno:    0,
			bounds: []Bound{
				newBound("inline", 1, 3),
				newBound("k", 1, 2),
			},
			expectedSlice: []float32{
				101, 102,
				109, 110,
			},
			expectedShape: []int{2, 2},
			expectedXAxis: newAxis("Sample", 8, 12, 2, 4),
			expectedYAxis: newAxis("Inline", 1, 3, 2, 2),
			expectedGeo:   [][]float64{{2, 0}, {8, 4}},
		},
		{
			name:      "Horizonal bounds for full axis range is the same as no bound",
			direction: "time",
			lineno:    12,
			bounds: []Bound{
				newBound("crossline", 10, 11),
				newBound("i", 0, 2),
			},
			expectedSlice: []float32{
				102, 106,
				110, 114,
				118, 122,
			},
			expectedShape: []int{3, 2},
			expectedXAxis: newAxis("Crossline", 10, 11, 2, 1),
			expectedYAxis: newAxis("Inline", 1, 5, 3, 2),
			expectedGeo:   [][]float64{{2, 0}, {14, 8}, {12, 11}, {0, 3}},
		},
		{
			name:      "Vertical bounds for full axis range is the same as no bound",
			direction: "inline",
			lineno:    5,
			bounds: []Bound{
				newBound("time", 4, 16),
			},
			expectedSlice: []float32{
				116, 117, 118, 119,
				120, 121, 122, 123,
			},
			expectedShape: []int{2, 4},
			expectedXAxis: newAxis("Sample", 4, 16, 4, 4),
			expectedYAxis: newAxis("Crossline", 10, 11, 2, 1),
			expectedGeo:   [][]float64{{14, 8}, {12, 11}},
		},
		{
			name:      "The last bound takes precedence",
			direction: "inline",
			lineno:    5,
			bounds: []Bound{
				newBound("time", 4, 8),
				newBound("time", 12, 16),
			},
			expectedSlice: []float32{
				118, 119,
				122, 123,
			},
			expectedShape: []int{2, 2},
			expectedXAxis: newAxis("Sample", 12, 16, 2, 4),
			expectedYAxis: newAxis("Crossline", 10, 11, 2, 1),
			expectedGeo:   [][]float64{{14, 8}, {12, 11}},
		},
		{
			name:      "Out-Of-Bounds bounds errors",
			direction: "inline",
			lineno:    5,
			bounds: []Bound{
				newBound("time", 8, 20),
			},
			expectedSlice: []float32{},
			expectedErr:   NewInvalidArgument(""),
		},
		{
			name:      "Incorrect vertical domain",
			direction: "inline",
			lineno:    5,
			bounds: []Bound{
				newBound("depth", 8, 20),
			},
			expectedSlice: []float32{},
			expectedErr:   NewInvalidArgument(""),
		},
	}

	for _, testCase := range testCases {
		direction, err := GetAxis(testCase.direction)
		require.NoError(t, err)

		handle, _ := NewDSHandle(well_known, "")
		defer handle.Close()
		buf, err := handle.GetSlice(
			testCase.lineno,
			direction,
			testCase.bounds,
		)

		require.IsTypef(t, testCase.expectedErr, err,
			"[case: %v] Got: %v",
			testCase.name,
			err,
		)
		if testCase.expectedErr != nil {
			continue
		}

		slice, err := toFloat32(buf)
		require.NoErrorf(t, err, "[case: %v] Err: %v", testCase.name, err)

		require.Equalf(t, testCase.expectedSlice, *slice, "[case: %v]", testCase.name)

		buf, err = handle.GetSliceMetadata(
			testCase.lineno,
			direction,
			testCase.bounds,
		)
		require.NoError(t, err,
			"[case: %v] Failed to get slice metadata, err: %v",
			testCase.name,
			err,
		)

		var meta SliceMetadata
		err = json.Unmarshal(buf, &meta)
		require.NoErrorf(t, err,
			"[case: %v] Failed to unmarshall response, err: %v",
			testCase.name,
			err,
		)

		require.Equalf(t, testCase.expectedShape, meta.Shape,
			"[case: %v] Shape not equal",
			testCase.name,
		)

		require.Equalf(t, testCase.expectedXAxis, meta.X,
			"[case: %v] X axis not equal",
			testCase.name,
		)

		require.Equalf(t, testCase.expectedYAxis, meta.Y,
			"[case: %v] Y axis not equal",
			testCase.name,
		)

		require.Equalf(t, testCase.expectedGeo, meta.Geospatial,
			"[case: %v] Geospatial not equal",
			testCase.name,
		)
	}
}

func TestDepthAxis(t *testing.T) {
	testcases := []struct {
		name      string
		direction int
		err       error
	}{
		{
			name:      "Depth",
			direction: AxisDepth,
			err: NewInvalidArgument(
				"Cannot fetch depth slice for VDS file with vertical axis unit: ms",
			),
		},
	}

	for _, testcase := range testcases {
		handle, _ := NewDSHandle(well_known, "")
		defer handle.Close()
		_, err := handle.GetSlice(0, testcase.direction, []Bound{})

		require.Equal(t, err, testcase.err)
	}
}

func TestSliceMetadata(t *testing.T) {
	lineno := 1
	direction := AxisJ
	expected := SliceMetadata{
		Array: Array{
			Format: "<f4",
		},
		X:          Axis{Annotation: "Sample", Min: 4, Max: 16, Samples: 4, StepSize: 4, Unit: "ms"},
		Y:          Axis{Annotation: "Inline", Min: 1, Max: 5, Samples: 3, StepSize: 2, Unit: "unitless"},
		Geospatial: [][]float64{{0, 3}, {12, 11}},
		Shape:      []int{3, 4},
	}
	handle, _ := NewDSHandle(well_known, "")
	defer handle.Close()
	buf, err := handle.GetSliceMetadata(lineno, direction, []Bound{})
	require.NoErrorf(t, err, "Failed to retrieve slice metadata, err %v", err)

	var meta SliceMetadata

	dec := json.NewDecoder(bytes.NewReader(buf))
	dec.DisallowUnknownFields()
	err = dec.Decode(&meta)
	require.NoErrorf(t, err, "Failed to unmarshall response, err: %v", err)

	require.Equal(t, expected, meta)
}

func TestSliceMetadataAxisOrdering(t *testing.T) {
	testcases := []struct {
		name         string
		direction    int
		lineno       int
		expectedAxis []string
	}{
		{
			name:         "Inline",
			direction:    AxisInline,
			lineno:       3,
			expectedAxis: []string{"Sample", "Crossline"},
		},
		{
			name:         "I",
			lineno:       0,
			direction:    AxisI,
			expectedAxis: []string{"Sample", "Crossline"},
		},
		{
			name:         "Crossline",
			direction:    AxisCrossline,
			lineno:       10,
			expectedAxis: []string{"Sample", "Inline"},
		},
		{
			name:         "J",
			lineno:       0,
			direction:    AxisJ,
			expectedAxis: []string{"Sample", "Inline"},
		},
		{
			name:         "Time",
			direction:    AxisTime,
			lineno:       4,
			expectedAxis: []string{"Crossline", "Inline"},
		},
		{
			name:         "K",
			lineno:       0,
			direction:    AxisK,
			expectedAxis: []string{"Crossline", "Inline"},
		},
	}

	for _, testcase := range testcases {
		handle, _ := NewDSHandle(well_known, "")
		defer handle.Close()
		buf, err := handle.GetSliceMetadata(
			testcase.lineno,
			testcase.direction,
			[]Bound{},
		)
		require.NoErrorf(t, err,
			"[case: %v] Failed to get slice metadata, err: %v",
			testcase.name,
			err,
		)

		var meta SliceMetadata
		err = json.Unmarshal(buf, &meta)
		require.NoErrorf(t, err,
			"[case: %v] Failed to unmarshall response, err: %v",
			testcase.name,
			err,
		)

		axis := []string{meta.X.Annotation, meta.Y.Annotation}

		require.Equalf(t, testcase.expectedAxis, axis, "[case: %v]", testcase.name)
	}
}

func TestSliceGeospatial(t *testing.T) {
	testcases := []struct {
		name      string
		direction int
		lineno    int
		expected  [][]float64
	}{
		{
			name:      "Inline",
			direction: AxisInline,
			lineno:    3,
			expected:  [][]float64{{8.0, 4.0}, {6.0, 7.0}},
		},
		{
			name:      "I",
			direction: AxisI,
			lineno:    1,
			expected:  [][]float64{{8.0, 4.0}, {6.0, 7.0}},
		},
		{
			name:      "Crossline",
			direction: AxisCrossline,
			lineno:    10,
			expected:  [][]float64{{2.0, 0.0}, {14.0, 8.0}},
		},
		{
			name:      "J",
			direction: AxisJ,
			lineno:    0,
			expected:  [][]float64{{2.0, 0.0}, {14.0, 8.0}},
		},
		{
			name:      "Time",
			direction: AxisTime,
			lineno:    4,
			expected:  [][]float64{{2.0, 0.0}, {14.0, 8.0}, {12.0, 11.0}, {0.0, 3.0}},
		},
		{
			name:      "K",
			direction: AxisK,
			lineno:    3,
			expected:  [][]float64{{2.0, 0.0}, {14.0, 8.0}, {12.0, 11.0}, {0.0, 3.0}},
		},
	}

	for _, testcase := range testcases {
		handle, _ := NewDSHandle(well_known, "")
		defer handle.Close()
		buf, err := handle.GetSliceMetadata(
			testcase.lineno,
			testcase.direction,
			[]Bound{},
		)
		require.NoError(t, err,
			"[case: %v] Failed to get slice metadata, err: %v",
			testcase.name,
			err,
		)

		var meta SliceMetadata
		err = json.Unmarshal(buf, &meta)
		require.NoError(t, err,
			"[case: %v] Failed to unmarshall response, err: %v",
			testcase.name,
			err,
		)

		require.Equalf(
			t,
			testcase.expected,
			meta.Geospatial,
			"[case %v] Incorrect geospatial information",
			testcase.name,
		)
	}
}
