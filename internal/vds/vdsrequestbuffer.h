#ifndef VDSREQUESTBUFFER_H
#define VDSREQUESTBUFFER_H

#include <memory>

#include "vds.h"

class VDSRequestBuffer {
    private:
        const int requestSizeInBytes;
        std::unique_ptr<char> pointer;
    public:
        VDSRequestBuffer(const int requestSizeInBytes)
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

        ~VDSRequestBuffer() {}
};

#endif /* VDSREQUESTBUFFER_H */
