#include <iostream>
#include <stdexcept>

#include "piranha.hpp"

using namespace piranha;

int main()
{
    init();

    // Various ways of constructing a rational.
    std::cout << rational{42} << '\n';
    std::cout << rational{"42"} << '\n';
    std::cout << rational{1.5} << '\n';
    std::cout << rational{42, 12} << '\n';
    std::cout << rational{"42/12"} << '\n';
    // The integer class can also be used to construct a rational.
    std::cout << rational{42_z} << '\n';
    std::cout << rational{42_z, 12_z} << '\n';
    // Mixed integral types in the numerator-denominator constructor can be used.
    std::cout << rational{42ull, 12_z} << '\n';

    // Arithmetics and interoperability with fundamental C++ types and integer.
    std::cout << (rational{42, 13} * 2) << '\n';
    std::cout << (1u + rational{42, 13}) << '\n';
    std::cout << (43. - rational{1, 2}) << '\n';
    std::cout << (rational{85} / 13) << '\n';
    std::cout << (rational{84, 2} == 42) << '\n';
    std::cout << (42_z >= rational{84, 3}) << '\n';

    // Exponentiation is provided by the math::pow() function.
    std::cout << math::pow(rational{42, 13}, 2) << '\n';
    std::cout << math::pow(rational{42, 13}, -3_z) << '\n';

    // Conversion to a C++ integral type can fail.
    try {
        (void)static_cast<unsigned char>(-rational{42, 5});
    } catch (const std::overflow_error &) {
        std::cout << "Overflow!\n";
    }
    // Conversion to integer cannot fail, and yields
    // the truncated value.
    std::cout << static_cast<integer>(rational{10, 3}) << '\n';

    // The "_q" suffix can be used to create rational literals.
    auto r = 42_q;
    std::cout << (r == 42) << '\n';
    // The literal can also be used to initialise a rational from numerator and
    // denominator without using any explicit constructor.
    r = 42 / 13_q;
    std::cout << r << '\n';

    // Calculate (42/13 choose 21) via the math::binomial() function.
    std::cout << math::binomial(42 / 13_q, 21_z) << '\n';
}
