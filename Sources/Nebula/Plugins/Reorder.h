#ifndef header_E3B262FA
#define header_E3B262FA

class Reorder : public Nebula::Transformation
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
    virtual void transform(const Color::RGB<RT::u1>* input, Color::RGB<RT::u1>* output);
    virtual void transformInPlace(Color::RGB<RT::u1>* colors);
};

#endif
