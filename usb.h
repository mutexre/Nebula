#ifndef header_DDE30372
#define header_DDE30372

#include <functional>
#include <libusb.h>

namespace USB {
    class Error
    {
    public:
        unsigned error;
        int usbError;

        Error(unsigned int error, int usbError);
    };

    void error(unsigned int ID, int usbError);

    const char* getTransferTypeString(unsigned char type);
    const char* getSynchonizationTypeString(unsigned char type);
    const char* getUsageTypeString(unsigned char type);
    const char* getClassCodeString(unsigned char code);

    void printDevice(const libusb_device_descriptor* descr);
    void printConfiguration(const libusb_config_descriptor* descr);
    void printInterface(const libusb_interface_descriptor* descr);
    void printEndpoint(const libusb_endpoint_descriptor* descr);

    void listDevices(libusb_context* context);
    libusb_device_handle* discoverDevice(libusb_context* context, std::function<bool (libusb_device*, libusb_device_descriptor*)> predicate);
}

#endif
