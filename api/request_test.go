package api

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func newSliceRequest(
	vds []string,
	sas []string,
	binaryOperator string,
	direction string,
	lineno int,
) SliceRequest {
	return SliceRequest{
		RequestedResource: RequestedResource{
			Vds:            vds,
			Sas:            sas,
			BinaryOperator: binaryOperator,
		},
		Direction: direction,
		Lineno:    &lineno,
	}
}

func newFenceRequest(
	vds []string,
	sas []string,
	binaryOperator string,
	coordinateSystem string,
	coordinates [][]float32,
	interpolation string,
) FenceRequest {
	return FenceRequest{
		RequestedResource: RequestedResource{
			Vds:            vds,
			Sas:            sas,
			BinaryOperator: binaryOperator,
		},
		CoordinateSystem: coordinateSystem,
		Coordinates:      coordinates,
		Interpolation:    interpolation,
	}
}

func newRequestedResource(
	vds []string,
	sas []string,
) RequestedResource {
	return RequestedResource{
		Vds: vds,
		Sas: sas,
	}
}

func TestSasIsOmmitedFromSliceHash(t *testing.T) {
	var lineNr int = 9961
	testCases := []struct {
		name     string
		request1 SliceRequest
		request2 SliceRequest
	}{
		{
			name: "Sas differ",
			request1: newSliceRequest(
				[]string{"some-path"},
				[]string{"some-sas"}, "", "inline", lineNr),
			request2: newSliceRequest(
				[]string{"some-path"},
				[]string{"different-sas"}, "", "inline", lineNr),
		},
		{
			name: "Sas differ 1",
			request1: newSliceRequest(
				[]string{"some-path", "some-path"},
				[]string{"some-sas", "some-sas"}, "subtraction", "inline", lineNr),
			request2: newSliceRequest(
				[]string{"some-path", "some-path"},
				[]string{"different-sas", "some-sas"}, "subtraction", "inline", lineNr),
		},
		{
			name: "Sas differ 2",
			request1: newSliceRequest(
				[]string{"some-path", "some-path"},
				[]string{"some-sas", "some-sas"}, "subtraction", "inline", lineNr),
			request2: newSliceRequest(
				[]string{"some-path", "some-path"},
				[]string{"some-sas", "different-sas"}, "subtraction", "inline", lineNr),
		},
		{
			name: "Binary operator specified",
			request1: newSliceRequest(
				[]string{"some-path"},
				[]string{"some-sas"}, "", "inline", lineNr),
			request2: SliceRequest{
				RequestedResource: RequestedResource{Vds: []string{"some-path"}, Sas: []string{"some-sas"}},
				Direction:         "inline",
				Lineno:            &lineNr,
			},
		},
	}

	for _, testCase := range testCases {
		hash1, err := testCase.request1.hash()
		require.NoErrorf(t, err,
			"[%s] Failed to compute hash, err: %v", testCase.name, err,
		)

		hash2, err := testCase.request2.hash()
		require.NoErrorf(t, err,
			"[%s] Failed to compute hash, err: %v", testCase.name, err,
		)

		require.Equalf(t, hash1, hash2, "Expected hashes to be equal")
	}
}

