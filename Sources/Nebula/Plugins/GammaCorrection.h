#ifndef header_47199CCD
#define header_47199CCD

class GammaCorrection : public Nebula::Transformation
{
private:
    RT::u4 n;

private:
    inline RT::u4 computeGroupStartIndex(RT::u4 i) {
        return n * (i / n);
    }

    inline RT::u4 transformOffsetInGroup(RT::u4 i) {
        return (n - 1) - (i % n);
    }

    inline RT::u4 transformIndex(RT::u4 i) {
        return computeGroupStartIndex(i) + transformOffsetInGroup(i);
    }

public:
    Reorder(RT::u4 numberOfLeds, RT::u4 n);
    virtual void transform(const Nebula::Color::RGB<RT::u1>* input, Nebula::Color::RGB<RT::u1>* output);
    virtual void transformInPlace(Nebula::Color::RGB<RT::u1>* colors);
};

#endif
