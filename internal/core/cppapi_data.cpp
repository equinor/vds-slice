#include "ctypes.h"

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

namespace {

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

/**
 * For every index in 'novals', write n successive floats with value
 * 'fillvalue' to dst.
 */
void write_fillvalue(
    char * dst,
    std::vector< std::size_t > const& novals,
    std::size_t nsamples,
    float fillvalue
) {
    std::vector< float > fill(nsamples, fillvalue);
    std::for_each(novals.begin(), novals.end(), [&](std::size_t i) {
        std::memcpy(
            dst + i * sizeof(float),
            fill.data(),
            fill.size() * sizeof(float)
        );
    });
}

template< typename T >
void append(std::vector< std::unique_ptr< AttributeMap > >& vec, T obj) {
    vec.push_back( std::unique_ptr< T >( new T( std::move(obj) ) ) );
}

} // namespace

namespace cppapi {

void slice(
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

void fence(
    DataHandle& handle,
    enum coordinate_system coordinate_system,
    const float* coordinates,
    size_t npoints,
    enum interpolation_method interpolation_method,
    const float* fillValue,
    response* out
) {
    MetadataHandle const& metadata = handle.get_metadata();

    std::vector< std::size_t > noval_indicies;

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
    Axis inline_axis    = metadata.iline();
    Axis crossline_axis = metadata.xline();
    Axis samples_axis   = metadata.sample();
    auto nsamples       = samples_axis.nsamples();

    for (size_t i = 0; i < npoints; i++) {
        const float x = *(coordinates++);
        const float y = *(coordinates++);

        auto coordinate = transform_coordinate(x, y);

        auto validate_boundary = [&] (const int voxel, Axis const& axis) {
            if (!axis.inrange(coordinate[voxel])) {
                if (fillValue == nullptr) {
                    const std::string coordinate_str =
                        "(" +std::to_string(x) + "," + std::to_string(y) + ")";
                    throw detail::bad_request(
                        "Coordinate " + coordinate_str + " is out of boundaries "+
                        "in dimension "+ std::to_string(voxel)+ "."
                    );
                }
                noval_indicies.push_back(i * nsamples);
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
    if (!noval_indicies.empty()){
            write_fillvalue(data.get(), noval_indicies, nsamples, *fillValue);
    }
    return to_response(std::move(data), size, out);
}

void horizon_size(
    DataHandle& handle,
    RegularSurface const& surface,
    float above,
    float below,
    std::size_t* out
) {
    if (above < 0) throw std::invalid_argument("'Above' must be >= 0");
    if (below < 0) throw std::invalid_argument("'below' must be >= 0");

    MetadataHandle const& metadata = handle.get_metadata();
    auto sample = metadata.sample();

    VerticalWindow window(above, below, sample.stride(), 2, sample.min());

    std::size_t const nsamples = surface.size() * window.size();
    *out = handle.samples_buffer_size(nsamples);
}


void horizon(
    DataHandle& handle,
    RegularSurface const& surface,
    float above,
    float below,
    enum interpolation_method interpolation,
    std::size_t from,
    std::size_t to,
    void* out
) {
    if (above < 0) throw std::invalid_argument("'Above' must be >= 0");
    if (below < 0) throw std::invalid_argument("'below' must be >= 0");

    if (to > surface.size()){
        throw std::invalid_argument("'to' must be less than surface size");
    }

    MetadataHandle const& metadata = handle.get_metadata();
    auto transform = metadata.coordinate_transformer();

    auto iline  = metadata.iline ();
    auto xline  = metadata.xline();
    auto sample = metadata.sample();

    VerticalWindow window(above, below, sample.stride(), 2, sample.min());

    float const fillvalue = surface.fillvalue();

    /** Missing input samples (marked by fillvalue) and out of bounds samples
     *
     * To not overcomplicate things for ourselfs (and the caller) we guarantee
     * that the output amplitude map is exacty the same dimensions as the input
     * height map (horizon). That gives us 2 cases to explicitly handle:
     *
     * 1) If a sample (region of samples) in the input values is marked as
     * missing by the fillvalue then the fillvalue is used in that position in
     * the output array too:
     *
     *      input[n][m] == fillvalue => output[n][m] == fillvalue
     *
     * 2) If a sample (or region of samples) in the input values is out of
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

    std::size_t const nsamples = (to - from) * window.size();
    if (nsamples == 0){
        return;
    }
    std::unique_ptr< voxel[] > samples(new voxel[nsamples]{{0}});

    std::size_t i = 0;
    for (int cell = from; cell < to; cell++) {
        auto row = cell / surface.ncols();
        auto col = cell % surface.ncols();

        float depth = surface.value(row, col);
        if (depth == fillvalue) {
            noval_indicies.push_back(i);
            i += window.size();
            continue;
        }
        
        depth = window.nearest(depth);

        auto const cdp = surface.to_cdp(row, col);

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
                ". Request: [" + std::to_string(top) +
                ", " + std::to_string(bottom) +
                "]. Seismic bounds: [" + std::to_string(sample.min())
                + ", " +std::to_string(sample.max()) + "]"
            );
        }

        ij[0]  = iline.to_sample_position(ij[0]);
        ij[1]  = xline.to_sample_position(ij[1]);


        top    = sample.to_sample_position(top);
        for (int idx = 0; idx < window.size(); ++idx) {
            samples[i][  iline.dimension() ] = ij[0];
            samples[i][  xline.dimension() ] = ij[1];
            samples[i][ sample.dimension() ] = top + idx;
            ++i;
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

    auto offset = from * window.size() * sizeof(float);
    std::memcpy((char*)out + offset, buffer.get(), size);
}


void attributes(
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

namespace {

struct SurfacesCrossoverValidator {
    // assuming that samples axis in the file has positive increasing values
    bool have_crossed(float primary, float secondary) {
        if (primary > secondary) {
            this->primary_is_bottom = true;
        }
        else if (primary < secondary) {
            this->primary_is_top = true;
        }
        return this->primary_is_top == this->primary_is_bottom;
    }

    bool is_primary_top() {
        return this->primary_is_top;
    }

private:
    bool primary_is_top = false;
    bool primary_is_bottom = false;
};

} //namespace

void align_surfaces(
    RegularSurface const &primary,
    RegularSurface const &secondary,
    RegularSurface &aligned,
    bool* primary_is_top
) {
    if (primary.size() != aligned.size() or !(primary.plane() == aligned.plane())) {
        throw std::runtime_error(
            "Expected primary and aligned surfaces to differ in data only.");
    }

    SurfacesCrossoverValidator surfaces;

    for (std::size_t i = 0; i < primary.size(); ++i) {
        if (primary.fillvalue() == primary.value(i)) {
            aligned.set_value(i, aligned.fillvalue());
            continue;
        }
        auto secondary_pos = secondary.from_cdp(primary.to_cdp(i));
        // calculated value can be out of bounds, also negative
        auto secondary_row = std::lround(secondary_pos.x);
        auto secondary_col = std::lround(secondary_pos.y);

        if (secondary_row < 0 || secondary_row >= secondary.nrows() ||
            (secondary_col < 0 || secondary_col >= secondary.ncols()))
        {
            aligned.set_value(i, aligned.fillvalue());
            continue;
        }

        auto secondary_value = secondary.value(secondary_row, secondary_col);

        if(secondary.fillvalue() == secondary_value) {
            aligned.set_value(i, aligned.fillvalue());
            continue;
        }

        aligned.set_value(i, secondary_value);

        if (surfaces.have_crossed(primary.value(i), aligned.value(i))) {
            std::size_t row = i / primary.ncols();
            std::size_t col = i % primary.ncols();
            throw detail::bad_request("Surfaces intersect at primary surface point ("
                                        + std::to_string(row) + ", "
                                        + std::to_string(col) + ")");
        }

    }
    *primary_is_top = surfaces.is_primary_top();
}

} // namespace cppapi
