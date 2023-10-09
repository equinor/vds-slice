#include "ctypes.h"
#include "capi.h"

#include "cppapi.hpp"

#include "exceptions.hpp"
#include "subvolume.hpp"

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
            BoundedGrid(Grid(xori, yori, xinc, yinc, rot), nrows, ncols),
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

int subvolume_new(
    Context* ctx,
    DataHandle* handle,
    RegularSurface* reference,
    RegularSurface* top,
    RegularSurface* bottom,
    SurfaceBoundedSubVolume** out
) {
    try {
        if (not out)
            throw detail::nullptr_error("Invalid out pointer");
        if (not handle)
            throw detail::nullptr_error("Invalid handle");
        if (not reference)
            throw detail::nullptr_error("Invalid reference surface");
        if (not top)
            throw detail::nullptr_error("Invalid top surface");
        if (not bottom)
            throw detail::nullptr_error("Invalid bottom surface");

        *out = make_subvolume(
            handle->get_metadata(),
            *reference,
            *top,
            *bottom
        );
        return STATUS_OK;
    } catch (...) {
        return handle_exception(ctx, std::current_exception());
    }
}

int subvolume_free(Context* ctx, SurfaceBoundedSubVolume* subvolume) {
    try {
        if (not subvolume)
            return STATUS_OK;

        delete subvolume;

        return STATUS_OK;
    } catch (const std::exception& e) {
        if (ctx)
            ctx->errmsg = e.what();
        return STATUS_RUNTIME_ERROR;
    }
}

int slice(
    Context* ctx,
    DataHandle* handle,
    int lineno,
    axis_name ax,
    struct Bound* bounds,
    size_t nbounds,
    response* out
) {
    try {
        if (not out)    throw detail::nullptr_error("Invalid out pointer");
        if (not handle) throw detail::nullptr_error("Invalid handle");

        Direction const direction(ax);

        std::vector< Bound > slice_bounds;
        for (int i = 0; i < nbounds; ++i) {
            slice_bounds.push_back(*bounds);
            bounds++;
        }

        cppapi::slice(*handle, direction, lineno, slice_bounds, out);
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
    struct Bound* bounds,
    size_t nbounds,
    response* out
) {
    try {
        if (not out)    throw detail::nullptr_error("Invalid out pointer");
        if (not handle) throw detail::nullptr_error("Invalid handle");

        Direction const direction(ax);

        std::vector< Bound > slice_bounds;
        for (int i = 0; i < nbounds; ++i) {
            slice_bounds.push_back(*bounds);
            bounds++;
        }

        cppapi::slice_metadata(*handle, direction, lineno, slice_bounds, out);
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
    const float* fillValue,
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
            fillValue,
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
    SurfaceBoundedSubVolume* subvolume,
    enum interpolation_method interpolation,
    size_t from,
    size_t to
) {
    try {
        if (not handle)    throw detail::nullptr_error("Invalid handle");
        if (not subvolume) throw detail::nullptr_error("Invalid subvolume");

        cppapi::horizon(
            *handle,
            *subvolume,
            interpolation,
            from,
            to
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
    SurfaceBoundedSubVolume* src_subvolume,
    enum attribute* attributes,
    size_t nattributes,
    float stepsize,
    size_t from,
    size_t to,
    void*  out
) {
    try {
        if (not out)           throw detail::nullptr_error("Invalid out pointer");
        if (not handle)        throw detail::nullptr_error("Invalid handle");
        if (not src_subvolume) throw detail::nullptr_error("Invalid subvolume");

        if (from >= to)  throw std::runtime_error("No data to iterate over");

        MetadataHandle const& metadata = handle->get_metadata();
        auto const& sample = metadata.sample();

        if (stepsize == 0) {
            stepsize = sample.stepsize();
        }

        ResampledSegmentBlueprint dst_segment_blueprint = ResampledSegmentBlueprint(stepsize);

        void* outs[nattributes];
        for (int i = 0; i < nattributes; ++i) {
            auto offset = src_subvolume->horizontal_grid().size() * sizeof(float) * i;
            outs[i] = static_cast< char* >(out) + offset;
        }

        cppapi::attributes(
            *src_subvolume,
            &dst_segment_blueprint,
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

int align_surfaces(
    Context* ctx,
    RegularSurface* primary,
    RegularSurface* secondary,
    RegularSurface* aligned,
    int* primary_is_top
) {
    try {
        if (not primary)        throw detail::nullptr_error("Invalid primary surface");
        if (not secondary)      throw detail::nullptr_error("Invalid secondary surface");
        if (not aligned)        throw detail::nullptr_error("Invalid aligned surface");
        if (not primary_is_top) throw detail::nullptr_error("Invalid primary is top pointer");

        bool b_primary_is_top;

        cppapi::align_surfaces(*primary, *secondary, *aligned, &b_primary_is_top);
        if (b_primary_is_top) {
            *primary_is_top = 1;
        } else {
            *primary_is_top = 0;
        }
        return STATUS_OK;
    } catch (...) {
        return handle_exception(ctx, std::current_exception());
    }
}
