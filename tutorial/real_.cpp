#include <iostream>
#include <stdexcept>

#include "piranha.hpp"

using namespace piranha;

int main()
{
	environment env;

	// Various ways of constructing a real.
	std::cout << real{42} << '\n';
	std::cout << real{42.3} << '\n';
	std::cout << real{"1.2345"} << '\n';
	// Construct with a custom precision of 500 bits.
	std::cout << real{42,500} << '\n';
	std::cout << real{42.3,500} << '\n';
	std::cout << real{"1.2345",500} << '\n';
	// Construct non-finite values.
	std::cout << real{"inf"} << '\n';
	std::cout << real{"-inf"} << '\n';
	std::cout << real{"nan"} << '\n';

	// Basic arithmetic operations.
	std::cout << real{42} + 1 << '\n';
	std::cout << real{42} * 2 << '\n';
	std::cout << 1.5 / real{42} << '\n';
	// The precision of the result is the highest precision among the operands.
	std::cout << (real{42} / real{7.1,300}) << '\n';
	std::cout << (real{42} / real{7.1,300}).get_prec() << '\n';

	// Conversion to a C++ integral type can fail.
	try {
		static_cast<unsigned char>(-real{42.5});
	} catch (const std::overflow_error &) {
		std::cout << "Overflow!\n";
	}
	// Conversion to integer cannot fail, and yields
	// the truncated value.
	std::cout << static_cast<integer>(real{10.3}) << '\n';

	// The "_r" suffix can be used to create real literals.
	auto r = 42.123_r;
	std::cout << (r == real{"42.123"}) << '\n';
}
