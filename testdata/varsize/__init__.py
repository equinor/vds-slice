from pathlib import Path
import tempfile

import common

from .make_varsize import create_varsize

tmpdir = tempfile.TemporaryDirectory()

def cloudpath(ilines, xlines, samples):
    return f"testdata/varsize/varsize_{ilines}_{xlines}_{samples}"


def defined_specification(ilines, xlines, samples, description="Used in performance testing"):
    return common.CommandSpecification(
        case=f"{ilines}_{xlines}_{samples}",
        description=description,
        filename=f"{ilines}_{xlines}_{samples}.vds",
        cloud_path=cloudpath(ilines, xlines, samples)
    )

class VarsizeCategory(common.Category):
    def __init__(self):
        super().__init__(
            name="varsize",
            description="Various-sized files with random data. Currently used for manual and automated performance testing.",
            dirpath=tmpdir.name,
            all_allowed=False
        )

    def specifications(self):
        args = [
            common.CommandArgument("inlines", int, "Number of inlines", True),
            common.CommandArgument("xlines", int, "Number of xlines", True),
            common.CommandArgument("samples", int, "Number of samples", True),
        ]

        return [
            common.CommandSpecification(
                case="custom",
                description="""General variable sized file: how many ilines, xlines and samples should be present.
                            By default file would be created in temp directory and cloud path will be set to
                            testdata/varsize/varsize_{ilines}_{xlines}_{samples}""",
                filename="custom.vds",
                custom_args=args
            ),
            # commented out specifications are for future reference only
            defined_specification(1500, 600, 2400, description="8 GB, smaller survey"),
            defined_specification(2200, 1500, 1700, description="20 GB, typical survey"),
            #defined_specification(1000, 4000, 2500, description="37 GB, typical survey"),
            #defined_specification(5000, 3000, 2500, description="140 GB, large survey"),
            #defined_specification(15000, 20000, 3000, description="3300 GB, very large survey"),
        ]

    def generate_testdata(self, testdata: common.TestdataCase):
        with tempfile.TemporaryDirectory() as tmp:
            segy_path = Path(tmp) / "temp.segy"

            if testdata.case == "custom":
                inlines = testdata.kwargs['inlines']
                xlines = testdata.kwargs['xlines']
                samples = testdata.kwargs['samples']
            else:
                inlines, xlines, samples = testdata.case.split("_")

            if not testdata.cloud_path:
                testdata.cloud_path = cloudpath(
                    inlines, xlines, samples)

            create_varsize(segy_path, int(inlines), int(xlines), int(samples))
            common.segy_to_vds(testdata, segy_path)
