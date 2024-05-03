import segyio
import numpy as np
import sys
import subprocess


def create_10_samples(path, samples, ilines, xlines, factor):
    """ Create file with 10 values in each trace.
    | xlines-ilines | 1               | 3                 | 5                 |
    |---------------|-----------------|-------------------|-------------------|
    | 10            | -4.5, -3.5 ...  |  25.5, -14.5 ...  |   25.5,  4.5 ...  |
    | 11            |  4.5,  3.5 ...  |  25.5,   0.5 ...  |   25.5, -4.5 ...  |

    UTM coordinates for headers:
    | xlines-ilines | 1           | 3             | 5             |
    |---------------|-------------|---------------|---------------|
    | 10            | x=2, y=0    | x=8, y=4      | x=14, y=8     |
    | 11            | x=0, y=3    | x=6, y=7      | x=12, y=11    |
    """
    spec = segyio.spec()

    spec.sorting = 2
    spec.format = 1
    spec.samples = samples
    spec.ilines = ilines
    spec.xlines = xlines

    # We use scaling constant of -10, meaning that values will be divided by 10
    il_step_x = int(3 * 10)
    il_step_y = int(2 * 10)
    xl_step_x = int(-2 * 10)
    xl_step_y = int(3 * 10)
    ori_x = int(2 * 10)
    ori_y = int(0 * 10)

    data = [
        # Traces linear in range [0, len(trace)]
        #  4       8     12     16     20     24     28     32     36     40
        [-4.5,  -3.5,  -2.5,  -1.5,  -0.5,   0.5,   1.5,   2.5,   3.5,   4.5],
        [4.5,   3.5,   2.5,   1.5,   0.5,  -0.5,  -1.5,  -2.5,  -3.5,  -4.5],
        # Traces linear in range [1:-1]
        [25.5, -14.5, -12.5, -10.5,  -8.5,  -6.5,  -4.5,  -2.5,  -0.5,  25.5],
        [25.5,   0.5,   2.5,   4.5,   6.5,   8.5,  10.5,  12.5,  14.5,  25.5],
        # Traces linear in range [1:6] and [6:len(trace)]
        [25.5,   4.5,   8.5,  12.5,  16.5,  20.5,  24.5,  20.5,  16.5,   8.5],
        [25.5,  -4.5,  -8.5, -12.5, -16.5, -20.5, -24.5, -20.5, -16.5,  -8.5],
    ]

    with segyio.create(path + ".segy", spec) as f:
        tr = 0
        for il in spec.ilines:
            for xl in spec.xlines:
                f.header[tr] = {
                    segyio.su.iline: il,
                    segyio.su.xline: xl,
                    segyio.su.cdpx:
                        (il - spec.ilines[0]) * il_step_x +
                        (xl - spec.xlines[0]) * xl_step_x +
                        ori_x,
                    segyio.su.cdpy:
                        (il - spec.ilines[0]) * il_step_y +
                        (xl - spec.xlines[0]) * xl_step_y +
                        ori_y,
                    segyio.su.scalco: -10,
                    segyio.su.delrt: 4,
                }
                f.trace[tr] = np.array(data[tr], dtype=np.float32)*factor
                tr += 1

        f.bin.update(tsort=segyio.TraceSortingFormat.INLINE_SORTING)


if __name__ == "__main__":
    parameters = [
        {"path": "10_double_value", "samples": [4, 8, 12, 16, 20, 24, 28, 32, 36, 40], "ilines": [
            1, 3, 5], "xlines": [10, 11], "factor": 2},
    ]
    for p in parameters:
        create_10_samples(**p)
        name = p["path"]
        subprocess.run(["SEGYImport", "--url", "file://.", "--vdsfile",
                       name+".vds", name+".segy", "--crs-wkt=\"utmXX\""])
        subprocess.run(["rm", name+".segy"])
