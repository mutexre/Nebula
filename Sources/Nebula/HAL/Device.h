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
                Unknown,
                USB
            };

            struct Info {
                Bus bus;
                uuid_t uuid;

                Info() : bus(Bus::Unknown) {}
                Info(Bus bus, uuid_t uuid) {
                    this->bus = bus;
                    uuid_copy(this->uuid, uuid);
                }
            };

            enum class Request {
                getDeviceRevisionUUID,
                setDeviceRevisionUUID,
                getFirmwareRevisionUUID,
                setFirmwareRevisionUUID,
                getDeviceUUID,
                setDeviceUUID,
                getNumberOfChannels,
                getNumberOfLeds,
                setNumberOfLeds,
                getColors,
                setColors
            };

            struct IoctlParameters {
                struct SetNumberOfLeds {
                    RT::u4 channel;
                    RT::u4 numberOfLeds;

                    SetNumberOfLeds() : channel(0), numberOfLeds(0) {}

                    SetNumberOfLeds(RT::u4 channel, RT::u4 numberOfLeds) {
                        this->channel = channel;
                        this->numberOfLeds = numberOfLeds;
                    }
                };

                struct Colors {
                    RT::u4 channel;
                    Color::RGB<RT::u1>* colors;
                    RT::u4 numberOfLeds;

                    Colors() : channel(0), colors(0), numberOfLeds(0) {}

                    Colors(RT::u4 channel, Color::RGB<RT::u1>* colors, RT::u4 numberOfLeds) {
                        this->channel = channel;
                        this->colors = colors;
                        this->numberOfLeds = numberOfLeds;
                    }
                };
            };

            class UnsupportedRequestException;
            class UnknownRequestException;
            class InvalidRequestException;

        protected:
            Info info;

        public:
            virtual ~Device() {};

            virtual Info getInfo() { return info; };
            virtual bool isRequestSupported(Request) = 0;
            virtual void control(Request, void* input, size_t inputSize, void* output, size_t outputSize) = 0;
            virtual void controlIn(Request, void*, size_t);
            virtual void controlOut(Request, void*, size_t);
        };
    }
}

#endif
