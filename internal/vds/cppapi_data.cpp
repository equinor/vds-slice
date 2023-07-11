#include "vds.h"

#include <cstdint>
#include <string>
#include <memory>

#include <OpenVDS/OpenVDS.h>
#include <OpenVDS/KnownMetadata.h>
#include <OpenVDS/IJKCoordinateTransformer.h>

#include "attribute.hpp"
#include "axis.hpp"
#include "datahandle.hpp"
#include "direction.hpp"
#include "exceptions.hpp"
#include "metadatahandle.hpp"
#include "regularsurface.hpp"
#include "subvolume.hpp"
#include "verticalwindow.hpp"

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
                return coordinate_transformer.IJKPositionToAnnotation({x, y, 0});
            case ANNOTATION:
                return OpenVDS::Vector<double, 3> {x, y, 0};
            case CDP:
                return coordinate_transformer.WorldToAnnotation({x, y, 0});
            default: {
                throw std::runtime_error("Unhandled coordinate system");
            }
        }
    };

    Axis inline_axis = metadata.iline();
    Axis crossline_axis = metadata.xline();

    for (size_t i = 0; i < npoints; i++) {
        const float x = *(coordinates++);
        const float y = *(coordinates++);

        auto coordinate = transform_coordinate(x, y);

        auto validate_boundary = [&] (const int voxel, Axis const& axis) {
            if(!axis.inrange(coordinate[voxel])){
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

        coords[i][   inline_axis.dimension()] = inline_axis.to_sample_position(coordinate[0]);
        coords[i][crossline_axis.dimension()] = crossline_axis.to_sample_position(coordinate[1]);
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

    auto iline  = metadata.iline ();
    auto xline  = metadata.xline();
    auto sample = metadata.sample();
    
    VerticalWindow window(above, below, sample.stride(), 2, sample.min());

    std::size_t const nsamples = surface.size() * window.size();

    std::unique_ptr< voxel[] > samples(new voxel[nsamples]{{0}});

    float const fillvalue = surface.fillvalue();

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

            auto ij = transform.WorldToAnnotation({cdp.x, cdp.y, 0});

            if (not iline.inrange(ij[0]) or not xline.inrange(ij[1])) {
                noval_indicies.push_back(i);
                i += window.size();
                continue;
            }

            double top    = depth - window.nsamples_above() * window.stepsize();
            double bottom = depth + window.nsamples_below() * window.stepsize();
            if (not sample.inrange(top) or not sample.inrange(bottom)) {
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

            ij[0]  = iline.to_sample_position(ij[0]);
            ij[1]  = xline.to_sample_position(ij[1]);
            top    = sample.to_sample_position(top);
            bottom = sample.to_sample_position(bottom);

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
            case VALUE:   { append(attrs,   Value(dst, size, index)   );   break; }
            case MIN:     { append(attrs,   Min(dst, size)            );   break; }
            case MAX:     { append(attrs,   Max(dst, size)            );   break; }
            case MAXABS:  { append(attrs,   MaxAbs(dst, size)         );   break; }
            case MEAN:    { append(attrs,   Mean(dst, size, vsize)    );   break; }
            case MEANABS: { append(attrs,   MeanAbs(dst, size, vsize) );   break; }
            case MEANPOS: { append(attrs,   MeanPos(dst, size)        );   break; }
            case MEANNEG: { append(attrs,   MeanNeg(dst, size)        );   break; }
            case MEDIAN:  { append(attrs,   Median(dst, size, vsize)  );   break; }
            case RMS:     { append(attrs,   Rms(dst, size, vsize)     );   break; }
            case VAR:     { append(attrs,   Var(dst, size, vsize)     );   break; }
            case SD:      { append(attrs,   Sd(dst, size, vsize)      );   break; }
            case SUMPOS:  { append(attrs,   SumPos(dst, size)         );   break; }
            case SUMNEG:  { append(attrs,   SumNeg(dst, size)         );   break; }

            default:
                throw std::runtime_error("Attribute not implemented");
        }
        ++attributes;
    }

    calc_attributes(horizon, surface, src_window, dst_window, attrs, from, to);
}
