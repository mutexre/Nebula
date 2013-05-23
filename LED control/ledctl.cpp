#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <functional>
#include <boost/program_options.hpp>
#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>
#include <libusb.h>
#include <Runtime/Runtime.h>
#include "SignalHandling.h"
#include "RGB.h"
#include "HSV.h"
#include "RgbHsv.h"
#include "usb.h"
#include "DeviceRequests.h"

#define STRINGOLIZER(a) #a
#define VERSION_STRING(version, date, time) "Version " STRINGOLIZER(version) " built on " date " " time

unsigned short convertLedNumber(unsigned int n) {
    return 3 * (n / 3) + (2 - n % 3);
}

void rotateColorCycle(RGB<uint8_t>* leds, unsigned int numberOfLeds, HSV<float>* hsv, float rate)
{
    if (hsv->h < 360.0f) {
        hsv->h += rate;

        for (int i = 0; i < numberOfLeds; i++) {
            float h = hsv->h + float(i) * (360.0f / float(numberOfLeds));
            if (h >= 360.0f) h -= 360.0f;

            HSV<float> _hsv(h, hsv->s, hsv->v);
            RGB<float> rgb = hsv2rgb(_hsv);

            leds[convertLedNumber(i)].set(255 * rgb.r, 255 * rgb.g, 255 * rgb.b);
        }
    }
    else
        hsv->h = 0.0;
}

const char* modes[6] = { "help", "version", "list", "continuous", "rainbow", "ambilight" };

enum class Mode {
    none = -1, help = 0, version = 1, list = 2, continuous = 3, rainbow = 4, ambilight = 5
};

Mode determineMode(const char* modeArg) {
    Mode mode = Mode::none;

    auto numberOfModes = sizeof(modes) / sizeof(modes[0]);

    for (auto i = 0; i < numberOfModes; i++)
        if (!strcmp(modes[i], modeArg)) {
            mode = Mode(i);
            break;
        }

    return mode;
}

void print_usage(boost::program_options::options_description& options) {
    std::cout << "Usage: " << getprogname() << " mode [options]\nwhere mode is one of: ";
    auto numberOfModes = sizeof(modes) / sizeof(modes[0]);
    for (auto i = 0; i < numberOfModes; i++) {
        std::cout << modes[i];
        if (i < numberOfModes - 1) std::cout << ", ";
    }
    std::cout << std::endl << options;
}

namespace Options {
    auto kDevice = "device,d";
    auto kNumberOfLeds = "nleds,n";
    auto kBrightness = "brightness,b";
    auto kColor = "color,c";
    auto kRainbow = "rainbow,r";
    auto kRate = "rate";
    auto kStrobe = "color,c";
    auto kDebug = "debug,d";
    auto kAmbilight = "ambilight,a";
    auto kList = "list,l";
    auto kHelp = "help,h";
    auto kVersion = "version,v";
};

class OptionsError {
private:
    const char* message;

public:
    OptionsError(const char* message) {
        this->message = message;
    }

    const char* getMessage() const { return message; }
};

void optionsError(const char* message) {
    throw OptionsError(message);
}

const char* getColorSpaceModelString(CGColorSpaceModel model) {
    switch (model) {
        case kCGColorSpaceModelUnknown: return "Unknown";
        case kCGColorSpaceModelMonochrome: return "Monochrome";
        case kCGColorSpaceModelRGB: return "RGB";
        case kCGColorSpaceModelCMYK: return "CMYK";
        case kCGColorSpaceModelLab: return "LAB";
        case kCGColorSpaceModelDeviceN: return "DeviceN";
        case kCGColorSpaceModelIndexed: return "Indexed";
        case kCGColorSpaceModelPattern: return "Pattern";
    };
    return "None";
}

void printCGColorSpaceInfo(CGColorSpaceRef colorSpace) {
    
    printf("number of components: %lu\n", CGColorSpaceGetNumberOfComponents(colorSpace));
    printf("model: %s\n", getColorSpaceModelString(CGColorSpaceGetModel(colorSpace)));
}

