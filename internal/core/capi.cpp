#include "ctypes.h"
#include "capi.h"

#include "cppapi.hpp"

#include "exceptions.hpp"

void response_delete(struct response* buf) {
    if (!buf)
        return;

    delete[] buf->data;
    *buf = response {};
}

struct Context {
    std::string errmsg;
};

Context* context_new() {
    return new Context{};
}

int context_free(Context* ctx) {
    if (not ctx) return STATUS_OK;

    delete ctx;
    return STATUS_OK;
}

const char* errmsg(Context* ctx) {
    if (not ctx) return nullptr;
    return ctx->errmsg.c_str();
}

int handle_exception(Context* ctx, std::exception_ptr eptr) {
    try {
        if (eptr) std::rethrow_exception(eptr);
    } catch (const detail::nullptr_error& e) {
        if (ctx) ctx->errmsg = e.what();
        return STATUS_NULLPTR_ERROR;
    } catch (const detail::bad_request& e) {
        if (ctx) ctx->errmsg = e.what();
        return STATUS_BAD_REQUEST;
    } catch (const std::exception& e) {
        if (ctx) ctx->errmsg = e.what();
        return STATUS_RUNTIME_ERROR;
    }

    return STATUS_OK;
}

int datahandle_new(
    Context* ctx,
    const char* url,
    const char* credentials,
    DataHandle** out
) {
    try {
        if (not out) throw detail::nullptr_error("Invalid out pointer");

        *out = make_datahandle(url, credentials);
        return STATUS_OK;
    } catch (...) {
        return handle_exception(ctx, std::current_exception());
    }
}

int datahandle_free(Context* ctx, DataHandle* f) {
    try {
        if (not f) return STATUS_OK;

        delete f;

        return STATUS_OK;
    } catch (const std::exception& e) {
        if (ctx) ctx->errmsg = e.what();
        return STATUS_RUNTIME_ERROR;
    }
}

int regular_surface_new(
    Context* ctx,
    float* data,
    size_t nrows,
    size_t ncols,
    float xori,
    float yori,
    float xinc,
    float yinc,
    float rot,
    float fillvalue,
    RegularSurface** out
) {
    try {
        if (not out) throw detail::nullptr_error("Invalid out pointer");

        *out = new RegularSurface(
            data,
            nrows,
            ncols,
            Plane(xori, yori, xinc, yinc, rot),
            fillvalue
        );
        return STATUS_OK;
    } catch (...) {
        return handle_exception(ctx, std::current_exception());
    }
}

int regular_surface_free(Context* ctx, RegularSurface* surface) {
    try {
        if (not surface) return STATUS_OK;

        delete surface;

        return STATUS_OK;
    } catch (const std::exception& e) {
        if (ctx) ctx->errmsg = e.what();
        return STATUS_RUNTIME_ERROR;
    }
}

int slice(
    Context* ctx,
    DataHandle* handle,
    int lineno,
    axis_name ax,
    response* out
) {
    try {
        if (not out)    throw detail::nullptr_error("Invalid out pointer");
        if (not handle) throw detail::nullptr_error("Invalid handle");

        Direction const direction(ax);
        cppapi::slice(*handle, direction, lineno, out);
        return STATUS_OK;
    } catch (...) {
        return handle_exception(ctx, std::current_exception());
    }
}

int slice_metadata(
    Context* ctx,
    DataHandle* handle,
    int lineno,
    axis_name ax,
    response* out
) {
    try {
        if (not out)    throw detail::nullptr_error("Invalid out pointer");
        if (not handle) throw detail::nullptr_error("Invalid handle");

        Direction const direction(ax);
        cppapi::slice_metadata(*handle, direction, lineno, out);
        return STATUS_OK;
    } catch (...) {
        return handle_exception(ctx, std::current_exception());
    }
}

int fence(
    Context* ctx,
    DataHandle* handle,
    enum coordinate_system coordinate_system,
    const float* coordinates,
    size_t npoints,
    enum interpolation_method interpolation_method,
    response* out
) {
    try {
        if (not out)    throw detail::nullptr_error("Invalid out pointer");
        if (not handle) throw detail::nullptr_error("Invalid handle");

        cppapi::fence(
            *handle,
            coordinate_system,
            coordinates,
            npoints,
            interpolation_method,
            out
        );
        return STATUS_OK;
    } catch (...) {
        return handle_exception(ctx, std::current_exception());
    }
}

int fence_metadata(
    Context* ctx,
    DataHandle* handle,
    size_t npoints,
    response* out
) {
    try {
        if (not out)    throw detail::nullptr_error("Invalid out pointer");
        if (not handle) throw detail::nullptr_error("Invalid handle");

        cppapi::fence_metadata(*handle, npoints, out);
        return STATUS_OK;
    } catch (...) {
        return handle_exception(ctx, std::current_exception());
    }
}

int metadata(
    Context* ctx,
    DataHandle* handle,
    response* out
) {
    try {
        if (not out)    throw detail::nullptr_error("Invalid out pointer");
        if (not handle) throw detail::nullptr_error("Invalid handle");

        cppapi::metadata(*handle, out);
        return STATUS_OK;
    } catch (...) {
        return handle_exception(ctx, std::current_exception());
    }
}

