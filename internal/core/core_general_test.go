package core

import (
	"encoding/binary"
	"errors"
	"fmt"
	"math"
	"testing"

	"github.com/stretchr/testify/require"
)

func make_connection(name string) Connection {
	path := fmt.Sprintf("../../testdata/%s", name)
	path = fmt.Sprintf("file://%s", path)
	return NewFileConnection(path)
}

var well_known_custom_axis_order = make_connection("well_known/well_known_custom_axis_order.vds")
var invalid_axes_names = make_connection("invalid_data/invalid_axes_names.vds")
var well_known = make_connection("well_known/well_known_default.vds")
var samples10 = make_connection("10_samples/10_samples_default.vds")
var prestack = make_connection("prestack/prestack_default.vds")

var fillValue = float32(-999.25)

type gridDefinition struct {
	xori     float32
	yori     float32
	xinc     float32
	yinc     float32
	rotation float32
}

/* The grid definition for the well_known vds test file
 *
 * When constructing horizons for testing it's useful to reuse the same grid
 * definition in the horizon as in the vds itself.
 */
var well_known_grid gridDefinition = gridDefinition{
	xori:     float32(2.0),
	yori:     float32(0.0),
	xinc:     float32(7.2111),
	yinc:     float32(3.6056),
	rotation: float32(33.69),
}

var samples10_grid = well_known_grid

func samples10Surface(data [][]float32) RegularSurface {
	return RegularSurface{
		Values:    data,
		Rotation:  &samples10_grid.rotation,
		Xori:      &samples10_grid.xori,
		Yori:      &samples10_grid.yori,
		Xinc:      samples10_grid.xinc,
		Yinc:      samples10_grid.yinc,
		FillValue: &fillValue,
	}
}

func toFloat32(buf []byte) (*[]float32, error) {
	fsize := 4 // sizeof(float32)
	if len(buf)%fsize != 0 {
		return nil, errors.New("Invalid buffersize")
	}

	outlen := len(buf) / 4

	out := make([]float32, outlen)
	for i := 0; i < outlen; i++ {
		offset := i * fsize
		tmp := binary.LittleEndian.Uint32(buf[offset : offset+fsize])
		out[i] = math.Float32frombits(tmp)
	}

	return &out, nil
}

func TestOnly3DSupported(t *testing.T) {
	handle, err := NewDSHandle(prestack)
	if err != nil {
		handle.Close()
	}

	require.ErrorContains(t, err, "3 dimensions, got 4")
}
