#include <iostream>

// Include the global Piranha header.
#include "piranha.hpp"

// Import the names from the Piranha namespace.
using namespace piranha;

int main()
{
	// Setup of the Piranha environment.
	environment env;
	// Print the rational number 4/3 to screen.
	std::cout << rational{4,3} << '\n';
}
