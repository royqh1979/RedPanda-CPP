// Derived from https://en.cppreference.com/w/cpp/language/lambda.html
#include <functional>

int main()
{
    int x = 5;

    auto simple = [x](int n) { return n < x; };
    auto func1 = [](int i = 6) { return i + 4; };
    std::function<int(int)> func2 = [](int i) { return i + 4; };

    auto nth_fibonacci = [](auto self, int n, int a = 0, int b = 1) -> int
    {
        return n ? self(self, n - 1, a + b, a) : b;
    };

    auto generic = []<class T>(T a, auto&& b) { return a < b; };

    auto y = [&r = x, x = x + 1]() -> int
    {
        r += 2;
        return x * x;
    }();
}
