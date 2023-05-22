#ifndef VDS_SLICE_EXCEPTIONS_H
#define VDS_SLICE_EXCEPTIONS_H

#include <stdexcept>

namespace detail {

struct nullptr_error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

} // namespace detail

#endif // VDS_SLICE_EXCEPTIONS_H
