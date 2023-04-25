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

## CI

E2E tests use secrets to access Azure environment and due to security reasons
secrets are not accessible in pull requests. Thus e2e tests can't run on PRs,
but are set up to run on merge to master.

If one wants to run e2e tests oneself, one must set up own fork with required
secrets to dedicated test storage account. Then e2e tests run can be triggered
either manually or by pushing code to a dedicated e2e_tests branch.
