package vds

/*
#cgo LDFLAGS: -lopenvds
#cgo CXXFLAGS: -std=c++17
#include <vds.h>
#include <stdlib.h>
*/
import "C"
import (
	"errors"
	"fmt"
	"math"
	"strings"
	"unsafe"
)

const (
	AxisI         = C.I
	AxisJ         = C.J
	AxisK         = C.K
	AxisInline    = C.INLINE
	AxisCrossline = C.CROSSLINE
	AxisDepth     = C.DEPTH
	AxisTime      = C.TIME
	AxisSample    = C.SAMPLE
)

const (
	CoordinateSystemIndex      = C.INDEX
	CoordinateSystemAnnotation = C.ANNOTATION
	CoordinateSystemCdp        = C.CDP
)

// @Description Axis description
type Axis struct {
	// Name/Annotation of axis
	Annotation string `json:"annotation" example:"Sample"`

	// Minimum axis value
	Min float64 `json:"min" example:"4.0"`

	// Maximum axis value
	Max float64 `json:"max" example:"4500.0"`

	// Number of samples along the axis
	Samples int `json:"samples" example:"1600"`

	// Axis units
	Unit string `json:"unit" example:"ms"`
} // @name Axis


// @Description The bounding box of the survey, defined by its 4 corner
// @Description coordinates. The bounding box is given in 3 different
// @Description coordinate systems. The points are sorted in the same order for
// @Description each system. E.g. [[1,2], [2,2], [2,3], [1,3]]
type BoundingBox struct {
	Cdp  [][]float64 `json:"cdp"`
	Ilxl [][]float64 `json:"ilxl"`
	Ij   [][]float64 `json:"ij"`
} //@name BoundingBox

// @Description Slice metadata
type SliceMetadata struct {
	// Data format is represented by a numpy-style formatcodes. E.g. f4 is 4
	// byte float, u1 is 1 byte unsinged int and u2 is 2 byte usigned int.
	// Currently format is always 4-byte floats, little endian (<f4).
	Format string `json:"format" example:"<f4"`

	// X-axis information
	X Axis `json:"x"`

	// Y-axis information
	Y Axis `json:"y"`

	// Horizontal bounding box of the slice. For inline/crossline slices this
	// is a linestring, while for time/depth slices this is essentially the
	// bounding box of the volume.
	Geospatial [][]float64 `json:"geospatial"`
} // @name SliceMetadata

// @Description Metadata
type Metadata struct {
	// Coordinate reference system
	Crs string `json:"crs" example:"PROJCS[\"ED50 / UTM zone 31N\",..."`

	// The original input file name
	InputFileName string `json:"inputFileName" example:"file.segy"`

	// Bounding box
	BoundingBox BoundingBox `json:"boundingBox"`

	// Axis descriptions
	//
	// Describes the axes of the requested 2-dimensional slice.
	Axis []*Axis `json:"axis"`
} // @name Metadata

// @Description Fence metadata
type FenceMetadata struct {
	// Data format is represented by numpy-style formatcodes. For fence the
	// format is always 4-byte floats, little endian (<f4).
	Format string `json:"format" example:"<f4"`

	// Shape of the returned data fence.
	Shape []int `json:"shape" swaggertype:"array,integer" example:"10,50"`
} // @name FenceMetadata

func GetAxis(direction string) (int, error) {
	switch direction {
	case "i":         return AxisI,         nil
	case "j":         return AxisJ,         nil
	case "k":         return AxisK,         nil
	case "inline":    return AxisInline,    nil
	case "crossline": return AxisCrossline, nil
	case "depth":     return AxisDepth,     nil
	case "time":      return AxisTime,      nil
	case "sample":    return AxisSample,    nil
	default:
		options := "i, j, k, inline, crossline or depth/time/sample"
		msg := "invalid direction '%s', valid options are: %s"
		return -1, NewInvalidArgument(fmt.Sprintf(msg, direction, options))
	}
}

