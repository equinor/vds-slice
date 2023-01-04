#ifndef POSTSTACK_H
#define POSTSTACK_H

#include <array>
#include <list>
#include <string>
#include <unordered_map>

#include "seismichandle.h"
#include "vds.h"

enum AxisDirection : int {
    X=0,
    Y=1,
    Z=2
};

/** Verifies dimension is 3D and axis order */
class PostStackHandle : public SeismicHandle {

public:
    PostStackHandle(std::string url, std::string conn);

    requestdata get_slice(
        const Axis axis,
        const int           line_number,
        const LevelOfDetail level_of_detail = LevelOfDetail::Default,
        const Channel       channel = Channel::Default
    );

    requestdata get_fence(
        const CoordinateSystem    coordinate_system,
        float const *             coordinates,
        const size_t              npoints,
        const InterpolationMethod interpolation_method,
        const LevelOfDetail       level_of_detail = LevelOfDetail::Default,
        const Channel             channel = Channel::Default
    );

    std::array<AxisMetadata, 2> get_slice_axis_metadata(const Axis axis) const;
    std::array<AxisMetadata, 3> get_all_axes_metadata() const;

protected:

    SubVolume slice_as_subvolume(
        const AxisDescriptor& axis_desc,
        const int lineno
    ) const;

    requestdata get_subvolume(
        const SubVolume subvolume,
        const LevelOfDetail level_of_detail,
        const Channel channel ) ;

    class PostStackValidator {

        static const std::unordered_map<std::string, std::list<std::string>> valid_z_axis_combinations_;

        const PostStackHandle& handle_;
        void validate_axes_order();

    public:

        PostStackValidator( const PostStackHandle& handle );

        void validate();

        static const std::unordered_map<std::string, std::list<std::string>>&
        get_valid_z_axis_combinations() {
            return valid_z_axis_combinations_;
        }
    };

    class SliceRequestValidator {

        public:

        SliceRequestValidator();

        void validate(const AxisDescriptor& axis_desc);

    };
};

#endif /* POSTSTACK_H */
