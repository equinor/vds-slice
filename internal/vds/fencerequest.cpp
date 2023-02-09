#include "fencerequest.h"

#include <stdexcept>

namespace openvds_adapter {

std::unique_ptr<FencePointList>
FenceRequest::get_vds_point_list(const FenceRequestParameters& parameters) {
    auto const* layout = this->access_manager.GetVolumeDataLayout();

    auto coordinate_transformer = OpenVDS::IJKCoordinateTransformer(layout);
    auto transform_coordinate = [&] (const float x, const float y) {
        switch (parameters.coordinate_system) {
            case INDEX:
                return OpenVDS::Vector<double, 3> {x, y, 0};
            case ANNOTATION:
                return coordinate_transformer.AnnotationToIJKPosition({x, y, 0});
            case CDP:
                return coordinate_transformer.WorldToIJKPosition({x, y, 0});
            default: break;
        }
        // Cannot be in default: as GCC otherwise complains about reaching end
        // of non-void function
        throw std::runtime_error("Unhandled coordinate system");
    };

    std::unique_ptr<FencePointList> coords(
        new float[parameters.number_of_points][OpenVDS::Dimensionality_Max]{{0}}
    );

    float const * coordinates = parameters.coordinates;
    for (size_t i = 0; i < parameters.number_of_points; i++) {
        const float x = *(coordinates++);
        const float y = *(coordinates++);

        auto coordinate = transform_coordinate(x, y);

        auto validate_boundary = [&] (const Axis& axis) {
            const auto min = -0.5;
            const auto max = axis.get_number_of_points() - 0.5;
            const int apiIndex = axis.get_api_index();
            if(coordinate[apiIndex] < min || coordinate[apiIndex] >= max) {
                const std::string coordinate_str =
                    "(" +std::to_string(x) + "," + std::to_string(y) + ")";
                throw std::runtime_error(
                    "Coordinate " + coordinate_str + " is out of boundaries "+
                    "in dimension "+ std::to_string(apiIndex)+ "."
                );
            }
        };

        const Axis inline_axis = metadata.get_inline();
        const Axis crossline_axis = metadata.get_crossline();

        validate_boundary(inline_axis);
        validate_boundary(crossline_axis);
        /* openvds uses rounding down for Nearest interpolation.
            * As it is counterintuitive, we fix it by snapping to nearest index
            * and rounding half-up.
            */
        if (parameters.interpolation_method == NEAREST) {
            coordinate[0] = std::round(coordinate[0] + 1) - 1;
            coordinate[1] = std::round(coordinate[1] + 1) - 1;
        }
        coords[i][   inline_axis.get_vds_index()] = coordinate[0];
        coords[i][crossline_axis.get_vds_index()] = coordinate[1];
    }
    return coords;
}

OpenVDS::InterpolationMethod FenceRequest::get_interpolation(
    const interpolation_method interpolation
) {
    switch (interpolation)
    {
        case NEAREST:    return OpenVDS::InterpolationMethod::Nearest;
        case LINEAR:     return OpenVDS::InterpolationMethod::Linear;
        case CUBIC:      return OpenVDS::InterpolationMethod::Cubic;
        case ANGULAR:    return OpenVDS::InterpolationMethod::Angular;
        case TRIANGULAR: return OpenVDS::InterpolationMethod::Triangular;
        default: break;
    }
    // Cannot be in default: as GCC otherwise complains about reaching end
    // of non-void function
    throw std::runtime_error("Unhandled interpolation method");
}

response FenceRequest::get_data(const FenceRequestParameters& parameters) {
    auto points = this->get_vds_point_list(parameters);

    // Get volume trace
    const std::int64_t request_size = this->access_manager.GetVolumeTracesBufferSize(
        parameters.number_of_points,
        this->trace_dimension,
        this->lod_level,
        this->channel_index
    );

    std::unique_ptr<char[]> buffer(new char[request_size]);
    const OpenVDS::InterpolationMethod interpolation_method = get_interpolation(
        parameters.interpolation_method
    );
    auto request = this->access_manager.RequestVolumeTraces(
        (float*)buffer.get(),
        request_size,
        OpenVDS::Dimensions_012,
        this->lod_level,
        this->channel_index,
        points.get(),
        parameters.number_of_points,
        interpolation_method,
        0 // Replacement value
    );

    const bool success = request.get()->WaitForCompletion();
    if (not success) {
        throw std::runtime_error("Failed to fetch fence from VDS");
    }
    return response{
        buffer.release(),
        nullptr,
        static_cast<unsigned long>(request_size)
    };
}

} /* namespace openvds_adapter */
