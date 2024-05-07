package core

/*
#include <capi.h>
#include <ctypes.h>
#include <stdlib.h>
*/
import "C"
import (
	"unsafe"
)

func newCSliceBounds(bounds []Bound) ([]C.struct_Bound, error) {
	var cBounds []C.struct_Bound
	for _, bound := range bounds {
		lower := *bound.Lower
		upper := *bound.Upper
		axisID, err := GetAxis(*bound.Direction)
		if err != nil {
			return nil, err
		}

		if upper < lower {
			msg := "Upper bound must be >= than lower bound"
			return nil, NewInvalidArgument(msg)
		}

		cBound := C.struct_Bound{
			C.int(lower),
			C.int(upper),
			C.enum_axis_name(axisID),
		}
		cBounds = append(cBounds, cBound)
	}

	return cBounds, nil
}

func (v DSHandle) GetSlice(lineno, direction int, bounds []Bound) ([]byte, error) {
	var result C.struct_response

	cBounds, err := newCSliceBounds(bounds)
	if err != nil {
		return nil, err
	}

	var bound *C.struct_Bound
	if len(cBounds) > 0 {
		bound = &cBounds[0]
	}

	cerr := C.slice(
		v.context(),
		v.DataHandle(),
		C.int(lineno),
		C.enum_axis_name(direction),
		bound,
		C.size_t(len(cBounds)),
		&result,
	)

	defer C.response_delete(&result)
	if err := v.Error(cerr); err != nil {
		return nil, err
	}

	buf := C.GoBytes(unsafe.Pointer(result.data), C.int(result.size))
	return buf, nil
}

func (v DSHandle) GetSliceMetadata(
	lineno int,
	direction int,
	bounds []Bound,
) ([]byte, error) {
	var result C.struct_response

	cBounds, err := newCSliceBounds(bounds)
	if err != nil {
		return nil, err
	}

	var bound *C.struct_Bound
	if len(cBounds) > 0 {
		bound = &cBounds[0]
	}

	cerr := C.slice_metadata(
		v.context(),
		v.DataHandle(),
		C.int(lineno),
		C.enum_axis_name(direction),
		bound,
		C.size_t(len(cBounds)),
		&result,
	)

	defer C.response_delete(&result)

	if err := v.Error(cerr); err != nil {
		return nil, err
	}

	buf := C.GoBytes(unsafe.Pointer(result.data), C.int(result.size))
	return buf, nil
}
