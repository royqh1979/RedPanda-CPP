// Derived from https://en.cppreference.com/w/cpp/ranges/filter_view.html
// and https://en.cppreference.com/w/cpp/ranges/join_view.html
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

void filter_example()
{
    auto even = [](int i) { return 0 == i % 2; };
    auto square = [](int i) { return i * i; };

    for (int i : std::views::iota(0, 6)
               | std::views::filter(even)
               | std::views::transform(square))
    {
    }
}

void join_example()
{
    using namespace std::literals;
    const auto bits = {"https:"sv, "//"sv, "cppreference"sv, "."sv, "com"sv};
    for (char const c : bits | std::views::join) { }

    const std::vector<std::vector<int>> vv{{1, 2}, {3, 4, 5}, {6}, {7, 8, 9}};
    for (int value : vv | std::views::join) { }
}
