#ifndef SEISMICHANDLE_H
#define SEISMICHANDLE_H

#include <string>

#include <OpenVDS/OpenVDS.h>

#include "boundingbox.h"
#include "seismicaxismapping.h"
#include "vds.h"

/// @brief Channel ids of commonly used data channels in volume data stores
///        (VDS).
///
///        At the moment we support only one channel called "Sample".
enum class Channel : int {
    Sample = 0,
    Default = Sample
};

/// @brief Supported "Level of detail". At the moment, we always work on the
///        finest level which refers to level 0.
enum class LevelOfDetail : int {
    Level0 = 0,
    Default = Level0
};

/// @brief Container of axis metadata
///
/// In contrast to OpenVDS' axis descriptor is uses C++ strings which makes
/// comparisons easier and it only exposes information currently used in
/// metadata requests
class AxisMetadata {
protected:
    /// @brief OpenVDS axis descriptor of the axis
    OpenVDS::VolumeDataAxisDescriptor axis_descriptor_;
public:
    /// @brief Constructor
    /// @param layout Pointer to OpenVDS::VolumeDataLayout of open VDS.
    /// @param voxel_dimension Dimension id of axis in voxel coordinate system.
    AxisMetadata(
        const OpenVDS::VolumeDataLayout* layout,
        const int voxel_dimension
    ) noexcept;
    /// @brief Gets the minimum value of current axis.
    /// @return Minimum value of current axis.
    int min() const;
    /// @brief Gets the maximum value of current axis.
    /// @return Maximum value of current axis.
    int max() const;
    /// @brief Gets the number of samples of current axis.
    /// @return Number of samples of current axis.
    int number_of_samples() const;
    /// @brief Get name of axis according to the VDS dataset.
    /// @return String containing the name of the axis.
    std::string name() const;
    /// @brief Get unit of current axis.
    /// @return String containing the unit of the axis.
    std::string unit() const;
};

/// @brief Struct containing the upper and lower bounds in the voxel coordinate
///        system that define the subvolume of a slice request.
struct SubVolume {
    /// @brief Anonymous struct to group lower and upper bound indices. The
    ///        voxel coordinate system is six dimensionsal, therefore the lower
    ///        and upper bounds are arrays size six.
    struct {
        /// @brief Lower bounds
        int lower[OpenVDS::VolumeDataLayout::Dimensionality_Max]{0, 0, 0, 0, 0, 0};
        /// @brief Upper bounds
        int upper[OpenVDS::VolumeDataLayout::Dimensionality_Max]{1, 1, 1, 1, 1, 1};
    } bounds;
};

/// @brief A class storing important information about the axis of a coordinate
///        system.
///
/// In contrast to the enum Axis or the class AxisMetadata, a AxisDescriptor is
/// aware of its coordinate system, its OpenVDS string representation and its
/// dimension.
class AxisDescriptor : public AxisMetadata {
    /// @brief Axis label
    Axis axis_;
    /// @brief Axis dimension id in voxel coordinate system.
    int voxel_dimension_;
public:
    /// @brief Constructor
    /// @param axis Enumeration specifying for which axis the CoordinateAxis
    ///             should be constructed.
    /// @param layout Pointer to OpenVDS::VolumeDataLayout of open VDS.
    /// @param voxel_dimension Dimension id of axis in voxel coordinate system.
    AxisDescriptor(
        const Axis axis,
        const OpenVDS::VolumeDataLayout* layout,
        const int voxel_dimension
    ) noexcept;

