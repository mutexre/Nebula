#include <stdio.h>
#include <unistd.h>
#include <functional>
#include <libusb.h>
#include "../Shared.h"
#include "../RGB.h"
#include "../HSV.h"
#include "../RgbHsv.h"

void error(unsigned int ID) {
    throw ID;
}

class USBError
{
public:
    unsigned error;
    int usbError;

    USBError(unsigned int error, int usbError) {
        this->error = error;
        this->usbError = usbError;
    }
};

void usbError(unsigned int ID, int usbError) {
    throw USBError(ID, usbError);
}

const char* getTransferTypeString(unsigned char type) {
    switch (type) {
        case LIBUSB_TRANSFER_TYPE_CONTROL: return "Control";
        case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS: return "Isochronous";
        case LIBUSB_TRANSFER_TYPE_BULK: return "Bulk";
        case LIBUSB_TRANSFER_TYPE_INTERRUPT: return "Interrupt";
    }
    return "Unknown";
}

const char* getSynchonizationTypeString(unsigned char type) {
    switch (type) {
        case LIBUSB_ISO_SYNC_TYPE_NONE: return "No Synchonization";
        case LIBUSB_ISO_SYNC_TYPE_ASYNC: return "Asynchronous";
        case LIBUSB_ISO_SYNC_TYPE_ADAPTIVE: return "Adaptive";
        case LIBUSB_ISO_SYNC_TYPE_SYNC: return "Synchronous";
    }
    return "Unknown";
}

const char* getUsageTypeString(unsigned char type) {
    switch (type) {
        case LIBUSB_ISO_USAGE_TYPE_DATA: return "Data";
        case LIBUSB_ISO_USAGE_TYPE_FEEDBACK: return "Feedback";
        case LIBUSB_ISO_USAGE_TYPE_IMPLICIT: return "Explicit Feedback Data";
    }
    return "Unknown";
}

const char* getClassCodeString(uint8_t code) {
    switch (code) {
        case LIBUSB_CLASS_PER_INTERFACE: return "Per Interface";
        case LIBUSB_CLASS_AUDIO: return "Audio";
        case LIBUSB_CLASS_COMM: return "Communication";
        case LIBUSB_CLASS_HID: return "HID";
        case LIBUSB_CLASS_PHYSICAL: return "Physical";
        case LIBUSB_CLASS_PRINTER: return "Printer";
        case LIBUSB_CLASS_IMAGE: return "Image";
        case LIBUSB_CLASS_MASS_STORAGE: return "Mass Storage";
        case LIBUSB_CLASS_HUB: return "HUB";
        case LIBUSB_CLASS_DATA: return "Data";
        case LIBUSB_CLASS_SMART_CARD: return "SmartCard";
        case LIBUSB_CLASS_CONTENT_SECURITY: return "Content Security";
        case LIBUSB_CLASS_VIDEO: return "Video";
        case LIBUSB_CLASS_PERSONAL_HEALTHCARE: return "Personal Healthcare";
        case LIBUSB_CLASS_DIAGNOSTIC_DEVICE: return "Diagnostic Device";
        case LIBUSB_CLASS_WIRELESS: return "Wireless";
        case LIBUSB_CLASS_APPLICATION: return "Application Specific";
        case LIBUSB_CLASS_VENDOR_SPEC: return "Vendor Specific";
        case 0x10: return "Audio/Video";
        case 0xef: return "Miscellaneous";
    }
    return "Unknown";
}

void printDevice(const libusb_device_descriptor* descr) {
    printf("\
  class: 0x%x (%s)\n\
  subclass: %u\n\
  protocol: %u\n\
  # configurations: %u\n\
  max packet size: %u\n\
  vendor id: 0x%x\n\
  product id: 0x%x\n\
  manufacturer: %u\n\
  product: %u\n\
  serial: %u\n",
        descr->bDeviceClass, getClassCodeString(descr->bDeviceClass), descr->bDeviceSubClass, descr->bDeviceProtocol,
        descr->bNumConfigurations, descr->bMaxPacketSize0,
        descr->idVendor, descr->idProduct,
        descr->iManufacturer, descr->iProduct, descr->iSerialNumber);
}

