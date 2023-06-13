#include "vds.h"

#include <array>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <memory>
#include <cmath>

#include "nlohmann/json.hpp"

#include <OpenVDS/OpenVDS.h>
#include <OpenVDS/KnownMetadata.h>
#include <OpenVDS/IJKCoordinateTransformer.h>

#include "attribute.hpp"
#include "axis.hpp"
#include "boundingbox.hpp"
#include "datahandle.hpp"
#include "direction.hpp"
#include "exceptions.hpp"
#include "metadatahandle.hpp"
#include "regularsurface.hpp"
#include "subvolume.hpp"
#include "verticalwindow.hpp"

void response_delete(struct response* buf) {
    if (!buf)
        return;

    delete[] buf->data;
    *buf = response {};
}

std::string fmtstr(OpenVDS::VolumeDataFormat format) {
    /*
     * We always request data in OpenVDS::VolumeDataFormat::Format_R32 format
     * as this seems to be intended way when working with openvds [1].
     * Thus users will always get data returned as f4.
     *
     * We also assume that server code is run on a little-endian machine.
     *
     * [1] https://community.opengroup.org/osdu/platform/domain-data-mgmt-services/seismic/open-vds/-/issues/156#note_165511
     */
    switch (format) {
        case OpenVDS::VolumeDataFormat::Format_R32: return "<f4";
        default: {
            throw std::runtime_error("unsupported VDS format type");
        }
    }
}

void to_response(nlohmann::json const& metadata, response* response) {
    auto const dump = metadata.dump();
    std::unique_ptr< char[] > tmp(new char[dump.size()]);
    std::copy(dump.begin(), dump.end(), tmp.get());

    response->data = tmp.release();
    response->size = dump.size();
}

void to_response(
    std::unique_ptr< char[] > data,
    std::int64_t const size,
    response* response
) {
    /* The data should *not* be free'd on success, as it's returned to CGO */
    response->data = data.release();
    response->size = static_cast<unsigned long>(size);
}

bool equal(const char* lhs, const char* rhs) {
    return std::strcmp(lhs, rhs) == 0;
}

/** Validate the request against the vds' vertical axis
 *  
 * Requests for Time and Depth are checked against the axis name and unit of
 * the actual file, while Sample acts as a fallback option where anything goes
 *
 *     Requested axis | VDS axis name   | VDS axis unit
 *     ---------------+-----------------+---------------------
 *     Sample         |      any        |      any
 *     Time           |  Time or Sample | "ms" or "s"
 *     Depth          | Depth or Sample | "m", "ft", or "usft"
 */
void validate_vertical_axis(
    Axis const& vertical_axis, 
    Direction const& request
) noexcept (false) {
    const auto& unit = vertical_axis.unit();
    const char* vdsunit = unit.c_str();
    
    const auto& name = vertical_axis.name();
    const char* vdsname = name.c_str();

    const auto requested_axis = request.name();

    using Unit = OpenVDS::KnownUnitNames;
    using Label = OpenVDS::KnownAxisNames;
    if (requested_axis == axis_name::DEPTH) {
        if (not equal(vdsname, Label::Depth()) and 
            not equal(vdsname, Label::Sample())
        ) {
            throw detail::bad_request(
                "Cannot fetch depth slice for VDS file with vertical axis label: "  + name
            );
        }

        if (
            not equal(vdsunit, Unit::Meter()) and 
            not equal(vdsunit, Unit::Foot())  and 
            not equal(vdsunit, Unit::USSurveyFoot())
        ) {
            throw detail::bad_request(
                "Cannot fetch depth slice for VDS file with vertical axis unit: "  + unit
            );
        }

    }

    if (requested_axis == axis_name::TIME) {
        if (not equal(vdsname, Label::Time()) and 
            not equal(vdsname, Label::Sample())
        ) {
            throw detail::bad_request(
                "Cannot fetch time slice for VDS file with vertical axis label: "  + name
            );
        }

        if (not equal(vdsunit, Unit::Millisecond()) and 
            not equal(vdsunit, Unit::Second())
        ) {
            throw detail::bad_request(
                "Cannot fetch time slice for VDS file with vertical axis unit: "  + unit
            );
        }
    }
}

