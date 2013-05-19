#ifndef header_CF489BAD
#define header_CF489BAD

template <typename T>
class RGB
{
public:
    T r, g, b;

    RGB() {
        this->r = T(0);
        this->g = T(0);
        this->b = T(0);
    }

    RGB(T r, T g, T b)
    {
        this->r = r;
        this->g = g;
        this->b = b;
    }
};

#endif
