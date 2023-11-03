import segyio
import numpy as np
import sys


def create_dim_data(path, 
                    ilines_number, xlines_number, samples_number,
                    ilines_offset, xlines_offset, samples_offset,
                    ilines_step, xlines_step, samples_step):
    """ File with provided dimensions filled wih some data"""
    spec = segyio.spec()

    spec.sorting = 2
    spec.format = 1
    spec.ilines = range (ilines_offset, ilines_offset+ (ilines_number*ilines_step), ilines_step)
    spec.xlines = range (xlines_offset, xlines_offset+ (xlines_number*xlines_step), xlines_step)
    spec.samples = range (samples_offset, samples_offset+ (samples_number*samples_step), samples_step)

    print(f"Creating file with dimensions {ilines_number}:{xlines_number}:{samples_number}")

    # We use scaling constant of -10, meaning that values will be divided by 10
    # note that lines are not perpendicular
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
                f.trace[tr] = np.linspace(start=data, stop=data+2, num=len(spec.samples), dtype=np.single)
                tr += 1

        f.bin.update(tsort=segyio.TraceSortingFormat.INLINE_SORTING)


if __name__ == "__main__":
    path = sys.argv[1]
    ilines = int(sys.argv[2])
    xlines = int(sys.argv[3])
    samples = int(sys.argv[4])
    ilines_offset = int(sys.argv[5])
    xlines_offset = int(sys.argv[6])
    samples_offset = int(sys.argv[7])
    ilines_step = int(sys.argv[8])
    xlines_step = int(sys.argv[9])
    samples_step = int(sys.argv[10])

    print(f"Creating file {path}")
    print(f"ilines: {ilines}")
    print(f"xlines: {xlines}")
    print(f"samples: {samples}")
    print(f"ilines_offset: {ilines_offset}")
    print(f"xlines_offset: {xlines_offset}")
    print(f"samples_offset: {samples_offset}")
    print(f"ilines_step: {ilines_step}")
    print(f"xlines_step: {xlines_step}")
    print(f"samples_step: {samples_step}")

    create_dim_data(path, ilines, xlines, samples, ilines_offset, xlines_offset, samples_offset, ilines_step, xlines_step, samples_step)
