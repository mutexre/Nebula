#ifndef header_ECD04562
#define header_ECD04562

namespace Nebula
{
    namespace Color
    {
        template <typename T>
        class HSV
        {
        public:
            T h, s, v;

            HSV() {
                this->h = T(0);
                this->s = T(0);
                this->v = T(0);
            }

            HSV(T h, T s, T v) {
                this->h = h;
                this->s = s;
                this->v = v;
            }

            HSV<T> operator*(T x) const {
                return HSV<T>(h, s, v * x);
            }
        };
    }
};

#endif
