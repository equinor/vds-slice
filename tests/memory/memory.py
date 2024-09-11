import os
import subprocess
from utils.cloud import generate_account_signature
from pathlib import Path


# Memory tests are better run against the data in storage account than local
# files as different memory mechanisms are in play, and we are interested in
# cloud behavior.
#
# So for simplicity we use python to create sas tokens and display the results.
# It is possible to create tokens in c++ as well, but it significantly increases
# compilation time and newest azure-sdk-for-cpp library version has issues when
# installed through FetchContent.

def runMemoryTests():
    storage_account_name = os.environ["STORAGE_ACCOUNT_NAME"]
    storage_account_key = os.environ["STORAGE_ACCOUNT_KEY"]

    os.environ["STORAGE_ACCOUNT_URL"] = "https://{}.blob.core.windows.net".format(
        storage_account_name
    )
    sas = generate_account_signature(storage_account_name, storage_account_key)
    os.environ["SAS"] = sas

    logpath = os.getenv('LOGPATH', '')
    stdoutname = Path(logpath) / "stdout.txt"
    stderrname = Path(logpath) / "stderr.txt"

    suppression_paths = os.getenv('VALGRIND_SUPPRESSION_PATH', '').split(',')
    suppressions = [
        f"--suppressions={path.strip()}" for path in suppression_paths
    ]

    executable_path = os.environ['EXECUTABLE_PATH']

    valgrind = subprocess.run(
        [
            "valgrind",
            "--leak-check=full",
            "--show-leak-kinds=all",
            "--errors-for-leak-kinds=all",
            "--show-error-list=yes",
            "--error-exitcode=1",
            "--track-origins=yes",
            "--keep-debuginfo=yes",
            "--read-var-info=yes",
            *suppressions,
            "--gen-suppressions=all",
            executable_path
        ],
        encoding="utf-8", capture_output=True)

    with open(stdoutname, "w") as text_file:
        text_file.write(valgrind.stdout)
    with open(stderrname, "w") as text_file:
        text_file.write(valgrind.stderr)

    if valgrind.returncode:
        valgrind.check_returncode()


if __name__ == "__main__":
    runMemoryTests()
