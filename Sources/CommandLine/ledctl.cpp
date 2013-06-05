#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <functional>
#include <thread>
#include <uuid/uuid.h>
#include <boost/program_options.hpp>
#include <CoreFoundation/CoreFoundation.h>
#include <Nebula/Nebula.h>
#include "SignalHandling.h"

#define STRINGOLIZER(a) #a
#define VERSION_STRING(version, date, time) "Version " STRINGOLIZER(version) " built on " date " " time

RT::u2 convertLedNumber(RT::u2 n) {
    return 3 * (n / 3) + (2 - n % 3);
}

const char* modes[] = { "help", "version", "list", "continuous", "rainbow", "ambilight", "run" };

enum class Mode {
    none = -1, help = 0, version = 1, list = 2, continuous = 3, rainbow = 4, ambilight = 5, run = 6
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
    auto kFrequency = "freq,f";
    auto kBrightness = "brightness,b";
    auto kColor = "color,c";
    auto kRate = "rate";
    auto kDebug = "debug,d";
};

class OptionsError : public std::exception {
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

void printDeviceInfo(Nebula::HAL::Device* device) {
    Nebula::HAL::Device::Info info = device->getInfo();
    uuid_string_t uuidString;
    uuid_unparse(info.uuid, uuidString);
    printf("device: bus = %u, uuid: %s\n", info.bus, uuidString);
}

struct TimerCallbackParameters {
    Nebula::HAL::Device* device;
    Nebula::HAL::Device::IoctlParameters::Colors* colorsIoctlParameters;
    std::mutex* colorsMutex;

    TimerCallbackParameters(Nebula::HAL::Device* device,
                            Nebula::HAL::Device::IoctlParameters::Colors* colorsIoctlParameters,
                            std::mutex* colorsMutex)
    {
        this->device = device;
        this->colorsIoctlParameters = colorsIoctlParameters;
        this->colorsMutex = colorsMutex;
    }
};

void observerCallback(CFRunLoopObserverRef observer, CFRunLoopActivity activity, void* info) {
    if (doTerminate)
        CFRunLoopStop(CFRunLoopGetCurrent());
}

template <typename mutex_type>
void guardedMemcpy(const void* source, void* destination, size_t size, mutex_type* mutex) {
    std::lock_guard<mutex_type> lock(*mutex);
    memcpy(destination, source, size);
}

void timerCallBack(CFRunLoopTimerRef timer, void* info) {
    auto data = (TimerCallbackParameters*)info;
    std::unique_ptr<Nebula::Color::RGB<RT::u1>> localColors(new Nebula::Color::RGB<RT::u1>[data->colorsIoctlParameters->numberOfLeds]);

    guardedMemcpy(data->colorsIoctlParameters->colors,
                  localColors.get(),
                  data->colorsIoctlParameters->numberOfLeds * sizeof(Nebula::Color::RGB<RT::u1>),
                  data->colorsMutex);

    Nebula::HAL::Device::IoctlParameters::Colors colorsIoctlParameters = *(data->colorsIoctlParameters);
    colorsIoctlParameters.colors = localColors.get();

    (data->device)->controlIn(Nebula::HAL::Device::Request::setColors,
                              &colorsIoctlParameters,
                              sizeof(Nebula::HAL::Device::IoctlParameters::Colors));
}

void runLoop(TimerCallbackParameters* timerCallbackParameters, float frequency)
{
    CFRunLoopObserverContext observerContext;
    observerContext.version = 0;
    observerContext.info = 0;
    observerContext.retain = 0;
    observerContext.release = 0;
    observerContext.copyDescription = 0;

    auto observer = CFRunLoopObserverCreate(kCFAllocatorDefault, kCFRunLoopBeforeTimers, true, 0, observerCallback, &observerContext);
    CFRunLoopAddObserver(CFRunLoopGetCurrent(), observer, kCFRunLoopCommonModes);

    CFRunLoopTimerContext timerContext;
    timerContext.version = 0;
    timerContext.info = timerCallbackParameters;
    timerContext.retain = 0;
    timerContext.release = 0;
    timerContext.copyDescription = 0;

    auto timer = CFRunLoopTimerCreate(kCFAllocatorDefault,
                                      CFAbsoluteTimeGetCurrent(),
                                      1.0 / frequency,
                                      0, 0,
                                      timerCallBack,
                                      &timerContext);

    CFRunLoopAddTimer(CFRunLoopGetCurrent(), timer, kCFRunLoopCommonModes);

    CFRunLoopRun();
}

Nebula::Generator* createGenerator(Mode mode, boost::program_options::variables_map vm) {
    auto numberOfLeds = vm["nleds"].as<RT::u4>();
    switch (mode) {
        case Mode::rainbow:
            return new Rainbow(numberOfLeds,
                               Nebula::Color::HSV<float>(0.0f, 1.0f, vm["brightness"].as<float>()),
                               vm["rate"].as<float>());

        case Mode::ambilight:
            return new Ambilight(numberOfLeds);

        case Mode::run:
            return new RunningLight(numberOfLeds, vm["nn"].as<RT::u4>());

        default: RT::error(0x39AC18F5);
    }

    return 0;
}

int main(int argc, const char** argv)
{
    int retval = 0;
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
                                                boost::program_options::value<RT::s4>()->default_value(0),
                                                "specify LED strip channel")
                                               (Options::kNumberOfLeds,
                                                boost::program_options::value<RT::s4>()->default_value(0),
                                                "set number of leds")
                                               ("nn",
                                                boost::program_options::value<RT::s4>()->default_value(100),
                                                "set running light increment")
                                               (Options::kFrequency,
                                                boost::program_options::value<float>()->default_value(24.0f),
                                                "set number of LEDs color updates per second")
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
        if (mode == Mode::none)
            optionsError("invalid mode was specified\n");

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

