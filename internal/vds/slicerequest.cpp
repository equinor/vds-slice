#include "slicerequest.h"

#include <array>
#include <list>
#include <stdexcept>
#include <unordered_map>

#include <OpenVDS/KnownMetadata.h>

namespace openvds_adapter {

void SliceRequest::validate_request_axis(
    const api_axis_name& name
) const {

    static const std::array< const char*, 3 > depthunits = {
        OpenVDS::KnownUnitNames::Meter(),
        OpenVDS::KnownUnitNames::Foot(),
        OpenVDS::KnownUnitNames::USSurveyFoot()
    };

    static const std::array< const char*, 2 > timeunits = {
        OpenVDS::KnownUnitNames::Millisecond(),
        OpenVDS::KnownUnitNames::Second()
    };

    static const std::array< const char*, 1 > sampleunits = {
        OpenVDS::KnownUnitNames::Unitless(),
    };

    const Axis request_axis = metadata.get_axis(name);
    const std::string request_axis_unit = request_axis.get_unit();
    auto isoneof = [request_axis_unit](const char* x) {
        return !std::strcmp(x, request_axis_unit.c_str());
    };

    bool has_valid_unit = true;
    switch (name) {
        case I:
        case J:
        case K:
        case INLINE:
        case CROSSLINE:
            break;
        case DEPTH:
            has_valid_unit = std::any_of(depthunits.begin(), depthunits.end(), isoneof);
            break;
        case TIME:
            has_valid_unit = std::any_of(timeunits.begin(), timeunits.end(), isoneof);
            break;
        case SAMPLE:
            has_valid_unit = std::any_of(sampleunits.begin(), sampleunits.end(), isoneof);
            break;
        default: {
            throw std::runtime_error("Unhandled axis");
        }
    }

    if (not has_valid_unit) {
        std::string msg = "Unable to use " + request_axis.get_api_name();
        msg += " on cube with depth units: " + request_axis_unit;
        throw std::runtime_error(msg);
    }
}

SubVolume SliceRequest::get_request_subvolume (
    const SliceRequestParameters& parameters
) const {
    SubVolume subvolume(this->metadata);
    const Axis axis = this->metadata.get_axis(parameters.axis_name);
    const int vds_axis_index = axis.get_vds_index();
    const int voxel_line = line_number_to_voxel_number(parameters);
    subvolume.bounds.lower[vds_axis_index] = voxel_line;
    subvolume.bounds.upper[vds_axis_index] = voxel_line + 1;
    return subvolume;
}

int SliceRequest::line_number_to_voxel_number (
    const SliceRequestParameters& parameters
) const {
    const Axis axis = this->metadata.get_axis(parameters.axis_name);

    const int number_of_points = axis.get_number_of_points();
    int min    = axis.get_min();
    int max    = axis.get_max();
    int stride = (max - min) / (number_of_points - 1);

    if (axis.get_coordinate_system() == coordinate_system::INDEX) {
        min    = 0;
        max    = number_of_points - 1;
        stride = 1;
    }

    const int line_number = parameters.line_number;
    if (line_number < min || line_number > max || (line_number - min) % stride ) {
        std::string msg = "Invalid lineno: " + std::to_string(line_number) +
                          ", valid range: [" + std::to_string(min) +
                          ":" + std::to_string(max) +
                          ":" + std::to_string(stride) + "]";
        throw std::runtime_error(msg);
    }
    const int voxel_line = (line_number - min) / stride;
    return voxel_line;
}

response SliceRequest::get_data(const SliceRequestParameters& parameters) {
    validate_request_axis(parameters.axis_name);

    const SubVolume slice_as_subvolume = get_request_subvolume(parameters);

    auto * const layout = this->access_manager.GetVolumeDataLayout();
    const auto channel_format = layout->GetChannelFormat(channel_index);

    const std::int64_t request_size = this->access_manager.GetVolumeSubsetBufferSize(
        slice_as_subvolume.bounds.lower,
        slice_as_subvolume.bounds.upper,
        channel_format,
        this->lod_level,
        this->channel_index
    );

    std::unique_ptr<char[]> buffer(new char[request_size]);
    auto request = this->access_manager.RequestVolumeSubset(
        buffer.get(),
        request_size,
        OpenVDS::Dimensions_012,
        this->lod_level,
        this->channel_index,
        slice_as_subvolume.bounds.lower,
        slice_as_subvolume.bounds.upper,
        channel_format
    );

    const bool success = request.get()->WaitForCompletion();
    if (not success) {
        throw std::runtime_error("Failed to fetch slice from VDS");
    }
    return response{
        buffer.release(),
        nullptr,
        static_cast<unsigned long>(request_size)
    };
}

} /* namespace openvds_adapter */
