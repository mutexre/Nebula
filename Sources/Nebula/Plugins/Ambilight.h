#ifndef header_DB1C1C1C
#define header_DB1C1C1C

class Ambilight : public Nebula::Generator
{
private:
    RT::u4 zoneSize;

private:
    RT::u4 calculateBarSize(RT::u4 n, RT::u4 i, RT::u4 size, float a);

    void computeColors(Color::RGB<RT::u1>* leds, const RT::u1* pixelData,
                       size_t bytesPerRow, size_t bytesPerPixel,
                       RT::u4 width, RT::u4 height,
                       RT::u2 left, RT::u2 right,
                       RT::u2 bottom, RT::u2 top);

public:
    Ambilight(RT::u4 numberOfLeds, RT::u4 zoneSize);
    virtual void generate(Color::RGB<RT::u1>* output);
};

#endif