        auto onDeviceAddition = [&](Nebula::HAL::Device* device) -> void {
            std::lock_guard<std::mutex> lock(devicesMutex);
            devices.insert(device);
        };

        auto onDeviceRemoval = [&](Nebula::HAL::Device* device) -> void {
            std::lock_guard<std::mutex> lock(devicesMutex);
            devices.erase(device);
        };

        std::unique_ptr<Nebula::HAL::Context> nebula(Nebula::HAL::createContext(onDeviceAddition, onDeviceRemoval));

        if (mode == Mode::list) {
            std::lock_guard<std::mutex> lock(devicesMutex);
            for (auto device : devices) printDeviceInfo(device);
        }
        else {
            auto firstDeviceIterator = devices.begin();
            if (firstDeviceIterator != devices.end())
            {
                auto device = *firstDeviceIterator;
                auto channel = vm["channel"].as<RT::s4>();
                auto numberOfLeds = vm["nleds"].as<RT::s4>();
                auto frequency = vm["freq"].as<float>();
                auto brightness = vm["brightness"].as<float>();
                bool animate = false;

                if (channel < 0) optionsError("channel should be >= 0\n");
                if (numberOfLeds <= 0) optionsError("number of LEDs should be > 0\n");
                if (frequency <= 0.0f) optionsError("frequency should be > 0\n");
                if (brightness < 0.0f) optionsError("brightness should be >= 0\n");

                std::unique_ptr<Nebula::Color::RGB<RT::u1>> colors(new Nebula::Color::RGB<RT::u1>[numberOfLeds]);
                std::mutex colorsMutex;

                Nebula::HAL::Device::IoctlParameters::SetNumberOfLeds setNumberOfLedsIoctlParameters(channel, numberOfLeds);
                Nebula::HAL::Device::IoctlParameters::Colors colorsIoctlParameters(channel, colors.get(), numberOfLeds);

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
                        for (auto i = 0; i < numberOfLeds; i++) colors.get()[convertLedNumber(i)].set(r, g, b);

                        device->controlIn(Nebula::HAL::Device::Request::setColors,
                                          &colorsIoctlParameters,
                                          sizeof(colorsIoctlParameters));
                    }
                    break;

                    case Mode::rainbow:
                    case Mode::ambilight:
                    case Mode::run:
                        animate = true;
                    break;

                    default: break;
                }

                if (animate) {
                    std::unique_ptr<Nebula::Generator> generator(createGenerator(mode, vm));
                    TimerCallbackParameters timerCallbackParameters(device, &colorsIoctlParameters, &colorsMutex);

                    std::thread pluginThread([&]() -> void {
                        std::unique_ptr<Nebula::Color::RGB<RT::u1>> localColors(new Nebula::Color::RGB<RT::u1>[numberOfLeds]);
                        time_t t1, t2;
                        RT::u8 count = 0;

                        time(&t1);
                        while (!doTerminate) {
                            generator->generate(localColors.get());
                            guardedMemcpy(localColors.get(), colors.get(),
                                          numberOfLeds * sizeof(Nebula::Color::RGB<RT::u1>),
                                          &colorsMutex);
                            count++;
                        }
                        time(&t2);

                        if (count > 0)
                            printf("updates per second = %f\n", float(count) / float(t2 - t1));
                    });

                    try {
                        runLoop(&timerCallbackParameters, frequency);
                    }
                    catch(...) {
                        pluginThread.join();
                        throw;
                    }

                    pluginThread.join();
                }
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
    }

    return retval;
}
