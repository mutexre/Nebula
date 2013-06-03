#include <Nebula/Firmware/Firmware.h>

TM1809Controller800Mhz<13> channel0_controller;
//TM1809Controller800Mhz<12> channel1_controller;

void Nebula::Firmware::initChannels(Channel* channels) {
    channels[0].controller = &channel0_controller;
    //channels[1].controller = &channel1_controller;
}
