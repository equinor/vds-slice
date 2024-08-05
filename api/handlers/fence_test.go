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
