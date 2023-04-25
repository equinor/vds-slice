import segyio
import numpy as np
import sys


def create_10_samples(path):
    """ Create file with 10 values in each trace.
    | xlines-ilines | 1           | 3             | 5             |
    |---------------|-------------|---------------|---------------|
    | 10            | -9, -9 ...  |  -11, -7 ...  |  -12, -9 ...  |
    | 11            | -9, -8 ...  |  -10, -8 ...  |  -12, -8 ...  |

    UTM coordinates for headers:
    | xlines-ilines | 1           | 3             | 5             |
    |---------------|-------------|---------------|---------------|
    | 10            | x=2, y=0    | x=8, y=4      | x=14, y=8     |
    | 11            | x=0, y=3    | x=6, y=7      | x=12, y=11    |
    """
    spec = segyio.spec()

    spec.sorting = 2
    spec.format = 1
    spec.samples = [4, 8, 12, 16, 20, 24, 28, 32, 36, 40]
    spec.ilines = [1, 3, 5]
    spec.xlines = [10, 11]

    # We use scaling constant of -10, meaning that values will be divided by 10
    il_step_x = int(3 * 10)
    il_step_y = int(2 * 10)
    xl_step_x = int(-2 * 10)
    xl_step_y = int(3 * 10)
    ori_x = int(2 * 10)
    ori_y = int(0 * 10)

    data = [
        [-9, -9, -6, -7, -3, 1, -3, 2, 7, 8],
        [-9, -8, -6, -4, -1, 3, 0, 4, 9, 7],

        [-11, -7, -7, -7, -8, -1, 0, 4, 6, 8],
        [-10, -8, -5, -1, 2, 3, 5, 10, 9, 10],

        [-12, -9, -8, -9, -3, 2, -1, 2, 2, 4],
        [-12, -8, -9, -7, -4, -3, -1, -1, 3, 3],
    ]

    with segyio.create(path, spec) as f:
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
                f.trace[tr] = data[tr]
                tr += 1

        f.bin.update(tsort=segyio.TraceSortingFormat.INLINE_SORTING)


if __name__ == "__main__":
    path = sys.argv[1]
    create_10_samples(path)