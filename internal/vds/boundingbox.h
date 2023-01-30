#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H

#include <utility>
#include <vector>

#include <OpenVDS/OpenVDS.h>
#include <OpenVDS/IJKCoordinateTransformer.h>

namespace vds {

/// @brief Class that describes the 2D (horizontal) bounding box in
///        different coordinate systems.
///
/// The supported coordinate systems are INDEX, ANNOTATION, and WORLD.
class BoundingBox {
public:
    /// @brief Constructor
    /// @param layout Pointer to VolumeDataLayout of the open VDS.
    explicit BoundingBox(
        const OpenVDS::VolumeDataLayout *layout
    ) : layout(layout)
    {
        transformer = OpenVDS::IJKCoordinateTransformer(layout);
    }

    /// @brief Get the bounding box in the index coordinate system.
    /// @return Vector of four pairs containing the inline, crossline index
    //          bounding points.
    std::vector< std::pair<int, int> >       index()      noexcept (true);
    /// @brief Get the bounding box in the annotation coordinate system.
    /// @return Vector of four pairs containing the annotation coordinates of the
    ///         bounding points.
    std::vector< std::pair<int, int> >       annotation() noexcept (true);
    /// @brief Get the bounding box in the world coordinate system.
    /// @return Vector of four pairs containing the world coordinates of the
    ///         bounding points.
    std::vector< std::pair<double, double> > world()      noexcept (true);
private:
    /// @brief Coordinate transformer that allows thoe conveniently transform
    ///        between OpenVDS' coordinate systems.
    OpenVDS::IJKCoordinateTransformer transformer;
    /// @brief Pointer to VolumeDataLayout of open VDS.
    const OpenVDS::VolumeDataLayout*  layout;
};

} /* namespace vds */

#endif /* BOUNDINGBOX_H */
