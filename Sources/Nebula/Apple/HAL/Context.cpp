#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <mach/mach.h>
#include <Runtime/Runtime.h>
#include <Nebula/Apple/HAL/HAL.h>

/*
            void DeviceAdded(void *refCon, io_iterator_t iterator)
            {
                kern_return_t		kr;
                io_service_t		usbDevice;
                IOCFPlugInInterface 	**plugInInterface=NULL;
                SInt32 			score;
                HRESULT 			res;

                while ( (usbDevice = IOIteratorNext(iterator)) )
                {
                    io_name_t		deviceName;
                    CFStringRef		deviceNameAsCFString;	
                    MyPrivateData		*privateDataRef = NULL;
                    UInt32			locationID;

                    printf("Device 0x%08x added.\n", usbDevice);
                    
                    // Add some app-specific information about this device.
                    // Create a buffer to hold the data.
                    
                    privateDataRef = malloc(sizeof(MyPrivateData));
                    bzero( privateDataRef, sizeof(MyPrivateData));
                    
                    // In this sample we'll just use the service's name.
                    //
                    kr = IORegistryEntryGetName(usbDevice, deviceName);
                if (KERN_SUCCESS != kr)
                    {
                        deviceName[0] = '\0';
                    }
                    
                    deviceNameAsCFString = CFStringCreateWithCString(kCFAllocatorDefault, deviceName, kCFStringEncodingASCII);
                    
                    // Dump our data to stdout just to see what it looks like.
                    //
                    CFShow(deviceNameAsCFString);
                    
                    privateDataRef->deviceName = deviceNameAsCFString;

                    // Now, get the locationID of this device.  In order to do this, we need to create an IOUSBDeviceInterface for
                    // our device.  This will create the necessary connections between our user land application and the kernel object
                    // for the USB Device.
                    //
                    kr = IOCreatePlugInInterfaceForService(usbDevice, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);

                    if ((kIOReturnSuccess != kr) || !plugInInterface)
                    {
                        printf("unable to create a plugin (%08x)\n", kr);
                        continue;
                    }

                    // I have the device plugin, I need the device interface
                    //
                    res = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (LPVOID)&privateDataRef->deviceInterface);
                    (*plugInInterface)->Release(plugInInterface);			// done with this
                    if (res || !privateDataRef->deviceInterface)
                    {
                        printf("couldn't create a device interface (%08x)\n", (int) res);
                        continue;
                    }

                    // Now that we have the IOUSBDeviceInterface, we can call the routines in IOUSBLib.h
                    // In this case, we just want the locationID.
                    //
                    kr = (*privateDataRef->deviceInterface)->GetLocationID(privateDataRef->deviceInterface, &locationID);
                    if (KERN_SUCCESS != kr)
                    {
                        printf("GetLocationID returned %08x\n", kr);
                        continue;
                    }
                    else
                    {
                        printf("Location ID: 0x%lx\n", locationID);
                        
                    }

                    privateDataRef->locationID = locationID;

                    // Register for an interest notification for this device. Pass the reference to our
                    // private data as the refCon for the notification.
                    //
                    kr = IOServiceAddInterestNotification(	gNotifyPort,			// notifyPort
                                                           usbDevice,			// service
                                                           kIOGeneralInterest,		// interestType
                                                           DeviceNotification,		// callback
                                                           privateDataRef,			// refCon
                                                           &(privateDataRef->notification)	// notification
                                                           );

                    if (KERN_SUCCESS != kr)
                    {
                        printf("IOServiceAddInterestNotification returned 0x%08x\n", kr);
                    }

                    // Done with this io_service_t
                    //
                    kr = IOObjectRelease(usbDevice);
                }
            }
*/

void Nebula::Apple::HAL::Context::matchedNotificationCallback(void* data, io_iterator_t iterator) {
    io_service_t device;
    auto context = (Context*)data;

    while ((device = IOIteratorNext(iterator))) {
        auto nebulaDevice = new USBDevice(device, context);
        if (nebulaDevice) context->onDeviceAddition(nebulaDevice);
    }
}

Nebula::Apple::HAL::Context::Context(std::function<void (Nebula::HAL::Device*)> onDeviceAddition,
                                     std::function<void (Nebula::HAL::Device*)> onDeviceRemoval)
    : Nebula::HAL::Context(onDeviceAddition, onDeviceRemoval)
{
    RT::u4 errorID;
    kern_return_t result;
    mach_port_t masterPort = MACH_PORT_NULL;
    IONotificationPortRef ioNotificationPort = 0;
    CFNumberRef number = 0;
    CFMutableDictionaryRef matchingDict = 0;
    int usbVendorID = 0x16c0;
    int usbProductID = 0x05dc;
    io_iterator_t deviceIterator;

    result = IOMasterPort(MACH_PORT_NULL, &masterPort);
    if (result != KERN_SUCCESS) {
        errorID = 0xCB8FA2B7;
        goto errorOut;
    }

    ioNotificationPort = IONotificationPortCreate(kIOMasterPortDefault);
    if (!ioNotificationPort) {
        errorID = 0x849A8CD6;
        goto errorOut;
    }

    matchingDict = IOServiceMatching(kIOUSBDeviceClassName);
    if (!matchingDict) {
        errorID = 0x0C7A3B7B;
        goto errorOut;
    }

    number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &usbVendorID);
    if (!number) {
        errorID = 0x1D3C3F2A;
        goto errorOut;
    }

    CFDictionarySetValue(matchingDict, CFSTR(kUSBVendorID), number); CFRelease(number);

    number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &usbProductID);
    if (!number) {
        errorID = 0xa6B0324c;
        goto errorOut;
    }

    CFDictionarySetValue(matchingDict, CFSTR(kUSBProductID), number); CFRelease(number);
#if 0
    runLoopSource = IONotificationPortGetRunLoopSource(gNotifyPort);
    if (!runLoopSource) {
        errorId = 0x4dc20286;
        goto errorOut;
    }
    runLoop = CFRunLoopGetCurrent();
    CFRunLoopAddSource(runLoop, runLoopSource, kCFRunLoopDefaultMode);
#endif
    result = IOServiceAddMatchingNotification(ioNotificationPort,
                                              kIOMatchedNotification,
                                              matchingDict,
                                              matchedNotificationCallback,
                                              this, &deviceIterator);
    if (result != KERN_SUCCESS) {
        errorID = 0x75495FE4;
        goto errorOut;
    }

    matchedNotificationCallback(0, deviceIterator);

    this->masterPort = masterPort;
    this->ioNotificationPort = ioNotificationPort;

    return;

errorOut:
    if (ioNotificationPort) IONotificationPortDestroy(ioNotificationPort);
    if (masterPort) mach_port_deallocate(mach_task_self(), masterPort);

    RT::error(errorID);
}

Nebula::Apple::HAL::Context::~Context() {
    if (ioNotificationPort) IONotificationPortDestroy(ioNotificationPort);
    if (masterPort) mach_port_deallocate(mach_task_self(), masterPort);
}

Nebula::HAL::Context*
Nebula::HAL::createContext(std::function<void (Device*)> onDeviceAddition,
                           std::function<void (Device*)> onDeviceRemoval)
{
    return new Nebula::Apple::HAL::Context(onDeviceAddition, onDeviceRemoval);
}
