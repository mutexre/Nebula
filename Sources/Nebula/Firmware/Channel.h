#ifndef header_8B150960
#define header_8B150960

#include <FastSPI_LED2.h>
#include <Nebula/Color/RGB.h>

namespace Nebula
{
    namespace Firmware
    {
        struct Channel {
            unsigned int numberOfLeds;
            Nebula::Color::RGB<uint8_t>* colors;
            CLEDController* controller;

            void init(unsigned int numberOfLeds, Nebula::Color::RGB<uint8_t>* colors, CLEDController* controller);
            Channel();
        };
    }
}

#endif
