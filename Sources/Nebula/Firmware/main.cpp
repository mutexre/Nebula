#include <util/delay.h>
#include <Nebula/Firmware/Firmware.h>
#include <Nebula/Firmware/VUSB/RunLoop.h>

int main() {
    Nebula::Firmware::VUSB::runLoop.init();
    Nebula::Firmware::VUSB::runLoop.run();
    return 0;
}
