#ifndef AXIS_H
#define AXIS_H

#include <memory>
#include <string>

#include <OpenVDS/OpenVDS.h>

#include "vds.h"

class Axis {
public:
    Axis(
        const api_axis_name               api_name,
        OpenVDS::VolumeDataLayout const * layout
    );

    int get_number_of_points() const noexcept(true);

    int get_min() const noexcept(true);
    int get_max() const noexcept(true);

    std::string get_unit() const noexcept(true);

    int get_vds_index() const noexcept(true);
    int get_api_index() const noexcept(true);

    std::string get_vds_name() const noexcept(true);
    std::string get_api_name() const;

    coordinate_system get_coordinate_system() const;
private:
    const api_axis_name api_name;
    int vds_index;
    int api_index;
    OpenVDS::VolumeDataAxisDescriptor vds_axis_descriptor;
};

#endif /* AXIS_H */