func GetCoordinateSystem(coordinateSystem string) (int, error) {
	switch strings.ToLower(coordinateSystem) {
	case "ij":
		return CoordinateSystemIndex, nil
	case "ilxl":
		return CoordinateSystemAnnotation, nil
	case "cdp":
		return CoordinateSystemCdp, nil
	default:
		options := "ij, ilxl, cdp"
		msg := "coordinate system not recognized: '%s', valid options are: %s"
		return -1, NewInvalidArgument(fmt.Sprintf(msg, coordinateSystem, options))
	}
}

func GetInterpolationMethod(interpolation string) (int, error) {
	switch strings.ToLower(interpolation) {
	case "":
		fallthrough
	case "nearest":
		return C.NEAREST, nil
	case "linear":
		return C.LINEAR, nil
	case "cubic":
		return C.CUBIC, nil
	case "angular":
		return C.ANGULAR, nil
	case "triangular":
		return C.TRIANGULAR, nil
	default:
		options := "nearest, linear, cubic, angular or triangular"
		msg := "invalid interpolation method '%s', valid options are: %s"
		return -1, NewInvalidArgument(fmt.Sprintf(msg, interpolation, options))
	}
}

func GetAttributeType(attribute string) (int, error) {
	switch strings.ToLower(attribute) {
	case "samplevalue": return C.VALUE,   nil
	case "min":         return C.MIN,     nil
	case "max":         return C.MAX,     nil
	case "maxabs":      return C.MAXABS,  nil
	case "mean":        return C.MEAN,    nil
	case "meanabs":     return C.MEANABS, nil
	case "meanpos":     return C.MEANPOS, nil
	case "meanneg":     return C.MEANNEG, nil
	case "median":      return C.MEDIAN,  nil
	case "rms":         return C.RMS,     nil
	case "var":         return C.VAR,     nil
	case "sd":          return C.SD,      nil
	case "sumpos":      return C.SUMPOS,  nil
	case "sumneg":      return C.SUMNEG,  nil
	case "":            fallthrough
	default:
		options := []string{
			"samplevalue", "min", "max", "maxabs", "mean", "meanabs", "meanpos",
			"meanneg", "median", "rms", "var", "sd", "sumpos", "sumneg",
		}
		msg := "invalid attribute '%s', valid options are: %s"
		return -1, NewInvalidArgument(fmt.Sprintf(
			msg,
			attribute,
			strings.Join(options, ", "),
		))
	}
}

/** Translate C status codes into Go error types */
func toError(status C.int, ctx *C.Context) error {
	if status == C.STATUS_OK {
		return nil
	}

	msg := C.GoString(C.errmsg(ctx))

	switch status {
	case C.STATUS_NULLPTR_ERROR: fallthrough
	case C.STATUS_RUNTIME_ERROR: return NewInternalError(msg)
	case C.STATUS_BAD_REQUEST:   return NewInvalidArgument(msg)
	default:
		return errors.New(msg)
	}
}

type RegularSurface struct {
	cSurface *C.struct_RegularSurface
}

func (r *RegularSurface) get() *C.struct_RegularSurface {
	return r.cSurface
}

func (r *RegularSurface) Close() error {
	var cCtx = C.context_new()
	defer C.context_free(cCtx)

	cErr :=	C.regular_surface_free(cCtx, r.cSurface)
	if err := toError(cErr, cCtx); err != nil {
		return err
	}

	return nil
}

func NewRegularSurface(
	data          [][]float32,
	originX       float32,
	originY       float32,
	increaseX     float32,
	increaseY     float32,
	rotation      float32,
	fillValue     float32,
) (RegularSurface, error) {
	var cCtx = C.context_new()
	defer C.context_free(cCtx)

	nrows := len(data)
	ncols := len(data[0])

	cdata := make([]C.float, nrows * ncols)
	for i := range data {
		if len(data[i]) != ncols  {
			msg := fmt.Sprintf(
				"Surface rows are not of the same length. "+
					"Row 0 has %d elements. Row %d has %d elements",
				ncols, i, len(data[i]),
			)
			return RegularSurface{}, NewInvalidArgument(msg)
		}

		for j := range data[i] {
			cdata[i * ncols  + j] = C.float(data[i][j])
		}
	}

	var cSurface *C.struct_RegularSurface
	cErr := C.regular_surface_new(
		cCtx,
		&cdata[0],
		C.size_t(nrows),
		C.size_t(ncols),
		C.float(originX),
		C.float(originY),
		C.float(increaseX),
		C.float(increaseY),
		C.float(rotation),
		C.float(fillValue),
		&cSurface,
	)

	if err := toError(cErr, cCtx); err != nil {
		C.regular_surface_free(cCtx, cSurface)
		return RegularSurface{}, err
	}

	return RegularSurface{ cSurface: cSurface }, nil
}

