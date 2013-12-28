#include <Nebula/Nebula.h>

Reorder::Reorder(RT::u4 numberOfLeds, RT::u4 n) : Transformation(numberOfLeds) {
    this->n = n;
}

void Reorder::transform(const Nebula::Color::RGB<RT::u1>* input, Nebula::Color::RGB<RT::u1>* output) {
    for (RT::u4 i = 0; i < numberOfLeds; i++) output[i] = input[transformIndex(i)];
}

void Reorder::transformInPlace(Nebula::Color::RGB<RT::u1>* colors) {
    for (RT::u4 group = 0; group <= numberOfLeds / n; group++) {
        for (RT::u4 pixel = 0; pixel < std::min(n / 2, numberOfLeds - group * n); pixel++) {
            Nebula::Color::RGB<RT::u1> tmp = colors[group * n + (n - 1) - pixel];
            colors[group * n + (n - 1) - pixel] = colors[group * n + pixel];
            colors[group * n + pixel] = tmp;
        }
    }
}
