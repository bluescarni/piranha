#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

#include <gmpxx.h>

#include "src/piranha.hpp"

using namespace piranha;

void *allocate_function(std::size_t alloc_size)
{
	std::cout << "allocate\n";
	return std::malloc(alloc_size);
}

void *reallocate_function(void *ptr, std::size_t, std::size_t new_size)
{
	std::cout << "reallocate\n";
	return std::realloc(ptr,new_size);
}

void free_function(void *ptr, size_t)
{
	std::cout << "free\n";
	std::free(ptr);
}

template <typename T>
struct hop_bucket
{
	hop_bucket():m_occupied(false),m_value(),m_bitset(0) {}
	bool		m_occupied;
	T		m_value;
	std::uint64_t	m_bitset;
};

int main()
{
//	mp_set_memory_functions(allocate_function,reallocate_function,free_function);

// 	integer i(1);
// 	i.multiply_accumulate(i,i);

// 	std::string str("tmp");
// 	std::hash<std::string> h;
// 	std::cout << h(str) << '\n';

	std::cout << mf_int_traits::msb(boost::integer_traits<mf_uint>::const_max) << '\n';
}
