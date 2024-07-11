from pathlib import Path
import tempfile

from .make_10_samples import create_10_samples

import common


class Case:
    def __init__(self, samples, ilines, xlines, spec, factor=1, crs="utmXX"):
        self.samples = samples
        self.ilines = ilines
        self.xlines = xlines
        self.spec = spec
        self.factor = factor
        self.crs = crs


cases = [
    Case(
        samples=[4, 8, 12, 16, 20, 24, 28, 32, 36, 40],
        ilines=[1, 3, 5],
        xlines=[10, 11],
        spec=common.CommandSpecification(
            case="default",
            description="Default 10 samples test file",
            filename="10_samples_default.vds",
            cloud_path="testdata/10_samples/10_samples_default"
        )
    ),
    Case(
        samples=[4, 8, 12, 16, 20, 24, 28, 32, 36, 40],
        ilines=[1, 3, 5],
        xlines=[10, 11],
        crs="utmXX_modified",
        spec=common.CommandSpecification(
            case="different_crs",
            description="Default 10 samples file, but with unusual crs",
            filename="10_default_crs.vds"
        )
    ),
    Case(
        samples=[4, 8, 12, 16, 20, 24, 28, 32, 36, 40],
        ilines=[1, 3, 5],
        xlines=[10, 11],
        factor=2,
        spec=common.CommandSpecification(
            case="double_value",
            description="As default, but values are multiplied by 2",
            filename="10_double_value.vds"
        )
    ),
    Case(
        samples=[4, 8, 12, 16, 20, 24, 28, 32, 36, 40],
        ilines=[1, 3, 5],
        xlines=[10],
        spec=common.CommandSpecification(
            case="single_xline",
            description="File contains just a single xline",
            filename="10_single_xline.vds"
        )
    ),
    Case(
        samples=[4],
        ilines=[1, 3, 5],
        xlines=[10, 11],
        spec=common.CommandSpecification(
            case="single_sample",
            description="File contains just a single sample (not 10)",
            filename="10_single_sample.vds"
        )
    ),
    Case(
        samples=[4, 8],
        ilines=[1, 3],
        xlines=[10, 11],
        spec=common.CommandSpecification(
            case="minimal_dimensions",
            description="Minimal viable cube 2 x 2 x 2",
            filename="10_min_dimensions.vds"
        )
    )
]


class Samples10Category(common.Category):
    def __init__(self):
        super().__init__(
            name="10_samples",
            description="Related to 10 samples data (sample values are not sequential and there are 10 by default)",
            dirpath="samples10"
        )

    def specifications(self):
        return [case.spec for case in cases]

    def generate_testdata(self, testdata: common.TestdataCase):
        case: Case = next(c for c in cases if c.spec.case == testdata.case)

        with tempfile.TemporaryDirectory() as tmp:
            segy_name = Path(testdata.filepath).with_suffix('.segy').name
            segy_path = Path(tmp) / segy_name
            create_10_samples(filepath=segy_path, samples=case.samples,
                              ilines=case.ilines, xlines=case.xlines, factor=case.factor)
            common.segy_to_vds(testdata, segy_path, crs=case.crs)
