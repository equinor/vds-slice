# Read seismic along a horizon

Samples are interpolated in both the vertical and horizontal plane. There are
several interpolation methods available. 

Samples that are out-of-range in the vertical plane are considered an error.
Samples that are out-of-range in the horizontal plane will we set to
`fillValue` in the resulting map. 

Any sample in the input horizon that has a value equal to `fillValue` will be
treated as missing, and the `fillValue` will be written back in the output map.

## Response
On success (200) the multipart/mixed response consists of two parts, metadata
and data.

### Metadata part
*Content-Type: application/json*
Metadata related to the returned horizon, such as data shape. See the
Horizon data model.

### Data part
*Content-Type: application/octet-stream*
A raw byte array containing the map itself. The byte array needs to be parsed
into a 2D array before use. The shape is identical to the shape of the input
horizon.

Data is always 4 byte IEEE floating point, little endian.

## Errors
On failure (400, 500) the response is of *Content-Type: application/json*. See
ErrorResponse model.
