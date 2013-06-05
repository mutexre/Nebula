#ifndef header_AFE7F2F9
#define header_AFE7F2F9

namespace Nebula
{
    namespace Color
    {
        HSV<float> rgb2hsv(RGB<float>& rgb);
        RGB<float> hsv2rgb(HSV<float>& hsv);
        RGB<RT::u1> transformSaturationOfRgb(float r, float g, float b, std::function<float (float saturation)> transformation);
        void generateColorCycle(HSV<float> hsv, RGB<RT::u1>* output, RT::u4 numberOfLeds);
    }
}

#endif
