#include <iostream>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

#include "src/piranha.hpp"

struct foo_base
{
	foo_base &operator=(const foo_base &)
	{
		std::cout << "foo ass\n";
		return *this;
	}
	foo_base &operator=(int)
	{
		std::cout << "foo int\n";
		return *this;
	}
};

struct foo_deriv: public foo_base
{
	template <typename T>
	foo_deriv &operator=(T &&x)
	{
		return static_cast<foo_deriv &>(foo_base::operator=(std::forward<T>(x)));
		//return *this;
	}
};

int main()
{
	foo_deriv f;
	f = f;
	f = 1;
}
