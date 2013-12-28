#ifndef header_ABE82430
#define header_ABE82430

namespace Nebula
{
    class Plugin {
    protected:
        RT::u4 numberOfLeds;

    public:
        Plugin(RT::u4 numberOfLeds) {
            this->numberOfLeds = numberOfLeds;
        }

        virtual ~Plugin() {}

        virtual bool isGenerator() const;
        std::set<Color::Space> getSupportedColorSpaces() const;
    };

    class RgbGenerator : public Plugin {
    public:
        RgbGenerator(RT::u4 numberOfLeds) : Plugin(numberOfLeds) {}
        virtual void generate(Color::RGB<float>* output) = 0;
        virtual void operator()(Color::RGB<float>* output) { generate(output); }
    };

    class HsvGenerator : public Plugin {
    public:
        HsvGenerator(RT::u4 numberOfLeds) : Plugin(numberOfLeds) {}
        virtual void generate(Color::HSV<float>* output) = 0;
        virtual void operator()(Color::HSV<float>* output) { generate(output); }
    };

    class RgbTransformation : public Plugin {
    public:
        RgbTransformation(RT::u4 numberOfLeds) : Plugin(numberOfLeds) {}
        virtual void transform(const Color::RGB<float>* input, Color::RGB<RT::u1>* output) = 0;
        virtual void transformInPlace(Color::RGB<float>* colors) = 0;
    };

    class HsvTransformation : public Plugin {
    public:
        HsvTransformation(RT::u4 numberOfLeds) : Plugin(numberOfLeds) {}
        virtual void transform(const Color::HSV<float>* input, Color::RGB<RT::u1>* output) = 0;
        virtual void transformInPlace(Color::HSV<float>* colors) = 0;
    };
/*
    class ColorSpaceConvertion : public Plugin {
    public:
        ColorSpaceConvertion(RT::u4)
    };
*/
}

#endif
