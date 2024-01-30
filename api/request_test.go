package api

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func newSliceRequest(
	vds string,
	sas string,
	direction string,
	lineno int,
) SliceRequest {
	return SliceRequest{
		RequestedResource: RequestedResource{
			Vds: vds,
			Sas: sas,
		},
		Direction: direction,
		Lineno:    &lineno,
	}
}

func newFenceRequest(
	vds string,
	sas string,
	coordinateSystem string,
	coordinates [][]float32,
	interpolation string,
) FenceRequest {
	return FenceRequest{
		RequestedResource: RequestedResource{
			Vds: vds,
			Sas: sas,
		},
		CoordinateSystem: coordinateSystem,
		Coordinates:      coordinates,
		Interpolation:    interpolation,
	}
}

func newRequestedResource(
	vds string,
	sas string,
) RequestedResource {
	return RequestedResource{
		Vds: vds,
		Sas: sas,
	}
}

func TestSasIsOmmitedFromSliceHash(t *testing.T) {
	request1 := newSliceRequest("some-path", "some-sas", "inline", 9961)
	request2 := newSliceRequest("some-path", "different-sas", "inline", 9961)

	hash1, err := request1.hash()
	require.NoErrorf(t, err,
		"Failed to compute hash, err: %v", err,
	)

	hash2, err := request2.hash()
	require.NoErrorf(t, err,
		"Failed to compute hash, err: %v", err,
	)

	require.Equalf(t, hash1, hash2, "Expected hashes to be equal")
}

func TestSasIsOmmitedFromFenceHash(t *testing.T) {
	request1 := newFenceRequest(
		"some-path",
		"some-sas",
		"ij",
		[][]float32{{1, 2}, {2, 3}},
		"linear",
	)
	request2 := newFenceRequest(
		"some-path",
		"different-sas",
		"ij",
		[][]float32{{1, 2}, {2, 3}},
		"linear",
	)

	hash1, err := request1.hash()
	require.NoErrorf(t, err,
		"Failed to compute hash, err: %v", err,
	)

	hash2, err := request2.hash()
	require.NoErrorf(t, err,
		"Failed to compute hash, err: %v", err,
	)

	require.Equalf(t, hash1, hash2, "Expected hashes to be equal")
}

func TestSliceGivesUniqueHash(t *testing.T) {
	testCases := []struct {
		name     string
		request1 SliceRequest
		request2 SliceRequest
	}{
		{
			name:     "Vds differ",
			request1: newSliceRequest("vds1", "sas", "inline", 10),
			request2: newSliceRequest("vds2", "sas", "inline", 10),
		},
		{
			name:     "direction differ",
			request1: newSliceRequest("vds", "sas", "inline", 10),
			request2: newSliceRequest("vds", "sas", "i", 10),
		},
		{
			name:     "lineno differ",
			request1: newSliceRequest("vds", "sas", "inline", 10),
			request2: newSliceRequest("vds", "sas", "inline", 11),
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
			request1: newFenceRequest("vds1", "sas", "ij", fence1, "linear"),
			request2: newFenceRequest("vds2", "sas", "ij", fence1, "linear"),
		},
		{
			name:     "Coordinate system differ",
			request1: newFenceRequest("vds", "sas", "ij", fence1, "linear"),
			request2: newFenceRequest("vds", "sas", "cdp", fence1, "linear"),
		},
		{
			name:     "Coordinates differ",
			request1: newFenceRequest("vds", "sas", "ij", fence1, "linear"),
			request2: newFenceRequest("vds", "sas", "ij", fence2, "linear"),
		},
		{
			name:     "Interpolation differ",
			request1: newFenceRequest("vds", "sas", "ij", fence1, "linear"),
			request2: newFenceRequest("vds", "sas", "ij", fence1, "cubic"),
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
		expected    string
		shouldError bool
	}{
		{
			request: newRequestedResource(
				"../../testdata/well_known/well_known_default.vds",
				"sastoken1",
			),
			expected:    "sastoken1",
			shouldError: false,
		},
		{
			request: newRequestedResource(
				"../../testdata/well_known/well_known_default.vds?sastoken2",
				"",
			),
			expected:    "sastoken2",
			shouldError: false,
		},
		{
			request: newRequestedResource(
				"../../testdata/well_known/well_known_default.vds?sastoken2",
				"sastoken1",
			),
			expected:    "Two sas tokens provided, only one sas token is allowed",
			shouldError: true,
		},
		{
			request: newRequestedResource(
				"../../testdata/well_known/well_known_default.vds",
				"",
			),
			expected:    "No valid Sas token is found in the request",
			shouldError: true,
		},
	}

	for _, testCase := range testCases {
		err := testCase.request.NormalizeConnection()
		if testCase.shouldError {
			require.ErrorContains(t, err, testCase.expected)
		} else {
			require.NoError(t, err)
			require.Equal(t, testCase.expected, testCase.request.Sas)
		}
	}
}

func TestPortPresenceInURL(t *testing.T) {

	testCases := []struct {
		request  RequestedResource
		expected string
	}{
		{
			request: newRequestedResource(
				"https://account.blob.core.windows.net:443/container/blob",
				"sastoken1",
			),
			expected: "https://account.blob.core.windows.net/container/blob",
		},
		{
			request: newRequestedResource(
				"https://account.blob.core.windows.net:443/container/blob?sastoken1",
				"",
			),
			expected: "https://account.blob.core.windows.net/container/blob",
		},
	}

	for _, testCase := range testCases {
		err := testCase.request.NormalizeConnection()
		require.NoError(t, err)
		require.Equal(t, testCase.expected, testCase.request.Vds)
	}
}
