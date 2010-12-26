#include <iostream>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

#include "src/piranha.hpp"

using namespace piranha;

int main()
{
	cvector<integer> v(1000000);
	v.resize(2000000);
	std::cout << integer(true) << '\n';
	std::cout << integer(false) << '\n';
}
