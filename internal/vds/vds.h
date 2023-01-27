#ifndef VDS_SLICE_CGO_VDS_H
#define VDS_SLICE_CGO_VDS_H
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct response {
    char*         data;
    char*         err;
    unsigned long size;
};

enum ApiAxisName {
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

struct response metadata(
    char const * const vds,
    char const * const credentials
);

struct response slice(
    char const * const     vds,
    char const * const     credentials,
    const int              lineno,
    const enum ApiAxisName axisName
);

struct response slice_metadata(
    char const * const     vds,
    char const * const     credentials,
    const enum ApiAxisName axisName
);

struct response fence(
    char const * const             vds,
    char const * const             credentials,
    const enum CoordinateSystem    coordinate_system,
    float const * const            points,
    const size_t                   npoints,
    const enum InterpolationMethod interpolation_method
);

struct response fence_metadata(
    char const * const vds,
    char const * const credentials,
    const size_t       npoints
);

void response_delete(struct response*);


#ifdef __cplusplus
}
#endif
#endif // VDS_SLICE_CGO_VDS_H
