package handlers

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
