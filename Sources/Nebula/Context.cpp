#include <Nebula/Nebula.h>

Nebula::HAL::Context::Context(std::function<void (Device*)> onDeviceAddition,
                              std::function<void (Device*)> onDeviceRemoval)
{
    this->onDeviceAddition = onDeviceAddition;
    this->onDeviceRemoval = onDeviceRemoval;
}

Nebula::HAL::Context::~Context() {
}
