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

enum api_axis_name {
    I         = 0,
    J         = 1,
    K         = 2,
    INLINE    = 3,
    CROSSLINE = 4,
    DEPTH     = 5,
    TIME      = 6,
    SAMPLE    = 7,
};

enum coordinate_system {
    INDEX      = 0,
    ANNOTATION = 1,
    CDP        = 2,
};

enum interpolation_method {
    NEAREST,
    LINEAR,
    CUBIC,
    ANGULAR,
    TRIANGULAR
};

struct response metadata(
    const char* vds,
    const char* credentials
);

struct response slice(
    const char* vds,
    const char* credentials,
    int lineno,
    enum api_axis_name direction
);

struct response slice_metadata(
    const char* vds,
    const char* credentials,
    enum api_axis_name direction
);

struct response fence(
    const char* vds,
    const char* credentials,
    enum coordinate_system coordinate_system,
    const float* points,
    size_t npoints,
    enum interpolation_method interpolation_method
);

struct response fence_metadata(
    const char* vds,
    const char* credentials,
    size_t npoints
);

void response_delete(struct response*);


#ifdef __cplusplus
}
#endif
#endif // VDS_SLICE_CGO_VDS_H
