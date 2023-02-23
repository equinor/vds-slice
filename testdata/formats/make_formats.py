import segyio
import numpy as np
import sys


def create_with_format(path, format):
    """ Create file with simple constant data written in provided format.
    | xlines-ilines | 1                  | 3                  | 5                  |
    |---------------|--------------------|--------------------|--------------------|
    | 10            | 100, 101, 102, 103 | 108, 109, 110, 111 | 116, 117, 118, 119 |
    | 11            | 104, 105, 106, 107 | 112, 113, 114, 115 | 120, 121, 122, 123 |
    """
    spec = segyio.spec()

    spec.sorting = 2
    spec.format = format
    spec.samples = [4, 8, 12, 16]
    spec.ilines = [1, 3, 5]
    spec.xlines = [10, 11]

    with segyio.create(path, spec) as f:
        data = 100
        tr = 0
        for il in spec.ilines:
            for xl in spec.xlines:
                f.header[tr] = {
                    segyio.su.iline: il,
                    segyio.su.xline: xl,
                    segyio.su.delrt: 4,
                }
                data = data + len(spec.samples)
                f.trace[tr] = np.arange(start=data - len(spec.samples),
                                        stop=data, step=1, dtype=np.single)
                tr += 1

        f.bin.update(tsort=segyio.TraceSortingFormat.INLINE_SORTING)


if __name__ == "__main__":
    path = sys.argv[1]
    format = sys.argv[2]
    create_with_format(path, format)
