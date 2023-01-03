#ifndef VDS_SLICE_CGO_VDS_H
#define VDS_SLICE_CGO_VDS_H
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct requestdata {
    char*         data;
    char*         err;
    unsigned long size;
};

enum Axis {
    I         = 0,
    J         = 1,
    K         = 2,
    INLINE    = 3,
    CROSSLINE = 4,
    DEPTH     = 5,
    TIME      = 6,
    SAMPLE    = 7,
};

enum CoordinateSystem {
    INDEX      = 0,
    ANNOTATION = 1,
    CDP        = 2,
};

enum InterpolationMethod {
    NEAREST,
    LINEAR,
    CUBIC,
    ANGULAR,
    TRIANGULAR
};

struct requestdata metadata(
    char const * const vds,
    char const * const credentials
);

struct requestdata slice(
    char const * const vds,
    char const * const credentials,
    const int lineno,
    const enum Axis direction
);

struct requestdata slice_metadata(
    char const * const vds,
    char const * const credentials,
    const enum Axis direction
);

struct requestdata fence(
    char const * const vds,
    char const * const credentials,
    const enum CoordinateSystem coordinate_system,
    float const * const points,
    const size_t npoints,
    const enum InterpolationMethod interpolation_method
);

struct requestdata fence_metadata(
    char const * const vds,
    char const * const credentials,
    const size_t npoints
);

void requestdata_delete(struct requestdata*);


#ifdef __cplusplus
}
#endif
#endif // VDS_SLICE_CGO_VDS_H
