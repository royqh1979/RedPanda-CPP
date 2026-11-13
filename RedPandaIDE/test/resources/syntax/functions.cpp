// Derived from https://en.cppreference.com/w/cpp/language/function.html
#include <iostream>
#include <string>

void f0(const std::string& arg = "world!")
{
    std::cout << "Hello, " << arg << '\n';
}

int f1();

void (*fp03())(const std::string&)
{
    return f0;
}

auto fp11() -> void(*)(const std::string&)
{
    return f0;
}

int main()
{
    f0();
    fp03()("test!");
    fp11()("again!");
    int f2(std::string) noexcept;
    std::cout << "f2(\"bad\"): " << f2("bad") << '\n';
    std::cout << "f2(\"42\"): " << f2("42") << '\n';
}

int f1()
{
    return 007;
}

int f2(std::string str) noexcept
try
{
    return std::stoi(str);
}
catch (const std::exception& e)
{
    std::cerr << "stoi() failed!\n";
    return 0;
}

void bar() = delete;
