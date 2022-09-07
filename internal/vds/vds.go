package vds

/*
#cgo LDFLAGS: -lopenvds
#cgo CXXFLAGS: -std=c++11
#include <vds.h>
#include <stdlib.h>
*/
import "C"
import (
	"errors"
	"fmt"
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

// @Description Axis description
type Axis struct {
	// Name/Annotation of axis
	Annotation string `json:"annotation"`

	// Minimum axis value
	Min float64 `json:"min"`

	// Maximum axis value
	Max float64 `json:"max"`

	// Number of samples along the axis
	Samples float64 `json:"samples"`

	// Axis units
	Unit string `json:"unit"`
} // @name Axis


// @Description Slice metadata
type Metadata struct {
	// Data format. See https://osdu.pages.opengroup.org/platform/domain-data-mgmt-services/seismic/open-vds/cppdoc/enum/OpenVDS_VolumeDataFormat.html
	Format int `json:"format"`

	// Axis descriptions
	//
	// Describes the axes of the requested 2-dimensional slice.
	Axis []*Axis `json:"axis"`
} // @name Metadata

// @Description Fence metadata
type FenceMetadata struct {
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
			msg := "Invalid direction '%s', valid options are: %s"
			return 0, errors.New(fmt.Sprintf(msg, direction, options))
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
		msg := "Invalid interpolation method '%s', valid options are: %s"
		return -1, errors.New(fmt.Sprintf(msg, interpolation, options))
	}
}

type Connection struct {
	Url        string
	Credential string
}

func MakeConnection(
	protocol,
	storageURL,
	vds,
	sas string,
) (*Connection, error) {
	var url  string
	var cred string

	if strings.HasPrefix(sas, "?") {
		sas = sas[1:]
	}

	if strings.HasSuffix(vds, "/") {
		vds = vds[:len(vds)-1]
	}

	switch protocol {
		case "azure://": {
			cred = fmt.Sprintf("BlobEndpoint=%v;SharedAccessSignature=?%v", storageURL, sas)
			url  = protocol + vds
		}
		case "file://": {
			cred = ""
			url  = protocol + vds
		}
		default: {
			msg := fmt.Sprintf("Unknown protocol: %v", protocol)
			return nil, errors.New(msg)
		}
	}

	return &Connection{ Url: url, Credential: cred }, nil
}

func GetMetadata(conn Connection) ([]byte, error) {
	curl := C.CString(conn.Url)
	defer C.free(unsafe.Pointer(curl))

	ccred := C.CString(conn.Credential)
	defer C.free(unsafe.Pointer(ccred))

	result := C.metadata(curl, ccred)
	
	defer C.vdsbuffer_delete(&result)

	if result.err != nil {
		err := C.GoString(result.err)
		return nil, errors.New(err)
	}

	buf := C.GoBytes(unsafe.Pointer(result.data), C.int(result.size))
	return buf, nil
}

func Slice(conn Connection, lineno, direction int) ([]byte, error) {
	curl := C.CString(conn.Url)
	defer C.free(unsafe.Pointer(curl))

	ccred := C.CString(conn.Credential)
	defer C.free(unsafe.Pointer(ccred))

	result := C.slice(
		curl,
		ccred,
		C.int(lineno),
		C.enum_axis(direction),
	)

	defer C.vdsbuffer_delete(&result)

	if result.err != nil {
		err := C.GoString(result.err)
		return nil, errors.New(err)
	}

	buf := C.GoBytes(unsafe.Pointer(result.data), C.int(result.size))
	return buf, nil
}

func SliceMetadata(conn Connection, lineno, direction int) ([]byte, error) {
	curl := C.CString(conn.Url)
	defer C.free(unsafe.Pointer(curl))

	ccred := C.CString(conn.Credential)
	defer C.free(unsafe.Pointer(ccred))

	result := C.slice_metadata(
		curl,
		ccred,
		C.int(lineno),
		C.enum_axis(direction),
	)

	defer C.vdsbuffer_delete(&result)

	if result.err != nil {
		err := C.GoString(result.err)
		return nil, errors.New(err)
	}

	buf := C.GoBytes(unsafe.Pointer(result.data), C.int(result.size))
	return buf, nil
}

func Fence(
	conn Connection,
	coordinate_system string,
	coordinates [][]float32,
	interpolation int,
) ([]byte, error) {
	cvds := C.CString(conn.Url)
	defer C.free(unsafe.Pointer(cvds))

	ccred := C.CString(conn.Credential)
	defer C.free(unsafe.Pointer(ccred))

	ccrd_system := C.CString(coordinate_system)
	defer C.free(unsafe.Pointer(ccrd_system))

	coordinate_len := 2
	ccoordinates := make([]C.float, len(coordinates) * coordinate_len)
	for i, _ := range coordinates {

		if len(coordinates[i]) != coordinate_len  {
			msg := fmt.Sprintf(
				"Invalid coordinate %v at position %d, expected [x y] pair",
				coordinates[i],
				i,
			)
			return nil, errors.New(msg)
		}

		for j, _ := range coordinates[i] {
			ccoordinates[i * coordinate_len  + j] = C.float(coordinates[i][j])
		}
	}

    result := C.fence(
		cvds,
		ccred,
		ccrd_system,
		&ccoordinates[0],
		C.size_t(len(coordinates)),
		C.enum_interpolation_method(interpolation),
	)

	defer C.vdsbuffer_delete(&result)

	if result.err != nil {
		err := C.GoString(result.err)
		return nil, errors.New(err)
    }

	buf := C.GoBytes(unsafe.Pointer(result.data), C.int(result.size))
	return buf, nil
}

func GetFenceMetadata(conn Connection, coordinates [][]float32) ([]byte, error) {
	curl := C.CString(conn.Url)
	defer C.free(unsafe.Pointer(curl))

	ccred := C.CString(conn.Credential)
	defer C.free(unsafe.Pointer(ccred))

	result := C.fence_metadata(
		curl,
		ccred,
		C.size_t(len(coordinates)),
	)

	defer C.vdsbuffer_delete(&result)

	if result.err != nil {
		err := C.GoString(result.err)
		return nil, errors.New(err)
	}

	buf := C.GoBytes(unsafe.Pointer(result.data), C.int(result.size))
	return buf, nil
}
