#ifndef header_689187CD
#define header_689187CD

#define R 1
#define G 4
#define B 2

namespace Nebula
{
    namespace Firmware
    {
        namespace Debug
        {
            void flashLED(double durationMs);
            void flashColorLED(unsigned char value);
        }
    }
}

#endif
