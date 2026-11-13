// Derived from https://en.cppreference.com/w/cpp/language/namespace.html
namespace Q
{
    namespace V
    {
        class C { void m(); };
        void f();
    }

    void V::f()
    {
        extern void h();
    }

    void V::C::m()
    {}
}

namespace A::B::C
{
    int nested_value;
}

namespace Lib
{
    inline namespace Lib_1
    {
        template<typename T> class A;
        template<typename T> void g(T) { }
    }
}

struct MyClass {};
namespace Lib
{
    template<> class A<MyClass> { };
}

namespace
{
    int unnamed_value;
}
