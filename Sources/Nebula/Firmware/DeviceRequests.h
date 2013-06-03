#ifndef header_AEABFDCE
#define header_AEABFDCE

namespace Nebula
{
    namespace Firmware
    {
        namespace Request
        {
            enum Request {
                UUID = 0,
                numberOfLeds = 1,
                colors = 2,
                numberOfChannels = 3
            };
        }

        // Options for kRequestUUID
        namespace UuidType
        {
            enum UuidType {
                deviceRevision = 0,
                firmwareRevision = 1,
                device = 2,
            };
        }
    }
}

#endif