nlohmann::json json_axis(
    Axis const& axis
) {
    nlohmann::json doc;
    doc = {
        { "annotation", axis.name()     },
        { "min",        axis.min()      },
        { "max",        axis.max()      },
        { "samples",    axis.nsamples() },
        { "unit",       axis.unit()     },
    };
    return doc;
}

void fetch_slice(
    DataHandle& handle,
    Direction const direction,
    int lineno,
    response* out
) {
    MetadataHandle const& metadata = handle.get_metadata();
    Axis const& axis = metadata.get_axis(direction);

    if (direction.is_sample()) {
        validate_vertical_axis(metadata.sample(), direction);
    }   

    SubVolume bounds(metadata);
    bounds.set_slice(axis, lineno, direction.coordinate_system());

    std::int64_t const size = handle.subvolume_buffer_size(bounds);

    std::unique_ptr< char[] > data(new char[size]);
    handle.read_subvolume(data.get(), size, bounds);

    return to_response(std::move(data), size, out);
}

void fetch_slice_metadata(
    DataHandle& handle,
    Direction const direction,
    response* out
) {
    MetadataHandle const& metadata = handle.get_metadata();

    nlohmann::json meta;
    meta["format"] = fmtstr(DataHandle::format());

    /*
     * SEGYImport always writes annotation 'Sample' for axis K. We, on the
     * other hand, decided that we base the valid input direction on the units
     * of said axis. E.g. ms/s -> Time, etc. This leads to an inconsistency
     * between what we require as input for axis K and what we return as
     * metadata. In the ms/s case we require the input to be asked for in axis
     * 'Time', but the return metadata can potentially say 'Sample'.
     *
     * TODO: Either revert the 'clever' unit validation, or patch the
     * K-annotation here. IMO the later is too clever for it's own good and
     * would be quite suprising for people that use this API in conjunction
     * with the OpenVDS library.
     */
    Axis const& inline_axis = metadata.iline();
    Axis const& crossline_axis = metadata.xline();
    Axis const& sample_axis = metadata.sample();

    if (direction.is_iline()) {
        meta["x"] = json_axis(sample_axis);
        meta["y"] = json_axis(crossline_axis);
    } else if (direction.is_xline()) {
        meta["x"] = json_axis(sample_axis);
        meta["y"] = json_axis(inline_axis);
    } else if (direction.is_sample()) {
        meta["x"] = json_axis(crossline_axis);
        meta["y"] = json_axis(inline_axis);
    } else {
        throw std::runtime_error("Unhandled direction");
    }

    return to_response(meta, out);
}

void fetch_fence(
    DataHandle& handle,
    enum coordinate_system coordinate_system,
    const float* coordinates,
    size_t npoints,
    enum interpolation_method interpolation_method,
    response* out
) {
    MetadataHandle const& metadata = handle.get_metadata();

    std::unique_ptr< voxel[] > coords(new voxel[npoints]{{0}});

    auto coordinate_transformer = metadata.coordinate_transformer();
    auto transform_coordinate = [&] (const float x, const float y) {
        switch (coordinate_system) {
            case INDEX:
                return OpenVDS::Vector<double, 3> {x, y, 0};
            case ANNOTATION:
                return coordinate_transformer.AnnotationToIJKPosition({x, y, 0});
            case CDP:
                return coordinate_transformer.WorldToIJKPosition({x, y, 0});
            default: {
                throw std::runtime_error("Unhandled coordinate system");
            }
        }
    };

    Axis const& inline_axis = metadata.iline();
    Axis const& crossline_axis = metadata.xline();

    for (size_t i = 0; i < npoints; i++) {
        const float x = *(coordinates++);
        const float y = *(coordinates++);

        auto coordinate = transform_coordinate(x, y);

        auto validate_boundary = [&] (const int voxel, Axis const& axis) {
            const auto min = -0.5;
            const auto max = axis.nsamples() - 0.5;
            if(coordinate[voxel] < min || coordinate[voxel] >= max) {
                const std::string coordinate_str =
                    "(" +std::to_string(x) + "," + std::to_string(y) + ")";
                throw std::runtime_error(
                    "Coordinate " + coordinate_str + " is out of boundaries "+
                    "in dimension "+ std::to_string(voxel)+ "."
                );
            }
        };

        validate_boundary(0, inline_axis);
        validate_boundary(1, crossline_axis);

        /* OpenVDS' transformers and OpenVDS data request functions have
         * different definition of where a datapoint is. E.g. A transformer
         * (To voxel or ijK) will return (0,0,0) for the first sample in
         * the cube. The request functions on the other hand assumes the
         * data is located in the center of a voxel. I.e. that the first
         * sample is at (0.5, 0.5, 0.5). This is a *VERY* sharp edge in the
         * OpenVDS API and borders on a bug. It means we cannot directly
         * use the output from the transformers as input to the request
         * functions.
         */
        coordinate[0] += 0.5;
        coordinate[1] += 0.5;

        coords[i][   inline_axis.dimension()] = coordinate[0];
        coords[i][crossline_axis.dimension()] = coordinate[1];
    }

    std::int64_t const size = handle.traces_buffer_size(npoints);

    std::unique_ptr< char[] > data(new char[size]);

    handle.read_traces(
        data.get(),
        size,
        coords.get(),
        npoints,
        interpolation_method
    );

    return to_response(std::move(data), size, out);
}

