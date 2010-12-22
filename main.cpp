#include <iostream>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

#include "src/piranha.hpp"

struct nontrivial
{
	nontrivial():v(std::vector<double>::size_type(1)) {}
	nontrivial(nontrivial &&nt):v(std::move(nt.v))
	{
// std::cout << "move ctor\n";
	}
	nontrivial &operator=(nontrivial &&nt)
	{
std::cout << "move ass\n";
		v = std::move(nt.v);
		return *this;
	}
	std::vector<double> v;
};

static inline nontrivial nontrivial_get()
{
	return nontrivial();
}

int main()
{
	piranha::cvector<nontrivial> v(10000);
	v.resize(10001);
}
