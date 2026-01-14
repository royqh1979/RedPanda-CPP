#include <iomanip>
#include <iostream>
#include <limits>
#include <typeinfo>
#define OUT(x) '\n' << std::setw(16) << #x << x
int main()
{
    std::cout
        << "Literal" "\t" "Printed value" << std::left
        << OUT( 58.            ) // double
        << OUT( 4e2            ) // double
        << OUT( 123.456e-67    ) // double
        << OUT( 123.456e-67f   ) // float, truncated to zero
        << OUT( .1E4f          ) // float
        << OUT( 0x10.1p0       ) // double
        << OUT( 0x1p5          ) // double
        << OUT( 0x1e5          ) // integer literal, not floating-point
        << OUT( 3.14'15'92     ) // double, single quotes ignored (C++14)
        << OUT( 1.18e-4932l    ) // long double
        << std::setprecision(39)
        << OUT( 3.4028234e38f  ) // float
        << OUT( 3.4028234e38   ) // double
        << OUT( 3.4028234e38l  ) // long double
        << '\n';
 
    static_assert(3.4028234e38f == std::numeric_limits<float>::max());
 
    static_assert(3.4028234e38f ==  // ends with 4
                  3.4028235e38f);   // ends with 5
 
    static_assert(3.4028234e38 !=   // ends with 4
                  3.4028235e38);    // ends with 5
 
    // Both floating-point constants below are 3.4028234e38
    static_assert(3.4028234e38f !=  // a float (then promoted to double)
                  3.4028234e38);    // a double
}