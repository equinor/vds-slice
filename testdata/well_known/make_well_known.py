import segyio
import numpy as np

def create_defined(path):
    """ Create file with simple constant data.
    | xlines-ilines | 1                  | 3                  | 5                  |
    |---------------|--------------------|--------------------|--------------------|
    | 10            | 100, 101, 102, 103 | 108, 109, 110, 111 | 116, 117, 118, 119 |
    | 11            | 104, 105, 106, 107 | 112, 113, 114, 115 | 120, 121, 122, 123 |

    UTM coordinates for headers:
    | xlines-ilines | 1           | 3             | 5             |
    |---------------|-------------|---------------|---------------|
    | 10            | x=2, y=0    | x=8, y=4      | x=14, y=8     |
    | 11            | x=0, y=3    | x=6, y=7      | x=12, y=11    |
    """
    spec = segyio.spec()

    spec.sorting = 2
    spec.format = 1
    spec.samples = [4, 8, 12, 16]
    spec.ilines = [1, 3, 5]
    spec.xlines = [10, 11]

    # We use scaling constant of -10, meaning that values will be divided by 10
    il_step_x = int(3 * 10)
    il_step_y = int(2* 10)
    xl_step_x = int(-2 * 10)
    xl_step_y = int(3 * 10)
    ori_x = int(2 * 10)
    ori_y = int(0 * 10)

    with segyio.create(path, spec) as f:
        data = 100
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
                data = data + len(spec.samples)
                f.trace[tr] = np.arange(start=data - len(spec.samples),
                                        stop=data, step=1, dtype=np.single)
                tr += 1

        f.bin.update(tsort=segyio.TraceSortingFormat.INLINE_SORTING)
