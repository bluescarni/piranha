#include <array>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cstdlib>
#include <iostream>
#include <random>

#include "src/piranha.hpp"

#include <algorithm>
#include <cstring>
#include <ext/mt_allocator.h>

// TODO: replace wih max align type?
__gnu_cxx::__mt_alloc<std::size_t> allocator;

void *allocate_function(std::size_t alloc_size)
{
	return static_cast<void *>(allocator.allocate(alloc_size / sizeof(std::size_t) + 1u));
}

void *reallocate_function(void *ptr, std::size_t old_size, std::size_t new_size)
{
	auto new_ptr = static_cast<void *>(allocator.allocate(new_size / sizeof(std::size_t) + 1u));
	std::memcpy(new_ptr,ptr,old_size);
	allocator.deallocate(static_cast<std::size_t *>(ptr),old_size / sizeof(std::size_t) + 1u);
	return new_ptr;
}

void free_function(void *ptr, std::size_t size)
{
	allocator.deallocate(static_cast<std::size_t *>(ptr),size / sizeof(std::size_t) + 1u);
}

using namespace piranha;

// template <typename Base>
// class base_complex: public Base
// {
// 	public:
// 		base_complex() = default;
// 		base_complex(const base_complex &) = default;
// 		base_complex(base_complex &&) = default;
// 		~base_complex() = default;
// 		base_complex &operator=(const base_complex &) = default;
// 		base_complex &operator=(base_complex &&other)
// 		{
// 			Base::operator=(std::move(other));
// 			return *this;
// 		}
// 		template <typename T>
// 		base_complex &operator=(T &&x)
// 		{
// std::cout << "complex assign!!!\n";
// 			Base::operator=(std::forward<T>(x));
// 			return *this;
// 		}
// 	protected:
// 		template <typename Series1, typename Series2>
// 		explicit base_complex(Series1 &&s1, Series2 &&s2, const echelon_descriptor<typename Base::term_type> &ed,
// 			typename std::enable_if<std::is_base_of<detail::base_series_tag,typename strip_cv_ref<Series1>::type>::value &&
// 			std::is_base_of<detail::base_series_tag,typename strip_cv_ref<Series2>::type>::value>::type * = piranha_nullptr)
// 		{
// std::cout << "LOLLER called!!!\n";
// 		}
// };

// void *allocate_function(std::size_t alloc_size);
// void *blab_allocate_function(std::size_t alloc_size)
// {
// 	return allocate_function(alloc_size);
// }
// 
// void *reallocate_function(void *ptr, std::size_t old_size, std::size_t new_size);
// 
// void free_function(void *ptr, std::size_t);

void gogo()
{
// 	thread_management::binder b;
	typedef polynomial<numerical_coefficient<__int128_t>,int16_t> p_type;
	p_type x("x"), y("y"), z("z"), t("t");
	auto f = 1 + x + y + z + t, tmp(f);
	for (int i = 1; i < 10; ++i) {
		f *= tmp;
	}
	std::cout << f.size() << '\n';
	auto g = f + 1;
	const boost::posix_time::ptime time0 = boost::posix_time::microsec_clock::local_time();
	auto retval = f * g;
return;
	std::cout << "Elapsed time: " << (double)(boost::posix_time::microsec_clock::local_time() - time0).total_microseconds() / 1000 << '\n';
	std::cout << retval.size() << '\n';
}

void blo()
{
	thread_management::binder b;
	typedef polynomial<numerical_coefficient<integer>,int16_t> p_type;
	p_type x("x"), y("y"), z("z"), t("t"), u("u");
	auto f = (x + y + z*z*2 + t*t*t*3 + u*u*u*u*u*5 + 1), tmp_f(f);
	auto g = (u + t + z*z*2 + y*y*y*3 + x*x*x*x*x*5 + 1), tmp_g(g);
	for (int i = 1; i < 12; ++i) {
		f *= tmp_f;
		g *= tmp_g;
	}
	std::cout << f.size() << '\n';
	std::cout << g.size() << '\n';
	const boost::posix_time::ptime time0 = boost::posix_time::microsec_clock::local_time();
	f *= g;
	std::cout << "Elapsed time: " << (double)(boost::posix_time::microsec_clock::local_time() - time0).total_microseconds() / 1000 << '\n';
	std::cout << f.size() << '\n';
}