func TestSasIsOmmitedFromFenceHash(t *testing.T) {
	fence := [][]float32{{1, 2}, {3, 4}}
	testCases := []struct {
		name     string
		request1 FenceRequest
		request2 FenceRequest
	}{
		{
			name: "Sas differ",
			request1: newFenceRequest(
				[]string{"some-path"},
				[]string{"some-sas"}, "", "ij", fence, "linear"),
			request2: newFenceRequest(
				[]string{"some-path"},
				[]string{"different-sas"}, "", "ij", fence, "linear"),
		},
		{
			name: "Sas differ 1",
			request1: newFenceRequest(
				[]string{"some-path", "some-path"},
				[]string{"some-sas", "some-sas"}, "subtraction", "ij", fence, "linear"),
			request2: newFenceRequest(
				[]string{"some-path", "some-path"},
				[]string{"different-sas", "some-sas"}, "subtraction", "ij", fence, "linear"),
		},
		{
			name: "Sas differ 2",
			request1: newFenceRequest(
				[]string{"some-path", "some-path"},
				[]string{"some-sas", "some-sas"}, "subtraction", "ij", fence, "linear"),
			request2: newFenceRequest(
				[]string{"some-path", "some-path"},
				[]string{"some-sas", "different-sas"}, "subtraction", "ij", fence, "linear"),
		},
		{
			name: "Binary operator specified",
			request1: newFenceRequest(
				[]string{"some-path"},
				[]string{"some-sas"}, "", "ij", fence, "linear"),
			request2: FenceRequest{
				RequestedResource: RequestedResource{Vds: []string{"some-path"}, Sas: []string{"some-sas"}},
				CoordinateSystem:  "ij",
				Coordinates:       fence,
				Interpolation:     "linear",
			},
		},
	}

	for _, testCase := range testCases {
		hash1, err := testCase.request1.hash()
		require.NoErrorf(t, err,
			"[%s] Failed to compute hash, err: %v", testCase.name, err,
		)

		hash2, err := testCase.request2.hash()
		require.NoErrorf(t, err,
			"[%s] Failed to compute hash, err: %v", testCase.name, err,
		)

		require.Equalf(t, hash1, hash2, "Expected hashes to be equal")
	}
}

func TestSliceGivesUniqueHash(t *testing.T) {
	testCases := []struct {
		name     string
		request1 SliceRequest
		request2 SliceRequest
	}{
		{
			name: "Vds differ",
			request1: newSliceRequest(
				[]string{"vds1"},
				[]string{"sas"}, "", "inline", 10),
			request2: newSliceRequest(
				[]string{"vds2"},
				[]string{"sas"}, "", "inline", 10),
		},
		{
			name: "Direction differ",
			request1: newSliceRequest(
				[]string{"vds"},
				[]string{"sas"}, "", "inline", 10),
			request2: newSliceRequest(
				[]string{"vds"},
				[]string{"sas"}, "", "i", 10),
		},
		{
			name: "Lineno differ",
			request1: newSliceRequest(
				[]string{"vds"},
				[]string{"sas"}, "", "inline", 10),
			request2: newSliceRequest(
				[]string{"vds"},
				[]string{"sas"}, "", "inline", 11),
		},
		{
			name: "Single vds versus double vds",
			request1: newSliceRequest(
				[]string{"vds", "vds"},
				[]string{"sas", "sas"}, "", "inline", 10),
			request2: newSliceRequest(
				[]string{"vds"},
				[]string{"sas"}, "", "inline", 10),
		},
		{
			name: "Binary operator differ",
			request1: newSliceRequest(
				[]string{"vds", "vds"},
				[]string{"sas", "sas"}, "subtraction", "inline", 10),
			request2: newSliceRequest(
				[]string{"vds", "vds"},
				[]string{"sas", "sas"}, "addition", "inline", 10),
		},
		{
			name: "Vds differ 1",
			request1: newSliceRequest(
				[]string{"vds", "vds"},
				[]string{"sas", "sas"}, "subtraction", "inline", 10),
			request2: newSliceRequest(
				[]string{"vds1", "vds"},
				[]string{"sas", "sas"}, "subtraction", "inline", 10),
		},
		{
			name: "Vds differ 2",
			request1: newSliceRequest(
				[]string{"vds", "vds"},
				[]string{"sas", "sas"}, "subtraction", "inline", 10),
			request2: newSliceRequest(
				[]string{"vds", "vds1"},
				[]string{"sas", "sas"}, "subtraction", "inline", 10),
		},
	}

	for _, testCase := range testCases {
		hash1, err := testCase.request1.hash()
		require.NoErrorf(t, err,
			"[%s] Failed to compute hash, err: %v", testCase.name, err,
		)

		hash2, err := testCase.request2.hash()
		require.NoErrorf(t, err,
			"[%s] Failed to compute hash, err: %v", testCase.name, err,
		)

		require.NotEqualf(t, hash1, hash2,
			"[%s] Expected unique hashes",
			testCase.name,
		)
	}
}