int horizon_buffer_offsets(
    Context* ctx,
    DataHandle* handle,
    RegularSurface* reference,
    RegularSurface* top,
    RegularSurface* bottom,
    size_t* out,
    size_t out_size
) {
    try {
        if (not out)      throw detail::nullptr_error("Invalid out pointer");
        if (not handle)   throw detail::nullptr_error("Invalid handle");
        if (not reference)throw detail::nullptr_error("Invalid reference surface");
        if (not top)      throw detail::nullptr_error("Invalid top surface");
        if (not bottom)   throw detail::nullptr_error("Invalid bottom surface");

        cppapi::horizon_buffer_offsets(
            *handle,
            *reference,
            *top,
            *bottom,
            out,
            out_size
        );
        return STATUS_OK;
    } catch (...) {
        return handle_exception(ctx, std::current_exception());
    }
}

int horizon(
    Context* ctx,
    DataHandle* handle,
    RegularSurface* reference,
    RegularSurface* top,
    RegularSurface* bottom,
    std::size_t* buffer_offsets,
    enum interpolation_method interpolation,
    size_t from,
    size_t to,
    void* out
) {
    try {
        if (not out)            throw detail::nullptr_error("Invalid out pointer");
        if (not handle)         throw detail::nullptr_error("Invalid handle");
        if (not reference)      throw detail::nullptr_error("Invalid reference surface");
        if (not top)            throw detail::nullptr_error("Invalid top surface");
        if (not bottom)         throw detail::nullptr_error("Invalid bottom surface");
        if (not buffer_offsets) throw detail::nullptr_error("Invalid data offset buffer");

        cppapi::horizon(
            *handle,
            *reference,
            *top,
            *bottom,
            buffer_offsets,
            interpolation,
            from,
            to,
            out
        );
        return STATUS_OK;
    } catch (...) {
        return handle_exception(ctx, std::current_exception());
    }
}

int attribute_metadata(
    Context* ctx,
    DataHandle* handle,
    size_t nrows,
    size_t ncols,
    response* out
) {
    try {
        if (not out)    throw detail::nullptr_error("Invalid out pointer");
        if (not handle) throw detail::nullptr_error("Invalid handle");

        cppapi::attributes_metadata(*handle, nrows, ncols, out);
        return STATUS_OK;
    } catch (...) {
        return handle_exception(ctx, std::current_exception());
    }
}

int attribute(
    Context* ctx,
    DataHandle* handle,
    RegularSurface* reference,
    RegularSurface* top,
    RegularSurface* bottom,
    size_t* data_offsets,
    const void* data,
    size_t size,
    enum attribute* attributes,
    size_t nattributes,
    float stepsize,
    size_t from,
    size_t to,
    void*  out
) {
    try {
        if (not out)          throw detail::nullptr_error("Invalid out pointer");
        if (not handle)       throw detail::nullptr_error("Invalid handle");
        if (not reference)    throw detail::nullptr_error("Invalid reference surface");
        if (not top)          throw detail::nullptr_error("Invalid top surface");
        if (not bottom)       throw detail::nullptr_error("Invalid bottom surface");
        if (not data)         throw detail::nullptr_error("Invalid data");
        if (not data_offsets) throw detail::nullptr_error("Invalid data offset buffer");

        if (from >= to)  throw std::runtime_error("No data to iterate over");

        std::size_t nsamples = size / sizeof(float);
        std::size_t hsize = reference->size();
        std::size_t vsize = nsamples / hsize;

        Horizon horizon((float*)data, hsize, vsize, data_offsets, reference->fillvalue());

        MetadataHandle const& metadata = handle->get_metadata();
        auto const& sample = metadata.sample();

        if (stepsize == 0) {
            stepsize = sample.stride();
        }

        /* calculations below are temporary */
        auto first_nonzero = std::find_if(data_offsets, &data_offsets[hsize + 1],
                                          [](std::size_t i) { return i!= 0; });
        auto nonfill_index =  std::distance(data_offsets, first_nonzero - 1);
        if (nonfill_index >= hsize){
            nonfill_index = 0;
        }

        auto above = reference->value(nonfill_index) - top->value(nonfill_index);
        auto below = bottom->value(nonfill_index) - reference->value(nonfill_index);

        VerticalWindow src_window(above, below, sample.stride(), 2, sample.min());
        VerticalWindow dst_window(above, below, stepsize);

        void* outs[nattributes];
        for (int i = 0; i < nattributes; ++i) {
            outs[i] = static_cast< char* >(out) + horizon.mapsize() * i;
        }

        cppapi::attributes(
            *handle,
            horizon,
            *reference,
            *top,
            *bottom,
            src_window,
            dst_window,
            attributes,
            nattributes,
            from,
            to,
            outs
        );
        return STATUS_OK;
    } catch (...) {
        return handle_exception(ctx, std::current_exception());
    }
}
