package main

import (
	"encoding/binary"
	"encoding/json"
	"fmt"
	"math"
	"net/http"
	"testing"

	"github.com/stretchr/testify/require"
)

const formatFile = "../../testdata/formats/format%d.vds"

/** Test we can read data in all openvds-supported formats
 *
 *  While this seems like we are testing openvds functionality, we prefer to
 *  do it as we are not 100% sure that we understand how to use openvds
 *  correctly.
 */
func TestSupportedFormats(t *testing.T) {
	var testcases []endpointTest
	addTests := func(format int) {
		sliceTest := sliceTest{
			baseTest{
				name:           fmt.Sprintf("slice request, format %d", format),
				method:         http.MethodGet,
				expectedStatus: http.StatusOK,
			},
			testSliceRequest{
				Vds:       fmt.Sprintf(formatFile, format),
				Direction: "i",
				Lineno:    1,
				Sas:       "n/a",
			},
		}
		fenceTest := fenceTest{
			baseTest{
				name:           fmt.Sprintf("fence request, format %d", format),
				method:         http.MethodPost,
				expectedStatus: http.StatusOK,
			},

			testFenceRequest{
				Vds:              fmt.Sprintf(formatFile, format),
				CoordinateSystem: "ij",
				Coordinates:      [][]float32{{1, 0}, {1, 1}},
				Sas:              "n/a",
			},
		}

		testcases = append(testcases, sliceTest)
		testcases = append(testcases, fenceTest)
	}

	supportedFormats := [8]int{1, 2, 3, 5, 8, 10, 11, 16}
	for _, format := range supportedFormats {
		addTests(format)
	}

	for _, testcase := range testcases {
		w := setupTest(t, testcase)
		requireStatus(t, testcase, w)

		parts := readMultipartData(t, w)

		metadata := make(map[string]interface{})
		require.NoError(t, json.Unmarshal(parts[0], &metadata))
		format := metadata["format"]

		require.Equalf(t, "<f4", format,
			"Test '%v'. Expected format to be <f4, was %v",
			testcase.base().name, format)

		toLittleEndianFloat32 := func(data []byte, floatsNumber int) []float32 {
			var floats []float32 = make([]float32, floatsNumber)
			for i := 0; i < floatsNumber; i++ {
				value := binary.LittleEndian.Uint32(data[i*4 : (i+1)*4])
				floats[i] = math.Float32frombits(value)
			}
			return floats
		}

		// test just part of returned data
		expected := []float32{60, 61, 62, 63, 64, 65, 66}
		actual := toLittleEndianFloat32(parts[1], len(expected))

		require.Equalf(t, expected, actual,
			"Test '%v'. Expected %v, was %v",
			testcase.base().name, expected, actual)
	}
}
