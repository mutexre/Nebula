#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <FastSPI_LED2.h>
extern "C" {
    #include "usbdrv.h"
}
#include "../RGB.h"
#include "../DeviceRequests.h"

RGB<uint8_t>* leds = 0;
TM1809Controller800Mhz<13> LED;

void led(double duration) {
    DDRB |= 0xFF; PORTB = 0xff; _delay_ms(10); PORTB = 0x0;
}

#define R 1
#define G 4
#define B 2

void debugLed(unsigned char value) {
    PORTB = value;
}

unsigned short numberOfLeds = 0;
unsigned int dataLength, numberOfBytesReceived;

extern "C" USB_PUBLIC unsigned char usbFunctionSetup(uchar data[8]) {
    usbRequest_t* request = (usbRequest_t*)data;
    switch (request->bRequest) {
        case kRequestSetNumberOfLeds:
            if (numberOfLeds < request->wValue.word) {
                if (leds) free(leds);
                leds = (RGB<uint8_t>*)malloc(request->wValue.word);
            }
            numberOfLeds = request->wValue.word;
        break;

        case kRequestSetColors:
            if (request->wLength.word == numberOfLeds * 3) {
                dataLength = request->wLength.word;
                numberOfBytesReceived = 0;
                return USB_NO_MSG;
            }
        break;
    }

    return 0;
}

extern "C" USB_PUBLIC unsigned char usbFunctionWrite(uchar *data, uchar len) {
    for (unsigned int i = 0; i < len; i++)
        ((uint8_t*)leds)[numberOfBytesReceived + i] = data[i];

    numberOfBytesReceived += len;

    if (numberOfBytesReceived >= dataLength) {
        if (numberOfLeds > 0) LED.showRGB((byte*)leds, numberOfLeds);
        return 1;
    }

    return 0;
}

#if 0
extern "C" USB_PUBLIC void usbFunctionWriteOut(uchar *data, uchar len) {
    led(100);
}
#endif

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
