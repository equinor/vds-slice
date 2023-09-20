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
#include "subcube.hpp"
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
    std::vector< Bound > const& slicebounds,
    response* out
) {
    MetadataHandle const& metadata = handle.get_metadata();
    Axis const& axis = metadata.get_axis(direction);

    if (direction.is_sample()) {
        validate_vertical_axis(metadata.sample(), direction);
    }

    for (auto const& bound : slicebounds) {
        auto bound_dir = Direction(bound.name);
        validate_vertical_axis(metadata.sample(), bound_dir);
    }

    SubCube bounds(metadata);
    bounds.constrain(metadata, slicebounds);
    bounds.set_slice(axis, lineno, direction.coordinate_system());

    std::int64_t const size = handle.subcube_buffer_size(bounds);

    std::unique_ptr< char[] > data(new char[size]);
    handle.read_subcube(data.get(), size, bounds);

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

void horizon_buffer_offsets(
    DataHandle& handle,
    RegularSurface const& reference,
    RegularSurface const& top,
    RegularSurface const& bottom,
    std::size_t* out,
    std::size_t out_size
) {
    if (reference.size() != top.size() or reference.size() != bottom.size()) {
        throw std::runtime_error("Expected surfaces to be of the same size");
    }

    if (!(reference.plane() == top.plane() && reference.plane() == bottom.plane())) {
        throw std::runtime_error("Expected surfaces to have the same plane");
    }

    if (out_size != (reference.size() + 1)) {
        throw std::runtime_error("out buffer must be the size of surface + 1");
    }

    MetadataHandle const& metadata = handle.get_metadata();
    auto transform = metadata.coordinate_transformer();

    auto iline  = metadata.iline ();
    auto xline  = metadata.xline();
    auto sample = metadata.sample();

    VerticalWindow window(sample.stepsize(), 2, sample.min());

    out[0] = 0;
    for (int i = 0; i < reference.size(); ++i) {
        float reference_depth = reference[i];
        float top_depth       = top[i];
        float bottom_depth    = bottom[i];

        if (reference_depth == reference.fillvalue() ||
            top_depth == top.fillvalue() ||
            bottom_depth == bottom.fillvalue())
        {
            out[i+1] = out[i];
            continue;
        }

        if (reference_depth < top_depth ||
            reference_depth > bottom_depth)
        {
            throw std::runtime_error(
                "Planes are not ordered as top <= reference <= bottom");
        }

        auto const cdp = reference.to_cdp(i);
        auto ij = transform.WorldToAnnotation({cdp.x, cdp.y, 0});

        if (not iline.inrange(ij[0]) or not xline.inrange(ij[1])) {
            out[i+1] = out[i];
            continue;
        }

        window.move(reference_depth - top_depth, bottom_depth - reference_depth);
        out[i+1] = out[i] + window.size();
    }
}


void horizon(
    DataHandle& handle,
    RegularSurface const& reference,
    RegularSurface const& top,
    RegularSurface const& bottom,
    std::size_t* buffer_offsets,
    enum interpolation_method interpolation,
    std::size_t from,
    std::size_t to,
    void* out
) {
    if (reference.size() != top.size() or reference.size() != bottom.size()) {
        throw std::runtime_error("Expected surfaces to be of the same size");
    }

    if (!(reference.plane() == top.plane() && reference.plane() == bottom.plane())) {
        throw std::runtime_error("Expected surfaces to have the same plane");
    }

    if (to > reference.size()){
        throw std::invalid_argument("'to' must be less than surface size");
    }

    MetadataHandle const& metadata = handle.get_metadata();
    auto transform = metadata.coordinate_transformer();

    auto iline  = metadata.iline ();
    auto xline  = metadata.xline();
    auto sample = metadata.sample();

    std::size_t const nsamples = buffer_offsets[to] - buffer_offsets[from];
    if (nsamples == 0){
        return;
    }
    std::unique_ptr< voxel[] > samples(new voxel[nsamples]{{0}});

    VerticalWindow window(sample.stepsize(), 2, sample.min());
    std::size_t cur = 0;
    for (int i = from; i < to; ++i) {
        if(buffer_offsets[i] == buffer_offsets[i+1]) {
            continue;
        }

        window.move(reference[i] - top[i], bottom[i] - reference[i]);

        double nearest_reference_depth = window.nearest(reference[i]);

        auto const cdp = reference.to_cdp(i);
        auto ij = transform.WorldToAnnotation({cdp.x, cdp.y, 0});

        double nearest_top_depth    = nearest_reference_depth -
                                      window.nsamples_above() * window.stepsize();
        double nearest_bottom_depth = nearest_reference_depth +
                                      window.nsamples_below() * window.stepsize();

        if (not sample.inrange(nearest_top_depth) or
            not sample.inrange(nearest_bottom_depth))
        {
            auto row = i / reference.ncols();
            auto col = i % reference.ncols();
            throw std::runtime_error(
                "Vertical window is out of vertical bounds at"
                " row: " + std::to_string(row) +
                " col:" + std::to_string(col) +
                ". Request: [" + std::to_string(nearest_top_depth) +
                ", " + std::to_string(nearest_bottom_depth) +
                "]. Seismic bounds: [" + std::to_string(sample.min())
                + ", " +std::to_string(sample.max()) + "]"
            );
        }

        ij[0]  = iline.to_sample_position(ij[0]);
        ij[1]  = xline.to_sample_position(ij[1]);

        double top_sample_depth = sample.to_sample_position(nearest_top_depth);
        for (int idx = 0; idx < window.size(); ++idx) {
            samples[cur][  iline.dimension() ] = ij[0];
            samples[cur][  xline.dimension() ] = ij[1];
            samples[cur][ sample.dimension() ] = top_sample_depth + idx;
            ++cur;
        }
    }

    if (cur != nsamples){
        throw std::runtime_error("calculated nsamples " + std::to_string(nsamples) +
                                 " and actual samples " + std::to_string(cur) + " differ");
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

    auto offset = buffer_offsets[from] * sizeof(float);
    std::memcpy((char*)out + offset, buffer.get(), size);
}


void attributes(
    Horizon const& horizon,
    RegularSurface const& reference,
    RegularSurface const& top,
    RegularSurface const& bottom,
    VerticalWindow& src_window,
    VerticalWindow& dst_window,
    enum attribute* attributes,
    std::size_t nattributes,
    std::size_t from,
    std::size_t to,
    void** out
) {
    if (reference.size() != top.size() or reference.size() != bottom.size()) {
        throw std::runtime_error("Expected surfaces to be of the same size");
    }

    if (!(reference.plane() == top.plane() && reference.plane() == bottom.plane())) {
        throw std::runtime_error("Expected surfaces to have the same plane");
    }

    std::size_t size = horizon.mapsize();

    std::vector< std::unique_ptr< AttributeMap > > attrs;
    for (int i = 0; i < nattributes; ++i) {
        void* dst = out[i];
        switch (*attributes) {
            case VALUE:   { append(attrs,   Value(dst, size)     );   break; }
            case MIN:     { append(attrs,   Min(dst, size)       );   break; }
            case MAX:     { append(attrs,   Max(dst, size)       );   break; }
            case MAXABS:  { append(attrs,   MaxAbs(dst, size)    );   break; }
            case MEAN:    { append(attrs,   Mean(dst, size)      );   break; }
            case MEANABS: { append(attrs,   MeanAbs(dst, size)   );   break; }
            case MEANPOS: { append(attrs,   MeanPos(dst, size)   );   break; }
            case MEANNEG: { append(attrs,   MeanNeg(dst, size)   );   break; }
            case MEDIAN:  { append(attrs,   Median(dst, size)    );   break; }
            case RMS:     { append(attrs,   Rms(dst, size)       );   break; }
            case VAR:     { append(attrs,   Var(dst, size)       );   break; }
            case SD:      { append(attrs,   Sd(dst, size)        );   break; }
            case SUMPOS:  { append(attrs,   SumPos(dst, size)    );   break; }
            case SUMNEG:  { append(attrs,   SumNeg(dst, size)    );   break; }

            default:
                throw std::runtime_error("Attribute not implemented");
        }
        ++attributes;
    }

    calc_attributes(horizon, reference, top, bottom, src_window, dst_window, attrs, from, to);
}

namespace {

struct SurfacesCrossoverValidator {
    // assuming that samples axis in the file has positive increasing values
    bool have_crossed(float primary, float secondary) {
        if (primary > secondary) {
            this->primary_is_bottom = true;
        } else if (primary < secondary) {
            this->primary_is_top = true;
        } else { //primary == secondary
            return false;
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
        if (primary.fillvalue() == primary[i]) {
            aligned[i] = aligned.fillvalue();
            continue;
        }
        auto secondary_pos = secondary.from_cdp(primary.to_cdp(i));
        // calculated value can be out of bounds, also negative
        auto secondary_row = std::lround(secondary_pos.x);
        auto secondary_col = std::lround(secondary_pos.y);

        if (secondary_row < 0 || secondary_row >= secondary.nrows() ||
            (secondary_col < 0 || secondary_col >= secondary.ncols()))
        {
            aligned[i] = aligned.fillvalue();
            continue;
        }

        auto secondary_value = secondary[as_pair(secondary_row, secondary_col)];

        if(secondary.fillvalue() == secondary_value) {
            aligned[i] = aligned.fillvalue();
            continue;
        }

        aligned[i] = secondary_value;

        if (surfaces.have_crossed(primary[i], aligned[i])) {
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
