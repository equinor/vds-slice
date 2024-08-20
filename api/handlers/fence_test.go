package handlers

import (
	"testing"

	"github.com/stretchr/testify/require"
)

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
		{
			name: "Vds order differs",
			request1: newFenceRequest(
				[]string{"vds1", "vds2"},
				[]string{"sas", "sas"}, "subtraction", "ij", fence1, "inline"),
			request2: newFenceRequest(
				[]string{"vds2", "vds1"},
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

func TestFenceFillValueGivesUniqueHash(t *testing.T) {
	request := FenceRequest{
		RequestedResource: RequestedResource{
			Vds:            []string{"vds"},
			Sas:            []string{"sas"},
			BinaryOperator: "",
		},
		CoordinateSystem: "cdp",
		Coordinates:      [][]float32{{0, 0}, {1, 1}},
	}

	fillvalue0 := float32(0.0)
	fillvalue100 := float32(100.0)

	request1 := request

	request2 := request
	request2.FillValue = &fillvalue0

	request3 := request
	request3.FillValue = &fillvalue100

	requests := []FenceRequest{request1, request2, request3}
	hashes := make(map[string]bool)

	for _, req := range requests {
		strReq, _ := req.toString()
		hash, err := req.hash()
		require.NoErrorf(t, err,
			"Failed to compute hash for request %v, err: %v", strReq, err,
		)

		exists := hashes[hash]
		require.Falsef(t, exists,
			"Expected unique hashes but collision for request %v", strReq,
		)

		hashes[hash] = true
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
