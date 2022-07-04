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
