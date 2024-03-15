import segyio
import numpy as np
import sys
import subprocess


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
    annotated_origin = np.array([-3.0, -12.0])

    # Index origin in annotated coordinates
    index_origin_in_annotated = np.array([spec.ilines[0], spec.xlines[0]])

    # Calculate index origin in CDP
    index_origin_in_cdp = annotated_origin + \
        (index_origin_in_annotated[0] * annotated_to_cdp_inline_step +
         index_origin_in_annotated[1] * annotated_to_cdp_xline_step)

    print()
    print("FileName: ", path)
    print("annotated_to_CPD_inline_step", annotated_to_cdp_inline_step)
    print("annotated_to_CPD_xline_step", annotated_to_cdp_xline_step)
    print("annotated_origin", annotated_origin)
    print("index_origin_in_annotated", index_origin_in_annotated)
    print("index_origin_in_CDP", index_origin_in_cdp)

    with segyio.create(path + ".segy", spec) as f:
        tr = 0

        for iline_value in ilines:
            for xline_value in xlines:

                cdp_coordinate = 10 * (annotated_origin +
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

                f.trace[tr] = samples + \
                    ((iline_value * 2**(16)) + (xline_value * 2**(8)))
                tr += 1

        f.bin.update(tsort=segyio.TraceSortingFormat.INLINE_SORTING)


if __name__ == "__main__":
    base_range_8 = np.arange(1, 8+1)
    base_range_32 = np.arange(1, 32+1)

    parameters = [
        {"path": "regular_8x3_cube",
         "samples": (0+base_range_32)*4,
         "ilines": (0+base_range_8)*3,
         "xlines": (0+base_range_8)*2},
        {"path": "shift_4_8x3_cube",
         "samples": (4+base_range_32)*4,
         "ilines": (4+base_range_8)*3,
         "xlines": (4+base_range_8)*2}
    ]

    for p in parameters:
        create_intersecting_data(**p)
        name = p["path"]
        subprocess.run(["SEGYImport", "--url", "file://.", "--vdsfile",
                       name+".vds", name+".segy", "--crs-wkt=\"utmXX\""])
        subprocess.run(["rm", name+".segy"])

# Copy to cloud require a fresh sas token.
# VDSCopy "regular_8x3_cube.vds"  "azureSAS://onevdstest.blob.core.windows.net/testdata/cube_intersection/regular_8x3_cube"  --compression-method=None -d "Suffix=?$SAS"
# VDSCopy "shift_4_8x3_cube.vds"  "azureSAS://onevdstest.blob.core.windows.net/testdata/cube_intersection/shift_4_8x3_cube"  --compression-method=None -d "Suffix=?$SAS"
