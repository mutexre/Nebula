//#include <IOKit/IOKitLib.h>
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

    UInt16 vendor;
    result = (*interface)->GetDeviceVendor(interface, &vendor);
    if (result != kIOReturnSuccess) {
//        result = (*interface)->USBDeviceClose(interface);
        RT::error(0xDF5633AF);
    }

    UInt16 product;
    result = (*interface)->GetDeviceProduct(interface, &product);
    if (result != kIOReturnSuccess) {
//        result = (*interface)->USBDevice(interface);
        RT::error(0x285447CF);
    }    

    getUUID(info.uuid, kUuidTypeDevice);

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
    kern_return_t result = (*interface)->USBDeviceOpen(interface);
    if (result != kIOReturnSuccess)	RT::error(0xF7DC2912);

    result = (*interface)->DeviceRequest(interface, request);
    if (result != kIOReturnSuccess) {
        result = (*interface)->USBDeviceClose(interface);
        RT::error(0xA8D9D7E3);
    }

    result = (*interface)->USBDeviceClose(interface);
}

void Nebula::Apple::HAL::USBDevice::getUUID(uuid_t uuid, int uuidType) {
    IOUSBDevRequest request;

    request.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBVendor, kUSBDevice);
	request.bRequest = kRequestUUID;
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
	request.bRequest = kRequestUUID;
	request.wValue = uuidType;
	request.wIndex = 0;
	request.wLength = sizeof(uuid_t);
	request.wLenDone = 0;
	request.pData = uuid;

    controlRequest(&request);
}

void Nebula::Apple::HAL::USBDevice::getNumberOfLeds(RT::u4* value) {
    IOUSBDevRequest request;

    request.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBVendor, kUSBDevice);
	request.bRequest = kRequestNumberOfLeds;
	request.wValue = 0;
	request.wIndex = 0;
	request.wLength = 0;
	request.wLenDone = 0;
	request.pData = 0;

    controlRequest(&request);
}

void Nebula::Apple::HAL::USBDevice::setNumberOfLeds(RT::u4 value) {
    IOUSBDevRequest request;

    request.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBDevice);
	request.bRequest = kRequestNumberOfLeds;
	request.wValue = value;
	request.wIndex = 0;
	request.wLength = 0;
	request.wLenDone = 0;
	request.pData = 0;

    controlRequest(&request);
}

void Nebula::Apple::HAL::USBDevice::getColors(Color::RGB<RT::u1>* colors, RT::u4 numberOfLeds) {
    IOUSBDevRequest request;

    request.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBVendor, kUSBDevice);
	request.bRequest = kRequestColors;
	request.wValue = 0;
	request.wIndex = 0;
	request.wLength = numberOfLeds * sizeof(Nebula::Color::RGB<RT::u1>);
	request.wLenDone = 0;
	request.pData = colors;

    controlRequest(&request);
}

void Nebula::Apple::HAL::USBDevice::setColors(Color::RGB<RT::u1>* colors, RT::u4 numberOfLeds) {
    IOUSBDevRequest request;

    request.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBDevice);
	request.bRequest = kRequestColors;
	request.wValue = 0;
	request.wIndex = 0;
	request.wLength = numberOfLeds * sizeof(Nebula::Color::RGB<RT::u1>);
	request.wLenDone = 0;
	request.pData = colors;

    controlRequest(&request);
}

void Nebula::Apple::HAL::USBDevice::control(Request request, void* input, size_t inputSize, void* output, size_t outputSize) {
    switch (request) {
        case Request::getDeviceRevisionUUID:
            if (outputSize >= sizeof(uuid_t)) getUUID((unsigned char*)output, kUuidTypeDeviceRevision);
            else RT::error(0x93A00F7C);
        break;

        case Request::setDeviceRevisionUUID:
            if (inputSize >= sizeof(uuid_t)) setUUID((unsigned char*)input, kUuidTypeDeviceRevision);
            else RT::error(0xFAD12DA8);
        break;

        case Request::getFirmwareRevisionUUID:
            if (outputSize >= sizeof(uuid_t)) getUUID((unsigned char*)output, kUuidTypeFirmwareRevision);
            else RT::error(0x93A00F7C);
        break;

        case Request::setFirmwareRevisionUUID:
            if (inputSize >= sizeof(uuid_t)) setUUID((unsigned char*)input, kUuidTypeFirmwareRevision);
            else RT::error(0xFAD12DA8);
        break;

        case Request::getDeviceUUID:
            if (outputSize >= sizeof(uuid_t)) getUUID((unsigned char*)output, kUuidTypeDevice);
            else RT::error(0x93A00F7C);
        break;

        case Request::setDeviceUUID:
            if (inputSize >= sizeof(uuid_t)) setUUID((unsigned char*)input, kUuidTypeDevice);
            else RT::error(0xFAD12DA8);
        break;

        case Request::getNumberOfLeds:
            if (outputSize >= sizeof(RT::u4)) getNumberOfLeds((RT::u4*)output);
            else RT::error(0x0120AC4E);
        break;

        case Request::setNumberOfLeds:
            if (inputSize >= sizeof(RT::u4)) setNumberOfLeds(*((RT::u4*)input));
            else RT::error(0x75959D83);
        break;

        case Request::getColors: {
            auto data = (Nebula::HAL::Device::ColorsIoctlData*)output;
            if (outputSize >= sizeof(Nebula::HAL::Device::ColorsIoctlData)) getColors(data->colors, data->numberOfLeds);
            else RT::error(0x879E9348);
        }
        break;

        case Request::setColors: {
            auto data = (Nebula::HAL::Device::ColorsIoctlData*)input;
            if (inputSize >= sizeof(Nebula::HAL::Device::ColorsIoctlData)) setColors(data->colors, data->numberOfLeds);
            else RT::error(0xBD01D774);
        }
        break;
    }
}
