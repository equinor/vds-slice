#ifndef POSTSTACK_H
#define POSTSTACK_H

#include <list>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "seismichandle.h"
#include "vds.h"

/// @brief Labels for first (X), second (Y) and third (Z) axis direction
///        of currently used coordinate system. Independent of whether the
///        actual coordinate system is Cartesian or not.
enum AxisDirection : int {
    X=0,
    Y=1,
    Z=2
};

/// @brief Class handling access to post-stack data stored in a VDS.
class PostStackHandle : public SeismicHandle {

public:
    /// @brief Constructor.
    /// @param url Connection URL.
    /// @param conn Connections string.
    PostStackHandle(std::string url, std::string conn);
    /// @brief Get slice data.
    /// @param axis Axis from which data shall be extracted.
    /// @param line_number Line number from which data shall be extracted.
    /// @param level_of_detail Level of detail for which data shall be extracted.
    /// @param channel Channel for which data shall be extracted.
    /// @return Requested data and/or error code of request.
    requestdata get_slice(
        const Axis axis,
        const int           line_number,
        const LevelOfDetail level_of_detail = LevelOfDetail::Default,
        const Channel       channel = Channel::Default
    );
    /// @brief Get fence data.
    /// @param coordinate_system Coordinate system the request is using to
    ///                          access data.
    /// @param coordinates List of coordinate pairs (x0,y0,x1,x2...,xN,yN) for
    ///                    which data shall be extracted.
    /// @param npoints Number of points in the list of coordinate pairs (N).
    /// @param interpolation_method Interpolation method to be used.
    /// @param level_of_detail Level of detail for which data shall be extracted.
    /// @param channel Channel for which data shall be extracted.
    /// @return Requested data and/or error code of request.
    requestdata get_fence(
        const CoordinateSystem    coordinate_system,
        float const *             coordinates,
        const size_t              npoints,
        const InterpolationMethod interpolation_method,
        const LevelOfDetail       level_of_detail = LevelOfDetail::Default,
        const Channel             channel = Channel::Default
    );

protected:
    /// @brief Computes the subvolume, i.e., the voxel coordinate bounds, of the
    ///        request.
    /// @param axis Axis from which data shall be extracted.
    /// @param lineno Line number from which data shall be extracted.
    /// @return Voxel coordinates that shall be extracted from the VDS.
    SubVolume slice_as_subvolume(
        const AxisDescriptor& axis_desc,
        const int lineno
    ) const;
    /// @brief Computes the point list of the fence request in the voxel
    ///        coordinate system.
    /// @param coordinate_system Coordinate system the request is using to
    ///                          access data.
    /// @param coordinates List of coordinate pairs (x0,y0,x1,x2...,xN,yN) for
    ///                    which data shall be extracted.
    /// @param npoints Number of points in the list of coordinate pairs (N).
    /// @param interpolation_method Interpolation method to be used.
    /// @return Points of the fence request transformed to the OpenVDS voxel
    ///         coordinate system. Each point is represented by a
    ///         "OpenVDS::Dimensionality_Max"-dimensional array.
    std::unique_ptr< float[][OpenVDS::Dimensionality_Max] > fence_as_point_list(
        const CoordinateSystem    coordinate_system,
        float const *             coordinates,
        const std::size_t         npoints,
        const InterpolationMethod interpolation_method) const;
    /// @brief Get data within the specified subvolume
    /// @param subvolume Subvolume with the bounds of the request in the voxel
    ///                  coordinate system.
    /// @param level_of_detail Level of detail for which data shall be extracted.
    /// @param channel Channel for which data shall be extracted.
    /// @return Requested data and/or error code of request.
    requestdata get_subvolume(
        const SubVolume subvolume,
        const LevelOfDetail level_of_detail,
        const Channel channel );
    /// @brief Get data at the specified points.
    /// @param points List of points in voxel coordinate system, for which data
    ///               shall be extracted. Each point is represented by a
    ///               "OpenVDS::Dimensionality_Max"-dimensional array.
    /// @param level_of_detail Level of detail for which data shall be extracted.
    /// @param channel Channel for which data shall be extracted.
    /// @return Requested data and/or error code of request.
    requestdata get_volume_trace(
        const std::unique_ptr< float[][OpenVDS::Dimensionality_Max] > points,
        const std::size_t npoints,
        const InterpolationMethod interpolation_method,
        const LevelOfDetail level_of_detail,
        const Channel channel );
    /// @brief Helper function to wait for a OpenVDS data request to finish and
    ///        to wrap the obtained data in a `requestdata` struct.
    /// @tparam REQUEST_TYPE Parametrization of the OpenVDS request type which
    ///         differs for slice and fence request.
    /// @param request Pointer to the request object.
    /// @param error_message Error message to throw in an exception if the
    ///                      request fails.
    /// @param data Pointer to memory into which OpenVDS writes the requested
    ///             data on successful request.
    /// @param size Number of data entries in the memory on successful request.
    /// @return Requested data and/or error code of request.
    template <typename REQUEST_TYPE>
    requestdata finalize_request(
        const std::shared_ptr<REQUEST_TYPE>& request,
        const std::string                    error_message,
        std::unique_ptr<char[]>&             data,
        const std::size_t                    size) const;
    /// @brief Class that validates basic assumptions about post-stack data
    ///        stored in VDS format.
    ///
    /// At the moment it checks whether the axes are in the expected order.
    /// We expect the axes in order sample, crossline, and inline axis.
    class PostStackValidator {
        /// @brief Map of the allowed combinations of names and units for the
        ///        third (time/depth/sample) axis. The axis names are keys of
        ///        map which refers to a list of valid unit names.
        static const std::unordered_map<std::string, std::list<std::string>> valid_z_axis_combinations_;
        /// @brief Handle to open VDS dataset.
        const PostStackHandle& handle_;
        /// @brief Helper function to validate that the coordinate axes are in
        ///        the correct order.
        void validate_axes_order();

    public:
        /// @brief Constructor
        /// @param handle Handle to open VDS.
        PostStackValidator( const PostStackHandle& handle );
        /// @brief Validates that the post-stack assumptions are valid for the
        ///        open VDS.
        void validate();
        /// @brief Get the valid (allowed) combinations of z(=sample) axis names
        ///        and units.
        /// @return A const reference to the map of valid names for the z-axis
        ///         and the corresponding valid units in that case.
        static const std::unordered_map<std::string, std::list<std::string>>&
        get_valid_z_axis_combinations() {
            return valid_z_axis_combinations_;
        }
    };

    /// @brief Class that validates basic assumptions about post-stack data when
    ///        a slice is requested.
    ///
    /// @note It check whether the axis in the request and the unit of the
    ///       coordinate system, i.e., the third axis is sound. It does not
    ///       check the actual units stored in the VDS.
    class SliceRequestValidator {
        public:
        /// @brief Constructor
        /// @param handle Handle to open VDS.
        SliceRequestValidator();
        /// @brief Validate whether the slice request is valid for the specified
        ///        axis direction.
        /// @param axis Axis along which the slice is obtained.
        void validate(const AxisDescriptor& axis_desc);
    };
};

template <typename REQUEST_TYPE>
requestdata PostStackHandle::finalize_request(
    const std::shared_ptr<REQUEST_TYPE>& request,
    const std::string                    error_message,
    std::unique_ptr<char[]>&             data,
    const std::size_t                    size) const {

    const bool success = request.get()->WaitForCompletion();
    if (not success) {
        throw std::runtime_error(error_message);
    }

    requestdata tmp{data.get(), nullptr, size};
    data.release();

    return tmp;
}

#endif /* POSTSTACK_H */
