#include <Nebula/Nebula.h>

void Nebula::HAL::Device::controlIn(Request request, void* data, size_t dataSize) {
    control(request, data, dataSize, 0, 0);
}

void Nebula::HAL::Device::controlOut(Request request, void* data, size_t dataSize) {
    control(request, 0, 0, data, dataSize);
}
