# oneseismic-api

oneseismic-api is a web API for accessing seismic slices and fences from VDS
files. It also serves basic metadata from the Survey.

## Motivation

This API exist in order to reduce the amount of compute- and network-resources
needed by a caller in order to fetch a single slice or fences from VDS data.
Requesting this data directly through the OpenVDS SDK involves downloading and
extracting much more data than asked for. In many scenarios that is completely
fine, but there are cases where that extra resource use is unacceptable.
Suppose you run a service that caters requests from hundreds of users and as
part of each request you have to fetch a slice. Then the resource overhead of
OpenVDS will quickly saturate your servers CPU and network throughput, leaving
few resource for the rest of you request pipeline.

With this API we move the resource use of OpenVDS away from your server, by
serving you exactly the data you ask for and nothing more, keeping each request
as lightweight as possible.

## Should I use oneseismic-api or the OpenVDS SDK?

This API does not serve you any data that cannot be access directly through
the OpenVDS SDK. As mentioned, the motivation for oneseismic-api is to reduce
resource use when fetching small amounts of data, such as slices or fences.
However, if your use-case involve reading larger amounts of data, such as
in a ML pipeline or other seismic processing you are better of using OpenVDS
directly.

# Development

## Running the server

Server can be run from the project root directory
```
go run cmd/query/main.go --storage-accounts "https://<account>.blob.core.windows.net"
```

Or it can be installed first and then run
```
export ONESEISMIC_API_INSTALL_DIR=<where to install the server>
GOBIN=$ONESEISMIC_API_INSTALL_DIR  go install ./...
ONESEISMIC_API_STORAGE_ACCOUNTS="https://<account>.blob.core.windows.net" $ONESEISMIC_API_INSTALL_DIR/query
```

Run `$ONESEISMIC_API_INSTALL_DIR/query --help` to print available server options.

Note that for server to build and run properly `openvds` library should be
reachable. For example:
```
PATH_TO_OPENVDS_LIBRARY=<location where openvds is installed>
export CGO_CPPFLAGS="-isystem $PATH_TO_OPENVDS_LIBRARY/include"
export CGO_LDFLAGS="-L$PATH_TO_OPENVDS_LIBRARY/lib"
export LD_LIBRARY_PATH=$PATH_TO_OPENVDS_LIBRARY/lib:$LD_LIBRARY_PATH
```

## Testing

Project has following test suits:

### 1. Developer test suites

#### Go
The `go` test suite is the main test suite of the project. In addition to
testing low level internal functionality, it spins up a test server and runs on
it high level setup tests. Note that all the tests run on vds files stored on
disk.

To run the tests switch to the root directory and call
```
go test -failfast -race ./...
```

#### C++
The `gtest` test suite is a test suite for `C++` code. When choosing where to
implement new tests keep in mind that `C++` is deeper internal code and thus its
interface is more prone to change than `go` one.

To run the tests one has to build `C++` project outside of `go build`. The
following builds the project in directory `build` and runs the tests:
```
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/openvds
cmake --build build
ctest --test-dir build --output-on-failure
```

### 2. E2E test suite

Python E2E test suite is a supplementary test suite. Its main goals are to
assure that server works correctly with Azure cloud infrastructure, server setup
which could not be directly tested through go tests is correct, and that
real-world user experience for each category of requests is without surprises.
Suite is not expected to go into details more than necessary.

To run the tests:

1. Define the following environment variables:

    ```
    export STORAGE_ACCOUNT_NAME=<name of Azure storage account/datalake gen2 resource>
    export STORAGE_ACCOUNT_KEY="<storage account key>"
    export ENDPOINT=<endpoint on which server will be running. Defaults to http://localhost:8080>
    ```

    Suite assumes that default vds `well_known/well_known_default` file is
    uploaded under container `testdata` in `STORAGE_ACCOUNT_NAME`. Files can be
    uploaded using the corresponding bash scripts under `testdata` directory.
    See variables declared in the files in e2e directory for better overview
    over what is expected.

