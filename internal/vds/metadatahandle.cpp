#include "metadatahandle.h"

#include <stdexcept>
#include <list>
#include <utility>

#include <OpenVDS/KnownMetadata.h>

void MetadataHandle::validate_dimensionality() const {
    constexpr int expected_dimensionality = 3;
    if (this->layout->GetDimensionality() != expected_dimensionality) {
        throw std::runtime_error(
            "Unsupported VDS, expected 3 dimensions, got " +
            std::to_string(this->layout->GetDimensionality())
        );
    }
}

/*
 * We're gonna assume that all VDS' are ordered like so:
 *
 *     voxel[0] -> depth/time/sample
 *     voxel[1] -> crossline
 *     voxel[2] -> inline
 */
void MetadataHandle::validate_axes_order() const {
    std::list<std::pair<int, char const *>> expected_axis_index_and_name = {
        {0, OpenVDS::KnownAxisNames::Sample()},
        {1, OpenVDS::KnownAxisNames::Crossline()},
        {2, OpenVDS::KnownAxisNames::Inline()}
    };

    for (const auto& index_and_name: expected_axis_index_and_name) {
        const int expected_axis_index = index_and_name.first;
        char const * expected_axis_name = index_and_name.second;
        char const * actual_axis_name = this->layout->GetDimensionName(
                                          expected_axis_index
                                      );

        if (strcmp(expected_axis_name, actual_axis_name) != 0) {
            std::string msg = "Unsupported axis ordering. Expected axis "
                              + std::string(expected_axis_name)
                              + " (axis index: "
                              + std::to_string(expected_axis_index)
                              + "), got "
                              + std::string(actual_axis_name);
            throw std::runtime_error(msg);
        }
    }
}

const Axis& MetadataHandle::get_inline() const noexcept (true) {
    return this->inline_axis;
}

const Axis& MetadataHandle::get_crossline() const noexcept (true) {
    return this->crossline_axis;
}

const Axis& MetadataHandle::get_sample() const noexcept (true) {
    return this->sample_axis;
}

const BoundingBox& MetadataHandle::get_bounding_box() const noexcept (true) {
    return this->bounding_box;
}

std::string MetadataHandle::get_format() const noexcept (true) {
    return "<f4";
}

std::string MetadataHandle::get_crs() const noexcept (true) {
    const auto crs = OpenVDS::KnownMetadata::SurveyCoordinateSystemCRSWkt();
    return this->layout->GetMetadataString(crs.GetCategory(), crs.GetName());
}
