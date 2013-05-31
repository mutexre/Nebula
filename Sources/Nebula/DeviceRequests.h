#ifndef header_AEABFDCE
#define header_AEABFDCE

enum Request {
    kRequestUUID = 0,
    kRequestNumberOfLeds = 1,
    kRequestColors = 2
};

// Options for kRequestUUID
enum UuidType {
    kUuidTypeDeviceRevision = 0,
    kUuidTypeFirmwareRevision = 1,
    kUuidTypeDevice = 2,
};

#endif
