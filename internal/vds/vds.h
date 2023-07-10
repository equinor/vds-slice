#ifndef VDS_SLICE_CGO_VDS_H
#define VDS_SLICE_CGO_VDS_H
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Return value status codes */
enum status_code {
    STATUS_OK = 0,
    STATUS_NULLPTR_ERROR,
    STATUS_RUNTIME_ERROR,
    STATUS_BAD_REQUEST
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
    unsigned long size;
};
typedef struct response response;

void response_delete(struct response*);

struct DataHandle;
typedef struct DataHandle DataHandle;

/** Create a new (VDS) DataHandle instance */
int datahandle_new(
    Context* ctx,
    const char* url,
    const char* credentials,
    DataHandle** f
);

/** Free up the handle
 *
 * Closes the attached OpenVDS handle and frees the handle instance
 * itself.
 */
int datahandle_free(Context* ctx, DataHandle* f);

struct RegularSurface;
typedef struct RegularSurface RegularSurface;

int regular_surface_new(
    Context* ctx,
    const float* data,
    size_t nrows,
    size_t ncols,
    float xori,
    float yori,
    float xinc,
    float yinc,
    float rot,
    float fillvalue,
    RegularSurface** out
);

int regular_surface_free(
    Context* ctx,
    RegularSurface* surface
);

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
    MAXABS,
    MEAN,
    MEANABS,
    MEANPOS,
    MEANNEG,
    MEDIAN,
    RMS,
    VAR,
    SD,
    SUMPOS,
    SUMNEG
};

int metadata(
    Context* ctx,
    DataHandle* handle,
    response* out
);

int slice(
    Context* ctx,
    DataHandle* handle,
    int lineno,
    enum axis_name direction,
    response* out
);

int slice_metadata(
    Context* ctx,
    DataHandle* handle,
    int lineno,
    enum axis_name direction,
    response* out
);

int fence(
    Context* ctx,
    DataHandle* handle,
    enum coordinate_system coordinate_system,
    const float* points,
    size_t npoints,
    enum interpolation_method interpolation_method,
    response* out
);

int fence_metadata(
    Context* ctx,
    DataHandle* handle,
    size_t npoints,
    response* out
);

int horizon(
    Context* ctx,
    DataHandle* handle,
    RegularSurface* surface,
    float above,
    float below,
    enum interpolation_method interpolation_method,
    response* out
);

int attribute_metadata(
    Context* ctx,
    DataHandle* handle,
    size_t nrows,
    size_t ncols,
    response* out
);

/** Attribute calculation
*
* Output buffer
* -------------
*
* Ideally we want to allocate n output buffers in Golang. One for every target
* attribute. Each buffer is passed through C with a void* and since we need N
* void* they should be passed as an array (void**).
* However Golang prohibits us from passing Go pointer to memory containing Go
* pointers (for memory safety reasons) [1].
*
* Thus this function instead accepts a single contiguous buffer with enough
* room for all attributes. The total buffer size should be mapsize *
* nattributes, where mapsize is the number of bytes for a single attribute
* result.
*
* [1] https://pkg.go.dev/cmd/cgo#hdr-Passing_pointers
*/
int attribute(
    Context* ctx,
    DataHandle* handle,
    RegularSurface* surface,
    const char* data,
    size_t size,
    enum attribute* attributes,
    size_t nattributes,
    float above,
    float below,
    float stepsize,
    size_t from,
    size_t to,
    void* out
);

#ifdef __cplusplus
}
#endif
#endif // VDS_SLICE_CGO_VDS_H
