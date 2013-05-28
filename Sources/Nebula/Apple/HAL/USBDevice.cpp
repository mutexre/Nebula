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
    IOUSBInterfaceInterface300** deviceInterface = 0;
    uuid_t uuid;

    result = IOCreatePlugInInterfaceForService(device, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &pluginInterface, 0);
    if (result != KERN_SUCCESS) RT::error(0x4846E5C9);

    result = (*pluginInterface)->QueryInterface(pluginInterface, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID300), (LPVOID*)&deviceInterface);
    (*pluginInterface)->Release(pluginInterface);
    if (result != KERN_SUCCESS || !deviceInterface) RT::error(0x55BB6E5D);

    result = IOServiceAddInterestNotification(context->ioNotificationPort,
                                              device,
                                              kIOGeneralInterest,
                                              deviceNotificationCallback,
                                              this,
                                              &interestNotification);
    if (result == KERN_SUCCESS) RT::error(0xCE1799EC);

    uuid_generate(uuid);

    this->context = context;
}

Nebula::Apple::HAL::USBDevice::~USBDevice() {
    kern_return_t result;

    if (deviceInterface)
        result = (*deviceInterface)->Release(deviceInterface);

    if (interestNotification)
        result = IOObjectRelease(interestNotification);
}

bool Nebula::Apple::HAL::USBDevice::isRequestSupported(Request request) {
    return true;
}

void Nebula::Apple::HAL::USBDevice::control(Request request, void* input, size_t inputSize, void* output, size_t outputSize) {
}
