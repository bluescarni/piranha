#include <iostream>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

#include "src/piranha.hpp"

piranha::integer get_integer()
{
	return piranha::integer();
}

int main()
{
	piranha::integer i, j;
	j = get_integer();
	piranha::integer k(std::move(j));
	std::cout << i << '\n';
	piranha::integer l("-45475934753489573453478957348975348978979878979");
	std::cout << l << '\n';
	std::cout << piranha::integer(1E56) << '\n';
	std::cout << piranha::integer(100) << '\n';
	std::cout << piranha::integer(100L) << '\n';
	std::cout << piranha::integer(100U) << '\n';
	std::cout << piranha::integer(100LU) << '\n';
	std::cout << piranha::integer(100LL) << '\n';
	std::cout << piranha::integer(100LLU) << '\n';
	piranha::integer foo;
	std::cin >> foo;
	std::cout << foo << '\n';
//	mpz_init(integ);
// 	std::cout << i << '\n';
}