void printConfiguration(const libusb_config_descriptor* descr) {
    printf("\
  total length: %u\n\
  # interfaces: %u\n\
  configuration value: %u\n\
  iConfiguration: %u\n\
  attributes: 0x%x (%s, %s)\n\
  max power: %umA\n",
        descr->wTotalLength, descr->bNumInterfaces, descr->bConfigurationValue, descr->iConfiguration,
        descr->bmAttributes, descr->bmAttributes & 0x40 ? "Self Powered" : "Bus Powered", descr->bmAttributes & 0x20 ? "Remote Wakeup" : "No Remote Wakeup",
        2 * descr->MaxPower);
}

void printInterface(const libusb_interface_descriptor* descr) {
    printf("\
  interface number: %u\n\
  alternate setting: %u\n\
  # endpoints: %u\n\
  class: 0x%x (%s)\n\
  subclass: %u\n\
  protocol: %u\n\
  iInterface: %u\n",
        descr->bInterfaceNumber, descr->bAlternateSetting, descr->bNumEndpoints,
        descr->bInterfaceClass, getClassCodeString(descr->bInterfaceClass), descr->bInterfaceSubClass, descr->bInterfaceProtocol,
        descr->iInterface);
}

void printEndpoint(const libusb_endpoint_descriptor* descr) {
    printf("  address: 0x%x (number = %u, direction = %s)\n", descr->bEndpointAddress, descr->bEndpointAddress & 7, descr->bEndpointAddress & LIBUSB_ENDPOINT_IN ? "In" : "Out");
    printf("  attributes: 0x%x (transfer type = %s", descr->bmAttributes, getTransferTypeString(descr->bmAttributes & 3));

    if ((descr->bmAttributes & 3) == 1)
        printf(", synchronization type = %s, usage type = %s)\n", getSynchonizationTypeString((descr->bmAttributes & 0xc) >> 2),
                                                                 getUsageTypeString((descr->bmAttributes & 0x30) >> 4));
    else
        printf(")\n");

    printf("  max packet size: %u\n  interval: %u\n  refresh: %u\n  synch address: %u\n",
        descr->wMaxPacketSize, descr->bInterval, descr->bRefresh, descr->bSynchAddress);
}

void listDevices(libusb_context* context) {
    libusb_device** list;

    ssize_t count = libusb_get_device_list(context, &list);
    ssize_t i = 0;

    for (i = 0; i < count; i++) {
        int usbRetval;
        libusb_device* device = list[i];
        libusb_device_descriptor descr;

        usbRetval = libusb_get_device_descriptor(device, &descr);
        if (usbRetval != LIBUSB_SUCCESS) usbError(0x18001D98, usbRetval);

        printf("Device:\n");
        printDevice(&descr);

        for (auto j = 0; j < descr.bNumConfigurations; j++) {
            libusb_config_descriptor* config;

            usbRetval = libusb_get_config_descriptor(device, j, &config);
            if (usbRetval != LIBUSB_SUCCESS) usbError(0xFAEF8129, usbRetval);

            printf("Configuration %u:\n", j);
            printConfiguration(config);

            for (auto k = 0; k < config->bNumInterfaces; k++) {
                for (auto l = 0; l < config->interface[k].num_altsetting; l++) {
                    printf("Interface %u:\n", k);
                    printInterface(config->interface[k].altsetting + l);
                    for (auto m = 0; m < config->interface[k].altsetting[l].bNumEndpoints; m++) {
                        printf("Endpoint %u:\n", m);
                        printEndpoint(config->interface[k].altsetting[l].endpoint + m);
                    }
                }
            }

            libusb_free_config_descriptor(config);
        }
    }

    libusb_free_device_list(list, 1);
}

