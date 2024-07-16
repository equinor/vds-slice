from pathlib import Path
import tempfile

import numpy as np
from .make_intersecting_cubes import create_intersecting_data

import common


base_range_4 = np.arange(1, 4+1, dtype=np.int32)
base_range_8 = np.arange(1, 8+1, dtype=np.int32)
base_range_32 = np.arange(1, 32+1, dtype=np.int32)


class Case:
    def __init__(self, samples, ilines, xlines, spec):
        self.samples = samples
        self.ilines = ilines
        self.xlines = xlines
        self.spec = spec


cases = [
    Case(
        samples=(0+base_range_32)*4,
        ilines=(0+base_range_8)*3,
        xlines=(0+base_range_8)*2,
        spec=common.CommandSpecification(
            case="regular_8x2",
            description="Sample: [4, 128], Inline: [3, 24], Xline: [2, 16]",
            filename="regular_8x2_cube.vds",
        ),
    ),
    Case(
        samples=(4+base_range_32)*4,
        ilines=(4+base_range_8)*3,
        xlines=(4+base_range_8)*2,
        spec=common.CommandSpecification(
            case="shift_4_8x2",
            description="Sample: [20, 144], Inline: [15, 36], Xline: [10, 24]",
            filename="shift_4_8x2_cube.vds",
        ),
    ),
    Case(
        samples=(30+base_range_32)*4,
        ilines=(6+base_range_8)*3,
        xlines=(6+base_range_8)*2,
        spec=common.CommandSpecification(
            case="shift_6_8x2",
            description="Sample: [124, 248], Inline: [21, 42], Xline: [14, 28]",
            filename="shift_6_8x2_cube.vds",
        ),
    ),
    Case(
        samples=base_range_32*4,
        ilines=(7+base_range_8)*3,
        xlines=base_range_8*2,
        spec=common.CommandSpecification(
            case="shift_7_inLine_8x2",
            description="Sample: [4, 128], Inline: [24, 45], Xline: [2, 16]",
            filename="shift_7_inLine_8x2_cube.vds",
        ),
    ),
    Case(
        samples=base_range_32*4,
        ilines=base_range_8*3,
        xlines=(7+base_range_8)*2,
        spec=common.CommandSpecification(
            case="shift_7_xLine_8x2",
            description="Sample: [4, 128], Inline: [3, 24], Xline: [16, 30]",
            filename="shift_7_xLine_8x2_cube.vds",
        ),
    ),
    Case(
        samples=(8+base_range_32)*4,
        ilines=(8+base_range_32)*3,
        xlines=(8+base_range_32)*2,
        spec=common.CommandSpecification(
            case="shift_8_32x3",
            description="Sample: [36, 160], Inline: [27, 120], Xline: [18, 80]",
            filename="shift_8_32x3_cube.vds",
        ),
    ),
    Case(
        samples=(31+base_range_32)*4,
        ilines=base_range_8*3,
        xlines=base_range_8*2,
        spec=common.CommandSpecification(
            case="shift_31_Sample_8x2",
            description="Sample: [128, 252], Inline: [3, 24], Xline: [2, 16]",
            filename="shift_31_Sample_8x2_cube.vds",
        ),
    ),
    Case(
        samples=(1+base_range_8)*4,
        ilines=(2+base_range_4)*3,
        xlines=(3+base_range_4)*2,
        spec=common.CommandSpecification(
            case="inner_4x2",
            description="Sample: [8, 36], Inline: [9, 18], Xline: [8, 14]. " +
                        "Samples: 1 sample from 0, inline: 2 samples from 0, xline: 3 samples from 0.",
            filename="inner_4x2_cube.vds",
        ),
    ),
    Case(
        samples=(0+base_range_8)*3,
        ilines=(0+base_range_4)*2,
        xlines=(0+base_range_4)*4,
        spec=common.CommandSpecification(
            case="unaligned_stepsize",
            description="Sample: [3, 24], Inline: [2, 8], Xline: [4, 16]",
            filename="unaligned_stepsize_cube.vds",
        ),
    ),
    Case(
        samples=(0+base_range_8)*4 + 3,
        ilines=(0+base_range_4)*3 + 2,
        xlines=(0+base_range_4)*2 + 1,
        spec=common.CommandSpecification(
            case="unaligned_shift",
            description="Sample: [7, 35], Inline: [5, 14], Xline: [3, 9]",
            filename="unaligned_shift_cube.vds",
        ),
    ),
]


class CubeIntersectionCategory(common.Category):
    def __init__(self):
        super().__init__(
            name="cube_intersection",
            description="Files to test Double features (retrieving data from intersection of two cubes).",
            dirpath="cube_intersection"
        )

    def specifications(self):
        return [case.spec for case in cases]

    def generate_testdata(self, testdata: common.TestdataCase):
        case: Case = next(c for c in cases if c.spec.case == testdata.case)

        with tempfile.TemporaryDirectory() as tmp:
            segy_name = Path(testdata.filepath).with_suffix('.segy').name
            segy_path = str(Path(tmp) / segy_name)
            create_intersecting_data(
                segy_path, case.samples, case.ilines, case.xlines)
            common.segy_to_vds(testdata, segy_path, crs="utmXX")
