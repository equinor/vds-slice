#ifndef SLICEREQUEST_H
#define SLICEREQUEST_H

#include <string>

#include <OpenVDS/OpenVDS.h>

#include "metadatahandle.h"
#include "vds.h"

struct SliceRequestParameters {
    const api_axis_name axis_name;
    const int           line_number;
};

struct SubVolume {
    struct {
        int lower[OpenVDS::VolumeDataLayout::Dimensionality_Max]{0, 0, 0, 0, 0, 0};
        int upper[OpenVDS::VolumeDataLayout::Dimensionality_Max]{1, 1, 1, 1, 1, 1};
    } bounds;

    SubVolume(const MetadataHandle& metadata) {
        static const auto axis_bounds_to_initialize = {
            api_axis_name::INLINE,
            api_axis_name::CROSSLINE,
            api_axis_name::SAMPLE
        };

        for (const auto axis_name: axis_bounds_to_initialize) {
            const Axis axis = metadata.get_axis(axis_name);
            this->bounds.upper[axis.get_vds_index()] =
                axis.get_number_of_points();
        }
    }
};

class SliceRequest {
public:
    SliceRequest(
        MetadataHandle& metadata,
        OpenVDS::VolumeDataAccessManager& access_manager
    ) : metadata(metadata), access_manager(access_manager) {}

    response get_data(
        const SliceRequestParameters& parameters
    );

private:
    MetadataHandle& metadata;
    OpenVDS::VolumeDataAccessManager& access_manager;

    static constexpr int channel_index = 0;
    static constexpr int lod_level     = 0;
    /*
    * Unit validation of time/depth/sample slices
    *
    * Verify that the units of the VDS' time/depth/sample axis matches the
    * requested slice axis. E.g. a Time slice is only valid if the units of the
    * corresponding axis in the VDS is  * "Seconds" or "Milliseconds".
    */
    void validate_request_axis(const api_axis_name& name) const;

    SubVolume get_request_subvolume(
        const SliceRequestParameters& parameters
    ) const;

    int line_number_to_voxel_number(
        const SliceRequestParameters& parameters
    ) const;
};

#endif /* SLICEREQUEST_H */
