import segyio
import numpy as np


def create_intersecting_data(path, samples, ilines, xlines):

    spec = segyio.spec()
    spec.sorting = 2
    spec.format = 1
    spec.samples = samples
    spec.ilines = ilines
    spec.xlines = xlines

    # Set in_line step size for Annotated to CDP
    annotated_to_cdp_inline_step = np.array([3, 2])

    # Set cross_line step size for Annotated to CDP
    annotated_to_cdp_xline_step = np.array([-2, 3])

    # Set annotated origin
    annotated_origin_in_cdp = np.array([-3.0, -12.0])

    # Index origin in annotated coordinates
    index_origin_in_annotated = np.array([spec.ilines[0], spec.xlines[0]])

    # Calculate index origin in CDP
    index_origin_in_cdp = annotated_origin_in_cdp + \
        (index_origin_in_annotated[0] * annotated_to_cdp_inline_step +
         index_origin_in_annotated[1] * annotated_to_cdp_xline_step)

    # Calculate polar coordinates
    polar_x_increment = np.linalg.norm(annotated_to_cdp_inline_step)
    polar_y_increment = np.linalg.norm(annotated_to_cdp_xline_step)
    polar_angle = np.angle(
        complex(annotated_to_cdp_inline_step[0], annotated_to_cdp_inline_step[1]))

    print()
    print("FileName                     : ", path + ".vds")
    print("Index origin in annotated    : ", index_origin_in_annotated)
    print("Index origin in CDP          : ", index_origin_in_cdp)
    print("Annotated_origin in CDP      : ", annotated_origin_in_cdp)
    print("Annotated to CPD inline_step : ", annotated_to_cdp_inline_step)
    print("Annotated to CPD xline_step  : ", annotated_to_cdp_xline_step)
    print("Annotated to CPD polar x_inc : ", np.round(polar_x_increment, 4))
    print("Annotated to CPD polar y_inc : ", np.round(polar_y_increment, 4))
    print("Annotated to CPD polar angle : ", np.round(
        polar_angle * 360 / (2*np.pi), 4), "[0-360]")
    print()

    with segyio.create(path, spec) as f:
        tr = 0

        for iline_value in ilines:
            for xline_value in xlines:

                cdp_coordinate = 10 * (annotated_origin_in_cdp +
                                       iline_value * annotated_to_cdp_inline_step +
                                       xline_value * annotated_to_cdp_xline_step)
                f.header[tr] = {
                    segyio.su.iline: iline_value,
                    segyio.su.xline: xline_value,
                    segyio.su.cdpx: int(cdp_coordinate[0]),
                    segyio.su.cdpy: int(cdp_coordinate[1]),
                    segyio.su.scalco: -10,
                    segyio.su.delrt: samples[0],
                }

                f.trace[tr] = np.float32(samples) + \
                    np.float32((iline_value * 2**(16)) +
                               (xline_value * 2**(8)))
                tr += 1

        f.bin.update(tsort=segyio.TraceSortingFormat.INLINE_SORTING)
