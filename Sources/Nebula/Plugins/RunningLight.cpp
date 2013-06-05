#include <Nebula/Nebula.h>

RunningLight::RunningLight(RT::u4 numberOfLeds, RT::u4 n) : Generator(numberOfLeds) {
    this->n = n;
}

void RunningLight::generate(Nebula::Color::RGB<RT::u1>* output) {
    static RT::u4 position = 0;
    static RT::u8 iterationNumber = 0;

    for (auto i = 0; i < numberOfLeds; i++) {
        if (i != position)
            output[i] = Nebula::Color::RGB<RT::u1>::black();
        else
            output[i] = Nebula::Color::RGB<RT::u1>::gray(150);
    }

    if (iterationNumber % n == 0) position++;
    if (position == numberOfLeds) position = 0;

    iterationNumber++;
}
