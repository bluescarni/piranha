#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include <gmpxx.h>

#include "src/hop_table.hpp"
#include "src/integer.hpp"

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

struct foo {};

void do_foo(const foo &) {}

int main()
{
//	mp_set_memory_functions(allocate_function,reallocate_function,free_function);

// 	integer i(1);
// 	i.multiply_accumulate(i,i);

// 	std::string str("tmp");
// 	std::hash<std::string> h;
// 	std::cout << h(str) << '\n';

// 	typedef integer int_type;
// 
// 	hop_table<int_type> ht;
// 	std::cout << (ht.begin() == ht.end()) << '\n';
// 	std::cout << (ht.find(int_type(1)) == ht.end()) << '\n';
// 	ht.emplace(int_type(10));
// 	ht.emplace(int_type(10));
// 	ht.emplace(int_type(11));
// 	for (int i = 0; i < 61; ++i) {
// 		ht.emplace(int_type(9));
// 	}
// 	ht.emplace(int_type(10));
// 	ht.emplace(int_type(10));


	hop_table<std::string> ht;
	std::string tmp = "";
	for (int i = 0; i < 1500; ++i) {
		tmp.push_back(unsigned(i));
		ht.emplace(tmp);
	}
}
