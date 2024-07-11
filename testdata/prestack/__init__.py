from .make_prestack import create_defined

from pathlib import Path
import tempfile

import common


class PrestackCategory(common.Category):
    def __init__(self):
        super().__init__(
            name="prestack",
            description="Prestack files.",
            dirpath="prestack"
        )

    def specifications(self):
        return [
            common.CommandSpecification(
                case="default",
                description="Default pre-stack file with 2 ilines, 2 xlines, 4 samples and 3 offsets",
                filename="prestack_default.vds",
            ),
        ]

    def generate_testdata(self, testdata: common.TestdataCase):
        with tempfile.TemporaryDirectory() as tmp:
            segy_path = Path(tmp) / "temp.segy"
            create_defined(segy_path)
            common.segy_to_vds(testdata, segy_path, "--prestack", crs="utmXX")
