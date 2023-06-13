# VDS slice

VDS slice is a web API for accessing seismic slices and fences from VDS
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

## Should I use VDS Slice or the OpenVDS SDK?

This API does not serve you any data that cannot be access directly through
the OpenVDS SDK. As mentioned, the motivation for VDS Slice is to reduce
resource use when fetching small amounts of data, such as slices or fences.
However, if your use-case involve reading larger amounts of data, such as
in a ML pipeline or other seismic processing you are better of using OpenVDS
directly.

# grpc

See: https://grpc.io/docs/languages/go/quickstart/

Get protoc. Then get go-plugins:

```
go install google.golang.org/protobuf/cmd/protoc-gen-go@v1.28
go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@v1.2
```

Then run
```
 protoc --go_out=. --go_opt=paths=source_relative --go-grpc_out=.
 --go-grpc_opt=paths=source_relative internal/grpc/server.proto
 ```

# Development

## Running the server

Server can be run from the project root directory
```
go run cmd/query/main.go --storage-accounts "https://<account>.blob.core.windows.net"
```

Or it can be installed first and then run
```
export VDSSLICE_INSTALL_DIR=<where to install the server>
GOBIN=$VDSSLICE_INSTALL_DIR  go install ./...
VDSSLICE_STORAGE_ACCOUNTS="https://<account>.blob.core.windows.net" $VDSSLICE_INSTALL_DIR/query
```

Run `$VDSSLICE_INSTALL_DIR/query --help` to print available server options.

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

### 1. Developer test suite

The `go` test suite is the main test suite of the project. In addition to
testing low level internal functionality, it spins up a test server and runs on
it high level setup tests. Note that all the tests run on vds files stored on
disk.

To run the tests switch to the root directory and call
```
go test -failfast -race ./...
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

3. Run E2E tests from the tests directory

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
    export PYTHONPATH=<path-to-the-repo>/vds-slice/tests:$PYTHONPATH
    ```
4. Set all environment variables
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
    export MEDTIME=<maximum acceptable median response time (ms) against provided server>
    export MAXTIME=<time (ms) in which 95% of all requests are expected to return against provided server>
    ```
5. Run the script
    ```
    python tests/performance/performance.py tests/performance/<chosen_script>.js
    ```

Errors and output reports can be found under the `LOGPATH` directory.

## CI

E2E tests use secrets to access Azure environment and due to security reasons
secrets are not accessible in pull requests. Thus e2e tests can't run on PRs,
but are set up to run on merge to master.

If one wants to run e2e tests oneself, one must set up own fork with required
secrets to dedicated test storage account. Then e2e tests run can be triggered
either manually or by pushing code to a dedicated e2e_tests branch.
