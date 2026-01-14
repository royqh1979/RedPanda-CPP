#include <cstddef>
#include <iostream>
#include <type_traits>
 
int main()
{
    std::cout << 123 << '\n'
              << 0123 << '\n'
              << 0x123 << '\n'
              << 0b10 << '\n'
              << 12345678901234567890ull << '\n'
              << 12345678901234567890u << '\n'; // the type is unsigned long long
                                                // even without a long long suffix
 
//  std::cout << -9223372036854775808 << '\n'; // error: the value
               // 9223372036854775808 cannot fit in signed long long, which is the
               // biggest type allowed for unsuffixed decimal integer literal
    std::cout << -9223372036854775808u << '\n'; // unary minus applied to unsigned
               // value subtracts it from 2^64, this gives 9223372036854775808
    std::cout << -9223372036854775807 - 1 << '\n'; // correct way to calculate
                                                   // the value -9223372036854775808
 
#if __cpp_size_t_suffix >= 202011L // C++23
    static_assert(std::is_same_v<decltype(0UZ), std::size_t>);
    static_assert(std::is_same_v<decltype(0Z), std::make_signed_t<std::size_t>>);
#endif
}