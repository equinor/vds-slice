#include "vdsslicerequest.h"

#include <list>
#include <unordered_map>

#include <OpenVDS/KnownMetadata.h>

void VDSSliceRequest::validateAxis(const ApiAxisName& apiAxisName) {
    const Axis axis = this->metadata.getAxis(apiAxisName);
    // Validate request axis name and unit name
    if (   apiAxisName == ApiAxisName::DEPTH
        or apiAxisName == ApiAxisName::TIME
        or apiAxisName == ApiAxisName::SAMPLE)
    {
        const std::string axisUnit = axis.getUnit();
        const std::string axisName = axis.getApiName();

        static const std::unordered_map<std::string, std::list<std::string>>
            allowedSampleAxisCombinations
            = {
                {
                    std::string( OpenVDS::KnownAxisNames::Depth() ),
                    {
                        std::string(OpenVDS::KnownUnitNames::Meter()),
                        std::string(OpenVDS::KnownUnitNames::Foot()),
                        std::string(OpenVDS::KnownUnitNames::USSurveyFoot())
                    }
                },
                {
                    std::string( OpenVDS::KnownAxisNames::Time() ),
                    {
                        std::string(OpenVDS::KnownUnitNames::Millisecond()),
                        std::string(OpenVDS::KnownUnitNames::Second())
                    }
                },
                {
                    std::string( OpenVDS::KnownAxisNames::Sample() ),
                    {
                        std::string(OpenVDS::KnownUnitNames::Unitless())
                    }
                }
            };

        const auto axisIterator = allowedSampleAxisCombinations.find(axisName);

        // This checks for legal name
        if (axisIterator == allowedSampleAxisCombinations.end()) {
            const std::string msg = "Unable to use " + axisName +
                    " on cube with depth units: " + axisUnit;
            throw std::runtime_error(msg);
        }

        const std::list<std::string>& units = axisIterator->second;
        auto allowedUnitsContain = [&units] (const std::string name) {
            return std::find(units.begin(), units.end(), name) != units.end();
        };
        // This checks for legal unit
        if (not allowedUnitsContain(axisUnit)) {
            const std::string msg = "Unable to use " + axisName +
                    " on cube with depth units: " + axisUnit;
            throw std::runtime_error(msg);
        }
    }
}

SubVolume VDSSliceRequest::requestAsSubvolume(
    const SliceRequestParameters& parameters
) {
    const Axis axis = this->metadata.getAxis(parameters.apiAxisName);
    SubVolume subvolume;

    const auto axisBoundsToInitialize = {
                                            ApiAxisName::INLINE,
                                            ApiAxisName::CROSSLINE,
                                            ApiAxisName::SAMPLE
                                        };
    for (const auto axisName: axisBoundsToInitialize) {
        const Axis tmpAxis = metadata.getAxis(axisName);
        subvolume.bounds.upper[tmpAxis.getVdsIndex()] = tmpAxis.getNumberOfPoints();
    }

    const int numberOfPoints = axis.getNumberOfPoints();
    int min    = axis.getMin();
    int max    = axis.getMax();
    int stride = (max - min) / (numberOfPoints - 1);

    // TODO: Should we rather check whether the coordinate system IS Index?!
    if (axis.getCoordinateSystem() != CoordinateSystem::ANNOTATION) {
        min    = 0;
        max    = numberOfPoints - 1;
        stride = 1;
    }

    const int lineNumber = parameters.lineNumber;
    if (lineNumber < min || lineNumber > max || (lineNumber - min) % stride ) {
        std::string msg = "Invalid lineno: " + std::to_string(lineNumber) +
                        ", valid range: [" + std::to_string(min) +
                        ":" + std::to_string(max);
        if  (axis.getCoordinateSystem() == CoordinateSystem::ANNOTATION) {
            msg += ":" + std::to_string(stride) + "]";
        }
        else {
            msg += ":1]";
        }
        throw std::runtime_error(msg);
    }
    const int voxelLine = (lineNumber - min) / stride;

    const int vdsAxisIndex = axis.getVdsIndex();
    subvolume.bounds.lower[vdsAxisIndex] = voxelLine;
    subvolume.bounds.upper[vdsAxisIndex] = voxelLine + 1;
    return subvolume;
}

response VDSSliceRequest::getData(const SubVolume& subvolume) {
    const int channelIndex = 0;
    const int levelOfDetailLevel = 0;
    const auto channelFormat = metadata.getChannelFormat(channelIndex);

    auto vdsAccessManager = OpenVDS::GetAccessManager(*metadata.getVDSHandle());
    const auto requestSize = vdsAccessManager.GetVolumeSubsetBufferSize(
                                subvolume.bounds.lower,
                                subvolume.bounds.upper,
                                channelFormat,
                                levelOfDetailLevel,
                                channelIndex
                            );

    VDSRequestBuffer requestedData(requestSize);
    auto request = vdsAccessManager.RequestVolumeSubset(
                    requestedData.getPointer(),
                    requestedData.getSizeInBytes(),
                    OpenVDS::Dimensions_012,
                    levelOfDetailLevel,
                    channelIndex,
                    subvolume.bounds.lower,
                    subvolume.bounds.upper,
                    channelFormat
                );

    const bool success = request.get()->WaitForCompletion();
    if (not success) {
        throw std::runtime_error("Failed to fetch slice from VDS");
    }
    return requestedData.getAsResponse();
}
