#include <signal.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOMessage.h>
#include <uuid/uuid.h>
#include <Runtime/Runtime.h>
#include <Nebula/Apple/HAL/HAL.h>

void Nebula::Apple::HAL::USBDevice::deviceNotificationCallback(void* data,
                                                               io_service_t service,
                                                               natural_t messageType,
                                                               void* messageArgument)
{
    auto nebulaDevice = (USBDevice*)data;
    if (messageType == kIOMessageServiceIsTerminated) {
        nebulaDevice->context->onDeviceRemoval(nebulaDevice);
        delete nebulaDevice;
    }
}

Nebula::Apple::HAL::USBDevice::USBDevice(io_service_t device, Context* context) {
    kern_return_t result;
    IOCFPlugInInterface** pluginInterface = 0;
    SInt32 score;
    IOUSBDeviceInterface** interface = 0;

    result = IOCreatePlugInInterfaceForService(device, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &pluginInterface, &score);
    if (result != KERN_SUCCESS) RT::error(0x4846E5C9);

    result = (*pluginInterface)->QueryInterface(pluginInterface, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (LPVOID*)&interface);
    (*pluginInterface)->Release(pluginInterface);
    if (result != KERN_SUCCESS || !interface) RT::error(0x55BB6E5D);

    result = IOServiceAddInterestNotification(context->ioNotificationPort,
                                              device,
                                              kIOGeneralInterest,
                                              deviceNotificationCallback,
                                              this,
                                              &interestNotification);
    if (result != KERN_SUCCESS) {
        (*interface)->Release(interface);
        RT::error(0xCE1799EC);
    }

    this->context = context;
    this->interface = interface;   

    getUUID(info.uuid, Nebula::Firmware::UuidType::device);

    info.bus = Bus::USB;
}

Nebula::Apple::HAL::USBDevice::~USBDevice() {
    if (interface) {
        auto result = (*interface)->Release(interface);
        if (result != KERN_SUCCESS) RT::error(0x2D8BFDE3);
    }

    if (interestNotification) {
        auto result = IOObjectRelease(interestNotification);
        if (result != KERN_SUCCESS) RT::error(0x47B09626);
    }
}

bool Nebula::Apple::HAL::USBDevice::isRequestSupported(Request request) {
    return true;
}

void Nebula::Apple::HAL::USBDevice::controlRequest(IOUSBDevRequest* request) {
    sigset_t maskSet;
    sigset_t oldMaskSet;

    sigfillset(&maskSet);
    sigprocmask(SIG_SETMASK, &maskSet, &oldMaskSet);

    kern_return_t result = (*interface)->USBDeviceOpen(interface);
    if (result != kIOReturnSuccess)	RT::error(0xF7DC2912);

    result = (*interface)->DeviceRequest(interface, request);
    if (result != kIOReturnSuccess) {
        result = (*interface)->USBDeviceClose(interface);
        RT::error(0xA8D9D7E3);
    }

    result = (*interface)->USBDeviceClose(interface);
    if (result != kIOReturnSuccess)	RT::error(0xC46D6830);

    sigprocmask(SIG_SETMASK, &oldMaskSet, 0);
}

void Nebula::Apple::HAL::USBDevice::getUUID(uuid_t uuid, int uuidType) {
    IOUSBDevRequest request;

    request.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBVendor, kUSBDevice);
	request.bRequest = Nebula::Firmware::Request::UUID;
	request.wValue = uuidType;
	request.wIndex = 0;
	request.wLength = sizeof(uuid_t);
	request.wLenDone = 0;
	request.pData = uuid;

    controlRequest(&request);
}

void Nebula::Apple::HAL::USBDevice::setUUID(uuid_t uuid, int uuidType) {
    IOUSBDevRequest request;

    request.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBDevice);
	request.bRequest = Nebula::Firmware::Request::UUID;
	request.wValue = uuidType;
	request.wIndex = 0;
	request.wLength = sizeof(uuid_t);
	request.wLenDone = 0;
	request.pData = uuid;

    controlRequest(&request);
}

RT::u4 Nebula::Apple::HAL::USBDevice::getNumberOfChannels() {
    IOUSBDevRequest request;

    request.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBVendor, kUSBDevice);
	request.bRequest = Nebula::Firmware::Request::numberOfChannels;
	request.wValue = 0;
	request.wIndex = 0;
	request.wLength = 0;
	request.wLenDone = 0;
	request.pData = 0;

    controlRequest(&request);

    return request.wValue;
}

RT::u4 Nebula::Apple::HAL::USBDevice::getNumberOfLeds(RT::u4 channel) {
    IOUSBDevRequest request;

    request.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBVendor, kUSBDevice);
	request.bRequest = Nebula::Firmware::Request::numberOfLeds;
	request.wValue = 0;
	request.wIndex = channel;
	request.wLength = 0;
	request.wLenDone = 0;
	request.pData = 0;

    controlRequest(&request);

    return request.wValue;
}

