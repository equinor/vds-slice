import os
import subprocess
from utils.cloud import generate_account_signature


def runMemoryTests():
    storage_account_name = os.environ["STORAGE_ACCOUNT_NAME"]
    storage_account_key = os.environ["STORAGE_ACCOUNT_KEY"]

    os.environ["STORAGE_ACCOUNT_URL"] = "https://{}.blob.core.windows.net".format(
        storage_account_name)
    sas = generate_account_signature(storage_account_name, storage_account_key)
    os.environ["SAS"] = sas

    logpath = os.getenv('LOGPATH', '')
    stdoutname = "{}/stdout.txt".format(logpath)
    stderrname = "{}/stderr.txt".format(logpath)

    catch_parameters = os.getenv('CATCH_PARAMS', '')
    valgrind = subprocess.run(
        [
            "valgrind",
            "--leak-check=full",
            "--show-leak-kinds=all",
            "--show-error-list=yes",
            "--error-exitcode=1",
            "./tests/memory/memory.out",
            catch_parameters
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
