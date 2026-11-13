// Derived from https://en.cppreference.com/w/cpp/language/enum.html
#include <cstdint>

enum smallenum: std::int16_t
{
    a,
    b,
    c
};

enum color
{
    red,
    yellow,
    green = 20,
    blue
};

enum class altitude: char
{
    high = 'h',
    low = 'l',
};

enum
{
    d,
    e,
    f = e + 2
};

enum struct E11 { x, y };

struct E98 { enum { x, y }; };
namespace N98 { enum { x, y }; }
struct S98 { static const int x = 0, y = 1; };

namespace cxx20
{
    enum class long_long_long_name { x, y };

    void using_enum_demo()
    {
        switch (auto rnd = []{return long_long_long_name::x;}; rnd())
        {
            using enum long_long_long_name;
            case x: break;
            case y: break;
        }
    }
}
