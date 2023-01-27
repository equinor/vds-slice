#ifndef AXIS_H
#define AXIS_H

#include <memory>
#include <string>

#include <OpenVDS/OpenVDS.h>

#include "boundingbox.h"
#include "vds.h"

// TODO: We could try to derive Axis from OpenVDS::VolumeDataAxisDescriptor, to
//       avoid pure wrapping, but then we are bound to naming of OpenVDS.
class Axis {
    private:
    const ApiAxisName apiAxisName;
    int vdsIndex;
    int apiIndex;
    OpenVDS::VolumeDataAxisDescriptor vdsAxisDescriptor;

    public:
    Axis(
        const ApiAxisName apiAxisName,
        OpenVDS::VolumeDataLayout const * vdsLayout
    );
    int getNumberOfPoints() const;
    int getMin() const;
    int getMax() const;
    std::string getUnit() const;
    int getVdsIndex() const;
    int getApiIndex() const; // TODO: Can I get rid of this? It is currently
                             // needed in boundary validation of fence requests.

    std::string getVdsName() const;
    std::string getApiName() const;

    CoordinateSystem getCoordinateSystem() const;
};

#endif /* AXIS_H */