libusb_device_handle*
discoverDevice(libusb_context* context, std::function<bool (libusb_device*, libusb_device_descriptor*)> predicate)
{
    int usbRetval;
    libusb_device** list;
    libusb_device* found = 0;
    libusb_device_handle* handle = 0;

    ssize_t count = libusb_get_device_list(context, &list);
    ssize_t i = 0;

    for (i = 0; i < count; i++) {
        libusb_device* device = list[i];
        libusb_device_descriptor descr;

        usbRetval = libusb_get_device_descriptor(device, &descr);
        if (usbRetval != LIBUSB_SUCCESS) usbError(0x18001D98, usbRetval);

        if (predicate(device, &descr))
        {
            printf("Device:\n");
            printDevice(&descr);

            for (auto j = 0; j < descr.bNumConfigurations; j++) {
                libusb_config_descriptor* config;

                usbRetval = libusb_get_config_descriptor(device, j, &config);
                if (usbRetval != LIBUSB_SUCCESS) usbError(0xFAEF8129, usbRetval);

                printf("Configuration %u:\n", j);
                printConfiguration(config);

                for (auto k = 0; k < config->bNumInterfaces; k++) {
                    printf("Interface %u:\n", k);
                    printInterface(config->interface[k].altsetting);
                    for (auto m = 0; m < config->interface[k].altsetting->bNumEndpoints; m++) {
                        printf("Endpoint %u:\n", m);
                        printEndpoint(config->interface[k].altsetting->endpoint + m);
                    }
                }

                libusb_free_config_descriptor(config);
            }

            found = device;
            break;
        }
    }

    if (found) {
        usbRetval = libusb_open(found, &handle);
        if (usbRetval != LIBUSB_SUCCESS) {
            error(0xA88676C7);
            handle = 0;
        }
    }

    libusb_free_device_list(list, 1);

    return handle;
}

struct RGB<uint8_t> leds[NUM_LEDS];
HSV<float> hsv(0.0, 1.0, 0.9);

void loop()
{
  if (hsv.h < 360.0)
  {
    hsv.h += 5.0;

    for(int i = 0; i < NUM_LEDS; i++) {
        float h = hsv.h + float(i) * (360.0 / float(NUM_LEDS));
        if (h >= 360.0) h -= 360.0;
        HSV<float> _hsv(h, hsv.s, hsv.v);
        RGB<float> rgb = hsv2rgb(_hsv);
        leds[i].r = 255 * rgb.r;
        leds[i].g = 255 * rgb.g;
        leds[i].b = 255 * rgb.b;
    }
  }
  else
  {
    hsv.h = 0.0;
  }
}

int main(int argc, const char * argv[])
{
    libusb_context* usbContext = 0;
    libusb_device_handle* deviceHandle = 0;
    
    try {
        int usbRetval;
        int activeConfiguration;

        usbRetval = libusb_init(&usbContext);
        if (usbRetval != LIBUSB_SUCCESS) usbError(0xB27D7F31, usbRetval);

        libusb_set_debug(usbContext, 0);

        deviceHandle = discoverDevice(usbContext, [](libusb_device* device, libusb_device_descriptor* deviceDescr) -> bool {
            if (deviceDescr->idVendor == 0x16c0 && deviceDescr->idProduct == 0x5dc)
            //if (deviceDescr->idVendor == 0xf182 && deviceDescr->idProduct == 0x3)
                return true;

            return false;
        });

        if (deviceHandle) {
            usbRetval = libusb_get_configuration(deviceHandle, &activeConfiguration);
            if (usbRetval != LIBUSB_SUCCESS) usbError(0x83A06C0E, usbRetval);

            printf("Configuration %u is active\n", activeConfiguration);

            usbRetval = libusb_set_configuration(deviceHandle, 0);
            if (usbRetval != LIBUSB_SUCCESS) usbError(0x6FB9CEAE, usbRetval);

            while (1) {
                //printf("sizeof=%lu\n", sizeof(leds));
                loop();
                usbRetval = libusb_control_transfer(deviceHandle, LIBUSB_REQUEST_TYPE_VENDOR, 0, 0, 0, (unsigned char*)leds, sizeof(leds), 0);
                if (usbRetval != LIBUSB_SUCCESS) usbError(0xE963E77E, usbRetval);
                sleep(1);
            }
        }
        else
            printf("No device found\n");
    }
    catch (unsigned int ID) {
        printf("Error 0x%x\n", ID);
    }
    catch (USBError e) {
        printf("Universal Serial Bus Error %d (%s) at 0x%x\n", e.usbError, libusb_error_name(e.usbError), e.error);
    }
    catch (...) {
    }

    if (deviceHandle) libusb_close(deviceHandle);
    if (usbContext) libusb_exit(usbContext);

    return 0;
}
