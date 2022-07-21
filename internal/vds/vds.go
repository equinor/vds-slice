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

type Axis struct {
	Annotation string  `json:"annotation"`
	Min        float64 `json:"min"`
	Max        float64 `json:"max"`
	Sample     float64 `json:"sample"`
	Unit       string  `json:"unit"`
}

type Metadata struct {
	Format int     `json:"format"`
	Axis   []*Axis `json:"axis"`
}

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
    )

	defer C.vdsbuffer_delete(&result)

	if result.err != nil {
		err := C.GoString(result.err)
		return nil, errors.New(err)
    }

	buf := C.GoBytes(unsafe.Pointer(result.data), C.int(result.size))
	return buf, nil
}
