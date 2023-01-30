#ifndef REQUESTBUFFER_H
#define REQUESTBUFFER_H

#include <memory>

#include "vds.h"

namespace vds {

class RequestBuffer {
    private:
        const int requestSizeInBytes;
        std::unique_ptr<char> pointer;
    public:
        RequestBuffer(const int requestSizeInBytes)
        : requestSizeInBytes(requestSizeInBytes),
            pointer(new char[requestSizeInBytes])
        {}

        char* getPointer() {return this->pointer.get();}

        response getAsResponse() {
            response responseData{
                this->pointer.get(),
                nullptr,
                static_cast<unsigned long>(this->requestSizeInBytes)
            };
            this->pointer.release();
            return responseData;
        }

        int getSizeInBytes() const {return this->requestSizeInBytes;}

        ~RequestBuffer() {}
};

} /* namespace vds */

#endif /* REQUESTBUFFER_H */
