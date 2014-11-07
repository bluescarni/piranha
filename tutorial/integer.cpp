#include <iostream>
#include <stdexcept>

#include "piranha.hpp"

using namespace piranha;

int main()
{
	environment env;

	// Various ways of constructing an integer.
	std::cout << integer{42} << '\n';
	std::cout << integer{"42"} << '\n';
	std::cout << integer{42.123} << '\n';

	// You can construct integers of arbitrary size with the string constructor.
	std::cout << integer{"12345678987654321234567898765432"} << '\n';

	// Arithmetics and interoperability with fundamental C++ types.
	std::cout << (integer{21} * 2) << '\n';
	std::cout << (1u + integer{41}) << '\n';
	std::cout << (43. - integer{1}) << '\n';
	std::cout << (integer{85} / 2) << '\n';
	std::cout << (integer{42} == 42) << '\n';

	// Exponentiation provided by the math::pow() function.
	std::cout << (math::pow(integer{42},2) / 42) << '\n';

	// Conversion to a C++ integral type can fail.
	try {
		static_cast<unsigned char>(-integer{"12345678987654321"});
	} catch (const std::overflow_error &) {
		std::cout << "Overflow!\n";
	}

	// The "_z" suffix can be used to create integer literals.
	auto n = 42_z;
	std::cout << (n == 42) << '\n';
}