type VDSHandle struct {
	handle *C.struct_DataHandle
	ctx    *C.struct_Context
}

func (v VDSHandle) Handle() *C.struct_DataHandle {
	return v.handle
}

func (v VDSHandle) context() *C.struct_Context{
	return v.ctx
}

func (v VDSHandle) Error(status C.int) error {
	return toError(status, v.context())
}

func (v VDSHandle) Close() error {
	defer C.context_free(v.ctx)

	cerr := C.datahandle_free(v.ctx, v.handle)
	return toError(cerr, v.ctx)
}

func NewVDSHandle(conn Connection) (VDSHandle, error) {
	curl := C.CString(conn.Url())
	defer C.free(unsafe.Pointer(curl))

	ccred := C.CString(conn.ConnectionString())
	defer C.free(unsafe.Pointer(ccred))

	var cctx = C.context_new()
	var handle *C.struct_DataHandle

	cerr := C.datahandle_new(cctx, curl, ccred, &handle);

	if err := toError(cerr, cctx); err != nil {
		defer C.context_free(cctx)
		return VDSHandle{}, err
	}

	return VDSHandle{ handle: handle, ctx: cctx }, nil
}

func (v VDSHandle) GetMetadata() ([]byte, error) {
	var result C.struct_response
	cerr := C.metadata(v.context(), v.Handle(), &result)

	defer C.response_delete(&result)

	if err := v.Error(cerr); err != nil {
		return nil, err
	}

	buf := C.GoBytes(unsafe.Pointer(result.data), C.int(result.size))
	return buf, nil
}

func (v VDSHandle) GetSlice(lineno, direction int) ([]byte, error) {
	var result C.struct_response
	cerr := C.slice(
		v.context(),
		v.Handle(),
		C.int(lineno),
		C.enum_axis_name(direction),
		&result,
	)

	defer C.response_delete(&result)
	if err := v.Error(cerr); err != nil {
		return nil, err
	}

	buf := C.GoBytes(unsafe.Pointer(result.data), C.int(result.size))
	return buf, nil
}

func (v VDSHandle) GetSliceMetadata(lineno, direction int) ([]byte, error) {
	var result C.struct_response
	cerr := C.slice_metadata(
		v.context(),
		v.Handle(),
		C.int(lineno),
		C.enum_axis_name(direction),
		&result,
	)

	defer C.response_delete(&result)

	if err := v.Error(cerr); err != nil {
		return nil, err
	}

	buf := C.GoBytes(unsafe.Pointer(result.data), C.int(result.size))
	return buf, nil
}

func (v VDSHandle) GetFence(
	coordinateSystem int,
	coordinates [][]float32,
	interpolation int,
) ([]byte, error) {
	coordinate_len := 2
	ccoordinates := make([]C.float, len(coordinates) * coordinate_len)
	for i := range coordinates {

		if len(coordinates[i]) != coordinate_len  {
			msg := fmt.Sprintf(
				"invalid coordinate %v at position %d, expected [x y] pair",
				coordinates[i],
				i,
			)
			return nil, NewInvalidArgument(msg)
		}

		for j := range coordinates[i] {
			ccoordinates[i * coordinate_len  + j] = C.float(coordinates[i][j])
		}
	}

	var result C.struct_response
	cerr := C.fence(
		v.context(),
		v.Handle(),
		C.enum_coordinate_system(coordinateSystem),
		&ccoordinates[0],
		C.size_t(len(coordinates)),
		C.enum_interpolation_method(interpolation),
		&result,
	)

	defer C.response_delete(&result)

	if err := v.Error(cerr); err != nil {
		return nil, err
	}

	buf := C.GoBytes(unsafe.Pointer(result.data), C.int(result.size))
	return buf, nil
}

