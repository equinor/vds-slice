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
    const char* vds,
    const char* credentials
);

struct requestdata slice(
    const char* vds,
    const char* credentials,
    int lineno,
    enum Axis direction
);

struct requestdata slice_metadata(
    const char* vds,
    const char* credentials,
    enum Axis direction
);

struct requestdata fence(
    const char* vds,
    const char* credentials,
    enum CoordinateSystem coordinate_system,
    const float* points,
    size_t npoints,
    enum InterpolationMethod interpolation_method
);

struct requestdata fence_metadata(
    const char* vds,
    const char* credentials,
    size_t npoints
);

void requestdata_delete(struct requestdata*);


#ifdef __cplusplus
}
#endif
#endif // VDS_SLICE_CGO_VDS_H
