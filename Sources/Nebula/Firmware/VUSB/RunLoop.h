#ifndef header_82AAFEF2
#define header_82AAFEF2

extern "C" {
    #include "usbdrv.h"
}

namespace Nebula
{
    namespace Firmware
    {
        namespace VUSB
        {
            class RunLoop : public Nebula::Firmware::RunLoop
            {
            private:
                usbRequest_t currentRequest;
                unsigned char tmpUUID[16];

            public:
                unsigned char usbFunctionSetup(unsigned char data[8]);
                unsigned char usbFunctionRead(unsigned char* data, unsigned char len);
                unsigned char usbFunctionWrite(unsigned char *data, unsigned char len);

            public:
                virtual void init();
                virtual void iterate();
            };

            extern RunLoop runLoop;
        }
    }
}

#endif
