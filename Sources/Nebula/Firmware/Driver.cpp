#include <avr/wdt.h>
#include <Nebula/Firmware/Firmware.h>

Nebula::Firmware::Driver::EepromContent EEMEM
Nebula::Firmware::eepromContent;

void Nebula::Firmware::Driver::initChannels(Channel* channels) {
    Nebula::Firmware::initChannels(channels);
    for (unsigned int i = 0; i < sizeof(channels) / sizeof(channels[0]); i++)
        channels[i].controller->init();
}

void Nebula::Firmware::Driver::iterate() {}

Nebula::Firmware::Driver::Driver() {
    DDRB |= 0xFF;
    wdt_enable(WDTO_1S); // enable 1s watchdog timer
    initChannels(channels);
}

void Nebula::Firmware::Driver::runLoop() {
    while (1) {
        wdt_reset(); // keep the watchdog happy
        iterate();
    }
}
