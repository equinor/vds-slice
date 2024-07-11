import segyio
import numpy as np


def create_with_format(path, format):
    """ Create file with simple constant data written in provided format.
    | xlines-ilines | 1           | 3           | 5           |
    |---------------|-------------|-------------|-------------|
    | 10            | 40, ..., 49 | 60, ..., 69 | 80, ..., 89 |
    | 11            | 50, ..., 59 | 70, ..., 79 | 90, ..., 99 |
    """
    spec = segyio.spec()

    spec.sorting = 2
    spec.format = format
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

    with segyio.create(path, spec) as f:
        data = 40
        tr = 0
        for il in spec.ilines:
            for xl in spec.xlines:
                f.header[tr] = {
                    segyio.su.iline: il,
                    segyio.su.xline: xl,
                    segyio.su.delrt: 4,
                    segyio.su.cdpx:
                        (il - spec.ilines[0]) * il_step_x +
                        (xl - spec.xlines[0]) * xl_step_x +
                        ori_x,
                    segyio.su.cdpy:
                        (il - spec.ilines[0]) * il_step_y +
                        (xl - spec.xlines[0]) * xl_step_y +
                        ori_y,
                    segyio.su.scalco: -10,
                }
                data = data + len(spec.samples)
                f.trace[tr] = np.arange(start=data - len(spec.samples),
                                        stop=data, step=1, dtype=np.single)
                tr += 1

        f.bin.update(tsort=segyio.TraceSortingFormat.INLINE_SORTING)
