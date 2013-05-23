#include <Runtime/Runtime.h>
#include "usb.h"

USB::Error::Error(unsigned int error, int usbError) {
    this->error = error;
    this->usbError = usbError;
}

void USB::error(unsigned int ID, int usbError) {
    throw USB::Error(ID, usbError);
}

const char* USB::getTransferTypeString(unsigned char type) {
    switch (type) {
        case LIBUSB_TRANSFER_TYPE_CONTROL: return "Control";
        case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS: return "Isochronous";
        case LIBUSB_TRANSFER_TYPE_BULK: return "Bulk";
        case LIBUSB_TRANSFER_TYPE_INTERRUPT: return "Interrupt";
    }
    return "Unknown";
}

const char* USB::getSynchonizationTypeString(unsigned char type) {
    switch (type) {
        case LIBUSB_ISO_SYNC_TYPE_NONE: return "No Synchonization";
        case LIBUSB_ISO_SYNC_TYPE_ASYNC: return "Asynchronous";
        case LIBUSB_ISO_SYNC_TYPE_ADAPTIVE: return "Adaptive";
        case LIBUSB_ISO_SYNC_TYPE_SYNC: return "Synchronous";
    }
    return "Unknown";
}

const char* USB::getUsageTypeString(unsigned char type) {
    switch (type) {
        case LIBUSB_ISO_USAGE_TYPE_DATA: return "Data";
        case LIBUSB_ISO_USAGE_TYPE_FEEDBACK: return "Feedback";
        case LIBUSB_ISO_USAGE_TYPE_IMPLICIT: return "Explicit Feedback Data";
    }
    return "Unknown";
}

const char* USB::getClassCodeString(uint8_t code) {
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

void USB::printDevice(const libusb_device_descriptor* descr) {
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

void USB::printConfiguration(const libusb_config_descriptor* descr) {
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

void USB::printInterface(const libusb_interface_descriptor* descr) {
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

void USB::printEndpoint(const libusb_endpoint_descriptor* descr) {
    printf("  address: 0x%x (number = %u, direction = %s)\n", descr->bEndpointAddress,
                                                              descr->bEndpointAddress & 7,
                                                              descr->bEndpointAddress & LIBUSB_ENDPOINT_IN ? "Device to Host" : "Host to Device");

    printf("  attributes: 0x%x (transfer type = %s", descr->bmAttributes,
                                                     getTransferTypeString(descr->bmAttributes & 3));

    if ((descr->bmAttributes & 3) == 1)
        printf(", synchronization type = %s, usage type = %s)\n", getSynchonizationTypeString((descr->bmAttributes & 0xc) >> 2),
                                                                 getUsageTypeString((descr->bmAttributes & 0x30) >> 4));
    else
        printf(")\n");

    printf("  max packet size: %u\n  interval: %u\n  refresh: %u\n  synch address: %u\n",
        descr->wMaxPacketSize, descr->bInterval, descr->bRefresh, descr->bSynchAddress);
}

void USB::listDevices(libusb_context* context) {
    libusb_device** list;

    ssize_t count = libusb_get_device_list(context, &list);
    ssize_t i = 0;

    for (i = 0; i < count; i++) {
        int usbRetval;
        libusb_device* device = list[i];
        libusb_device_descriptor descr;

        usbRetval = libusb_get_device_descriptor(device, &descr);
        if (usbRetval != LIBUSB_SUCCESS) error(0x18001D98, usbRetval);

        printf("Device:\n");
        printDevice(&descr);

        for (auto j = 0; j < descr.bNumConfigurations; j++) {
            libusb_config_descriptor* config;

            usbRetval = libusb_get_config_descriptor(device, j, &config);
            if (usbRetval != LIBUSB_SUCCESS) error(0xFAEF8129, usbRetval);

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
USB::discoverDevice(libusb_context* context, std::function<bool (libusb_device*, libusb_device_descriptor*)> predicate)
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
        if (usbRetval != LIBUSB_SUCCESS) error(0x18001D98, usbRetval);

        if (predicate(device, &descr))
        {
            printf("Device:\n");
            printDevice(&descr);

            for (auto j = 0; j < descr.bNumConfigurations; j++) {
                libusb_config_descriptor* config;

                usbRetval = libusb_get_config_descriptor(device, j, &config);
                if (usbRetval != LIBUSB_SUCCESS) error(0xFAEF8129, usbRetval);

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
            RT::error(0xA88676C7);
            handle = 0;
        }
    }

    libusb_free_device_list(list, 1);

    return handle;
}
