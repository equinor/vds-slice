#ifndef FENCEREQUEST_H
#define FENCEREQUEST_H

#include <memory>
#include <string>

#include <OpenVDS/OpenVDS.h>

#include "metadatahandle.h"
#include "vds.h"

struct FenceRequestParameters {
    const enum coordinate_system    coordinate_system;
    float const *                   coordinates;
    const size_t                    number_of_points;
    const enum interpolation_method interpolation_method;
};

using FencePointList = float[][OpenVDS::Dimensionality_Max];

class FenceRequest {
public:
    FenceRequest(
        MetadataHandle& metadata,
        OpenVDS::VolumeDataAccessManager& access_manager
    ) : metadata(metadata), access_manager(access_manager) {}

    response get_data(
        const FenceRequestParameters& parameters
    );

private:
    MetadataHandle& metadata;
    OpenVDS::VolumeDataAccessManager& access_manager;

    static constexpr int channel_index   = 0;
    static constexpr int lod_level       = 0;
    // TODO: Verify that trace dimension is always 0
    static constexpr int trace_dimension = 0;

    std::unique_ptr<FencePointList>
    get_vds_point_list(const FenceRequestParameters& parameters);

    OpenVDS::Vector<double, 3> transform_to_vds_coordinate(
        const float x,
        const float y
    ) const;

    static OpenVDS::InterpolationMethod get_interpolation(
        const interpolation_method interpolation
    );
};

#endif /* FENCEREQUEST_H */
