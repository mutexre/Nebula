#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <functional>
#include <uuid/uuid.h>
#include <boost/program_options.hpp>
#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>
#include <Nebula/Nebula.h>
#include "SignalHandling.h"

#define STRINGOLIZER(a) #a
#define VERSION_STRING(version, date, time) "Version " STRINGOLIZER(version) " built on " date " " time

RT::u2 convertLedNumber(RT::u2 n) {
    return 3 * (n / 3) + (2 - n % 3);
}

void rotateColorCycle(Nebula::Color::RGB<RT::u1>* leds, RT::u2 numberOfLeds, Nebula::Color::HSV<float>* hsv, float rate)
{
    if (hsv->h < 360.0f) {
        hsv->h += rate;

        for (int i = 0; i < numberOfLeds; i++) {
            float h = hsv->h + float(i) * (360.0f / float(numberOfLeds));
            if (h >= 360.0f) h -= 360.0f;

            Nebula::Color::HSV<float> _hsv(h, hsv->s, hsv->v);
            Nebula::Color::RGB<float> rgb = hsv2rgb(_hsv);

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
    std::cout << "Usage: " << getprogname() << " mode [options]\nWhere mode is one of the following: ";
    auto numberOfModes = sizeof(modes) / sizeof(modes[0]);
    for (auto i = 0; i < numberOfModes; i++) {
        std::cout << modes[i];
        if (i < numberOfModes - 1) std::cout << ", ";
    }
    std::cout << std::endl << options;
}

namespace Options {
    auto kDevice = "device,d";
    auto kChannel = "channel";
    auto kNumberOfLeds = "nleds,n";
    auto kBrightness = "brightness,b";
    auto kColor = "color,c";
    auto kRate = "rate";
    auto kDebug = "debug,d";
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

void getPixelData(CGImageRef imageRef, RT::u1* pixelData) {
    auto width = CGImageGetWidth(imageRef);
    auto height = CGImageGetHeight(imageRef);
    auto colorSpace = CGColorSpaceCreateDeviceRGB();
    RT::u4 bytesPerPixel = 4;
    auto bytesPerRow = bytesPerPixel * width;
    RT::u4 bitsPerComponent = 8;

    CGContextRef context = CGBitmapContextCreate(pixelData, width, height,
                                                 bitsPerComponent, bytesPerRow, colorSpace,
                                                 kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(colorSpace);

    CGContextDrawImage(context, CGRectMake(0, 0, width, height), imageRef);
    CGContextRelease(context);
}

Nebula::Color::RGB<RT::u1> computeAvarageColor(const RT::u1* pixelData,
                                               size_t bytesPerRow, size_t bytesPerPixel,
                                               size_t rectX, size_t rectY,
                                               size_t rectWidth, size_t rectHeight,
                                               std::function<Nebula::Color::RGB<RT::u1> (float r, float g, float b)> colorTransformation)
{
    RT::u4 rSum = 0, gSum = 0, bSum = 0;

    for (auto y = 0; y < rectHeight; y++) {
        auto rowDataOffset = bytesPerRow * (rectY + y) + bytesPerPixel * rectX;
        for (auto x = 0; x < rectWidth; x += 1) {
            auto pixelDataOffset = rowDataOffset + bytesPerPixel * x;
            rSum += pixelData[pixelDataOffset + 2];
            gSum += pixelData[pixelDataOffset + 1];
            bSum += pixelData[pixelDataOffset + 0];
        }
    }

    auto numberOfPixels = float(rectWidth * rectHeight);
//    printf("%f\n", (float(rSum) / numberOfPixels) / 255.0f);

    return colorTransformation((float(rSum) / numberOfPixels) / 255.0f,
                               (float(gSum) / numberOfPixels) / 255.0f,
                               (float(bSum) / numberOfPixels) / 255.0f);
}

RT::u4 calculateBarSize(RT::u4 n, RT::u4 i, RT::u4 size, float a) {
    return 70.0f;
    auto half_n = 0.5f * float(n);
    auto f = (1.0f - a) * (1.0f - fabs(float(i) - half_n) / half_n) + a;
    return (size / 2) * f;
}

void computeAmbilightColors(Nebula::Color::RGB<RT::u1>* leds, const RT::u1* pixelData,
                            size_t bytesPerRow, size_t bytesPerPixel,
                            RT::u4 width, RT::u4 height,
                            RT::u2 left, RT::u2 right,
                            RT::u2 bottom, RT::u2 top,
                            std::function<Nebula::Color::RGB<RT::u1> (float r, float g, float b)> colorTransformation)
{
    RT::u4 ledIndex = 0;

    for (auto y = left - 1; y >= 0; y--)
        leds[convertLedNumber(ledIndex++)] = computeAvarageColor(pixelData,
                                                                 bytesPerRow, bytesPerPixel,
                                                                 0, y * height / left,
                                                                 calculateBarSize(left, y, width, 0.3f), height / left,
                                                                 colorTransformation);

    for (auto x = 0; x < top; x++)
        leds[convertLedNumber(ledIndex++)] = computeAvarageColor(pixelData,
                                                                 bytesPerRow, bytesPerPixel,
                                                                 x * width / top, 0,
                                                                 width / top, calculateBarSize(top, x, height, 0.3f),
                                                                 colorTransformation);

    for (auto y = 0; y < right; y++)
        leds[convertLedNumber(ledIndex++)] = computeAvarageColor(pixelData,
                                                                 bytesPerRow, bytesPerPixel,
                                                                 width - 200, y * height / right,
                                                                 calculateBarSize(right, y, width, 0.3f), height / right,
                                                                 colorTransformation);
}

void printDeviceInfo(Nebula::HAL::Device* device) {
    Nebula::HAL::Device::Info info = device->getInfo();
    uuid_string_t uuidString;
    uuid_unparse(info.uuid, uuidString);
    printf("device: bus = %u, uuid: %s\n", info.bus, uuidString);
}

int main(int argc, const char** argv)
{
    int retval = 0;
    Nebula::HAL::Context* nebula = 0;
    boost::program_options::options_description general_options_description("Options", 140, 60);
    boost::program_options::variables_map vm;
    std::string colorArg, deviceArg;

    try {
        std::set<Nebula::HAL::Device*> devices;
        std::mutex devicesMutex;

        installSignalHandlers();

        general_options_description.add_options()
                                               (Options::kDevice,
                                                boost::program_options::value<std::string>(),
                                                "specify device")
                                               (Options::kChannel,
                                                boost::program_options::value<RT::u4>()->default_value(0),
                                                "specify LED strip channel")
                                               (Options::kNumberOfLeds,
                                                boost::program_options::value<RT::u4>()->default_value(0),
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
                                                boost::program_options::value<RT::u4>()->default_value(0),
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

        nebula = Nebula::HAL::createContext(
            [&](Nebula::HAL::Device* device) -> void {
                devicesMutex.lock();
                devices.insert(device);
                devicesMutex.unlock();
            },
            [&](Nebula::HAL::Device* device) -> void {
                devicesMutex.lock();
                devices.erase(device);
                devicesMutex.unlock();
            }
        );
        if (!nebula) RT::error(0xA733A758);

        if (mode == Mode::list) {
            devicesMutex.lock();
            for (auto device : devices) printDeviceInfo(device);
            devicesMutex.unlock();
        }
        else {
            auto firstDeviceIterator = devices.begin();
            if (firstDeviceIterator != devices.end())
            {
                auto device = *firstDeviceIterator;
                auto channel = vm["channel"].as<RT::u4>();
                auto numberOfLeds = vm["nleds"].as<RT::u4>();
                auto brightness = vm["brightness"].as<float>();

                auto colors = new Nebula::Color::RGB<RT::u1>[numberOfLeds];
                if (!colors) RT::error(0x5DCA4D53);

                Nebula::HAL::Device::IoctlParameters::SetNumberOfLeds setNumberOfLedsIoctlParameters(channel, numberOfLeds);
                Nebula::HAL::Device::IoctlParameters::Colors colorsIoctlParameters(channel, colors, numberOfLeds);

                device->controlIn(Nebula::HAL::Device::Request::setNumberOfLeds,
                                  &setNumberOfLedsIoctlParameters,
                                  sizeof(setNumberOfLedsIoctlParameters));

                switch (mode) {
                    case Mode::continuous: {
                        RT::u4 r, g, b;
                        sscanf(vm["color"].as<std::string>().c_str(), "%u,%u,%u", &r, &g, &b);

                        r *= brightness;
                        g *= brightness;
                        b *= brightness;
                        for (auto i = 0; i < numberOfLeds; i++) colors[convertLedNumber(i)].set(r, g, b);

                        device->controlIn(Nebula::HAL::Device::Request::setColors,
                                          &colorsIoctlParameters,
                                          sizeof(colorsIoctlParameters));
                    }
                    break;

                    case Mode::rainbow: {
                        Nebula::Color::HSV<float> hsv(0.0f, 1.0f, brightness);

                        while (!doTerminate) {
                            rotateColorCycle(colors, numberOfLeds, &hsv, vm["rate"].as<float>());
                            device->controlIn(Nebula::HAL::Device::Request::setColors,
                                              &colorsIoctlParameters,
                                              sizeof(colorsIoctlParameters));
                        }
                    }
                    break;

                    case Mode::ambilight: {
                        RT::u4 count = 0;
                        time_t t1, t2;

                        time(&t1);

                        while (!doTerminate) {

                            CGImageRef image = CGDisplayCreateImage(kCGDirectMainDisplay);
                            if (!image) RT::error(0x01BB3D1E);

                            //printCGImageInfo(image);

                            CFDataRef data = CGDataProviderCopyData(CGImageGetDataProvider(image));
                            const RT::u1* pixelData =  CFDataGetBytePtr(data);

                            auto width = (RT::u4)CGImageGetWidth(image);
                            auto height = (RT::u4)CGImageGetHeight(image);

                            auto colorTransformation = bind(Nebula::Color::transformSaturationOfRgb,
                                                            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                                                            [=](float saturation) -> float { return powf(saturation, 0.3f); });

                            computeAmbilightColors(colors, pixelData,
                                                   CGImageGetBytesPerRow(image), CGImageGetBitsPerPixel(image) >> 3,
                                                   width, height,
                                                   9, 9, 0, 12,
                                                   colorTransformation);

                            CFRelease(data);
                            CGImageRelease(image);

                            device->controlIn(Nebula::HAL::Device::Request::setColors,
                                              &colorsIoctlParameters,
                                              sizeof(colorsIoctlParameters));

                            count++;
                        }

                        time(&t2);

                        auto fps = float(count) / float(t2 - t1);
                        printf("fps=%f\n", fps);
                    }
                    break;

                    default: break;
                }

                delete [] colors;
            }
            else
                printf("No device found\n");
        }
    }
    catch (RT::u4 ID) {
        printf("Error 0x%x\n", ID);
    }
    catch (OptionsError e) {
        printf("Invalid program invocation: %s", e.getMessage());
        print_usage(general_options_description);
        retval = -1;
    }
    catch (...) {
        printf("Exception occured\n");
        RT::printBacktrace(STDERR_FILENO);
    }

    if (nebula) delete nebula;

    return retval;
}