func TestFenceGivesUniqueHash(t *testing.T) {
	fence1 := [][]float32{{1, 2}, {3, 4}}
	fence2 := [][]float32{{2, 2}, {3, 4}}

	testCases := []struct {
		name     string
		request1 FenceRequest
		request2 FenceRequest
	}{
		{
			name:     "Vds differ",
			request1: newFenceRequest([]string{"vds1"}, []string{"sas"}, "", "ij", fence1, "linear"),
			request2: newFenceRequest([]string{"vds2"}, []string{"sas"}, "", "ij", fence1, "linear"),
		},
		{
			name:     "Coordinate system differ",
			request1: newFenceRequest([]string{"vds"}, []string{"sas"}, "", "ij", fence1, "linear"),
			request2: newFenceRequest([]string{"vds"}, []string{"sas"}, "", "cdp", fence1, "linear"),
		},
		{
			name:     "Coordinates differ",
			request1: newFenceRequest([]string{"vds"}, []string{"sas"}, "", "ij", fence1, "linear"),
			request2: newFenceRequest([]string{"vds"}, []string{"sas"}, "", "ij", fence2, "linear"),
		},
		{
			name:     "Interpolation differ",
			request1: newFenceRequest([]string{"vds"}, []string{"sas"}, "", "ij", fence1, "linear"),
			request2: newFenceRequest([]string{"vds"}, []string{"sas"}, "", "ij", fence1, "cubic"),
		},
		{
			name: "Single vds versus double vds",
			request1: newFenceRequest(
				[]string{"vds"},
				[]string{"sas"}, "", "ij", fence1, "linear"),
			request2: newFenceRequest(
				[]string{"vds", "vds"},
				[]string{"sas", "sas"}, "", "ij", fence1, "inline"),
		},
		{
			name: "Binary operator differ",
			request1: newFenceRequest(
				[]string{"vds", "vds"},
				[]string{"sas", "sas"}, "subtraction", "ij", fence1, "inline"),
			request2: newFenceRequest(
				[]string{"vds", "vds"},
				[]string{"sas", "sas"}, "addition", "ij", fence1, "inline"),
		},
		{
			name: "Vds differ 1",
			request1: newFenceRequest(
				[]string{"vds", "vds"},
				[]string{"sas", "sas"}, "subtraction", "ij", fence1, "inline"),
			request2: newFenceRequest(
				[]string{"vds1", "vds"},
				[]string{"sas", "sas"}, "subtraction", "ij", fence1, "inline"),
		},
		{
			name: "Vds differ 2",
			request1: newFenceRequest(
				[]string{"vds", "vds"},
				[]string{"sas", "sas"}, "subtraction", "ij", fence1, "inline"),
			request2: newFenceRequest(
				[]string{"vds", "vds2"},
				[]string{"sas", "sas"}, "subtraction", "ij", fence1, "inline"),
		},
	}

	for _, testCase := range testCases {
		hash1, err := testCase.request1.hash()
		require.NoErrorf(t, err,
			"[%s] Failed to compute hash, err: %v", testCase.name, err,
		)

		hash2, err := testCase.request2.hash()
		require.NoErrorf(t, err,
			"[%s] Failed to compute hash, err: %v", testCase.name, err,
		)

		require.NotEqualf(t, hash1, hash2,
			"[%s] Expected unique hashes",
			testCase.name,
		)
	}
}

