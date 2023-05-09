#ifndef VDS_SLICE_CGO_VDS_H
#define VDS_SLICE_CGO_VDS_H
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Return value status codes */
enum status_code {
    STATUS_OK = 0
};

/** Carry additional context between caller and functions
 *
 * Any function that accepts a context as one of its input parameters can use
 * it to write additional information that might be of use to the caller. This
 * includes, but is not limited, to writting error messages into the context if
 * the function should fail. In that case the caller can call errmsg(Context*
 * ctx) to retrieve the error message.
 */
struct Context;
typedef struct Context Context;

/** Create a new context instance
 *
 * The returned instance must always be explicitly free'd by a
 * call to context_free().
 */
Context* context_new();

/** Free up the resources managed internally by the context */
int context_free(Context* ctx);

/** Read out the last error msg set on the context */
const char* errmsg(Context* ctx);

struct response {
    char*         data;
    char*         err;
    unsigned long size;
};

void response_delete(struct response*);

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
    MIN,
    MAX,
    MEAN,
    RMS,
    SD
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
    const char* data,
    size_t size,
    size_t vertical_window,
    float  fillvalue,
    enum attribute* attributes,
    size_t nattributes
);

#ifdef __cplusplus
}
#endif
#endif // VDS_SLICE_CGO_VDS_H
