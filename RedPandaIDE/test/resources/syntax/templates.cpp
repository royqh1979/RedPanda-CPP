// Derived from https://en.cppreference.com/w/cpp/language/templates.html
// and https://en.cppreference.com/w/cpp/language/namespace.html
#include <string>

namespace vec
{
    template<typename T>
    class vector
    {
    };
}

template<typename T>
struct A
{
    void f() {}
};

struct MyClass {};

template<> class A<MyClass> {};

template<typename T>
using Ptr = T*;

struct S
{
    template<typename T>
    void f(T&& n);

    template<class CharT>
    struct NestedS
    {
        std::basic_string<CharT> s;
    };
};

template<typename... Args>
void f(Args..., ...);

template<typename T, T::type n = 0>
class X;

struct WithType
{
    using type = int;
};

using T5 = X<WithType>;
