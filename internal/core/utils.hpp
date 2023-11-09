#ifndef VDS_SLICE_UTILS_H
#define VDS_SLICE_UTILS_H

#include <iomanip>
#include <sstream>
#include <string>

namespace utils {

template<typename T>
std::string to_string_with_precision(const T val, const int n = 2) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(n) << val;
    return out.str();
}
}
#endif //VDS_SLICE_UTILS_H
