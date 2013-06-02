#ifndef header_E2754FE6
#define header_E2754FE6

namespace Nebula
{
    namespace Apple
    {
        namespace HAL
        {
            class USBDevice : public Nebula::HAL::Device
            {
            protected:
                Context* context = 0;
                io_object_t interestNotification = 0;
                IOUSBDeviceInterface** interface = 0;

                static void deviceNotificationCallback(void* data,
                                                       io_service_t service,
                                                       natural_t messageType,
                                                       void* messageArgument);

                void controlRequest(IOUSBDevRequest*);
                void getUUID(uuid_t, int uuidType);
                void setUUID(uuid_t, int uuidType);
                RT::u4 getNumberOfChannels();
                RT::u4 getNumberOfLeds(RT::u4 channel);
                void setNumberOfLeds(RT::u4 channel, RT::u4 n);
                void getColors(RT::u4 channel, Color::RGB<RT::u1>*, RT::u4 n);
                void setColors(RT::u4 channel, Color::RGB<RT::u1>*, RT::u4 n);

            public:
                USBDevice(io_service_t device, Context* context);
                virtual ~USBDevice();

                virtual bool isRequestSupported(Request);
                virtual void control(Request, void* input, size_t inputSize, void* output, size_t outputSize);
            };
        }
    }
}

#endif
