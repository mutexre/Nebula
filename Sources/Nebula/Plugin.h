#ifndef header_ABE82430
#define header_ABE82430

namespace Nebula {
    class Plugin {
    protected:
        RT::u4 numberOfLeds;

    public:
        Plugin(RT::u4 numberOfLeds) {
            this->numberOfLeds = numberOfLeds;
        }

        virtual ~Plugin() {}
    };

    class Generator : public Plugin {
    public:
        Generator(RT::u4 numberOfLeds) : Plugin(numberOfLeds) {}
        virtual void generate(Color::RGB<RT::u1>* output) = 0;
        virtual void operator()(Nebula::Color::RGB<RT::u1>* output) { generate(output); }
    };

    class Transformation: public Plugin {
    public:
        Transformation(RT::u4 numberOfLeds) : Plugin(numberOfLeds) {}
        virtual void transform(const Color::RGB<RT::u1>* input, Color::RGB<RT::u1>* output) = 0;
        virtual void transformInPlace(const Color::RGB<RT::u1>* colors) = 0;
    };
}

#endif
