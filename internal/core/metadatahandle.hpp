#ifndef ONESEISMIC_API_METADATAHANDLE_HPP
#define ONESEISMIC_API_METADATAHANDLE_HPP

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
public:
    Axis iline() const noexcept(true);
    Axis xline() const noexcept(true);
    Axis sample() const noexcept(true);
    Axis get_axis(Direction const direction) const noexcept(false);

    BoundingBox bounding_box() const noexcept(false);
    virtual std::string crs() const noexcept(false) = 0;
    virtual std::string input_filename() const noexcept(false) = 0;
    virtual std::string import_time_stamp() const noexcept(false) = 0;

    virtual CoordinateTransformer const& coordinate_transformer() const noexcept(false) = 0;

protected:
    MetadataHandle(std::unordered_map<AxisType, Axis> axes_map);

    std::unordered_map<AxisType, Axis> m_axes_map;
};

class SingleMetadataHandle : public MetadataHandle {
    friend class DoubleMetadataHandle;
public:
    static SingleMetadataHandle create(OpenVDS::VolumeDataLayout const* const layout);

    std::string crs() const noexcept(false);
    std::string input_filename() const noexcept(false);
    std::string import_time_stamp() const noexcept(false);

    SingleCoordinateTransformer const& coordinate_transformer() const noexcept(false);

protected:
    SingleMetadataHandle(OpenVDS::VolumeDataLayout const* const layout, std::unordered_map<AxisType, Axis> axes_map);

private:
    OpenVDS::VolumeDataLayout const* const m_layout;

    SingleCoordinateTransformer m_coordinate_transformer;
};

class DoubleMetadataHandle : public MetadataHandle {
public:
    static DoubleMetadataHandle create(
        SingleMetadataHandle const* const metadata_a,
        SingleMetadataHandle const* const metadata_b,
        enum binary_operator binary_symbol
    );

    std::string crs() const noexcept(false);
    std::string input_filename() const noexcept(false);
    std::string import_time_stamp() const noexcept(false);

    DoubleCoordinateTransformer const& coordinate_transformer() const noexcept(false);

protected:
    DoubleMetadataHandle(
        SingleMetadataHandle const* const metadata_a,
        SingleMetadataHandle const* const metadata_b,
        std::unordered_map<AxisType, Axis> axes_map,
        enum binary_operator binary_symbol
    );

private:
    SingleMetadataHandle const* const m_metadata_a;
    SingleMetadataHandle const* const m_metadata_b;
    enum binary_operator m_binary_symbol;

    DoubleCoordinateTransformer m_coordinate_transformer;

    std::string operator_string() const noexcept(false);
};
#endif /* ONESEISMIC_API_METADATAHANDLE_HPP */
