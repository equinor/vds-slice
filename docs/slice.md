# Fetch a slice form a VDS

Fetch a slice in any direction. Supports 0-indexing of lines and
index-by-annotation such as inline and crossline numbers and depth intervals.
See model SliceData for more info on request parameters.

## Response
On success (200) the response consists of two parts, metadata and data.

### Metadata part
*Content-Type: application/json*
Metadata related to the returned slice, such as axis dimensions, labels and
units and data type. See the Metadata data model.

### Data part
*Content-Type: application/octet-stream*
A raw byte array containing the slice itself. The byte array needs to be parsed
into a 2D array before use. Shape and type information is found in the metadata
part. Data is always little endian.