const char* getCGImageAlphaInfoString(CGImageAlphaInfo alphaInfo) {
    switch (alphaInfo) {
        case kCGImageAlphaNone: return "None";
        case kCGImageAlphaPremultipliedLast: return "Prumultiplied Last (E.g.: premultiplied RGBA)";  /* For example, premultiplied RGBA */
        case kCGImageAlphaPremultipliedFirst: return "Premultiplied First (E.g.: premultiplied ARGB"; /* For example, premultiplied ARGB */
        case kCGImageAlphaLast: return "Last (E.g.: non-premultiplied RGBA)";                         /* For example, non-premultiplied RGBA */
        case kCGImageAlphaFirst: return "First (E.g.: non-premultiplied ARGB)";                       /* For example, non-premultiplied ARGB */
        case kCGImageAlphaNoneSkipLast: return "None, skip last (E.g.: RGBX)";                        /* For example, RBGX. */
        case kCGImageAlphaNoneSkipFirst: return "None, skip first (E.g.: XRGB)";                      /* For example, XRGB. */
        case kCGImageAlphaOnly: return "Alpha Only";
    };
    return "Unknown";
}

void printCGImageInfo(CGImageRef image) {
    printf("bits per component: %lu\n", CGImageGetBitsPerComponent(image));
    printf("bits per pixel: %lu\n", CGImageGetBitsPerPixel(image));
    printf("bytes per row: %lu\n", CGImageGetBytesPerRow(image));
    printf("alpha info: %s\n", getCGImageAlphaInfoString(CGImageGetAlphaInfo(image)));
    printCGColorSpaceInfo(CGImageGetColorSpace(image));
}

