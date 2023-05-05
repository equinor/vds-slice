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

enum axis_name {
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

enum attribute {
    VALUE,
    MIN,
    MAX,
    MEAN,
    RMS
};

struct response metadata(
    const char* vds,
    const char* credentials
);

struct response slice(
    const char* vds,
    const char* credentials,
    int lineno,
    enum axis_name direction
);

struct response slice_metadata(
    const char* vds,
    const char* credentials,
    enum axis_name direction
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

struct response horizon(
    const char* vds,
    const char* credentials,
    const float* data,
    size_t nrows,
    size_t ncols,
    float xori,
    float yori,
    float xinc,
    float yinc,
    float rot,
    float fillvalue,
    float above,
    float below,
    enum interpolation_method interpolation_method
);

struct response horizon_metadata(
    const char*  vdspath,
    const char* credentials,
    size_t nrows,
    size_t ncols
);

struct response attribute(
    const char* vdspath,
    const char* credentials,
    const char* data,
    size_t size,
    size_t vertical_window,
    float  fillvalue,
    enum attribute* attributes,
    size_t nattributes,
    float above
);

void response_delete(struct response*);


#ifdef __cplusplus
}
#endif
#endif // VDS_SLICE_CGO_VDS_H
