#include <Nebula/Firmware/Firmware.h>

void Nebula::Firmware::Channel::init(unsigned int numberOfLeds, Nebula::Color::RGB<uint8_t>* colors, CLEDController* controller) {
    this->numberOfLeds = numberOfLeds;
    this->colors = colors;
    this->controller = controller;
}

Nebula::Firmware::Channel::Channel() {
    init(0, 0, 0);
}
