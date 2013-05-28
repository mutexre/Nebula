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
                Context* context;
                io_object_t interestNotification = 0;
                IOUSBInterfaceInterface300** deviceInterface = 0;

                static void deviceNotificationCallback(void* data,
                                                       io_service_t service,
                                                       natural_t messageType,
                                                       void* messageArgument);

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
