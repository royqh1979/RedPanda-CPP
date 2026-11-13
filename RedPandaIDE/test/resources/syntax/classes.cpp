// Derived from https://en.cppreference.com/w/cpp/language/class.html
#include <cstddef>
#include <string>
#include <vector>

class S
{
    int d1;
    int a[10] = {1, 2};

    static const int d2 = 1;

    virtual void f1(int) = 0;

    std::string d3, *d4, f2(int);

    enum { NORTH, SOUTH, EAST, WEST };

    struct NestedS
    {
        std::string s;
    } d5, *d6;

    typedef NestedS value_type, *pointer_type;
};

class M
{
    std::size_t C;
    std::vector<int> data;
public:
    M(std::size_t R, std::size_t C) : C(C), data(R*C) {}

    int operator()(std::size_t r, std::size_t c) const
    {
        return data[r * C + c];
    }

    int& operator()(std::size_t r, std::size_t c)
    {
        return data[r * C + c];
    }
};

class Base
{
protected:
    int d;
};

class Derived : public Base
{
public:
    using Base::d;
    using Base::Base;
};

struct MemberTemplates
{
    template<typename T>
    void f(T&& n);

    template<class CharT>
    struct NestedS
    {
        std::basic_string<CharT> s;
    };
};
