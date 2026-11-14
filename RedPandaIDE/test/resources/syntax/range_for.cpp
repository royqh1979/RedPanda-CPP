// Derived from https://en.cppreference.com/w/cpp/language/range-for.html
#include <vector>

int main()
{
    std::vector<int> v = {0, 1, 2, 3, 4, 5};

    for (const int& i : v) { }
    for (auto i : v) { }
    for (auto&& i : v) { }

    const auto& cv = v;
    for (auto&& i : cv) { }

    for (int n : {0, 1, 2, 3, 4, 5}) { }

    int a[] = {0, 1, 2, 3, 4, 5};
    for (int n : a) { }

    for ([[maybe_unused]] int n : a) { }

    for (auto n = v.size(); auto i : v) { }

    for (typedef decltype(v)::value_type elem_t; elem_t i : v) { }
    for (using elem_t = decltype(v)::value_type; elem_t i : v) { }
}
