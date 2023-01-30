#include "vdsfencerequest.h"

std::unique_ptr<VDSFencePointList>
VDSFenceRequest::requestAsPointList(const FenceRequestParameters& parameters) {
    auto vdsHandle = this->metadata.getVDSHandle();
    auto const* vdsLayout = OpenVDS::GetLayout(*vdsHandle);

    //Get point list
    std::unique_ptr<VDSFencePointList> coords(
        new float[parameters.numberOfPoints][OpenVDS::Dimensionality_Max]{{0}}
    );

    auto coordinateTransformer = OpenVDS::IJKCoordinateTransformer(vdsLayout);
    auto transformCoordinate = [&] (const float x, const float y) {
        switch (parameters.coordinateSystem) {
            case INDEX:
                return OpenVDS::Vector<double, 3> {x, y, 0};
            case ANNOTATION:
                return coordinateTransformer.AnnotationToIJKPosition({x, y, 0});
            case CDP:
                return coordinateTransformer.WorldToIJKPosition({x, y, 0});
            default: {
                throw std::runtime_error("Unhandled coordinate system");
            }
        }
    };

    float const * coordinates = parameters.coordinates;
    for (size_t i = 0; i < parameters.numberOfPoints; i++) {
        const float x = *(coordinates++);
        const float y = *(coordinates++);

        auto coordinate = transformCoordinate(x, y);

        auto validateBoundary = [&] (const Axis& axis) {
            const auto min = -0.5;
            const auto max = axis.getNumberOfPoints() - 0.5;
            const int apiIndex = axis.getApiIndex();
            if(coordinate[apiIndex] < min || coordinate[apiIndex] >= max) {
                const std::string coordinate_str =
                    "(" +std::to_string(x) + "," + std::to_string(y) + ")";
                throw std::runtime_error(
                    "Coordinate " + coordinate_str + " is out of boundaries "+
                    "in dimension "+ std::to_string(apiIndex)+ "."
                );
            }
        };

        const Axis inlineAxis = metadata.getInline();
        const Axis crosslineAxis = metadata.getCrossline();

        validateBoundary(inlineAxis);
        validateBoundary(crosslineAxis);
        /* openvds uses rounding down for Nearest interpolation.
            * As it is counterintuitive, we fix it by snapping to nearest index
            * and rounding half-up.
            */
        if (parameters.interpolationMethod == NEAREST) {
            coordinate[0] = std::round(coordinate[0] + 1) - 1;
            coordinate[1] = std::round(coordinate[1] + 1) - 1;
        }
        coords[i][   inlineAxis.getVdsIndex()] = coordinate[0];
        coords[i][crosslineAxis.getVdsIndex()] = coordinate[1];
        //}
    }
    return coords;
}

response VDSFenceRequest::getData(
    std::unique_ptr<VDSFencePointList>& points,
    const FenceRequestParameters& parameters
) {
    auto vdsHandle = this->metadata.getVDSHandle();
    auto vdsAccessManager = OpenVDS::GetAccessManager(*vdsHandle);

    //Get volume trace
    // TODO: Verify that trace dimension is always 0
    const int channelIndex = 0;
    const int levelOfDetailLevel = 0;
    const int traceDimension = 0;
    const auto requestSize = vdsAccessManager.GetVolumeTracesBufferSize(
        parameters.numberOfPoints,
        traceDimension,
        levelOfDetailLevel,
        channelIndex
    );

    VDSRequestBuffer requestedData(requestSize);
    const OpenVDS::InterpolationMethod interpolationMethod
        = VDSMetadataHandler::getInterpolation(parameters.interpolationMethod);
    auto request = vdsAccessManager.RequestVolumeTraces(
                        (float*)requestedData.getPointer(),
                        requestedData.getSizeInBytes(),
                        OpenVDS::Dimensions_012,
                        levelOfDetailLevel,
                        channelIndex,
                        points.get(),
                        parameters.numberOfPoints,
                        interpolationMethod,
                        0 // Replacement value
                    );

    const bool success = request.get()->WaitForCompletion();
    if (not success) {
        throw std::runtime_error("Failed to fetch fence from VDS");
    }
    return requestedData.getAsResponse();
}
