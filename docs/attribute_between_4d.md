# Calculate attribute(s) along a horizon in a 4D cube

On the difference of the two cubes the function calculate attributes such as `maxpos`, `mean` and `rms` between two provided
surfaces. See `AttributeBetween4dSurfacesRequest` for more details. Multiple
attributes can be computed by a single request. It's advisable to do so,
compared to doing one request per attribute, as a single request would be *much*
faster.

## Bounds on input map

Samples that are out-of-range of the seismic volume in the vertical plane are
considered an error. Samples that are out-of-range of the seismic volume in the
horizontal plane will be set to `fillValue` in the resulting attribute(s).

Any sample in the height map that has a value equal to `fillValue` will be
treated as missing, and the `fillValue` will be written to the attribute maps.

## Supported attributes

Name        | Description
------------|------------
samplevalue | Seismic sample value at the exact surface position
min         | Minimum value
max         | Maximum value
maxabs      | Absolute maximum value
mean        | Mean value
meanabs     | Mean of absolute values
meanpos     | Mean of positive values
meanneg     | Mean of negative values
median      | Median value
rms         | Root mean square
var         | Variance
sd          | Standard deviation
sumpos      | Sum of positive samples
sumneg      | Sum of negative samples


## Response
On success (200) the multipart/mixed response consists of n parts. The first
part is a json document with metadata about the attributes. Each of the next n -
1 parts contains one attribute. The ordering of the attributes from the request
is preserved.

### Metadata part
*Content-Type: application/json*
Metadata related to the returned horizon, such as data shape. See the
AttributeMetadata data model.

### Data part(s)
*Content-Type: application/octet-stream*
One part per requested attribute. Each part contains an attribute as a raw byte
array. The byte arrays need to be parsed into 2D arrays before use. The shape
is identical to the shape of the input values, but can also be found in the
returned metadata.

Data is always 4 byte IEEE floating point, little endian.

## Errors
On failure (400, 500) the response is of *Content-Type: application/json*. See
ErrorResponse model.
