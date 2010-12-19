#include <iostream>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

#include "src/cvector.hpp"

struct trivial
{
	int n;
};

piranha::cvector<trivial> get()
{
	try {
		return piranha::cvector<trivial>(10000000);
	} catch (...) {
		throw;
	}
}

int main()
{
	piranha::cvector<trivial> t;
	t = get();
	std::cout << t.size() << '\n';
}
