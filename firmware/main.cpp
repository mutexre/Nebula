#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <FastSPI_LED2.h>
extern "C" {
    #include "usbdrv.h"
}
#include "../RGB.h"
#include "../Shared.h"

// #define DEBUG
#ifdef DEBUG
#define DPRINT Serial.print
#define DPRINTLN Serial.println
#else
#define DPRINT(x)
#define DPRINTLN(x)
#endif

RGB<uint8_t> leds[NUM_LEDS];
TM1809Controller800Mhz<13> LED;

#if 0

int count = 0;
long start = 0;//millis();

unsigned char val = 1;
char dir = 1;

#if 1

HSV hsv(0.0, 1.0, 0.9);

void loop()
{
  if (hsv.h < 360.0)
  {
    hsv.h += 5.0;

    for(int i = 0; i < NUM_LEDS; i++) {
        float h = hsv.h + float(i) * (360.0 / float(NUM_LEDS));
        if (h >= 360.0) h -= 360.0;
        HSV _hsv(h, hsv.s, hsv.v);
        RGB rgb = _hsv.rgb();
        leds[i].r = 255 * rgb.r;
        leds[i].g = 255 * rgb.g;
        leds[i].b = 255 * rgb.b;
    }
  }
  else
  {
    hsv.h = 0.0;
  }
  LED.showRGB((byte*)leds, NUM_LEDS);
}

#endif

int main(void)
{
    DDRB |= 0xFF;
    PORTB ^= 7;
    LED.init();
    while (1) loop();
    /*
    DDRB |= 0xFF;           // make the LED pin an output
    for(;;){
        char i;
        for(i = 0; i < 1; i++){
            _delay_ms(100);  // max is 262.14 ms / F_CPU in MHz
        }
        PORTB ^= 0xFC;    // toggle the LED
    }*/
    return 0;               // never reached
}

#else

#if 0
extern "C" PROGMEM const char usbDescriptorDevice[18] = {    /* USB device descriptor */
    18,         /* sizeof(usbDescriptorDevice): length of descriptor in bytes */
    USBDESCR_DEVICE,        /* descriptor type */
    0x10, 0x01,             /* USB version supported */
    USB_CFG_DEVICE_CLASS,
    USB_CFG_DEVICE_SUBCLASS,
    0,                      /* protocol */
    8,                      /* max packet size */
    /* the following two casts affect the first byte of the constant only, but
     * that's sufficient to avoid a warning with the default values.
     */
    (char)USB_CFG_VENDOR_ID,/* 2 bytes */
    (char)USB_CFG_DEVICE_ID,/* 2 bytes */
    USB_CFG_DEVICE_VERSION, /* 2 bytes */
    1,//USB_CFG_DESCR_PROPS_STRING_VENDOR != 0 ? 1 : 0,         /* manufacturer string index */
    2, //USB_CFG_DESCR_PROPS_STRING_PRODUCT != 0 ? 2 : 0,        /* product string index */
    3, //USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER != 0 ? 3 : 0,  /* serial number string index */
    1,          /* number of configurations */
};