func (v VDSHandle) GetFenceMetadata(coordinates [][]float32) ([]byte, error) {
	var result C.struct_response
	cerr := C.fence_metadata(
		v.context(),
		v.Handle(),
		C.size_t(len(coordinates)),
		&result,
	)

	defer C.response_delete(&result)

	if err := v.Error(cerr); err != nil {
		return nil, err
	}

	buf := C.GoBytes(unsafe.Pointer(result.data), C.int(result.size))
	return buf, nil
}

func min(a, b int) int {
	if a < b {
		return a
	}
	return b
}


func (v VDSHandle) GetAttributeMetadata(data [][]float32) ([]byte, error) {
	var result C.struct_response
	cerr := C.attribute_metadata(
		v.context(),
		v.Handle(),
		C.size_t(len(data)),
		C.size_t(len(data[0])),
		&result,
	)

	defer C.response_delete(&result)

	if err := v.Error(cerr); err != nil {
		return nil, err
	}

	buf := C.GoBytes(unsafe.Pointer(result.data), C.int(result.size))
	return buf, nil
}

func (v VDSHandle) GetAttributes(
	data          [][]float32,
	originX       float32,
	originY       float32,
	increaseX     float32,
	increaseY     float32,
	rotation      float32,
	fillValue     float32,
	above         float32,
	below         float32,
	stepsize      float32,
	attributes    []string,
	interpolation int,
) ([][]byte, error) {
	var targetAttributes []int
	for _, attr := range attributes {
		id, err := GetAttributeType(attr)
		if err != nil {
			return nil, err
		}
		targetAttributes = append(targetAttributes, id);
	}

	var nrows   = len(data)
	var ncols   = len(data[0])
	var hsize   = nrows * ncols
	var mapsize = hsize * 4

	cattributes := make([]C.enum_attribute, len(targetAttributes))
	for i := range targetAttributes {
		cattributes[i] = C.enum_attribute(targetAttributes[i])
	}

	surface, err := NewRegularSurface(
		data,
		originX,
		originY,
		increaseX,
		increaseY,
		rotation,
		fillValue,
	)
	if err != nil {
		return nil, err
	}
	defer surface.Close()

	var horizon C.struct_response
	cerr := C.horizon(
		v.context(),
		v.Handle(),
		surface.get(),
		C.float(above),
		C.float(below),
		C.enum_interpolation_method(interpolation),
		&horizon,
	)

	if err := v.Error(cerr); err != nil {
		return nil, err
	}

	defer C.response_delete(&horizon)

	buffer := make([]byte, mapsize * len(targetAttributes))

	maxConcurrency := 32
	windowsPerRoutine := int(math.Ceil(float64(hsize) / float64(maxConcurrency)))
	
	errs := make(chan error, maxConcurrency)	

	from := 0
	remaining := hsize;
	nRoutines := 0;
	for remaining > 0 {
		nRoutines++

		size := min(windowsPerRoutine, remaining)
		to := from + size

		go func(from, to int) {
			var cCtx = C.context_new()
			defer C.context_free(cCtx)

			cErr := C.attribute(
				cCtx,
				v.Handle(),
				surface.get(),
				horizon.data,
				horizon.size,
				&cattributes[0],
				C.size_t(len(targetAttributes)),
				C.float(above),
				C.float(below),
				C.float(stepsize),
				C.size_t(from),
				C.size_t(to),
				unsafe.Pointer(&buffer[0]),
			)

			errs <- toError(cErr, cCtx)
		}(from, to)

		from = to
		remaining -= size
	}

	for i := 0; i < nRoutines; i++ {
		err := <- errs
		if err != nil {
			return nil, err
		}
	}

	out := make([][]byte, len(targetAttributes))
	for i := range targetAttributes {
		out[i] = buffer[i * mapsize: (i + 1) * mapsize]
	}

	return out, nil
}
