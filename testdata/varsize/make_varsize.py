import segyio
import numpy as np
import sys


def create_varsize(path, ilines_number, xlines_number, samples_number):
    """ File with provided dimensions filled wih some data"""
    spec = segyio.spec()

    spec.sorting = 2
    spec.format = 1
    spec.ilines = range(int(ilines_number))
    spec.xlines = range(int(xlines_number))
    spec.samples = range(int(samples_number))

    print("Creating file with dimensions {}:{}:{}".format(
        ilines_number, xlines_number, samples_number))

    # We use scaling constant of -10, meaning that values will be divided by 10
    il_step_x = int(1.1 * 10)
    il_step_y = int(0 * 10)
    xl_step_x = int(0 * 10)
    xl_step_y = int(3.3 * 10)
    ori_x = int(1 * 10)
    ori_y = int(3 * 10)

    with segyio.create(path, spec) as f:
        data = -5
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
                }
                data = data + 0.00001
                f.trace[tr] = np.linspace(
                    start=data, stop=data+2, num=len(spec.samples), dtype=np.single)
                tr += 1

        f.bin.update(tsort=segyio.TraceSortingFormat.INLINE_SORTING)


if __name__ == "__main__":
    path = sys.argv[1]
    ilines = sys.argv[2]
    xlines = sys.argv[3]
    samples = sys.argv[4]
    create_varsize(path, ilines, xlines, samples)
