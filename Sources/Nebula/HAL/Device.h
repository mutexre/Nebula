#ifndef header_DCF7D6F1
#define header_DCF7D6F1

#include <stdlib.h>
#include <uuid/uuid.h>

namespace Nebula
{
    namespace HAL
    {
        class Device
        {
        public:
            enum class Bus {
                USB
            };

            struct Info {
                Bus bus;
                uuid_t uuid;

                Info(Bus bus, uuid_t uuid) {
                    this->bus = bus;
                    uuid_copy(this->uuid, uuid);
                }
            };

            enum class Request {
                getNumberOfLeds,
                setNumberOfLeds,
                getLedsColors,
                setLedsColors
            };

            class UnsupportedRequestException;
            class UnknownRequestException;
            class InvalidRequestException;

        protected:
            Bus bus;
            uuid_t uuid;

        public:
            
            virtual Info getInfo();
            virtual bool isRequestSupported(Request) = 0;
            virtual void control(Request, void* input, size_t inputSize, void* output, size_t outputSize) = 0;
        };
    }
}

#endif