void fetch_fence_metadata(
    DataHandle& handle,
    size_t npoints,
    response* out
) {
    MetadataHandle const& metadata = handle.get_metadata();

    nlohmann::json meta;
    Axis const& sample_axis = metadata.sample();
    meta["shape"] = nlohmann::json::array({npoints, sample_axis.nsamples() });
    meta["format"] = fmtstr(DataHandle::format());

    return to_response(meta, out);
}

void metadata(DataHandle& handle, response* out) {
    MetadataHandle const& metadata = handle.get_metadata();

    nlohmann::json meta;

    meta["crs"] = metadata.crs();
    meta["inputFileName"] = metadata.input_filename();

    auto bbox = metadata.bounding_box();
    meta["boundingBox"]["ij"]   = bbox.index();
    meta["boundingBox"]["cdp"]  = bbox.world();
    meta["boundingBox"]["ilxl"] = bbox.annotation();


    Axis const& inline_axis = metadata.iline();
    meta["axis"].push_back(json_axis(inline_axis));

    Axis const& crossline_axis = metadata.xline();
    meta["axis"].push_back(json_axis(crossline_axis));

    Axis const& sample_axis = metadata.sample();
    meta["axis"].push_back(json_axis(sample_axis));

    return to_response(meta, out);
}

/**
 * For every index in 'novals', write n successive floats with value
 * 'fillvalue' to dst. Where n is 'vertical_size'.
 */
void write_fillvalue(
    char * dst,
    std::vector< std::size_t > const& novals,
    std::size_t vertical_size,
    float fillvalue
) {
    std::vector< float > fill(vertical_size, fillvalue);
    std::for_each(novals.begin(), novals.end(), [&](std::size_t i) {
        std::memcpy(
            dst + i * sizeof(float),
            fill.data(),
            fill.size() * sizeof(float)
        );
    });
}

