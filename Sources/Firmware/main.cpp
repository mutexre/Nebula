#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <FastSPI_LED2.h>
extern "C" {
    #include "usbdrv.h"
}
#include "../Nebula/Color/RGB.h"
#include "../Nebula/DeviceRequests.h"

Nebula::Color::RGB<uint8_t>* leds = 0;
TM1809Controller800Mhz<13> LED;

void led(double durationMs) {
    DDRB |= 0xFF; PORTB = 0xff; _delay_ms(10); PORTB = 0x0;
}

#define R 1
#define G 4
#define B 2

void debugLed(unsigned char value) {
    PORTB = value;
}

// EEPROM content:
// byte 0-15: device revision UUID
// byte 16-31: firmware revision UUID
// byte 32-47: device UUID
// byte 0: UUID consistency state (0xff = consistent or else identifier of UUID being written [inconsistent state])

typedef unsigned char uuid_t[16];

struct EepromContent {
    uuid_t uuid[3];
    unsigned char uuidState;
};

EepromContent EEMEM eepromContent;

unsigned short numberOfLeds = 0;
unsigned int /*dataLength, */numberOfBytesProcessed;

//unsigned char requestNumber;
//unsigned short requestValue;
usbRequest_t currentRequest;

extern "C" USB_PUBLIC unsigned char usbFunctionSetup(uchar data[8]) {
    usbRequest_t* request = (usbRequest_t*)data;
    switch (request->bRequest) {
        case kRequestUUID:
            if (request->wLength.word == sizeof(uuid_t)) {
                //dataLength = request->wLength.word;
                numberOfBytesProcessed = 0;
                currentRequest = *request;
                return USB_NO_MSG;
            }
        break;

        case kRequestNumberOfLeds:
            if (request->bmRequestType & 1)
                request->wValue.word = numberOfLeds;
            else {
                if (numberOfLeds < request->wValue.word) {
                    if (leds) free(leds);
                    leds = (Nebula::Color::RGB<uint8_t>*)malloc(request->wValue.word);
                }
                numberOfLeds = request->wValue.word;
            }
//            for (int i = 0; i < numberOfLeds; i++) led(100);
            //debugLed(R);
        break;

        case kRequestColors:
            if (request->wLength.word == numberOfLeds * sizeof(Nebula::Color::RGB<uint8_t>)) {
                //debugLed(G); _delay_ms(100);
                //dataLength = request->wLength.word;
                numberOfBytesProcessed = 0;
                currentRequest = *request;
                return USB_NO_MSG;
            }
        break;
    }

    return 0;
}

unsigned char tmpUUID[16];

extern "C" USB_PUBLIC unsigned char usbFunctionRead(uchar* data, uchar len) {
    switch (currentRequest.bRequest) {
        case kRequestUUID:
            eeprom_read_block(data, eepromContent.uuid[currentRequest.wValue.word] + numberOfBytesProcessed, len);
        break;

        case kRequestColors:
            memcpy(data, leds + numberOfBytesProcessed, len);
        break;
    }

    numberOfBytesProcessed += len;
    if (numberOfBytesProcessed >= currentRequest.wLength.word) return 1;

    return 0;
}

extern "C" USB_PUBLIC unsigned char usbFunctionWrite(uchar *data, uchar len) {
    switch (currentRequest.bRequest) {
        case kRequestUUID: {
            memcpy(tmpUUID + numberOfBytesProcessed, data, len);
            numberOfBytesProcessed += len;

            if (numberOfBytesProcessed >= currentRequest.wLength.word) {
                eeprom_write_byte(&(eepromContent.uuidState), currentRequest.wValue.word & 0xff);
                eeprom_write_block(eepromContent.uuid[currentRequest.wValue.word], tmpUUID, sizeof(uuid_t));
                eeprom_write_byte(&(eepromContent.uuidState), 0xff);
                return 1;
            }
        }
        break;

        case kRequestColors: {
            //debugLed(B); _delay_ms(100);
            memcpy(leds + numberOfBytesProcessed, data, len);
            numberOfBytesProcessed += len;

            if (numberOfBytesProcessed >= currentRequest.wLength.word) {
                if (numberOfLeds > 0) LED.showRGB((byte*)leds, numberOfLeds);
                return 1;
            }
        }
        break;
    }

    return 0;
}

int main() {
    DDRB |= 0xFF;

    wdt_enable(WDTO_1S); // enable 1s watchdog timer

    LED.init();
    usbInit();

    usbDeviceDisconnect(); // enforce re-enumeration
    for (int i = 0; i < 250; i++) { // wait 500 ms
        wdt_reset(); // keep the watchdog happy
        _delay_ms(2);
    }
    usbDeviceConnect();

    sei(); // Enable interrupts after re-enumeration

    while (1) {
        wdt_reset(); // keep the watchdog happy
        usbPoll();
    }

    return 0;
}
