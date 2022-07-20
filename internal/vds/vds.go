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

func Slice(vds, credentials string, lineno, direction int) ([]byte, error) {
	cvds := C.CString(vds)
	defer C.free(unsafe.Pointer(cvds))

	ccred := C.CString(credentials)
	defer C.free(unsafe.Pointer(ccred))

	result := C.slice(
		cvds,
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

func SliceMetadata(vds, credentials string, lineno, direction int) ([]byte, error) {
	cvds := C.CString(vds)
	defer C.free(unsafe.Pointer(cvds))

	ccred := C.CString(credentials)
	defer C.free(unsafe.Pointer(ccred))

	result := C.slice_metadata(
		cvds,
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
