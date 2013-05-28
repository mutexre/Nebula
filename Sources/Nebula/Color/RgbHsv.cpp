#include <algorithm>
#include <functional>
#include <Nebula/Nebula.h>

Nebula::Color::HSV<float>
Nebula::Color::rgb2hsv(RGB<float>& rgb)
{
    HSV<float> hsv;
    float Max, Min;

    Max = std::max(rgb.r, std::max(rgb.g, rgb.b));
    Min = std::min(rgb.r, std::min(rgb.g, rgb.b));

    if (Max == Min) {
        hsv.h = 0.0;
    }
    else
    {
        if (rgb.r == Max) {
            if (rgb.g >= rgb.b) hsv.h = 60.0 * ((rgb.g - rgb.b) / (Max - Min));
            else hsv.h = 60.0 * ((rgb.g - rgb.b) / (Max - Min)) + 360.0;
        }
        else {
            if (rgb.g == Max) hsv.h = 60.0 * ((rgb.b - rgb.r) / (Max - Min)) + 120.0;
            else hsv.h = 60.0 * ((rgb.r - rgb.g) / (Max - Min)) + 240.0;
        }
    }

    if (Max == 0.0) hsv.s = 0.0;
    else hsv.s = (1.0 - (Min / Max));

    hsv.v = Max;

    return hsv;
}

Nebula::Color::RGB<float>
Nebula::Color::hsv2rgb(HSV<float>& hsv)
{
    RGB<float> rgb;
    float f, p, q, t;
    int Hi;

    f = (hsv.h / 60.0) - float(int(hsv.h / 60.0));
    p = hsv.v * (1.0 - hsv.s);
    q = hsv.v * (1.0 - f * hsv.s);
    t = hsv.v * (1.0 - (1.0 - f) * hsv.s);

    Hi = int(hsv.h / 60.0) % 6;
    switch (Hi) {
        case 0: {
            rgb.r = hsv.v;
            rgb.g = t;
            rgb.b = p;
        }
        break;

        case 1: {
            rgb.r = q;
            rgb.g = hsv.v;
            rgb.b = p;
        }
        break;

        case 2: {
            rgb.r = p;
            rgb.g = hsv.v;
            rgb.b = t;
        }
        break;

        case 3: {
            rgb.r = p;
            rgb.g = q;
            rgb.b = hsv.v;
        }
        break;

        case 4: {
            rgb.r = t;
            rgb.g = p;
            rgb.b = hsv.v;
        }
        break;

        case 5: {
            rgb.r = hsv.v;
            rgb.g = p;
            rgb.b = q;
        }
        break;
    }

    return rgb;
}

Nebula::Color::RGB<RT::u1>
Nebula::Color::transformSaturationOfRgb(float r, float g, float b, std::function<float (float saturation)> transformation)
{
    RGB<float> rgb(r, g, b);
    HSV<float> hsv = rgb2hsv(rgb);

    hsv.s = transformation(hsv.s);
    rgb = hsv2rgb(hsv);

    return RGB<RT::u1>((RT::u4(rgb.r * 255.0f)) & 0xff,
                       (RT::u4(rgb.g * 255.0f)) & 0xff,
                       (RT::u4(rgb.b * 255.0f)) & 0xff);
}
