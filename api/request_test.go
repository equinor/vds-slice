package api

import (
	"testing"
)

func newSliceRequest(
	vds       string,
	sas       string,
	direction string,
	lineno    int,
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
	vds              string,
	sas              string,
	coordinateSystem string,
	coordinates      [][]float32,
	interpolation    string,
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

func TestSasIsOmmitedFromSliceHash(t *testing.T) {
	request1 := newSliceRequest("some-path", "some-sas", "inline", 9961)
	request2 := newSliceRequest("some-path", "different-sas", "inline", 9961)

	hash1, err := request1.Hash()
	if err != nil {
		t.Fatalf("Failed to compute hash, err: %v", err)
	}

	hash2, err := request2.Hash()
	if err != nil {
		t.Fatalf("Failed to compute hash, err: %v", err)
	}

	if hash1 != hash2 {
		t.Fatalf("Expected hashes to be equal, was %v and %v", hash1, hash2)
	}
}

func TestSasIsOmmitedFromFenceHash(t *testing.T) {
	request1 := newFenceRequest(
		"some-path",
		"some-sas",
		"ij",
		[][]float32{{1,2}, {2,3}},
		"linear",
	)
	request2 := newFenceRequest(
		"some-path",
		"different-sas",
		"ij",
		[][]float32{{1,2}, {2,3}},
		"linear",
	)

	hash1, err := request1.Hash()
	if err != nil {
		t.Fatalf("Failed to compute hash, err: %v", err)
	}
	
	hash2, err := request2.Hash()
	if err != nil {
		t.Fatalf("Failed to compute hash, err: %v", err)
	}

	if hash1 != hash2 {
		t.Fatalf("Expected hashes to be equal, was %v and %v", hash1, hash2)
	}
}

func TestSliceGivesUniqueHash(t *testing.T) {
	testCases := []struct{
		name     string
		request1 SliceRequest
		request2 SliceRequest
	}{
		{
			name: "Vds differ",
			request1: newSliceRequest("vds1", "sas", "inline", 10),
			request2: newSliceRequest("vds2", "sas", "inline", 10),
		},
		{
			name: "direction differ",
			request1: newSliceRequest("vds", "sas", "inline", 10),
			request2: newSliceRequest("vds", "sas", "i",      10),
		},
		{
			name: "lineno differ",
			request1: newSliceRequest("vds", "sas", "inline", 10),
			request2: newSliceRequest("vds", "sas", "inline", 11),
		},
	}

	for _, testCase := range testCases {
		hash1, err := testCase.request1.Hash()
		if err != nil {
			t.Fatalf("[%s] Failed to compute hash, err: %v", testCase.name, err)
		}
		
		hash2, err := testCase.request2.Hash()
		if err != nil {
			t.Fatalf("[%s] Failed to compute hash, err: %v", testCase.name, err)
		}

		if hash1 == hash2 {
			t.Fatalf(
				"[%s] Expected unique hashes, got: %v == %v",
				testCase.name,
				hash1,
				hash2,
			)
		}
	}
}

func TestFenceGivesUniqueHash(t *testing.T) {
	fence1 := [][]float32{{ 1, 2 }, {3, 4}}
	fence2 := [][]float32{{ 2, 2 }, {3, 4}}

	testCases := []struct{
		name     string
		request1 FenceRequest
		request2 FenceRequest
	}{
		{
			name: "Vds differ",
			request1: newFenceRequest("vds1", "sas", "ij", fence1, "linear"),
			request2: newFenceRequest("vds2", "sas", "ij", fence1, "linear"),
		},
		{
			name: "Coordinate system differ",
			request1: newFenceRequest("vds", "sas", "ij",  fence1, "linear"),
			request2: newFenceRequest("vds", "sas", "cdp", fence1, "linear"),
		},
		{
			name: "Coordinates differ",
			request1: newFenceRequest("vds", "sas", "ij", fence1, "linear"),
			request2: newFenceRequest("vds", "sas", "ij", fence2, "linear"),
		},
		{
			name: "Interpolation differ",
			request1: newFenceRequest("vds", "sas", "ij", fence1, "linear"),
			request2: newFenceRequest("vds", "sas", "ij", fence1, "cubic"),
		},
	}

	for _, testCase := range testCases {
		hash1, err := testCase.request1.Hash()
		if err != nil {
			t.Fatalf("[%s] Failed to compute hash, err: %v", testCase.name, err)
		}
		
		hash2, err := testCase.request2.Hash()
		if err != nil {
			t.Fatalf("[%s] Failed to compute hash, err: %v", testCase.name, err)
		}

		if hash1 == hash2 {
			t.Fatalf(
				"[%s] Expected unique hashes, got: %v == %v",
				testCase.name,
				hash1,
				hash2,
			)
		}
	}
}