    /// @brief Query which coordinate system this AxisDescriptor belongs to
    /// @return Enumeration CoordinateSystem specifying the coordinate system.
    ///          Can be `INDEX` or `ANNOTATION`.
    CoordinateSystem system() const;
    /// @brief Query which Axis type is underlying the current CoordinateAxis.
    /// @return Enumeration type specifying axis
    Axis value() const noexcept;
    /// @brief Get numeric identifier of in spatial coordinate system.
    /// @return Numeric identifier of current axis. Can only by 0, 1 or 2.
    int space_dimension() const;
    /// @brief Get numeric identifier of in voxel coordinate system.
    /// @return Numeric identifier of current axis. Can only by 0, 1 or 2.
    int voxel_dimension() const;
    /// @brief Get name of axis according to API. This may be different from
    ///        the name used in the VDS dataset.
    /// @return String containing the name of the axis in the API's notation.
    std::string api_name() const;
};

/// @brief Base class for handling VDS stores containing seismic data.
class SeismicHandle {
public:
    /// @brief Constructor.
    /// @param url Connection URL.
    /// @param connection Connection string, e.g., containing SAS token.
    /// @param default_channel Default channel to extract data from.
    /// @param default_lod Default level of detail to extract data from.
    /// @param axis_map Axis map that knows in what order the inline, crossline
    ///                 and sample direction are stored.
    SeismicHandle(
        const std::string    url,
        const std::string    connection,
        const Channel        default_channel,
        const LevelOfDetail  default_lod,
        const std::unique_ptr<SeismicAxisMap> axis_map
    );

    /// @brief Get axis descriptor for an Axis enum.
    /// @param axis Axis for which one requests the descriptor.
    /// @return Axis descriptor of `axis`.
    AxisDescriptor get_axis(Axis axis) const;
    /// @brief Return bounding box of current data set in voxel coordinates.
    /// @return Returns bounding box element containing bounding box.
    BoundingBox get_bounding_box() const;
    /// @brief Get coordinate reference system string.
    /// @return Identifier stored in coordinate reference system field of VDS.
    std::string get_crs() const;
    /// @brief Get data format of channel.
    /// @param ch Specifies data channel of VDS.
    /// @return Data format (data type) of data stored in channel specified.
    std::string get_format(Channel ch = Channel::Default) const;
    /// @brief Get metadata of all available axes. The order of the returned
    ///        metadata might have to be reordered by the calling function.
    /// @return Vector containing the metadata of all axes in order Sample,
    ///         Crossline, Inline.
    std::vector<AxisMetadata> get_all_axes_metadata() const;

protected:
    /// @brief Class that validates basic assumptions about the opened VDS.
    class SeismicValidator {
        static constexpr int expected_dimensionality_{3};
        public:
        /// @brief Validates that the VDS has exactly three dimensions.
        /// @param vds_handle Handle to open VDS.
        void validate(const SeismicHandle& vds_handle);
     };

    /// @brief Handle to open VDS.
    OpenVDS::ScopedVDSHandle handle_;
    /// @brief Access manager to open VDS.
    OpenVDS::VolumeDataAccessManager access_manager_;
    /// @brief Pointer to VolumeDataLayout of open VDS.
    OpenVDS::VolumeDataLayout const * layout_;
    /// @brief Map storing information about the order of coordinate axis
    ///        (inline, crossline and sample direction).
    std::unique_ptr<SeismicAxisMap> axis_map_;
    /// @brief Default channel used when no channel is specified.
    const Channel default_channel_;
    /// @brief Default level of detail used when no level of detail is specified.
    const LevelOfDetail default_lod_;
    /// @brief Converts a given line number to its voxel number based on the
    ///        given axis descriptor. The conversion also does bound checking.
    /// @param axis_desc Axis descriptor.
    /// @param lineno Line number.
    /// @return Number of line in voxel coordinate system.
    int to_voxel( const AxisDescriptor& axis_desc, const int lineno ) const;
    /// @brief Convert InterpolationMethod to corresponding OpenVDS label.
    /// @param interpolation Label of interpolation method in the request's
    ///                      notation.
    /// @return Label of interpolation method in OpenVDS' notation.
    static OpenVDS::InterpolationMethod get_interpolation(
        InterpolationMethod interpolation);
};

#endif /* SEISMICHANDLE_H */
