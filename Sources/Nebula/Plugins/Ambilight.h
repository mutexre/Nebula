#ifndef header_DB1C1C1C
#define header_DB1C1C1C

class Ambilight : public Nebula::Generator
{
public:
    Ambilight(RT::u4 numberOfLeds);
    virtual void generate(Nebula::Color::RGB<RT::u1>* output);
};

#endif
