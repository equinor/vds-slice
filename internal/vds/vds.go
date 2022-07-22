package vds

/*
#cgo LDFLAGS: -lopenvds
#cgo CXXFLAGS: -std=c++11
#include <vds.h>
#include <stdlib.h>
*/
import "C"
import "unsafe"

import (
	"errors"
	"fmt"
	"strings"
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
	Sample float64 `json:"sample"`

	// Axis units
	Unit string `json:"unit"`
} // @name VdsAxis


// @Description Slice metadata
type Metadata struct {
	// Data format. See https://osdu.pages.opengroup.org/platform/domain-data-mgmt-services/seismic/open-vds/cppdoc/enum/OpenVDS_VolumeDataFormat.html
	Format int `json:"format"`

	// Axis descriptions
	//
	// Describes the axes of the requested 2-dimensional slice.
	Axis []*Axis `json:"axis"`
} // @name SliceMetadata

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
