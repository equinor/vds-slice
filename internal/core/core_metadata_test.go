package core

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/require"
)

func TestMetadata(t *testing.T) {
	expected := Metadata{
		Axis: []*Axis{
			{Annotation: "Inline", Min: 1, Max: 5, Samples: 3, StepSize: 2, Unit: "unitless"},
			{Annotation: "Crossline", Min: 10, Max: 11, Samples: 2, StepSize: 1, Unit: "unitless"},
			{Annotation: "Sample", Min: 4, Max: 16, Samples: 4, StepSize: 4, Unit: "ms"},
		},
		BoundingBox: BoundingBox{
			Cdp:  [][]float64{{2, 0}, {14, 8}, {12, 11}, {0, 3}},
			Ilxl: [][]float64{{1, 10}, {5, 10}, {5, 11}, {1, 11}},
			Ij:   [][]float64{{0, 0}, {2, 0}, {2, 1}, {0, 1}},
		},
		Crs:             "utmXX",
		InputFileName:   "well_known.segy",
		ImportTimeStamp: `^\d{4}-\d{2}-\d{2}[A-Z]\d{2}:\d{2}:\d{2}\.\d{3}[A-Z]$`,
	}

	handle, _ := NewDSHandle(well_known)
	defer handle.Close()
	buf, err := handle.GetMetadata()
	require.NoErrorf(t, err, "Failed to retrieve metadata, err %v", err)

	var meta Metadata
	err = json.Unmarshal(buf, &meta)
	require.NoErrorf(t, err, "Failed to unmarshall response, err: %v", err)

	require.Regexp(t, expected.ImportTimeStamp, meta.ImportTimeStamp)

	expected.ImportTimeStamp = "dummy"
	meta.ImportTimeStamp = "dummy"

	require.Equal(t, meta, expected)
}

func TestMetadataCustomAxisOrder(t *testing.T) {
	expected := []*Axis{
		{Annotation: "Inline", Min: 1, Max: 5, Samples: 3, StepSize: 2, Unit: "unitless"},
		{Annotation: "Crossline", Min: 10, Max: 11, Samples: 2, StepSize: 1, Unit: "unitless"},
		{Annotation: "Sample", Min: 4, Max: 16, Samples: 4, StepSize: 4, Unit: "ms"},
	}
	handle, err := NewDSHandle(well_known_custom_axis_order)
	require.NoErrorf(t, err, "Failed to open vds file")

	defer handle.Close()
	buf, err := handle.GetMetadata()
	require.NoErrorf(t, err, "Failed to retrieve metadata")

	var meta Metadata
	err = json.Unmarshal(buf, &meta)
	require.NoErrorf(t, err, "Failed to unmarshall response")
	require.Equal(t, meta.Axis, expected)
}

func TestMetadataAxesNames(t *testing.T) {
	expected := "Requested axis not found under names "
	_, err := NewDSHandle(invalid_axes_names)
	require.ErrorContains(t, err, expected)
}
