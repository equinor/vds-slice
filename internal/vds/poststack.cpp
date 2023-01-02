#include "poststack.h"

#include <stdexcept>


PostStackHandle::PostStackHandle(std::string url, std::string conn)
    : SeismicHandle(url,
                    conn,
                    Channel::Sample,
                    LevelOfDetail::Default,
                    std::make_unique<PostStackAxisMap>( 2, 1, 0 ))  // hardcode axis map, this assumption will be validated
{
    // TODO hardcode defaults for lod and channel and send to SeismicVDSHandle
    //if not PostStackValidator().validate(this) throw std::runtime_error("");
}

requestdata PostStackHandle::slice(const Axis          axis,
                                   const int           line_number,
                                   const LevelOfDetail level_of_detail,
                                   const Channel       channel) {
    throw std::runtime_error("Slicing not implemented");
}
