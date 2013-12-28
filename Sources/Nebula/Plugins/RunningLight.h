#ifndef header_B18802A6
#define header_B18802A6

class RunningLight : public Nebula::Generator
{
private:
    RT::u4 n;

public:
    RunningLight(RT::u4 numberOfLeds, RT::u4 n);
    virtual void generate(Color::RGB<RT::u1>* output);
};

#endif
