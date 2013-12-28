#ifndef header_3264868B
#define header_3264868B

class Rainbow : public Nebula::Generator
{
private:
    Color::HSV<float> hsv;
    float speed;

public:
    Rainbow(RT::u4 numberOfLeds, Color::HSV<float> hsv, float speed);
    virtual void generate(Color::RGB<RT::u1>* output);
};

#endif
