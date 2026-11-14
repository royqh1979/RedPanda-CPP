// Derived from https://en.cppreference.com/w/cpp/language/structured_binding.html
#include <set>
#include <string>

int main()
{
    std::set<std::string> myset{"hello"};

    if (auto [iter, success] = myset.insert("Hello"); success)
    {
    }

    struct BitFields
    {
        int b : 4 {1}, d : 4 {2}, p : 4 {3}, q : 4 {4};
    };

    {
        const auto [b, d, p, q] = BitFields{};
    }

    {
        const auto [b, d, p, q] = []{ return BitFields{4, 3, 2, 1}; }();
    }

    {
        BitFields s;
        auto& [b, d, p, q] = s;
        b = 4, d = 3, p = 2, q = 1;
    }
}