void getPixelData(CGImageRef imageRef, unsigned char* pixelData) {
    auto width = CGImageGetWidth(imageRef);
    auto height = CGImageGetHeight(imageRef);
    auto colorSpace = CGColorSpaceCreateDeviceRGB();
    unsigned int bytesPerPixel = 4;
    auto bytesPerRow = bytesPerPixel * width;
    unsigned int bitsPerComponent = 8;

    CGContextRef context = CGBitmapContextCreate(pixelData, width, height,
                                                 bitsPerComponent, bytesPerRow, colorSpace,
                                                 kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(colorSpace);

    CGContextDrawImage(context, CGRectMake(0, 0, width, height), imageRef);
    CGContextRelease(context);
}

RGB<uint8_t> computeAvarageColor(const unsigned char* pixelData, size_t bytesPerRow, size_t bytesPerPixel, size_t rectX, size_t rectY, size_t rectWidth, size_t rectHeight) {
    unsigned int rSum = 0, gSum = 0, bSum = 0;

    for (auto y = 0; y < rectHeight; y++) {
        auto rowDataOffset = bytesPerRow * (rectY + y) + bytesPerPixel * rectX;
        for (auto x = 0; x < rectWidth; x += 1) {
            auto pixelDataOffset = rowDataOffset + bytesPerPixel * x;
            rSum += pixelData[pixelDataOffset + 2];
            gSum += pixelData[pixelDataOffset + 1];
            bSum += pixelData[pixelDataOffset + 0];
        }
    }

    auto numberOfPixels = rectWidth * rectHeight;
    RGB<float> rgb((float(rSum) / float(numberOfPixels)) / 255.0f,
                   (float(gSum) / float(numberOfPixels)) / 255.0f,
                   (float(bSum) / float(numberOfPixels)) / 255.0f);
    HSV<float> hsv = rgb2hsv(rgb);
    hsv.s = sqrtf(sqrtf(hsv.s));
    rgb = hsv2rgb(hsv);

    RGB<uint8_t> retval(((unsigned int)(rgb.r * 255.0f)) & 0xff,
                        ((unsigned int)(rgb.g * 255.0f)) & 0xff,
                        ((unsigned int)(rgb.b * 255.0f)) & 0xff);
    return retval;
//    return RGB<uint8_t>(0, 0, 0xff);
    return RGB<uint8_t>((rSum / numberOfPixels) & 0xff, (gSum / numberOfPixels) & 0xff, (bSum / numberOfPixels) & 0xff);
}

void computeAmbilightColors(RGB<uint8_t>* leds, const unsigned char* pixelData,
                            size_t bytesPerRow, size_t bytesPerPixel,
                            unsigned int width, unsigned int height,
                            unsigned short left, unsigned short right,
                            unsigned short bottom, unsigned short top)
{
    unsigned int ledIndex = 0;

    for (auto y = 0; y < left; y++)
        leds[convertLedNumber(ledIndex++)] = computeAvarageColor(pixelData, bytesPerRow, bytesPerPixel, 0, y * height / left, 200, height / left);

    for (auto x = 0; x < top; x++)
        leds[convertLedNumber(ledIndex++)] = computeAvarageColor(pixelData, bytesPerRow, bytesPerPixel, x * width / top, 0, width / top, 200);

    for (auto y = right - 1; y >= 0; y--)
        leds[convertLedNumber(ledIndex++)] = computeAvarageColor(pixelData, bytesPerRow, bytesPerPixel, width - 200, y * height / right, 200, height / right);
}

int main(int argc, const char** argv)
{
    int retval = 0;
    libusb_context* usbContext = 0;
    libusb_device_handle* deviceHandle = 0;
    boost::program_options::options_description general_options_description("Options", 140, 60);
    boost::program_options::variables_map vm;
    std::string colorArg, deviceArg;

    try {
        installSignalHandlers();

        general_options_description.add_options()
                                               (Options::kDevice,
                                                boost::program_options::value<std::string>(),
                                                "specify device")
                                               (Options::kNumberOfLeds,
                                                boost::program_options::value<unsigned short>()->default_value(0),
                                                "set number of leds")
                                               (Options::kColor,
                                                boost::program_options::value<std::string>()->default_value(std::string("255,255,255")),
                                                "set LEDs color")
                                               (Options::kBrightness,
                                                boost::program_options::value<float>()->default_value(0.5f),
                                                "set LEDs brightness")
                                               (Options::kRate,
                                                boost::program_options::value<float>()->default_value(1.0f),
                                                "set dynamic lighting effect rate")
                                               //(Options::kStrobe, "run in stroboscope mode")
                                               (Options::kDebug,
                                                boost::program_options::value<int>()->default_value(0),
                                                "stroboscope mode");

        if (argc < 2) optionsError("mode was not specified\n");

        auto mode = determineMode(argv[1]);
        if (mode == Mode::none) optionsError("invalid mode was specified\n");

        boost::program_options::store(boost::program_options::command_line_parser(argc - 1, argv + 1).options(general_options_description).run(), vm, true);
        boost::program_options::notify(vm);

        switch (mode) {
            case Mode::help:
                print_usage(general_options_description);
                exit(0);

            case Mode::version:
                std::cout << VERSION_STRING(VERSION, __DATE__, __TIME__) << std::endl;
                exit(0);

            default: break;
        }

        auto usbRetval = libusb_init(&usbContext);
        if (usbRetval != LIBUSB_SUCCESS) USB::error(0xB27D7F31, usbRetval);

        libusb_set_debug(usbContext, vm["debug"].as<int>());

        if (mode == Mode::list)
            USB::listDevices(usbContext);
        else {
            deviceHandle = USB::discoverDevice(usbContext, [](libusb_device* device, libusb_device_descriptor* deviceDescr) -> bool {
                if (deviceDescr->idVendor == 0x16c0 && deviceDescr->idProduct == 0x5dc)
                //if (deviceDescr->idVendor == 0xf182 && deviceDescr->idProduct == 0x3)
                    return true;

                return false;
            });

            if (deviceHandle) {
                int activeConfiguration;
                auto numberOfLeds = vm["nleds"].as<unsigned short>();
                auto brightness = vm["brightness"].as<float>();

                usbRetval = libusb_get_configuration(deviceHandle, &activeConfiguration);
                if (usbRetval != LIBUSB_SUCCESS) USB::error(0x83A06C0E, usbRetval);

                printf("Configuration %u is active\n", activeConfiguration);

                usbRetval = libusb_set_configuration(deviceHandle, 0);
                if (usbRetval != LIBUSB_SUCCESS) USB::error(0x6FB9CEAE, usbRetval);

                usbRetval = libusb_control_transfer(deviceHandle, LIBUSB_REQUEST_TYPE_VENDOR, kRequestSetNumberOfLeds, numberOfLeds, 0, 0, 0, 10000);
                if (usbRetval != LIBUSB_SUCCESS) USB::error(0x2CDFB90F, usbRetval);

                RGB<uint8_t>* leds = new RGB<uint8_t>[numberOfLeds];
                if (!leds) RT::error(0x5DCA4D53);

                switch (mode) {
                    case Mode::continuous: {
                        unsigned int r, g, b;
                        sscanf(vm["color"].as<std::string>().c_str(), "%u,%u,%u", &r, &g, &b);

                        r *= brightness;
                        g *= brightness;
                        b *= brightness;
                        for (auto i = 0; i < numberOfLeds; i++) leds[convertLedNumber(i)].set(r, g, b);
/*
for (auto i = 0; i < numberOfLeds; i++) leds[i].set(0, 0, 0);
leds[0].set(200, 200, 200);

                        unsigned char* leds2 = new unsigned char[3 * numberOfLeds];
                        for (auto i = 0; i < numberOfLeds; i++) {
                            leds2[3 * i + 0] = 0;
                            leds2[3 * i + 1] = 0;
                            leds2[3 * i + 2] = 0;
                        }

                        int i = 3;
                            leds2[3 * i + 0] = 100;
                            leds2[3 * i + 1] = 100;
                            leds2[3 * i + 2] = 100;
*/
                        usbRetval = libusb_control_transfer(deviceHandle, LIBUSB_REQUEST_TYPE_VENDOR,
                                                            kRequestSetColors, 0, 0,
                                                            (unsigned char*)leds, numberOfLeds * 3,
                                                            10000);
                        if (usbRetval != LIBUSB_SUCCESS) {
                            delete [] leds;
                            USB::error(0x18804548, usbRetval);
                        }
                    }
                    break;

                    case Mode::rainbow: {
                        HSV<float> hsv(0.0f, 1.0f, brightness);

                        while (!didReceivedSIGTERM) {
                            rotateColorCycle(leds, numberOfLeds, &hsv, vm["rate"].as<float>());
                            usbRetval = libusb_control_transfer(deviceHandle, LIBUSB_REQUEST_TYPE_VENDOR, kRequestSetColors, 0, 0, (unsigned char*)leds, numberOfLeds * sizeof(RGB<uint8_t>), 10000);
                            if (usbRetval != LIBUSB_SUCCESS) {
                                delete [] leds;
                                USB::error(0xE963E77E, usbRetval);
                            }
                        }
                    }
                    break;

                    case Mode::ambilight: {
                        unsigned int count = 0;
                        time_t t1, t2;
                        //unsigned char* pixelData = 0;
                        //size_t pixelDataSize = 0;

                        time(&t1);

                        while (!didReceivedSIGTERM) {

                            CGImageRef image = CGDisplayCreateImage(kCGDirectMainDisplay);
                            if (!image) RT::error(0x01BB3D1E);

                            //printCGImageInfo(image);

                            CFDataRef data = CGDataProviderCopyData(CGImageGetDataProvider(image));
                            const unsigned char* pixelData =  CFDataGetBytePtr(data);

                            auto width = (unsigned int)CGImageGetWidth(image);
                            auto height = (unsigned int)CGImageGetHeight(image);
                            //auto currentPixelDataSize = 4 * width * height;
/*
                            if (pixelData) {
                                if (pixelDataSize < currentPixelDataSize) {
                                    free(pixelData);
                                    pixelData = (unsigned char*)calloc(currentPixelDataSize, sizeof(unsigned char));
                                    if (!pixelData) RT::error(0x2A72C4CF);
                                    pixelDataSize = currentPixelDataSize;
                                }
                            }
                            else {
                                pixelData = (unsigned char*)calloc(currentPixelDataSize, sizeof(unsigned char));
                                if (!pixelData) RT::error(0x07D33573);
                            }

                            getPixelData(image, pixelData);*/

                            computeAmbilightColors(leds, pixelData, CGImageGetBytesPerRow(image), CGImageGetBitsPerPixel(image) >> 3, width, height, 9, 9, 0, 12);
//printf("eee\n");
                            CFRelease(data);
                            CGImageRelease(image);

                            usbRetval = libusb_control_transfer(deviceHandle, LIBUSB_REQUEST_TYPE_VENDOR,
                                                                kRequestSetColors, 0, 0,
                                                                (unsigned char*)leds, numberOfLeds * 3,
                                                                10000);
                            if (usbRetval != LIBUSB_SUCCESS) {
                             //   delete [] leds;
                              //  USB::error(0xE963E77E, usbRetval);
                            }

                            count++;
                        }

                        time(&t2);

                        auto fps = float(count) / float(t2 - t1);
                        printf("fps=%f\n", fps);
                    }
                    break;

                    default: break;
                }

                delete [] leds;
            }
            else
                printf("No device found\n");
        }
    }
    catch (unsigned int ID) {
        printf("Error 0x%x\n", ID);
    }
    catch (USB::Error e) {
        printf("USB Error %d (%s) at 0x%x\n", e.usbError, libusb_error_name(e.usbError), e.error);
        retval = -1;
    }
    catch (OptionsError e) {
        printf("Invalid program options: %s\n", e.getMessage());
        print_usage(general_options_description);
        retval = -1;
    }
    catch (...) {
        printf("Exception\n");
    }

    if (deviceHandle) libusb_close(deviceHandle);
    if (usbContext) libusb_exit(usbContext);

    return retval;
}
