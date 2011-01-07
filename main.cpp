#include <algorithm>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/pool/pool_alloc.hpp>
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
#include "src/settings.hpp"

using namespace piranha;

boost::pool_allocator<char> p;

void *allocate_function(std::size_t alloc_size)
{
// 	std::cout << "allocate\n";
// 	return std::malloc(alloc_size);
	return p.allocate(alloc_size);
}

void *reallocate_function(void *ptr, std::size_t old_size, std::size_t new_size)
{
// 	std::cout << "reallocate\n";
// 	return std::realloc(ptr,new_size);
	void *new_ptr = p.allocate(new_size);
	std::memcpy(new_ptr,ptr,std::min(new_size,old_size));
	p.deallocate((char *)ptr,old_size);
	return new_ptr;
}

void free_function(void *ptr, size_t size)
{
// 	std::cout << "free\n";
// 	std::free(ptr);
	p.deallocate((char *)ptr,size);
}

int main()
{
	settings::set_n_threads(4);
// 	mp_set_memory_functions(allocate_function,reallocate_function,free_function);

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


// 	hop_table<std::string> ht0(0);
// 	hop_table<std::string> ht1(1);
// 	hop_table<std::string> ht2(2);
// 	hop_table<std::string> ht3(3);
// 	hop_table<std::string> ht4(4);
// 	ht0.emplace("ciao");
// 	ht0.emplace("pirlone");
// 	ht0.emplace("e cretino!");
// 	ht0.emplace("zio scatenato!!!");

	cvector<integer> ht(20000000);
// 	std::cout << ht.n_buckets() << '\n';
	return 0;
// 	std::for_each(boost::counting_iterator<int>(0),boost::counting_iterator<int>(2000000),[&ht](int n){ht.emplace(n);});

}
