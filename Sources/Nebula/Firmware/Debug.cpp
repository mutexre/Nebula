#include <avr/io.h>
#include <util/delay.h>
#include <Nebula/Firmware/Firmware.h>

void Nebula::Firmware::Debug::flashLED(double durationMs) {
    DDRB |= 0xFF; PORTB = 0xff; _delay_ms(10); PORTB = 0x0;
}

void Nebula::Firmware::Debug::flashColorLED(unsigned char value) {
    PORTB = value;
}