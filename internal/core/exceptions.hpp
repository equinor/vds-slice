#ifndef ONESEISMIC_API_EXCEPTIONS_H
#define ONESEISMIC_API_EXCEPTIONS_H

#include <stdexcept>

namespace detail {

struct nullptr_error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct bad_request : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

} // namespace detail

#endif // ONESEISMIC_API_EXCEPTIONS_H
