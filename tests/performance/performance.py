import os
import sys
import subprocess
from datetime import datetime
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

    command = ["k6", "run"]

    if os.getenv("K6_PROMETHEUS_RW_SERVER_URL"):
        # expected environment variables
        # - K6_REMOTE_RW_URL
        # - TENANT_ID
        # - K6_REMOTE_RW_CLIENT_ID
        # - K6_REMOTE_RW_CLIENT_SECRET
        # - K6_PROMETHEUS_RW_PUSH_INTERVAL

        os.environ["K6_PROMETHEUS_RW_TREND_STATS"] = "max,med"
        os.environ["K6_PROMETHEUS_RW_STALE_MARKERS"] = "true"

        date_tag = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        endpoint = os.environ.get('ENDPOINT', '')
        if endpoint.endswith('/'):
            endpoint = endpoint[:-1]

        command += ["--out", "experimental-prometheus-rw", "--tag",
                    "testid="+date_tag, "--tag", "environment="+endpoint]

    command += ["--console-output", logname, filepath]

    performance = subprocess.run(
        command, encoding="utf-8", capture_output=True)

    with open(stdoutname, "w") as text_file:
        text_file.write(performance.stdout)
    with open(stderrname, "w") as text_file:
        text_file.write(performance.stderr)

    if performance.returncode:
        performance.check_returncode()


if __name__ == "__main__":
    filepath = sys.argv[1]
    runPerformanceTests(filepath)