2. Start the server if tests are supposed to run against local server
    ```
    go run cmd/query/main.go --storage-accounts "https://$STORAGE_ACCOUNT_NAME.blob.core.windows.net" --cache-size 50
    ```
    Cache is expected to be turned on.


3. Create Python test environment (if needed)
    These commands will create, activate and install the required packages for the E2E tests.
    ```
    cd tests
    python -m venv venv
    source venv/bin/activate
    pip install -r e2e/requirements-dev.txt
    ```
4. Run E2E tests from the tests directory

    ```
    cd tests
    python -m pytest -s -rsx e2e
    ```

### 3. Performance test suite
Javascript performance test suite which is run through python is intended to be
run on cloud CI, not locally. As getting data as fast as possible is the main
point behind the project, making sure response time on target server never
declines significantly regardless on the inner changes is sensible. However in
reality it is difficult to compare performance due to both environment setup and
network/local speed fluctuations. At the moment we are still assessing how much
practical use this suite has.

To locally run a file `<chosen_script>.js` from the suite:

1. Install [k6](https://k6.io/open-source/) and make it available on `PATH`
2. pip-install requirements under `tests/performance/requirements-dev.txt`
3. Add test directory to python path
    ```
    export PYTHONPATH=<path-to-the-repo>/oneseismic-api/tests:$PYTHONPATH
    ```
4. Set all required environment variables
    ```
    # set the 3 following variables needed to create sas

    export STORAGE_ACCOUNT_NAME=<storage account name>
    export STORAGE_ACCOUNT_KEY="<storage account key>"
    export EXPECTED_RUN_TIME=<validity of the sas token (seconds)>

    # or instead provide SAS itself

    export SAS="<token>"
    ```
    ```
    export ENDPOINT=<endpoint on which server is running>
    export VDS=<https://account.blob.core.windows.net/path-to-the-vds>
    export LOGPATH=<directory to store output files>
    ```

    Refer to github workflows for available optional environment variables.
5. Run the script
    ```
    python tests/performance/performance.py tests/performance/<chosen_script>.js
    ```

Errors and output reports can be found under the `LOGPATH` directory.

### 4. Memory test suite

The memory test suite uses valgrind to verify that common application calls do
not leak memory.

However the suite is written for C library, not for go one, so it doesn't do
good enough job for really testing the server. There are many hurdles for
writing such tests in go and cost of doing that seems way greater than benefit.

As we currently test only C core, tests may be sensitive to internal code
changes and thus suite might become more of a problem than a useful tool.

Suite best be run using docker:

1. Build openvds in debug mode

   ```
   docker build -f Dockerfile --build-arg BUILD_TYPE=Debug --target openvds -t openvdsdebug .
   ```

2. Define the following environment variables:

   ```
   export STORAGE_ACCOUNT_NAME=<name of Azure storage account>
   export STORAGE_ACCOUNT_KEY="<storage account key>"
   export LOGPATH=<directory to store output files inside container>
   ```

   Suite assumes that `well_known/well_known_default` and `10_samples/10_samples_default` files are
   uploaded under container `testdata` in `STORAGE_ACCOUNT_NAME`.

3. Build the image

   ```
   docker build -f tests/memory/Dockerfile --build-arg OPENVDS_IMAGE=openvdsdebug -t memory_tests .
   ```

4. Run tests

   ```
   docker run -e STORAGE_ACCOUNT_NAME -e STORAGE_ACCOUNT_KEY -e LOGPATH memory_tests
   ```

   Output and error logs would be stored inside the container at `LOGPATH`
   location (use `-v` to map to localhost if needed).

## CI

E2E tests use secrets to access Azure environment and due to security reasons
secrets are not accessible in pull requests. Thus e2e tests can't run on PRs,
but are set up to run on merge to main.

If one wants to run e2e tests oneself, one must set up own fork with required
secrets to dedicated test storage account. Then e2e tests run can be triggered
either manually or by pushing code to a dedicated e2e_tests branch.

The same applies to performance and memory tests.