void fetch_horizon(
    DataHandle& handle,
    RegularSurface const& surface,
    float above,
    float below,
    enum interpolation_method interpolation,
    response* out
) {
    if (above < 0) throw std::invalid_argument("'Above' must be >= 0");
    if (below < 0) throw std::invalid_argument("'below' must be >= 0");

    MetadataHandle const& metadata = handle.get_metadata();
    auto transform = metadata.coordinate_transformer();

    auto const& iline  = metadata.iline ();
    auto const& xline  = metadata.xline();
    auto const& sample = metadata.sample();
    
    VerticalWindow window(above, below, sample.stride(), 2, sample.min());

    std::size_t const nsamples = surface.size() * window.size();

    std::unique_ptr< voxel[] > samples(new voxel[nsamples]{{0}});

    float const fillvalue = surface.fillvalue();

    auto inrange = [](Axis const& axis, double const voxel) {
        return (0 <= voxel) and (voxel < axis.nsamples());
    };

    /** Missing input samples (marked by fillvalue) and out of bounds samples
     *
     * To not overcomplicate things for ourselfs (and the caller) we guarantee
     * that the output amplitude map is exacty the same dimensions as the input
     * height map (horizon). That gives us 2 cases to explicitly handle:
     *
     * 1) If a sample (region of samples) in the input horizon is marked as
     * missing by the fillvalue then the fillvalue is used in that position in
     * the output array too:
     *
     *      input[n][m] == fillvalue => output[n][m] == fillvalue
     *
     * 2) If a sample (or region of samples) in the input horizon is out of
     * bounds in the horizontal plane, the output sample is populated by the
     * fillvalue.
     *
     * openvds provides no options to handle these cases and to keep the output
     * buffer aligned with the input we cannot drop samples that satisfy 1) or
     * 2). Instead we let openvds read a dummy voxel  ({0, 0, 0, 0, 0, 0}) and
     * keep track of the indices. After openvds is done we copy in the
     * fillvalue.
     *
     * The overhead of this approach is that we overfetch (at most) one one
     * chunk and we need an extra loop over output array.
     */
    std::vector< std::size_t > noval_indicies;

    std::size_t i = 0;
    for (int row = 0; row < surface.nrows(); row++) {
        for (int col = 0; col < surface.ncols(); col++) {
            float depth = surface.value(row, col);
            if (depth == fillvalue) {
                noval_indicies.push_back(i);
                i += window.size();
                continue;
            }
            
            depth = window.nearest(depth);

            auto const cdp = surface.coordinate(row, col);

            auto ij = transform.WorldToIJKPosition({cdp.x, cdp.y, 0});
            auto k  = transform.AnnotationToIJKPosition({0, 0, depth});

            /* OpenVDS' transformers and OpenVDS data request functions have
             * different definition of where a datapoint is. E.g. A transformer
             * (To voxel or ijK) will return (0,0,0) for the first sample in
             * the cube. The request functions on the other hand assumes the
             * data is located in the center of a voxel. I.e. that the first
             * sample is at (0.5, 0.5, 0.5). This is a *VERY* sharp edge in the
             * OpenVDS API and borders on a bug. It means we cannot directly
             * use the output from the transformers as input to the request
             * functions.
             */
            ij[0] += 0.5;
            ij[1] += 0.5;
             k[2] += 0.5;

            if (not inrange(iline, ij[0]) or not inrange(xline, ij[1])) {
                noval_indicies.push_back(i);
                i += window.size();
                continue;
            }

            double top    = k[2] - window.nsamples_above();
            double bottom = k[2] + window.nsamples_below();
            if (not inrange(sample, top) or not inrange(sample, bottom)) {
                throw std::runtime_error(
                    "Vertical window is out of vertical bounds at"
                    " row: " + std::to_string(row) +
                    " col:" + std::to_string(col) +
                    ". Request: [" + std::to_string(bottom) +
                    ", " + std::to_string(top) +
                    "]. Seismic bounds: [" + std::to_string(sample.min())
                    + ", " +std::to_string(sample.max()) + "]"
                );
            }
            for (double cur_depth = top; cur_depth <= bottom; cur_depth++) {
                samples[i][  iline.dimension() ] = ij[0];
                samples[i][  xline.dimension() ] = ij[1];
                samples[i][ sample.dimension() ] = cur_depth;
                ++i;
            }
        }
    }

    auto const size = handle.samples_buffer_size(nsamples);

    std::unique_ptr< char[] > buffer(new char[size]());
    handle.read_samples(
        buffer.get(),
        size,
        samples.get(),
        nsamples,
        interpolation
    );

    write_fillvalue(buffer.get(), noval_indicies, window.size(), fillvalue);

    return to_response(std::move(buffer), size, out);
}

template< typename T >
void append(std::vector< std::unique_ptr< AttributeMap > >& vec, T obj) {
    vec.push_back( std::unique_ptr< T >( new T( std::move(obj) ) ) );
}

void calculate_attribute(
    DataHandle& handle,
    Horizon const& horizon,
    RegularSurface const& surface,
    VerticalWindow const& src_window,
    VerticalWindow const& dst_window,
    enum attribute* attributes,
    std::size_t nattributes,
    std::size_t from,
    std::size_t to,
    void** out
) {
    MetadataHandle const& metadata = handle.get_metadata();
    std::size_t index = dst_window.nsamples_above();

    std::size_t size = horizon.mapsize();
    std::size_t vsize = dst_window.size();

    std::vector< std::unique_ptr< AttributeMap > > attrs;
    for (int i = 0; i < nattributes; ++i) {
        void* dst = out[i];
        switch (*attributes) {
            case VALUE: { append(attrs, Value(dst, size, index)); break; }
            case MIN:   { append(attrs,   Min(dst, size)       ); break; }
            case MAX:   { append(attrs,   Max(dst, size)       ); break; }
            case MEAN:  { append(attrs,  Mean(dst, size, vsize)); break; }
            case RMS:   { append(attrs,   Rms(dst, size, vsize)); break; }
            case SD:    { append(attrs,    Sd(dst, size, vsize)); break; }
            default:
                throw std::runtime_error("Attribute not implemented");
        }
        ++attributes;
    }

    calc_attributes(horizon, surface, src_window, dst_window, attrs, from, to);
}

