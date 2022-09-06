import os
import sys
import subprocess
from utils.cloud import generate_account_signature


def runPerformanceTests(filepath):
    sas = os.getenv("SAS")
    if not sas:
        storage_account_name = os.environ["STORAGE_ACCOUNT_NAME"]
        storage_account_key = os.environ["STORAGE_ACCOUNT_KEY"]
        expected_run_time = int(os.environ["EXPECTED_RUN_TIME"])
        sas = generate_account_signature(
            storage_account_name, storage_account_key, expected_run_time)
    os.environ["SAS"] = sas

    logpath = os.environ["LOGPATH"]
    logname = "{}/loadtest.log".format(logpath)
    stdoutname = "{}/stdout.txt".format(logpath)
    stderrname = "{}/stderr.txt".format(logpath)

    performance = subprocess.run(
        [
            "k6",
            "run",
            "--console-output",
            logname,
            filepath
        ],
        encoding="utf-8", capture_output=True)

    with open(stdoutname, "w") as text_file:
        text_file.write(performance.stdout)
    with open(stderrname, "w") as text_file:
        text_file.write(performance.stderr)

    if performance.returncode:
        performance.check_returncode()


if __name__ == "__main__":
    filepath = sys.argv[1]
    runPerformanceTests(filepath)
