# Return traces along an arbitrary path

Return traces along an arbitrary path of x,y coordinates, for example along a
wellbore. Coordinates can be specified in various coordinate systems, and
multiple interpolation methods are available. 

## Response
On success (200) the response consists of two parts, metadata and data.

### Metadata part
*Content-Type: application/json*
Not implemented yet. Content-Length is 0.

### Data part
*Content-Type: application/octet-stream*
A raw byte array containing the fence itself. The byte array needs to be parsed
into a 2D array before use. The shape (x, y) is given by:

**x**: the length of "coordinates" in the request
**y**: number of samples in depth/time/sample/k direction. Can be found by
       querying /metadata

Data is always 4 byte IEEE floating point, little endian.
