// Derived from https://en.cppreference.com/w/cpp/language/function.html
int a = 1, *p = nullptr, f(), (*pf)(double);

struct S
{
    virtual int f(char) const, g(int) &&;
};

int complex_parameters(int a, int* p, int (*(*x)(double))[3]);
int complex_defaults(int a = 7, int* p = nullptr, int (*(*x)(double))[3] = nullptr);
int unnamed_parameters(int, int*, int (*(*)(double))[3]);

auto fpif(int)->int(*)(int);

constexpr auto trailing_noexcept(int[] b) noexcept -> int;

int printf_like(const char* fmt, ...);

template<typename... Args>
void variadic_template(Args..., ...);