func TestExtractSasFromUrl(t *testing.T) {

	testCases := []struct {
		request     RequestedResource
		expected    []string
		shouldError bool
	}{
		{
			request: newRequestedResource(
				[]string{"../../testdata/well_known/well_known_default.vds"},
				[]string{"sastoken1"},
			),
			expected:    []string{"sastoken1"},
			shouldError: false,
		},
		{
			request: newRequestedResource(
				[]string{"../../testdata/well_known/well_known_default.vds?sastoken2"},
				[]string{""},
			),
			expected:    []string{"sastoken2"},
			shouldError: false,
		},
		{
			request: newRequestedResource(
				[]string{"../../testdata/well_known/well_known_default.vds?sastoken2"},
				[]string{"sastoken1"},
			),
			expected:    []string{"Signed urls are not accepted when providing sas-tokens. Vds url nr 1 is signed"},
			shouldError: true,
		},
		{
			request: newRequestedResource(
				[]string{"../../testdata/well_known/well_known_default.vds"},
				[]string{""},
			),
			expected:    []string{"No valid Sas token found at the end of vds url nr 1"},
			shouldError: true,
		},
		{
			request: newRequestedResource(
				[]string{
					"../../testdata/well_known/well_known_default.vds",
					"../../testdata/well_known/well_known_default.vds"},
				[]string{"sastoken1", "sastoken2"},
			),
			expected:    []string{"sastoken1", "sastoken2"},
			shouldError: false,
		},
		{
			request: newRequestedResource(
				[]string{
					"../../testdata/well_known/well_known_default.vds?sastoken1",
					"../../testdata/well_known/well_known_default.vds?sastoken2"},
				[]string{"", ""},
			),
			expected:    []string{"Signed urls are not accepted when providing sas-tokens. Vds url nr 1 is signed"},
			shouldError: true,
		},
		{
			request: newRequestedResource(
				[]string{
					"../../testdata/well_known/well_known_default.vds?sastoken1",
					"../../testdata/well_known/well_known_default.vds?sastoken2"},
				[]string{""},
			),
			expected:    []string{"sastoken1", "sastoken2"},
			shouldError: false,
		},

		{
			request: newRequestedResource(
				[]string{
					"../../testdata/well_known/well_known_default.vds?sastoken1",
					"../../testdata/well_known/well_known_default.vds"},
				[]string{""},
			),
			expected:    []string{"No valid Sas token found at the end of vds url nr 2"},
			shouldError: true,
		},
	}

	for _, testCase := range testCases {
		err := testCase.request.NormalizeConnection()
		if testCase.shouldError {
			require.ErrorContains(t, err, testCase.expected[0])
		} else {
			require.NoError(t, err)
			for i := 0; i < len(testCase.expected); i++ {
				require.Equal(t, testCase.expected[i], testCase.request.Sas[i])
			}
		}
	}
}

func TestPortPresenceInURL(t *testing.T) {

	testCases := []struct {
		request  RequestedResource
		expected []string
	}{
		{
			request: newRequestedResource(
				[]string{"https://account.blob.core.windows.net:443/container/blob"},
				[]string{"sastoken1"},
			),
			expected: []string{"https://account.blob.core.windows.net/container/blob"},
		},
		{
			request: newRequestedResource(
				[]string{"https://account.blob.core.windows.net:443/container/blob?sastoken1"},
				[]string{""},
			),
			expected: []string{"https://account.blob.core.windows.net/container/blob"},
		},
		{
			request: newRequestedResource(
				[]string{
					"https://account.blob.core.windows.net:443/container/blob",
					"https://account.blob.core.windows.net:443/container/blob"},
				[]string{"sastoken1", "sastoken2"},
			),
			expected: []string{
				"https://account.blob.core.windows.net/container/blob",
				"https://account.blob.core.windows.net/container/blob"},
		},
		{
			request: newRequestedResource(
				[]string{
					"https://account.blob.core.windows.net:443/container/blob?sastoken1",
					"https://account.blob.core.windows.net:443/container/blob?sastoken2"},
				[]string{""},
			),
			expected: []string{
				"https://account.blob.core.windows.net/container/blob",
				"https://account.blob.core.windows.net/container/blob"},
		},
		{
			request: newRequestedResource(
				[]string{
					"https://account.blob.core.windows.net/container/blob?sastoken1",
					"https://account.blob.core.windows.net:443/container/blob?sastoken2"},
				[]string{""},
			),
			expected: []string{
				"https://account.blob.core.windows.net/container/blob",
				"https://account.blob.core.windows.net/container/blob"},
		},
	}

	for _, testCase := range testCases {
		err := testCase.request.NormalizeConnection()
		require.NoError(t, err)
		for i := 0; i < len(testCase.expected); i++ {
			require.Equal(t, testCase.expected[i], testCase.request.Vds[i])
		}
	}
}
