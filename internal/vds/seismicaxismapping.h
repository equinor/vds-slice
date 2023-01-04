#ifndef SEISMICMAP_H
#define SEISMICMAP_H


/// @brief Mapping between API coordinate system and VDS coordinate system axis indices
////       base class
class SeismicAxisMap {

public:
    /// @brief Dimension index of "Inline" axis in VDS coordinate system
    virtual int iline()  const = 0;
    /// @brief Dimension index of "Crossline" axis in VDS coordinate system
    virtual int xline()  const = 0;
    /// @brief Dimension index of "Sample" axis in VDS coordinate system
    virtual int sample() const = 0;
    /// @brief Optional: Dimension index of "Offset" axis in VDS coordinate system
    virtual int offset() const = 0; // Planing for a future of Prestack support
    /// @brief Maps a VDS voxel dimension to spatial (request) dimension
    virtual int dimension_from( const int voxel ) const = 0;
    /// @brief Maps a spatial (request) dimension to VDS voxel dimension
    virtual int voxel_from( const int dimension ) const = 0;

    virtual ~SeismicAxisMap() {}
};

/// @brief Mapping between API coordinate system and VDS coordinate system axis indices
///        for post stack data.
///
/// For post-stack data it is expected that one has a three dimensional coordinate
/// system and that the inline, crossline and sample direction are present in the VDS
/// dataset. In the setting of VDS it means that one expects the annotation system to
/// be defined.
class PostStackAxisMap : public SeismicAxisMap {
public:

    PostStackAxisMap(int i, int x, int s);

    int iline()  const override final;
    int xline()  const override final;
    int sample() const override final;
    int offset() const override final;
    int dimension_from( const int voxel ) const override final;
    int voxel_from( const int dimension ) const override final;

private:
    const int inline_axis_id_;
    const int crossline_axis_id_;
    const int sample_axis_id_;
};

#endif /* SEISMICMAP_H */
