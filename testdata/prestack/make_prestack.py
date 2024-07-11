import segyio
import numpy as np

def create_defined(path):
    """ Create file with offset data.
    | xlines-ilines | 1                  | 2                  |
    |---------------|--------------------|--------------------|
    | 10            | 100, 101, 102, 103 | 124, 125, 126, 127 |
    |               | 104, 105, 106, 107 | 128, 129, 130, 131 |
    |               | 108, 109, 110, 111 | 132, 133, 134, 135 |
    |---------------|--------------------|--------------------|
    | 11            | 112, 113, 114, 115 | 136, 137, 138, 139 |
    |               | 116, 117, 118, 119 | 140, 141, 142, 143 |
    |               | 120, 121, 122, 123 | 144, 145, 146, 147 |
    """
    spec = segyio.spec()

    # is that enough to declare/create prestack data?

    spec.sorting = 2
    spec.format = 1
    spec.samples = [4, 8, 12, 16]
    spec.ilines = [1, 2]
    spec.xlines = [10, 11]
    spec.offsets = [100, 200, 300]

    with segyio.create(path, spec) as f:
        data = 100
        tr = 0
        for il in spec.ilines:
            for xl in spec.xlines:
                for off in spec.offsets:
                    f.header[tr] = {
                        segyio.su.iline: il,
                        segyio.su.xline: xl,
                        segyio.su.offset: off,
                        segyio.su.delrt: 4,
                    }
                    data = data + len(spec.samples)
                    f.trace[tr] = np.arange(start=data - len(spec.samples),
                                            stop=data, step=1, dtype=np.single)
                    tr += 1

        f.bin.update(tsort=segyio.TraceSortingFormat.INLINE_SORTING)
