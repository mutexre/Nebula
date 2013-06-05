#ifndef header_7E332C69
#define header_7E332C69

#include <avr/eeprom.h>
#include <Nebula/Firmware/Debug.h>
#include <Nebula/Firmware/Channel.h>
#include <Nebula/Firmware/Requests.h>
#include <Nebula/Firmware/RunLoop.h>

namespace Nebula {
    namespace Firmware {
        extern RunLoop::EepromContent EEMEM eepromContent;
        void initChannels(Channel* channels);
    }
}

#endif
