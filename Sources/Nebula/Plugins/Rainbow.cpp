#include <Nebula/Nebula.h>

Rainbow::Rainbow(RT::u4 numberOfLeds, Nebula::Color::HSV<float> hsv, float speed) : Generator(numberOfLeds) {
    this->hsv = hsv;
    this->speed = speed;
}

void Rainbow::generate(Nebula::Color::RGB<RT::u1>* output) {
    Nebula::Color::generateColorCycle(hsv, output, numberOfLeds);
    if (speed > 0.0f) {
        hsv.h += speed;
        if (hsv.h >= 360.0f) {
            auto n = truncf(hsv.h / 360.0f);
            hsv.h -= n * 360.0f;
        }
    }
    else {
        if (speed < 0.0f) {
            hsv.h += speed;
            if (hsv.h < 0.0f) {
                auto n = 1.0f + truncf(-hsv.h / 360.0f);
                hsv.h += n * 360.0f;
            }
        }
    }
}