extern "C" PROGMEM const char usbDescriptorConfiguration[9 + 9 + 7] = {    /* USB configuration descriptor */
    9,          /* sizeof(usbDescriptorConfiguration): length of descriptor in bytes */
    USBDESCR_CONFIG,    /* descriptor type */
    18 + 7 * USB_CFG_HAVE_INTRIN_ENDPOINT + 7 * USB_CFG_HAVE_INTRIN_ENDPOINT3 +
                (USB_CFG_DESCR_PROPS_HID & 0xff) + 7, 0,
                /* total length of data returned (including inlined descriptors) */
    1,          /* number of interfaces in this configuration */
    1,          /* index of this configuration */
    0,          /* configuration name string index */
#if USB_CFG_IS_SELF_POWERED
    (1 << 7) | USBATTR_SELFPOWER,       /* attributes */
#else
    (1 << 7),                           /* attributes */
#endif
    500 / 2,            /* max USB current in 2mA units */
/* interface descriptor follows inline: */
    9,          /* sizeof(usbDescrInterface): length of descriptor in bytes */
    USBDESCR_INTERFACE, /* descriptor type */
    0,          /* index of this interface */
    0,          /* alternate setting for this interface */
    1 + USB_CFG_HAVE_INTRIN_ENDPOINT + USB_CFG_HAVE_INTRIN_ENDPOINT3, /* endpoints excl 0: number of endpoint descriptors to follow */
    USB_CFG_INTERFACE_CLASS,
    USB_CFG_INTERFACE_SUBCLASS,
    USB_CFG_INTERFACE_PROTOCOL,
    0,          /* string index for interface */
#if (USB_CFG_DESCR_PROPS_HID & 0xff)    /* HID descriptor */
    9,          /* sizeof(usbDescrHID): length of descriptor in bytes */
    USBDESCR_HID,   /* descriptor type: HID */
    0x01, 0x01, /* BCD representation of HID version */
    0x00,       /* target country code */
    0x01,       /* number of HID Report (or other HID class) Descriptor infos to follow */
    0x22,       /* descriptor type: report */
    USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH, 0,  /* total length of report descriptor */
#endif
#if USB_CFG_HAVE_INTRIN_ENDPOINT    /* endpoint descriptor for endpoint 1 */
    7,          /* sizeof(usbDescrEndpoint) */
    USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
    (char)0x81, /* IN endpoint number 1 */
    0x03,       /* attrib: Interrupt endpoint */
    8, 0,       /* maximum packet size */
    USB_CFG_INTR_POLL_INTERVAL, /* in ms */
#endif
#if USB_CFG_HAVE_INTRIN_ENDPOINT3   /* endpoint descriptor for endpoint 3 */
    7,          /* sizeof(usbDescrEndpoint) */
    USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
    (char)(0x80 | USB_CFG_EP3_NUMBER), /* IN endpoint number 3 */
    0x03,       /* attrib: Interrupt endpoint */
    8, 0,       /* maximum packet size */
    USB_CFG_INTR_POLL_INTERVAL, /* in ms */
#endif
    7,          /* sizeof(usbDescrEndpoint) */
    USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
    (char)0x81, /* IN endpoint number 1 */
    0x01,       /* attrib: Interrupt endpoint */
    8, 0,       /* maximum packet size */
    1//USB_CFG_INTR_POLL_INTERVAL, /* in ms */
};

#ifdef USB_CFG_DESCR_PROPS_DEVICE
    #undef USB_CFG_DESCR_PROPS_DEVICE
    #define USB_CFG_DESCR_PROPS_DEVICE 18
    //sizeof(usbDescriptorDevice)
#endif

#ifdef USB_CFG_DESCR_PROPS_CONFIGURATION
    #undef USB_CFG_DESCR_PROPS_CONFIGURATION
    #define USB_CFG_DESCR_PROPS_CONFIGURATION 9 + 9 + 7
    //           sizeof(usbDescriptorConfiguration)
#endif

#endif 

unsigned char state = 0;

#if 1
extern "C" USB_PUBLIC uchar usbFunctionSetup(uchar data[8]) {
    usbRequest_t* rq = (usbRequest_t*)data;

    state = !state;
    // DDRB |= 0xFF; if (state == 1) PORTB = 0xff; else PORTB = 0;

    for(int i = 0; i < NUM_LEDS; i++) {
        leds[i].r = state * 255;
        leds[i].g = 150;
        leds[i].b = 0;
    }

    LED.showRGB((byte*)leds, NUM_LEDS);

/*
    switch(rq->bRequest) { // custom command is in the bRequest field
    case USB_LED_ON:
        PORTB |= 1; // turn LED on
        return 0;
    case USB_LED_OFF:
        PORTB &= ~1; // turn LED off
        return 0;
    }
*/

    return 0;
}
#endif

extern "C" USB_PUBLIC void usbFunctionWriteOut(uchar *data, uchar len) {}

int main() {
    //DDRB |= 0xFF; PORTB = 0xff;

    wdt_enable(WDTO_1S); // enable 1s watchdog timer

    LED.init();
    usbInit();

    usbDeviceDisconnect(); // enforce re-enumeration
    for (int i = 0; i < 250; i++) { // wait 500 ms
        wdt_reset(); // keep the watchdog happy
        _delay_ms(2);
    }
    usbDeviceConnect();

    sei(); // Enable interrupts after re-enumeration

    while (1) {
        wdt_reset(); // keep the watchdog happy
        usbPoll();
    }

    return 0;
}

#endif
