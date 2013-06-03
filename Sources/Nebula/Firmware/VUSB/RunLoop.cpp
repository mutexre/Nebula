#include <avr/wdt.h>
#include <util/delay.h>
#include <Nebula/Firmware/Firmware.h>
#include <Nebula/Firmware/VUSB/RunLoop.h>

Nebula::Firmware::VUSB::RunLoop Nebula::Firmware::VUSB::runLoop;

extern "C" USB_PUBLIC unsigned char usbFunctionSetup(uchar data[8]) {
    return Nebula::Firmware::VUSB::runLoop.usbFunctionSetup(data);
}

extern "C" USB_PUBLIC unsigned char usbFunctionRead(uchar* data, uchar len) {
    return Nebula::Firmware::VUSB::runLoop.usbFunctionRead(data, len);
}

extern "C" USB_PUBLIC unsigned char usbFunctionWrite(uchar *data, uchar len) {
    return Nebula::Firmware::VUSB::runLoop.usbFunctionWrite(data, len);
}

unsigned char Nebula::Firmware::VUSB::RunLoop::usbFunctionSetup(unsigned char data[8]) {
    usbRequest_t* request = (usbRequest_t*)data;
    switch (request->bRequest) {
        case Nebula::Firmware::Request::UUID:
            if (request->wLength.word == sizeof(uuid_t)) {
                numberOfBytesProcessed = 0;
                currentRequest = *request;
                return USB_NO_MSG;
            }
        break;

        case Nebula::Firmware::Request::numberOfChannels:
            if (request->bmRequestType & 1)
                //eeprom_read_word(&(request->wValue.word), &(eepromContent.numberOfChannels));
                request->wValue.word = NUMBER_OF_CHANNELS;
        break;

        case Nebula::Firmware::Request::numberOfLeds: {
            Nebula::Firmware::Channel& channel = channels[request->wIndex.word];
            if (request->bmRequestType & 1)
                request->wValue.word = channel.numberOfLeds;
            else {
                if (channel.numberOfLeds < request->wValue.word) {
                    if (channel.colors) free(channel.colors);
                    channel.colors = (Nebula::Color::RGB<uint8_t>*)malloc(request->wValue.word);
                }
                channel.numberOfLeds = request->wValue.word;
            }
        }
        break;

        case Nebula::Firmware::Request::colors: {
            Nebula::Firmware::Channel& channel = channels[request->wIndex.word];
            if (request->wLength.word == channel.numberOfLeds * sizeof(Nebula::Color::RGB<uint8_t>)) {
                numberOfBytesProcessed = 0;
                currentRequest = *request;
                return USB_NO_MSG;
            }
        }
        break;
    }

    return 0;
}

unsigned char Nebula::Firmware::VUSB::RunLoop::usbFunctionRead(unsigned char* data, unsigned char len) {
    switch (currentRequest.bRequest) {
        case Nebula::Firmware::Request::UUID:
            eeprom_read_block(data, eepromContent.uuid[currentRequest.wValue.word] + numberOfBytesProcessed, len);
        break;

        case Nebula::Firmware::Request::colors:
            memcpy(data, ((byte*)channels[currentRequest.wIndex.word].colors) + numberOfBytesProcessed, len);
        break;
    }

    numberOfBytesProcessed += len;
    if (numberOfBytesProcessed >= currentRequest.wLength.word) return 1;

    return 0;
}

unsigned char Nebula::Firmware::VUSB::RunLoop::usbFunctionWrite(unsigned char *data, unsigned char len) {
    switch (currentRequest.bRequest) {
        case Nebula::Firmware::Request::UUID: {
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

        case Nebula::Firmware::Request::colors: {
            Nebula::Firmware::Channel& channel = channels[currentRequest.wIndex.word];

            memcpy(((unsigned char*)channel.colors) + numberOfBytesProcessed, data, len);
            numberOfBytesProcessed += len;

            if (numberOfBytesProcessed >= currentRequest.wLength.word) {
                if (channel.numberOfLeds > 0)
                    channel.controller->showRGB((byte*)channel.colors, channel.numberOfLeds);

                return 1;
            }
        }
        break;
    }

    return 0;
}

void Nebula::Firmware::VUSB::RunLoop::init() {
    Nebula::Firmware::RunLoop::init();

    usbInit();

    usbDeviceDisconnect();
    for (int i = 0; i < 250; i++) {
        wdt_reset();
        _delay_ms(2);
    }
    usbDeviceConnect();

    sei();
}

void Nebula::Firmware::VUSB::RunLoop::iterate() { usbPoll(); }
