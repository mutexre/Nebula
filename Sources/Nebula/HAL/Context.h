#ifndef header_C7B476E9
#define header_C7B476E9

#include <functional>
#include <list>
#include <uuid/uuid.h>

namespace Nebula
{
    namespace HAL
    {
        class Context
        {
        protected:
            std::function<void (Device*)> onDeviceAddition;
            std::function<void (Device*)> onDeviceRemoval;

        public:
            Context(std::function<void (Device*)> onDeviceAddition,
                    std::function<void (Device*)> onDeviceRemoval);

            virtual ~Context();
        };

        Context* createContext(std::function<void (Device*)> onDeviceAddition,
                               std::function<void (Device*)> onDeviceRemoval);
    }
}

#endif
