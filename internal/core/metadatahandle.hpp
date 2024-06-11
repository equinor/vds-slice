#ifndef VDS_SLICE_METADATAHANDLE_HPP
#define VDS_SLICE_METADATAHANDLE_HPP

#include <OpenVDS/OpenVDS.h>
#include <string>
#include <unordered_map>

#include "axis.hpp"
#include "axis_type.hpp"
#include "boundingbox.hpp"
#include "coordinate_transformer.hpp"
#include "direction.hpp"
#include "exceptions.hpp"

using voxel = float[OpenVDS::Dimensionality_Max];

class MetadataHandle {
    friend class DoubleMetadataHandle;

public:
    virtual Axis iline() const noexcept(true) = 0;
    virtual Axis xline() const noexcept(true) = 0;
    virtual Axis sample() const noexcept(true) = 0;
    virtual Axis get_axis(Direction const direction) const noexcept(false) = 0;

    BoundingBox bounding_box() const noexcept(false);
    virtual std::string crs() const noexcept(false) = 0;
    virtual std::string input_filename() const noexcept(false) = 0;
    virtual std::string import_time_stamp() const noexcept(false) = 0;

    virtual CoordinateTransformer const& coordinate_transformer() const noexcept(false) = 0;

protected:
};

class SingleMetadataHandle : public MetadataHandle {
    friend class DoubleMetadataHandle;
public:
    static SingleMetadataHandle create(OpenVDS::VolumeDataLayout const* const layout);

    Axis iline() const noexcept(true);
    Axis xline() const noexcept(true);
    Axis sample() const noexcept(true);
    Axis get_axis(Direction const direction) const noexcept(false);
    Axis get_axis(int dimension) const noexcept(false);

    std::string crs() const noexcept(false);
    std::string input_filename() const noexcept(false);
    std::string import_time_stamp() const noexcept(false);

    SingleCoordinateTransformer const& coordinate_transformer() const noexcept(false);

protected:
    SingleMetadataHandle(OpenVDS::VolumeDataLayout const* const layout, std::unordered_map<AxisType, Axis> axes_map);

private:
    OpenVDS::VolumeDataLayout const* const m_layout;

    std::unordered_map<AxisType, Axis> m_axes_map;

    SingleCoordinateTransformer m_coordinate_transformer;
};

class DoubleMetadataHandle : public MetadataHandle {
public:
    DoubleMetadataHandle(
        SingleMetadataHandle const* const m_metadata_a,
        SingleMetadataHandle const* const m_metadata_b,
        enum binary_operator binary_symbol
    );

    Axis iline() const noexcept(true);
    Axis xline() const noexcept(true);
    Axis sample() const noexcept(true);
    Axis get_axis(Direction const direction) const noexcept(false);

    std::string crs() const noexcept(false);
    std::string input_filename() const noexcept(false);
    std::string import_time_stamp() const noexcept(false);

    DoubleCoordinateTransformer const& coordinate_transformer() const noexcept(false);

protected:

private:
    SingleMetadataHandle const* const m_metadata_a;
    SingleMetadataHandle const* const m_metadata_b;
    enum binary_operator m_binary_symbol;

    Axis m_iline;
    Axis m_xline;
    Axis m_sample;

    DoubleCoordinateTransformer m_coordinate_transformer;

    int get_dimension(std::vector<std::string> const& names) const;
    std::string operator_string() const noexcept(false);
};
#endif /* VDS_SLICE_METADATAHANDLE_HPP */