void fetch_attribute_metadata(
    DataHandle& handle,
    std::size_t nrows,
    std::size_t ncols,
    response* out
) {
    MetadataHandle const& metadata = handle.get_metadata();

    nlohmann::json meta;
    meta["shape"] = nlohmann::json::array({nrows, ncols});
    meta["format"] = fmtstr(DataHandle::format());

    return to_response(meta, out);
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

        OpenVDS::Error error;
        auto handle = OpenVDS::Open(url, credentials, error);
        if(error.code != 0) {
            throw std::runtime_error("Could not open VDS: " + error.string);
        }

        *out = new DataHandle(std::move(handle));
        return STATUS_OK;
    } catch (const detail::nullptr_error& e) {
        if (ctx) ctx->errmsg = e.what();
        return STATUS_NULLPTR_ERROR;
    } catch (const std::exception& e) {
        if (ctx) ctx->errmsg = e.what();
        return STATUS_RUNTIME_ERROR;
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
) {
    try {
        if (not out) throw detail::nullptr_error("Invalid out pointer");

        *out = new RegularSurface(
            data,
            nrows,
            ncols,
            xori,
            yori,
            xinc,
            yinc,
            rot,
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
        fetch_slice(*handle, direction, lineno, out);
        return STATUS_OK;
    } catch (...) {
        return handle_exception(ctx, std::current_exception());
    }
}

int slice_metadata(
    Context* ctx,
    DataHandle* handle,
    axis_name ax,
    response* out
) {
    try {
        if (not out)    throw detail::nullptr_error("Invalid out pointer");
        if (not handle) throw detail::nullptr_error("Invalid handle");

        Direction const direction(ax);
        fetch_slice_metadata(*handle, direction, out);
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

        fetch_fence(
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

        fetch_fence_metadata(*handle, npoints, out);
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

        metadata(*handle, out);
        return STATUS_OK;
    } catch (...) {
        return handle_exception(ctx, std::current_exception());
    }
}

int horizon(
    Context* ctx,
    DataHandle* handle,
    RegularSurface* surface,
    float above,
    float below,
    enum interpolation_method interpolation,
    response* out
) {
    try {
        if (not out)     throw detail::nullptr_error("Invalid out pointer");
        if (not handle)  throw detail::nullptr_error("Invalid handle");
        if (not surface) throw detail::nullptr_error("Invalid surface");

        fetch_horizon(
            *handle,
            *surface,
            above,
            below,
            interpolation,
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

        fetch_attribute_metadata(*handle, nrows, ncols, out);
        return STATUS_OK;
    } catch (...) {
        return handle_exception(ctx, std::current_exception());
    }
}

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
    void*  out
) {
    try {
        if (not out)     throw detail::nullptr_error("Invalid out pointer");
        if (not handle)  throw detail::nullptr_error("Invalid handle");
        if (not surface) throw detail::nullptr_error("Invalid surface");

        std::size_t nsamples = size / sizeof(float);
        std::size_t hsize = surface->size();
        std::size_t vsize = nsamples / hsize;

        Horizon horizon((float*)data, hsize, vsize, surface->fillvalue());

        MetadataHandle const& metadata = handle->get_metadata();
        auto const& sample = metadata.sample();

        if (stepsize == 0) {
            stepsize = sample.stride();
        }

        VerticalWindow src_window(above, below, sample.stride(), 2, sample.min());
        VerticalWindow dst_window(above, below, stepsize);

        void* outs[nattributes];
        for (int i = 0; i < nattributes; ++i) {
            outs[i] = static_cast< char* >(out) + horizon.mapsize() * i;
        }

        calculate_attribute(
            *handle,
            horizon,
            *surface,
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
