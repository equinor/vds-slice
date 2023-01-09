#ifndef SEISMICHANDLE_H
#define SEISMICHANDLE_H

#include <string>

#include <OpenVDS/OpenVDS.h>

#include "boundingbox.h"
#include "seismicaxismapping.h"
#include "vds.h"

enum class Channel : int {
    Sample = 0,
    Default = Sample
};

enum class LevelOfDetail : int {
    Level0 = 0,
    Default = Level0
};

class AxisMetadata {
protected:
    OpenVDS::VolumeDataAxisDescriptor axis_descriptor_;
public:
    AxisMetadata(
        const OpenVDS::VolumeDataLayout* layout,
        const int voxel_dimension
    ) noexcept;

    int min() const;
    int max() const;
    int number_of_samples() const;

    std::string name() const;
    std::string unit() const;
};

struct SubVolume {
    struct {
        int lower[OpenVDS::VolumeDataLayout::Dimensionality_Max]{0, 0, 0, 0, 0, 0};
        int upper[OpenVDS::VolumeDataLayout::Dimensionality_Max]{1, 1, 1, 1, 1, 1};
    } bounds;
};

class AxisDescriptor : public AxisMetadata {
    Axis axis_;
    int voxel_dimension_;
public:
    AxisDescriptor(
        const Axis axis,
        const OpenVDS::VolumeDataLayout* layout,
        const int voxel_dimension
    ) noexcept;
    CoordinateSystem system() const;
    Axis value() const noexcept;
    int space_dimension() const;
    int voxel_dimension() const;
    std::string api_name() const;
};

class SeismicHandle {
public:
    SeismicHandle(
        const std::string    url,
        const std::string    connection,
        const Channel        default_channel,
        const LevelOfDetail  default_lod,
        const std::unique_ptr<SeismicAxisMap> axis_map
    );

    /// @brief  Maps from our Axis to a VDS axisDescriptor.
    /// @param axis
    /// @return
    AxisDescriptor get_axis(Axis axis) const;
    BoundingBox get_bounding_box() const;
    std::string get_crs() const;
    std::string get_format(Channel ch = Channel::Default) const;
    std::vector<AxisMetadata> get_all_axes_metadata() const;

protected:
    class SeismicValidator {
        static constexpr int expected_dimensionality_{3};
        public:
        void validate(const SeismicHandle& vds_handle);
     };

    OpenVDS::ScopedVDSHandle handle_;
    OpenVDS::VolumeDataAccessManager access_manager_;
    OpenVDS::VolumeDataLayout const * layout_;
    std::unique_ptr<SeismicAxisMap> axis_map_;
    const Channel default_channel_;
    const LevelOfDetail default_lod_;

    int to_voxel( const AxisDescriptor& axis_desc, const int lineno ) const;
    static OpenVDS::InterpolationMethod get_interpolation(
        InterpolationMethod interpolation);
};

#endif /* SEISMICHANDLE_H */
