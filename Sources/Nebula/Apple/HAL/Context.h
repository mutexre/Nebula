#ifndef header_BE24563F
#define header_BE24563F

namespace Nebula
{
    namespace Apple
    {
        namespace HAL
        {
            class Context : public Nebula::HAL::Context
            {
                friend class USBDevice;

            private:
                mach_port_t masterPort = MACH_PORT_NULL;
                IONotificationPortRef ioNotificationPort = 0;

                static void matchedNotificationCallback(void* data, io_iterator_t iterator);

            public:
                Context(std::function<void (Nebula::HAL::Device*)> onDeviceAddition,
                        std::function<void (Nebula::HAL::Device*)> onDeviceRemoval);

                virtual ~Context();
            };
        }
    }
}

#endif
