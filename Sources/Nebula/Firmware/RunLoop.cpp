#include <avr/wdt.h>
#include <util/delay.h>
#include <Nebula/Firmware/Firmware.h>

Nebula::Firmware::RunLoop::EepromContent EEMEM
Nebula::Firmware::eepromContent;

void Nebula::Firmware::RunLoop::initChannels(Channel* channels) {
    Nebula::Firmware::initChannels(channels);
    for (unsigned int i = 0; i < NUMBER_OF_CHANNELS; i++)
        channels[i].controller->init();
}

void Nebula::Firmware::RunLoop::iterate() {}

void Nebula::Firmware::RunLoop::init() {
    DDRB |= 0xFF;
    wdt_enable(WDTO_1S);
    initChannels(channels);
}

void Nebula::Firmware::RunLoop::run() {
    while (1) {
        wdt_reset();
        iterate();
    }
}
