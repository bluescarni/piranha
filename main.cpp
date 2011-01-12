#include <algorithm>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ext/pool_allocator.h>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <gmpxx.h>

#include "src/hop_table.hpp"
#include "src/integer.hpp"
#include "src/settings.hpp"
#include "src/thread_group.hpp"
#include "src/type_traits.hpp"

using namespace piranha;

__gnu_cxx::__pool_alloc<char> p;

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

static unsigned long constant = (boost::posix_time::microsec_clock::local_time() - boost::posix_time::ptime(boost::gregorian::date(1970,1,1))).total_microseconds();

int main()
{
	settings::set_n_threads(1);
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

	struct foo
	{
		long x;
		double y;
	};
	
	struct foo_hash
	{
		std::size_t operator()(const foo &f) const
		{
			return std::hash<long>()(f.x);
		}
	};

	struct foo_equal
	{
		std::size_t operator()(const foo &f1, const foo &f2) const
		{
			return f1.x == f2.x;
		}
	};

const boost::posix_time::ptime time0 = boost::posix_time::microsec_clock::local_time();
	hop_table<std::string> ht/*(10000000)*/;
// 	foo f;
// 	f.y = 0.;
	for (int i = 0; i < 600000; ++i) {
// 		f.x = i;
		//ht._unique_insert(f,ht.bucket(f));
		ht.insert(boost::lexical_cast<std::string>(i));
		//std::string("foo");
	}
std::cout << "Elapsed time: " << (double)(boost::posix_time::microsec_clock::local_time() - time0).total_microseconds() / 1000 << '\n';

// std::exit(1);

// 	std::cout << ht.load_factor() << '\n';

// 	struct td
// 	{
// 		td():m_value(std::sqrt(constant % 123456) * std::cos(1.5) * std::tan(3.56)) {std::cout << "called\n";}
// 		td(const td &other):m_value(std::sqrt(constant % 123456) + other.m_value) {}
// 		double m_value;
// 	};
// 
// 	std::cout << is_trivially_copyable<td>::value << '\n';
// 	
// 	cvector<td> ht(20000000);
// 	ht.resize(25000000);
// 	std::cout << (unsigned long)ht[0].m_value << '\n';
// 	std::cout << (unsigned long)ht[10].m_value << '\n';
// 	std::cout << (unsigned long)ht[100].m_value << '\n';
// 	std::cout << ht.n_buckets() << '\n';
// 	return 0;
// 	std::for_each(boost::counting_iterator<int>(0),boost::counting_iterator<int>(2000000),[&ht](int n){ht.emplace(n);});

}
