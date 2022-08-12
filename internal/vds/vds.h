#ifndef VDS_SLICE_CGO_VDS_H
#define VDS_SLICE_CGO_VDS_H
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vdsbuffer {
    char*         data;
    char*         err;
    unsigned long size;
};

enum axis {
    I         = 0,
    J         = 1,
    K         = 2,
    INLINE    = 3,
    CROSSLINE = 4,
    DEPTH     = 5,
    TIME      = 6,
    SAMPLE    = 7,
};

enum interpolation_method {
    NEAREST,
    LINEAR,
    CUBIC,
    ANGULAR,
    TRIANGULAR
};

struct vdsbuffer slice(
    const char* vds,
    const char* credentials,
    int lineno,
    enum axis direction
);

struct vdsbuffer slice_metadata(
    const char* vds,
    const char* credentials,
    int lineno,
    enum axis direction
);

struct vdsbuffer fence(
    const char* vds,
    const char* credentials,
    const char* coordinate_system,
    const float* points,
    size_t npoints,
    enum interpolation_method interpolation_method
);

void vdsbuffer_delete(struct vdsbuffer*);


#ifdef __cplusplus
}
#endif
#endif // VDS_SLICE_CGO_VDS_H