inline std::ostream &operator<<(std::ostream &os, const __uint128_t &n)
{
	std::array<char,40u> buffer;
	auto it = buffer.begin();
	auto tmp = n;
	while (true) {
		unsigned q = tmp % 10u;
		piranha_assert(it != buffer.end());
		*it = q + '0';
		++it;
		tmp /= 10u;
		// TODO: avoid one division by checking if tmp < 10u?
		if (!tmp) {
			break;
		}
	}
	// TODO: use reverse iterator instead.
	std::reverse(buffer.begin(),it);
	std::for_each(buffer.begin(),it,[&](char c) -> void {os << c;});
	return os;
}

int main()
{
// 	mp_set_memory_functions(allocate_function,reallocate_function,free_function);
	std::fma(1.,2.,3.);
	settings::set_n_threads(2);
	gogo();
	return 0;
	
	typedef polynomial<numerical_coefficient<double>,int32_t> p_type;
	typedef p_type::term_type term_type;
	typedef term_type::cf_type cf_type;
	typedef term_type::key_type key_type;
	std::uniform_real_distribution<double> rd(-99.,99.);
	std::minstd_rand re;
	std::mt19937 ie;
	for (int n = 0; n < 1250; n += 20) {
		std::uniform_int_distribution<int32_t> id(1,10 + n);
		p_type f(rd(re)), g(rd(re)), x("x");
		f += x;
		f -= x;
		g += x;
		g -= x;
		int cur_exp1 = 0, cur_exp2 = 0;
		for (auto i = 1; i < 8192; ++i) {
			cur_exp1 += id(ie);
			f.insert(term_type(cf_type(rd(re),f.m_ed),key_type({cur_exp1})),f.m_ed);
			cur_exp2 += id(ie);
			g.insert(term_type(cf_type(rd(re),g.m_ed),key_type({cur_exp2})),g.m_ed);
		}
		std::cout << "cur_exp1 " << cur_exp1 << '\n';
		std::cout << "cur_exp2 " << cur_exp2 << '\n';
		auto input_size = f.size();
		f *= g;
		std::cout << "final = " << f.size() << '\n';
		std::cout << "W = " << (((double)input_size * input_size) / f.size()) << '\n';
	}
// 	thread_group tg;
// 	tg.create_thread(gogo);
// 	tg.create_thread(blo);
// 	tg.create_thread(gogo);
// 	tg.join_all();
// 	gogo();
// 	return 0;

// 	std::unordered_set<integer> hs(12000000);
// 	for (int i = 0; i < 6000000; ++i) {
// 		hs.insert(integer(-i * i));
// 	}
// 	std::cout << hs.size() << '\n';

// 	typedef polynomial<numerical_coefficient<double>,int16_t> p_type;
// 	p_type x("x"), y("y"), z("z"), t("t");
// 	auto f = 1 + x + y + z + t, tmp(f);
// 	for (int i = 1; i < 20; ++i) {
// 		f *= tmp;
// 	}
// 	std::cout << f.size() << '\n';
// 	const boost::posix_time::ptime time0 = boost::posix_time::microsec_clock::local_time();
// 	auto retval = f * (f + 1);
// 	std::cout << "Elapsed time: " << (double)(boost::posix_time::microsec_clock::local_time() - time0).total_microseconds() / 1000 << '\n';
// 	std::cout << retval.size() << '\n';
// 	return 0;

// 	std::unordered_set<integer> ht;
// 	for (int i = 0; i < 200000; ++i) {
// 		integer tmp(i);
// 		ht.insert(tmp);
// 	}
}