void Nebula::Apple::HAL::USBDevice::setNumberOfLeds(RT::u4 channel, RT::u4 value) {
    IOUSBDevRequest request;

    request.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBDevice);
	request.bRequest = Nebula::Firmware::Request::numberOfLeds;
	request.wValue = value;
	request.wIndex = channel;
	request.wLength = 0;
	request.wLenDone = 0;
	request.pData = 0;

    controlRequest(&request);
}

void Nebula::Apple::HAL::USBDevice::getColors(RT::u4 channel, Color::RGB<RT::u1>* colors, RT::u4 numberOfLeds) {
    IOUSBDevRequest request;

    request.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBVendor, kUSBDevice);
	request.bRequest = Nebula::Firmware::Request::colors;
	request.wValue = 0;
	request.wIndex = channel;
	request.wLength = numberOfLeds * sizeof(Nebula::Color::RGB<RT::u1>);
	request.wLenDone = 0;
	request.pData = colors;

    controlRequest(&request);
}

void Nebula::Apple::HAL::USBDevice::setColors(RT::u4 channel, Color::RGB<RT::u1>* colors, RT::u4 numberOfLeds) {
    IOUSBDevRequest request;

    request.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBDevice);
	request.bRequest = Nebula::Firmware::Request::colors;
	request.wValue = 0;
	request.wIndex = channel;
	request.wLength = numberOfLeds * sizeof(Nebula::Color::RGB<RT::u1>);
	request.wLenDone = 0;
	request.pData = colors;

    controlRequest(&request);
}

void Nebula::Apple::HAL::USBDevice::control(Request request, void* input, size_t inputSize, void* output, size_t outputSize) {
    switch (request) {
        case Request::getDeviceRevisionUUID:
            if (outputSize >= sizeof(uuid_t))
                getUUID((unsigned char*)output, Nebula::Firmware::UuidType::deviceRevision);
            else
                RT::error(0x93A00F7C);
        break;

        case Request::setDeviceRevisionUUID:
            if (inputSize >= sizeof(uuid_t))
                setUUID((unsigned char*)input, Nebula::Firmware::UuidType::deviceRevision);
            else
                RT::error(0xFAD12DA8);
        break;

        case Request::getFirmwareRevisionUUID:
            if (outputSize >= sizeof(uuid_t))
                getUUID((unsigned char*)output, Nebula::Firmware::UuidType::firmwareRevision);
            else
                RT::error(0x93A00F7C);
        break;

        case Request::setFirmwareRevisionUUID:
            if (inputSize >= sizeof(uuid_t))
                setUUID((unsigned char*)input, Nebula::Firmware::UuidType::firmwareRevision);
            else
                RT::error(0xFAD12DA8);
        break;

        case Request::getDeviceUUID:
            if (outputSize >= sizeof(uuid_t))
                getUUID((unsigned char*)output, Nebula::Firmware::UuidType::device);
            else
                RT::error(0x93A00F7C);
        break;

        case Request::setDeviceUUID:
            if (inputSize >= sizeof(uuid_t))
                setUUID((unsigned char*)input, Nebula::Firmware::UuidType::device);
            else
                RT::error(0xFAD12DA8);
        break;

        case Request::getNumberOfChannels:
            if (outputSize >= sizeof(RT::u4))
                *((RT::u4*)output) = getNumberOfChannels();
            else
                RT::error(0x0120AC4E);
        break;

        case Request::getNumberOfLeds:
            if (inputSize >= sizeof(RT::u4) && outputSize >= sizeof(RT::u4)) {
                auto channel = *((RT::u4*)input);
                *((RT::u4*)output) = getNumberOfLeds(channel);
            }
            else
                RT::error(0x0120AC4E);
        break;

        case Request::setNumberOfLeds:
            if (inputSize >= sizeof(Nebula::HAL::Device::IoctlParameters::SetNumberOfLeds)) {
                auto data = (Nebula::HAL::Device::IoctlParameters::SetNumberOfLeds*)input;
                setNumberOfLeds(data->channel, data->numberOfLeds);
            }
            else
                RT::error(0x75959D83);
        break;

        case Request::getColors: {
            if (outputSize >= sizeof(Nebula::HAL::Device::IoctlParameters::Colors)) {
                auto data = (Nebula::HAL::Device::IoctlParameters::Colors*)output;
                getColors(data->channel, data->colors, data->numberOfLeds);
            }
            else
                RT::error(0x879E9348);
        }
        break;

        case Request::setColors: {
            if (inputSize >= sizeof(Nebula::HAL::Device::IoctlParameters::Colors)) {
                auto data = (Nebula::HAL::Device::IoctlParameters::Colors*)input;
                setColors(data->channel, data->colors, data->numberOfLeds);
            }
            else
                RT::error(0xBD01D774);
        }
        break;
    }
}
