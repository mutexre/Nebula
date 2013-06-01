#ifndef header_ABE82430
#define header_ABE82430

namespace Nebula {
    class Plugin {
    public:
        
    };

    class Generator : public Plugin {
    public:
        void generate(RGB<RT::u1>* output, RT::u4 numberOfLeds) = 0;
    };

    class Transformation: public Plugin {
    public:
        void transform(const RGB<RT::u1>* input, RGB<RT::u1>* output, RT::u4 numberOfLeds) = 0;
        void transformInPlace(const RGB<RT::u1>* colors, RT::u4 numberOfLeds) = 0;
    };
}

#endif
