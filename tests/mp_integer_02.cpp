/***************************************************************************
 *   Copyright (C) 2009-2011 by Francesco Biscani                          *
 *   bluescarni@gmail.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "../src/mp_integer.hpp"

#define BOOST_TEST_MODULE mp_integer_02_test
#include <boost/test/unit_test.hpp>

#define FUSION_MAX_VECTOR_SIZE 20

#include <algorithm>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/lexical_cast.hpp>
#include <cstddef>
#include <gmp.h>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "../src/config.hpp"
#include "../src/debug_access.hpp"
#include "../src/environment.hpp"
#include "../src/exceptions.hpp"
#include "../src/math.hpp"
#include "../src/type_traits.hpp"

using integral_types = boost::mpl::vector<char,
	signed char,short,int,long,long long,
	unsigned char,unsigned short,unsigned,unsigned long,unsigned long long,
	wchar_t,char16_t,char32_t>;

using floating_point_types = boost::mpl::vector<float,double,long double>;

static std::mt19937 rng;
static const int ntries = 1000;

using namespace piranha;
using mpz_raii = detail::mpz_raii;

using size_types = boost::mpl::vector<std::integral_constant<int,0>,std::integral_constant<int,8>,std::integral_constant<int,16>,
	std::integral_constant<int,32>
#if defined(PIRANHA_UINT128_T)
	,std::integral_constant<int,64>
#endif
	>;

static inline std::string mpz_lexcast(const mpz_raii &m)
{
	std::ostringstream os;
	const std::size_t size_base10 = ::mpz_sizeinbase(&m.m_mpz,10);
	if (unlikely(size_base10 > std::numeric_limits<std::size_t>::max() - static_cast<std::size_t>(2))) {
		piranha_throw(std::overflow_error,"number of digits is too large");
	}
	const auto total_size = size_base10 + 2u;
	std::vector<char> tmp;
	tmp.resize(static_cast<std::vector<char>::size_type>(total_size));
	if (unlikely(tmp.size() != total_size)) {
		piranha_throw(std::overflow_error,"number of digits is too large");
	}
	os << ::mpz_get_str(&tmp[0u],10,&m.m_mpz);
	return os.str();
}

struct mp_integer_access_tag {};

namespace piranha
{

template <>
struct debug_access<mp_integer_access_tag>
{
	template <int NBits>
	static detail::integer_union<NBits> &get(mp_integer<NBits> &i)
	{
		return i.m_int;
	}
};

}

template <int NBits>
static inline detail::integer_union<NBits> &get_m(mp_integer<NBits> &i)
{
	return piranha::debug_access<mp_integer_access_tag>::get(i);
}

struct addmul_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		BOOST_CHECK(has_multiply_accumulate<int_type>::value);
		int_type a,b,c;
		math::multiply_accumulate(a,b,c);
		BOOST_CHECK_EQUAL(a.sign(),0);
		b = 3;
		c = 2;
		a.multiply_accumulate(b,c);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),"6");
		b = -5;
		c = 2;
		math::multiply_accumulate(a,b,c);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),"-4");
		// Random testing.
		std::uniform_int_distribution<int> promote_dist(0,1);
		std::uniform_int_distribution<int> int_dist(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
		mpz_raii m_a, m_b, m_c;
		for (int i = 0; i < ntries; ++i) {
			auto tmp1 = int_dist(rng), tmp2 = int_dist(rng), tmp3 = int_dist(rng);
			int_type a{tmp1}, b{tmp2}, c{tmp3};
			::mpz_set_si(&m_a.m_mpz,static_cast<long>(tmp1));
			::mpz_set_si(&m_b.m_mpz,static_cast<long>(tmp2));
			::mpz_set_si(&m_c.m_mpz,static_cast<long>(tmp3));
			// Promote randomly a and/or b and/or c.
			if (promote_dist(rng) == 1 && a.is_static()) {
				a.promote();
			}
			if (promote_dist(rng) == 1 && b.is_static()) {
				b.promote();
			}
			if (promote_dist(rng) == 1 && c.is_static()) {
				c.promote();
			}
			::mpz_addmul(&m_a.m_mpz,&m_b.m_mpz,&m_c.m_mpz);
			math::multiply_accumulate(a,b,c);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(m_a));
		}
		// Trigger overflow with three static ints.
		{
		// Overflow from multiplication.
		int_type a,b,c;
		a = 42;
		b = c = 0.;
		BOOST_CHECK(a.is_static());
		BOOST_CHECK(b.is_static());
		BOOST_CHECK(c.is_static());
		using limb_t = typename detail::integer_union<T::value>::s_storage::limb_t;
		auto &st_a = get_m(a).g_st(), &st_b = get_m(b).g_st(), &st_c = get_m(c).g_st();
		st_b.set_bit(static_cast<limb_t>(st_a.limb_bits * 2u - 1u));
		st_c.set_bit(static_cast<limb_t>(st_a.limb_bits * 2u - 1u));
		a.multiply_accumulate(b,c);
		BOOST_CHECK(!a.is_static());
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(42 + b * c),boost::lexical_cast<std::string>(a));
		}
		{
		// Overflow from addition.
		int_type a,b,c;
		BOOST_CHECK(a.is_static());
		BOOST_CHECK(b.is_static());
		BOOST_CHECK(c.is_static());
		using limb_t = typename detail::integer_union<T::value>::s_storage::limb_t;
		auto &st_a = get_m(a).g_st(), &st_b = get_m(b).g_st(), &st_c = get_m(c).g_st();
		for (limb_t i = 0u; i < st_a.limb_bits * 2u; ++i) {
			st_a.set_bit(i);
		}
		auto old_a(a);
		st_b.set_bit(static_cast<limb_t>(0));
		st_c.set_bit(static_cast<limb_t>(0));
		a.multiply_accumulate(b,c);
		BOOST_CHECK(!a.is_static());
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(old_a + b * c),boost::lexical_cast<std::string>(a));
		}
		{
		// Promotion bug.
		using limb_t = typename detail::integer_union<T::value>::s_storage::limb_t;
		int_type a, b(2);
		mpz_raii m_a, m_b;
		::mpz_set_si(&m_b.m_mpz,2);
		get_m(a).g_st().set_bit(static_cast<limb_t>(get_m(a).g_st().limb_bits * 2u - 1u));
		::mpz_setbit(&m_a.m_mpz,static_cast< ::mp_bitcnt_t>(get_m(a).g_st().limb_bits * 2u - 1u));
		a.multiply_accumulate(a,b);
		::mpz_addmul(&m_a.m_mpz,&m_a.m_mpz,&m_b.m_mpz);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(m_a));
		}
		{
		// Promotion bug.
		using limb_t = typename detail::integer_union<T::value>::s_storage::limb_t;
		int_type a;
		mpz_raii m_a;
		get_m(a).g_st().set_bit(static_cast<limb_t>(get_m(a).g_st().limb_bits * 2u - 1u));
		::mpz_setbit(&m_a.m_mpz,static_cast< ::mp_bitcnt_t>(get_m(a).g_st().limb_bits * 2u - 1u));
		a.multiply_accumulate(a,a);
		::mpz_addmul(&m_a.m_mpz,&m_a.m_mpz,&m_a.m_mpz);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(m_a));
		}
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_addmul_test)
{
	environment env;
	boost::mpl::for_each<size_types>(addmul_tester());
}

struct in_place_mp_integer_div_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		BOOST_CHECK(is_divisible_in_place<int_type>::value);
		BOOST_CHECK((!is_divisible_in_place<const int_type,int_type>::value));
		int_type a, b;
		BOOST_CHECK((std::is_same<decltype(a /= b),int_type &>::value));
		BOOST_CHECK_THROW(a /= b,zero_division_error);
		BOOST_CHECK_EQUAL(a.sign(),0);
		BOOST_CHECK_EQUAL(b.sign(),0);
		b = 1;
		a /= b;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),"0");
		BOOST_CHECK(a.is_static());
		a = 5;
		b = 2;
		a /= b;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),"2");
		BOOST_CHECK(a.is_static());
		a = 7;
		b = -2;
		a /= b;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),"-3");
		BOOST_CHECK(a.is_static());
		a = -3;
		b = 2;
		a /= b;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),"-1");
		BOOST_CHECK(a.is_static());
		a = -10;
		b = -2;
		a /= b;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),"5");
		BOOST_CHECK(a.is_static());
		// Random testing.
		std::uniform_int_distribution<int> promote_dist(0,1);
		std::uniform_int_distribution<int> int_dist(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
		mpz_raii m_a, m_b;
		for (int i = 0; i < ntries; ++i) {
			auto tmp1 = int_dist(rng), tmp2 = int_dist(rng);
			// If tmp2 is zero, avoid doing the division.
			if (tmp2 == 0) {
				continue;
			}
			int_type a{tmp1}, b{tmp2};
			::mpz_set_si(&m_a.m_mpz,static_cast<long>(tmp1));
			::mpz_set_si(&m_b.m_mpz,static_cast<long>(tmp2));
			// Promote randomly a and/or b.
			if (promote_dist(rng) == 1 && a.is_static()) {
				a.promote();
			}
			if (promote_dist(rng) == 1 && b.is_static()) {
				b.promote();
			}
			a /= b;
			::mpz_tdiv_q(&m_a.m_mpz,&m_a.m_mpz,&m_b.m_mpz);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(m_a));
			if (tmp2 >= 1) {
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),boost::lexical_cast<std::string>(tmp1 / tmp2));
			}
		}
	}
};

struct in_place_int_div_tester
{
	template <typename U>
	struct runner
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			BOOST_CHECK((is_divisible_in_place<int_type,T>::value));
			BOOST_CHECK((!is_divisible_in_place<const int_type,T>::value));
			using int_cast_t = typename std::conditional<std::is_signed<T>::value,long long,unsigned long long>::type;
			int_type n1;
			n1 /= T(1);
			BOOST_CHECK((std::is_same<decltype(n1 /= T(1)),int_type &>::value));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),"0");
			n1 = 1;
			BOOST_CHECK_THROW(n1 /= T(0),zero_division_error);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),"1");
			n1 = T(100);
			n1 /= T(50);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),"2");
			n1 = T(99);
			n1 /= T(50);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),"1");
			// Random testing.
			std::uniform_int_distribution<T> int_dist(std::numeric_limits<T>::min(),std::numeric_limits<T>::max());
			mpz_raii m1, m2;
			for (int i = 0; i < ntries; ++i) {
				T tmp1 = int_dist(rng), tmp2 = int_dist(rng);
				if (tmp2 == T(0)) {
					continue;
				}
				int_type n(tmp1);
				n /= tmp2;
				::mpz_set_str(&m1.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp1)).c_str(),10);
				::mpz_set_str(&m2.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp2)).c_str(),10);
				::mpz_tdiv_q(&m1.m_mpz,&m1.m_mpz,&m2.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),mpz_lexcast(m1));
			}
			// int /= mp_integer.
			BOOST_CHECK((is_divisible_in_place<T,int_type>::value));
			BOOST_CHECK((!is_divisible_in_place<const T,int_type>::value));
			T n2(8);
			n2 /= int_type(2);
			BOOST_CHECK((std::is_same<T &,decltype(n2 /= int_type(2))>::value));
			BOOST_CHECK_EQUAL(n2,T(4));
			BOOST_CHECK_THROW(n2 /= int_type(0),zero_division_error);
			BOOST_CHECK_EQUAL(n2,T(4));
			for (int i = 0; i < ntries; ++i) {
				T tmp1 = int_dist(rng), tmp2 = int_dist(rng);
				::mpz_set_str(&m1.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp1)).c_str(),10);
				::mpz_set_str(&m2.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp2)).c_str(),10);
				if (tmp2 == T(0)) {
					continue;
				}
				try {
					tmp1 /= int_type(tmp2);
				} catch (const std::overflow_error &) {
					// Catch overflow errors and move on.
					continue;
				}
				::mpz_tdiv_q(&m1.m_mpz,&m1.m_mpz,&m2.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp1)),mpz_lexcast(m1));
			}
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<integral_types>(runner<T>());
	}
};

struct in_place_float_div_tester
{
	template <typename U>
	struct runner
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			BOOST_CHECK((is_divisible_in_place<int_type,T>::value));
			BOOST_CHECK((!is_divisible_in_place<const int_type,T>::value));
			int_type n1(2);
			n1 /= T(2);
			BOOST_CHECK((std::is_same<decltype(n1 /= T(2)),int_type &>::value));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),"1");
			n1 = T(4);
			n1 /= T(-2);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),"-2");
			n1 = T(-4);
			n1 /= T(2);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),"-2");
			n1 = T(-4);
			n1 /= T(-2);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),"2");
			BOOST_CHECK_THROW(n1 /= T(0),zero_division_error);
			// Random testing
			std::uniform_real_distribution<T> urd1(T(0),std::numeric_limits<T>::max()), urd2(std::numeric_limits<T>::lowest(),T(0));
			for (int i = 0; i < ntries / 100; ++i) {
				const T tmp1 = urd1(rng);
				if (tmp1 == T(0)) {
					continue;
				}
				int_type n(tmp1);
				n /= tmp1;
				BOOST_CHECK(boost::lexical_cast<std::string>(n) == "0" || boost::lexical_cast<std::string>(n) == "1");
				const T tmp2 = urd2(rng);
				if (tmp2 == T(0)) {
					continue;
				}
				n = tmp2;
				n /= tmp2;
				BOOST_CHECK(boost::lexical_cast<std::string>(n) == "0" || boost::lexical_cast<std::string>(n) == "1");
			}
			// float /= mp_integer.
			BOOST_CHECK((is_divisible_in_place<T,int_type>::value));
			BOOST_CHECK((!is_divisible_in_place<const T,int_type>::value));
			T x1(3);
			x1 /= int_type(2);
			BOOST_CHECK((std::is_same<T &,decltype(x1 /= int_type(2))>::value));
			BOOST_CHECK_EQUAL(x1,T(3) / T(2));
			BOOST_CHECK_THROW(x1 /= int_type(0),zero_division_error);
			// NOTE: limit the number of times we run this test, as the conversion from int to float
			// is slow as the random values are taken from the entire float range and thus use a lot of digits.
			for (int i = 0; i < ntries / 100; ++i) {
				T tmp1(1), tmp2 = urd1(rng);
				if (tmp2 == T(0)) {
					continue;
				}
				tmp1 /= int_type{tmp2};
				BOOST_CHECK_EQUAL(tmp1,T(1)/static_cast<T>(int_type{tmp2}));
				tmp1 = T(1);
				tmp2 = urd2(rng);
				if (tmp2 == T(0)) {
					continue;
				}
				tmp1 /= int_type{tmp2};
				BOOST_CHECK_EQUAL(tmp1,T(1)/static_cast<T>(int_type{tmp2}));
			}
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<floating_point_types>(runner<T>());
	}
};

struct binary_div_tester
{
	template <typename U>
	struct runner1
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			using int_cast_t = typename std::conditional<std::is_signed<T>::value,long long,unsigned long long>::type;
			BOOST_CHECK((is_divisible<int_type,T>::value));
			BOOST_CHECK((is_divisible<T,int_type>::value));
			int_type n(4);
			T m(2);
			BOOST_CHECK((std::is_same<decltype(n / m),int_type>::value));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n / m),"2");
			BOOST_CHECK_THROW(n / T(0),zero_division_error);
			BOOST_CHECK_THROW(T(1) / int_type(0),zero_division_error);
			// Random testing.
			std::uniform_int_distribution<T> int_dist(std::numeric_limits<T>::min(),std::numeric_limits<T>::max());
			mpz_raii m1, m2, res;
			for (int i = 0; i < ntries; ++i) {
				T tmp1 = int_dist(rng), tmp2 = int_dist(rng);
				if (tmp2 == T(0)) {
					continue;
				}
				int_type n(tmp1);
				::mpz_set_str(&m1.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp1)).c_str(),10);
				::mpz_set_str(&m2.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp2)).c_str(),10);
				::mpz_tdiv_q(&res.m_mpz,&m1.m_mpz,&m2.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n / tmp2),mpz_lexcast(res));
				if (tmp1 == T(0)) {
					continue;
				}
				::mpz_tdiv_q(&res.m_mpz,&m2.m_mpz,&m1.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(tmp2 / n),mpz_lexcast(res));
			}
		}
	};
	template <typename U>
	struct runner2
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			BOOST_CHECK((is_divisible<int_type,T>::value));
			BOOST_CHECK((is_divisible<T,int_type>::value));
			int_type n;
			T m;
			BOOST_CHECK((std::is_same<decltype(n / m),T>::value));
			BOOST_CHECK_THROW(int_type(1) / T(0),zero_division_error);
			BOOST_CHECK_THROW(T(1) / int_type(0),zero_division_error);
			// Random testing
			std::uniform_real_distribution<T> urd1(T(0),std::numeric_limits<T>::max()), urd2(std::numeric_limits<T>::lowest(),T(0));
			for (int i = 0; i < ntries; ++i) {
				int_type n(1);
				const T tmp1 = urd1(rng);
				if (tmp1 == T(0)) {
					continue;
				}
				BOOST_CHECK_EQUAL(n / tmp1, T(1) / tmp1);
				BOOST_CHECK_EQUAL(tmp1 / n,tmp1 / T(1));
				const T tmp2 = urd2(rng);
				if (tmp2 == T(0)) {
					continue;
				}
				BOOST_CHECK_EQUAL(n / tmp2, T(1) / tmp2);
				BOOST_CHECK_EQUAL(tmp2 / n, tmp2 / T(1));
			}
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		BOOST_CHECK(is_divisible<int_type>::value);
		int_type n1(4), n2(2);
		BOOST_CHECK((std::is_same<decltype(n1 / n2),int_type>::value));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1/n2),"2");
		n1 = 2;
		n2 = 4;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1/n2),"0");
		n1 = -6;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1/n2),"-1");
		n2 = -3;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1/n2),"2");
		BOOST_CHECK_THROW(n1 / int_type(0),zero_division_error);
		// Random testing.
		std::uniform_int_distribution<int> promote_dist(0,1);
		std::uniform_int_distribution<int> int_dist(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
		mpz_raii m_a, m_b;
		for (int i = 0; i < ntries; ++i) {
			auto tmp1 = int_dist(rng), tmp2 = int_dist(rng);
			if (tmp2 == 0) {
				continue;
			}
			int_type a{tmp1}, b{tmp2};
			::mpz_set_si(&m_a.m_mpz,static_cast<long>(tmp1));
			::mpz_set_si(&m_b.m_mpz,static_cast<long>(tmp2));
			// Promote randomly a and/or b.
			if (promote_dist(rng) == 1 && a.is_static()) {
				a.promote();
			}
			if (promote_dist(rng) == 1 && b.is_static()) {
				b.promote();
			}
			::mpz_tdiv_q(&m_a.m_mpz,&m_a.m_mpz,&m_b.m_mpz);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a / b),mpz_lexcast(m_a));
		}
		// Test vs hardware int and float types.
		boost::mpl::for_each<integral_types>(runner1<T>());
		boost::mpl::for_each<floating_point_types>(runner2<T>());
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_div_test)
{
	boost::mpl::for_each<size_types>(in_place_mp_integer_div_tester());
	boost::mpl::for_each<size_types>(in_place_int_div_tester());
	boost::mpl::for_each<size_types>(in_place_float_div_tester());
	boost::mpl::for_each<size_types>(binary_div_tester());
}

struct in_place_mp_integer_mod_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		int_type a, b;
		BOOST_CHECK((std::is_same<decltype(a %= b),int_type &>::value));
		BOOST_CHECK_THROW(a %= b,zero_division_error);
		BOOST_CHECK_EQUAL(a.sign(),0);
		BOOST_CHECK_EQUAL(b.sign(),0);
		b = 1;
		a %= b;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),"0");
		BOOST_CHECK(a.is_static());
		a = 5;
		b = 2;
		a %= b;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),"1");
		BOOST_CHECK(a.is_static());
		a = 7;
		b = -2;
		a %= b;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),"1");
		BOOST_CHECK(a.is_static());
		a = -3;
		b = 2;
		a %= b;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),"-1");
		BOOST_CHECK(a.is_static());
		a = -10;
		b = -2;
		a %= b;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),"0");
		BOOST_CHECK(a.is_static());
		// Random testing.
		std::uniform_int_distribution<int> promote_dist(0,1);
		std::uniform_int_distribution<int> int_dist(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
		mpz_raii m_a, m_b;
		for (int i = 0; i < ntries; ++i) {
			auto tmp1 = int_dist(rng), tmp2 = int_dist(rng);
			// If tmp2 is zero, avoid doing the modulo.
			if (tmp2 == 0) {
				continue;
			}
			int_type a{tmp1}, b{tmp2};
			::mpz_set_si(&m_a.m_mpz,static_cast<long>(tmp1));
			::mpz_set_si(&m_b.m_mpz,static_cast<long>(tmp2));
			// Promote randomly a and/or b.
			if (promote_dist(rng) == 1 && a.is_static()) {
				a.promote();
			}
			if (promote_dist(rng) == 1 && b.is_static()) {
				b.promote();
			}
			a %= b;
			::mpz_tdiv_r(&m_a.m_mpz,&m_a.m_mpz,&m_b.m_mpz);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(m_a));
			if (tmp2 >= 1) {
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),boost::lexical_cast<std::string>(tmp1 % tmp2));
			}
		}
	}
};

struct in_place_int_mod_tester
{
	template <typename U>
	struct runner
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			using int_cast_t = typename std::conditional<std::is_signed<T>::value,long long,unsigned long long>::type;
			int_type n1;
			n1 %= T(1);
			BOOST_CHECK((std::is_same<decltype(n1 %= T(1)),int_type &>::value));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),"0");
			n1 = 1;
			BOOST_CHECK_THROW(n1 %= T(0),zero_division_error);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),"1");
			n1 = T(100);
			n1 %= T(50);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),"0");
			n1 = T(99);
			n1 %= T(50);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),"49");
			// Random testing.
			std::uniform_int_distribution<T> int_dist(std::numeric_limits<T>::min(),std::numeric_limits<T>::max());
			mpz_raii m1, m2;
			for (int i = 0; i < ntries; ++i) {
				T tmp1 = int_dist(rng), tmp2 = int_dist(rng);
				if (tmp2 == T(0)) {
					continue;
				}
				int_type n(tmp1);
				n %= tmp2;
				::mpz_set_str(&m1.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp1)).c_str(),10);
				::mpz_set_str(&m2.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp2)).c_str(),10);
				::mpz_tdiv_r(&m1.m_mpz,&m1.m_mpz,&m2.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),mpz_lexcast(m1));
			}
			// int %= mp_integer.
			T n2(8);
			n2 %= int_type(2);
			BOOST_CHECK((std::is_same<T &,decltype(n2 %= int_type(2))>::value));
			BOOST_CHECK_EQUAL(n2,T(0));
			BOOST_CHECK_THROW(n2 %= int_type(0),zero_division_error);
			BOOST_CHECK_EQUAL(n2,T(0));
			for (int i = 0; i < ntries; ++i) {
				T tmp1 = int_dist(rng), tmp2 = int_dist(rng);
				::mpz_set_str(&m1.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp1)).c_str(),10);
				::mpz_set_str(&m2.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp2)).c_str(),10);
				if (tmp2 == T(0)) {
					continue;
				}
				try {
					tmp1 %= int_type(tmp2);
				} catch (const std::overflow_error &) {
					// Catch overflow errors and move on.
					continue;
				}
				::mpz_tdiv_r(&m1.m_mpz,&m1.m_mpz,&m2.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp1)),mpz_lexcast(m1));
			}
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<integral_types>(runner<T>());
	}
};

struct binary_mod_tester
{
	template <typename U>
	struct runner1
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			using int_cast_t = typename std::conditional<std::is_signed<T>::value,long long,unsigned long long>::type;
			int_type n(4);
			T m(2);
			BOOST_CHECK((std::is_same<decltype(n % m),int_type>::value));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n % m),"0");
			BOOST_CHECK_THROW(n % T(0),zero_division_error);
			BOOST_CHECK_THROW(T(1) % int_type(0),zero_division_error);
			// Random testing.
			std::uniform_int_distribution<T> int_dist(std::numeric_limits<T>::min(),std::numeric_limits<T>::max());
			mpz_raii m1, m2, res;
			for (int i = 0; i < ntries; ++i) {
				T tmp1 = int_dist(rng), tmp2 = int_dist(rng);
				if (tmp2 == T(0)) {
					continue;
				}
				int_type n(tmp1);
				::mpz_set_str(&m1.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp1)).c_str(),10);
				::mpz_set_str(&m2.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp2)).c_str(),10);
				::mpz_tdiv_r(&res.m_mpz,&m1.m_mpz,&m2.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n % tmp2),mpz_lexcast(res));
				if (tmp1 == T(0)) {
					continue;
				}
				::mpz_tdiv_r(&res.m_mpz,&m2.m_mpz,&m1.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(tmp2 % n),mpz_lexcast(res));
			}
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		int_type n1(4), n2(2);
		BOOST_CHECK((std::is_same<decltype(n1 % n2),int_type>::value));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1%n2),"0");
		n1 = 2;
		n2 = 4;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1%n2),"2");
		n1 = -6;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1%n2),"-2");
		n2 = -5;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1%n2),"-1");
		BOOST_CHECK_THROW(n1 % int_type(0),zero_division_error);
		// Random testing.
		std::uniform_int_distribution<int> promote_dist(0,1);
		std::uniform_int_distribution<int> int_dist(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
		mpz_raii m_a, m_b;
		for (int i = 0; i < ntries; ++i) {
			auto tmp1 = int_dist(rng), tmp2 = int_dist(rng);
			if (tmp2 == 0) {
				continue;
			}
			int_type a{tmp1}, b{tmp2};
			::mpz_set_si(&m_a.m_mpz,static_cast<long>(tmp1));
			::mpz_set_si(&m_b.m_mpz,static_cast<long>(tmp2));
			// Promote randomly a and/or b.
			if (promote_dist(rng) == 1 && a.is_static()) {
				a.promote();
			}
			if (promote_dist(rng) == 1 && b.is_static()) {
				b.promote();
			}
			::mpz_tdiv_r(&m_a.m_mpz,&m_a.m_mpz,&m_b.m_mpz);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a % b),mpz_lexcast(m_a));
		}
		// Test vs hardware int types.
		boost::mpl::for_each<integral_types>(runner1<T>());
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_mod_test)
{
	boost::mpl::for_each<size_types>(in_place_mp_integer_mod_tester());
	boost::mpl::for_each<size_types>(in_place_int_mod_tester());
	boost::mpl::for_each<size_types>(binary_mod_tester());
}

struct mp_integer_cmp_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		BOOST_CHECK(is_equality_comparable<int_type>::value);
		BOOST_CHECK(is_less_than_comparable<int_type>::value);
		int_type a, b;
		BOOST_CHECK((std::is_same<decltype(a == b),bool>::value));
		BOOST_CHECK((std::is_same<decltype(a != b),bool>::value));
		BOOST_CHECK((std::is_same<decltype(a < b),bool>::value));
		BOOST_CHECK((std::is_same<decltype(a > b),bool>::value));
		BOOST_CHECK((std::is_same<decltype(a <= b),bool>::value));
		BOOST_CHECK((std::is_same<decltype(a >= b),bool>::value));
		BOOST_CHECK(a == b);
		BOOST_CHECK(a <= b);
		BOOST_CHECK(a <= a);
		BOOST_CHECK(a >= b);
		BOOST_CHECK(a >= a);
		BOOST_CHECK(!(a < b));
		BOOST_CHECK(!(a < a));
		BOOST_CHECK(!(b < a));
		BOOST_CHECK(!(a > b));
		BOOST_CHECK(!(a > a));
		BOOST_CHECK(!(b > a));
		BOOST_CHECK(!(a != b));
		b = 1;
		a = -1;
		BOOST_CHECK(!(a == b));
		BOOST_CHECK(a != b);
		BOOST_CHECK(a < b);
		BOOST_CHECK(a <= b);
		BOOST_CHECK(b > a);
		BOOST_CHECK(b >= a);
		BOOST_CHECK(!(b < a));
		BOOST_CHECK(!(a > b));
		// Random testing.
		std::uniform_int_distribution<int> promote_dist(0,1);
		std::uniform_int_distribution<int> int_dist(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
		for (int i = 0; i < ntries; ++i) {
			auto tmp1 = int_dist(rng), tmp2 = int_dist(rng);
			int_type a{tmp1}, b{tmp2};
			// Promote randomly a and/or b.
			if (promote_dist(rng) == 1 && a.is_static()) {
				a.promote();
			}
			if (promote_dist(rng) == 1 && b.is_static()) {
				b.promote();
			}
			BOOST_CHECK(a == a);
			BOOST_CHECK(a >= a);
			BOOST_CHECK(a <= a);
			BOOST_CHECK(!(a < a));
			BOOST_CHECK(!(a > a));
			BOOST_CHECK(b == b);
			BOOST_CHECK((a == b) == (tmp1 == tmp2));
			BOOST_CHECK((a < b) == (tmp1 < tmp2));
			BOOST_CHECK((a > b) == (tmp1 > tmp2));
			BOOST_CHECK((a != b) == (tmp1 != tmp2));
			BOOST_CHECK((a >= b) == (tmp1 >= tmp2));
			BOOST_CHECK((a <= b) == (tmp1 <= tmp2));
		}
	}
};

struct int_cmp_tester
{
	template <typename U>
	struct runner
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			using int_cast_t = typename std::conditional<std::is_signed<T>::value,long long,unsigned long long>::type;
			BOOST_CHECK((is_equality_comparable<int_type,T>::value));
			BOOST_CHECK((is_equality_comparable<T,int_type>::value));
			BOOST_CHECK((is_less_than_comparable<int_type,T>::value));
			BOOST_CHECK((is_less_than_comparable<T,int_type>::value));
			int_type n1;
			BOOST_CHECK((std::is_same<decltype(n1 == T(0)),bool>::value));
			BOOST_CHECK((std::is_same<decltype(T(0) == n1),bool>::value));
			BOOST_CHECK((std::is_same<decltype(n1 < T(0)),bool>::value));
			BOOST_CHECK((std::is_same<decltype(T(0) < n1),bool>::value));
			BOOST_CHECK((std::is_same<decltype(n1 > T(0)),bool>::value));
			BOOST_CHECK((std::is_same<decltype(T(0) > n1),bool>::value));
			BOOST_CHECK((std::is_same<decltype(n1 <= T(0)),bool>::value));
			BOOST_CHECK((std::is_same<decltype(T(0) <= n1),bool>::value));
			BOOST_CHECK((std::is_same<decltype(n1 >= T(0)),bool>::value));
			BOOST_CHECK((std::is_same<decltype(T(0) >= n1),bool>::value));
			BOOST_CHECK(n1 == T(0));
			BOOST_CHECK(T(0) == n1);
			BOOST_CHECK(n1 <= T(0));
			BOOST_CHECK(T(0) <= n1);
			BOOST_CHECK(n1 >= T(0));
			BOOST_CHECK(T(0) >= n1);
			BOOST_CHECK(!(n1 < T(0)));
			BOOST_CHECK(!(n1 > T(0)));
			BOOST_CHECK(!(T(0) < n1));
			BOOST_CHECK(!(T(0) > n1));
			n1 = -1;
			BOOST_CHECK(n1 != T(0));
			BOOST_CHECK(n1 < T(0));
			BOOST_CHECK(n1 <= T(0));
			BOOST_CHECK(T(0) > n1);
			BOOST_CHECK(T(0) >= n1);
			BOOST_CHECK(T(0) != n1);
			BOOST_CHECK(!(T(0) < n1));
			BOOST_CHECK(!(T(0) <= n1));
			BOOST_CHECK(!(n1 > T(0)));
			BOOST_CHECK(!(n1 >= T(0)));
			// Random testing.
			std::uniform_int_distribution<T> int_dist(std::numeric_limits<T>::min(),std::numeric_limits<T>::max());
			mpz_raii m1, m2;
			for (int i = 0; i < ntries; ++i) {
				T tmp1 = int_dist(rng), tmp2 = int_dist(rng);
				int_type n(tmp1);
				::mpz_set_str(&m1.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp1)).c_str(),10);
				::mpz_set_str(&m2.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp2)).c_str(),10);
				BOOST_CHECK(n == tmp1);
				BOOST_CHECK(tmp1 == n);
				BOOST_CHECK(n <= tmp1);
				BOOST_CHECK(tmp1 <= n);
				BOOST_CHECK(n >= tmp1);
				BOOST_CHECK(tmp1 >= n);
				BOOST_CHECK(!(n < tmp1));
				BOOST_CHECK(!(tmp1 < n));
				BOOST_CHECK(!(n > tmp1));
				BOOST_CHECK(!(tmp1 > n));
				BOOST_CHECK_EQUAL(n == tmp2, ::mpz_cmp(&m1.m_mpz,&m2.m_mpz) == 0);
				BOOST_CHECK_EQUAL(tmp2 == n, ::mpz_cmp(&m1.m_mpz,&m2.m_mpz) == 0);
				BOOST_CHECK_EQUAL(n != tmp2, ::mpz_cmp(&m1.m_mpz,&m2.m_mpz) != 0);
				BOOST_CHECK_EQUAL(tmp2 != n, ::mpz_cmp(&m1.m_mpz,&m2.m_mpz) != 0);
				BOOST_CHECK_EQUAL(n < tmp2, ::mpz_cmp(&m1.m_mpz,&m2.m_mpz) < 0);
				BOOST_CHECK_EQUAL(tmp2 < n, ::mpz_cmp(&m2.m_mpz,&m1.m_mpz) < 0);
				BOOST_CHECK_EQUAL(n > tmp2, ::mpz_cmp(&m1.m_mpz,&m2.m_mpz) > 0);
				BOOST_CHECK_EQUAL(tmp2 > n, ::mpz_cmp(&m2.m_mpz,&m1.m_mpz) > 0);
				BOOST_CHECK_EQUAL(n <= tmp2, ::mpz_cmp(&m1.m_mpz,&m2.m_mpz) <= 0);
				BOOST_CHECK_EQUAL(tmp2 <= n, ::mpz_cmp(&m2.m_mpz,&m1.m_mpz) <= 0);
				BOOST_CHECK_EQUAL(n >= tmp2, ::mpz_cmp(&m1.m_mpz,&m2.m_mpz) >= 0);
				BOOST_CHECK_EQUAL(tmp2 >= n, ::mpz_cmp(&m2.m_mpz,&m1.m_mpz) >= 0);
			}
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<integral_types>(runner<T>());
	}
};

struct float_cmp_tester
{
	template <typename U>
	struct runner
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			BOOST_CHECK((is_equality_comparable<int_type,T>::value));
			BOOST_CHECK((is_equality_comparable<T,int_type>::value));
			BOOST_CHECK((is_less_than_comparable<int_type,T>::value));
			BOOST_CHECK((is_less_than_comparable<T,int_type>::value));
			int_type n1;
			BOOST_CHECK((std::is_same<decltype(n1 == T(0)),bool>::value));
			BOOST_CHECK((std::is_same<decltype(T(0) == n1),bool>::value));
			BOOST_CHECK((std::is_same<decltype(n1 < T(0)),bool>::value));
			BOOST_CHECK((std::is_same<decltype(T(0) < n1),bool>::value));
			BOOST_CHECK((std::is_same<decltype(n1 > T(0)),bool>::value));
			BOOST_CHECK((std::is_same<decltype(T(0) > n1),bool>::value));
			BOOST_CHECK((std::is_same<decltype(n1 <= T(0)),bool>::value));
			BOOST_CHECK((std::is_same<decltype(T(0) <= n1),bool>::value));
			BOOST_CHECK((std::is_same<decltype(n1 >= T(0)),bool>::value));
			BOOST_CHECK((std::is_same<decltype(T(0) >= n1),bool>::value));
			BOOST_CHECK(n1 == T(0));
			BOOST_CHECK(T(0) == n1);
			BOOST_CHECK(n1 <= T(0));
			BOOST_CHECK(T(0) <= n1);
			BOOST_CHECK(n1 >= T(0));
			BOOST_CHECK(T(0) >= n1);
			BOOST_CHECK(!(n1 != T(0)));
			BOOST_CHECK(!(T(0) != n1));
			BOOST_CHECK(!(n1 < T(0)));
			BOOST_CHECK(!(T(0) < n1));
			BOOST_CHECK(!(n1 > T(0)));
			BOOST_CHECK(!(T(0) > n1));
			n1 = -1;
			BOOST_CHECK(n1 != T(0));
			BOOST_CHECK(T(0) != n1);
			BOOST_CHECK(!(n1 == T(0)));
			BOOST_CHECK(!(T(0) == n1));
			BOOST_CHECK(n1 < T(0));
			BOOST_CHECK(n1 <= T(0));
			BOOST_CHECK(!(T(0) < n1));
			BOOST_CHECK(!(T(0) <= n1));
			BOOST_CHECK(!(n1 > T(0)));
			BOOST_CHECK(T(0) > n1);
			BOOST_CHECK(!(n1 >= T(0)));
			BOOST_CHECK(T(0) >= n1);
			// Random testing
			std::uniform_real_distribution<T> urd1(T(0),std::numeric_limits<T>::max()), urd2(std::numeric_limits<T>::lowest(),T(0));
			for (int i = 0; i < ntries / 100; ++i) {
				const T tmp1 = urd1(rng);
				int_type n(tmp1);
				BOOST_CHECK((n == tmp1) == (static_cast<T>(n) == tmp1));
				BOOST_CHECK((tmp1 == n) == (static_cast<T>(n) == tmp1));
				BOOST_CHECK((n != tmp1) == (static_cast<T>(n) != tmp1));
				BOOST_CHECK((tmp1 != n) == (static_cast<T>(n) != tmp1));
				BOOST_CHECK((n < tmp1) == (static_cast<T>(n) < tmp1));
				BOOST_CHECK((tmp1 < n) == (tmp1 < static_cast<T>(n)));
				BOOST_CHECK((n > tmp1) == (static_cast<T>(n) > tmp1));
				BOOST_CHECK((tmp1 > n) == (tmp1 > static_cast<T>(n)));
				BOOST_CHECK((n <= tmp1) == (static_cast<T>(n) <= tmp1));
				BOOST_CHECK((tmp1 <= n) == (tmp1 <= static_cast<T>(n)));
				BOOST_CHECK((n >= tmp1) == (static_cast<T>(n) >= tmp1));
				BOOST_CHECK((tmp1 >= n) == (tmp1 >= static_cast<T>(n)));
				const T tmp2 = urd2(rng);
				n = tmp2;
				BOOST_CHECK((n == tmp2) == (static_cast<T>(n) == tmp2));
				BOOST_CHECK((tmp2 == n) == (static_cast<T>(n) == tmp2));
				BOOST_CHECK((n != tmp2) == (static_cast<T>(n) != tmp2));
				BOOST_CHECK((tmp2 != n) == (static_cast<T>(n) != tmp2));
				BOOST_CHECK((n < tmp2) == (static_cast<T>(n) < tmp2));
				BOOST_CHECK((tmp2 < n) == (tmp2 < static_cast<T>(n)));
				BOOST_CHECK((n > tmp2) == (static_cast<T>(n) > tmp2));
				BOOST_CHECK((tmp2 > n) == (tmp2 > static_cast<T>(n)));
				BOOST_CHECK((n <= tmp2) == (static_cast<T>(n) <= tmp2));
				BOOST_CHECK((tmp2 <= n) == (tmp2 <= static_cast<T>(n)));
				BOOST_CHECK((n >= tmp2) == (static_cast<T>(n) >= tmp2));
				BOOST_CHECK((tmp2 >= n) == (tmp2 >= static_cast<T>(n)));
			}
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<floating_point_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_cmp_test)
{
	boost::mpl::for_each<size_types>(mp_integer_cmp_tester());
	boost::mpl::for_each<size_types>(int_cmp_tester());
	boost::mpl::for_each<size_types>(float_cmp_tester());
}

struct int_pow_tester
{
	template <typename U>
	struct runner
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			using int_cast_t = typename std::conditional<std::is_signed<T>::value,long long,unsigned long long>::type;
			BOOST_CHECK((is_exponentiable<int_type,T>::value));
			BOOST_CHECK((!is_exponentiable<int_type,float>::value));
			BOOST_CHECK((!is_exponentiable<int_type,double>::value));
			BOOST_CHECK((!is_exponentiable<int_type,long double>::value));
			int_type n;
			BOOST_CHECK((std::is_same<int_type,decltype(math::pow(n,T(0)))>::value));
			BOOST_CHECK_EQUAL(n.pow(T(0)),1);
			if (std::is_signed<T>::value) {
				BOOST_CHECK_THROW(n.pow(T(-1)),zero_division_error);
			}
			n = 1;
			BOOST_CHECK_EQUAL(n.pow(T(0)),1);
			if (std::is_signed<T>::value) {
				BOOST_CHECK_EQUAL(n.pow(T(-1)),1);
			}
			n = -1;
			BOOST_CHECK_EQUAL(n.pow(T(0)),1);
			if (std::is_signed<T>::value) {
				BOOST_CHECK_EQUAL(n.pow(T(-1)),-1);
			}
			n = 2;
			BOOST_CHECK_EQUAL(n.pow(T(0)),1);
			BOOST_CHECK_EQUAL(n.pow(T(1)),2);
			BOOST_CHECK_EQUAL(n.pow(T(2)),4);
			BOOST_CHECK_EQUAL(n.pow(T(4)),16);
			BOOST_CHECK_EQUAL(n.pow(T(5)),32);
			if (std::is_signed<T>::value) {
				BOOST_CHECK_EQUAL(n.pow(T(-1)),0);
			}
			n = -3;
			BOOST_CHECK_EQUAL(n.pow(T(0)),1);
			BOOST_CHECK_EQUAL(n.pow(T(1)),-3);
			BOOST_CHECK_EQUAL(n.pow(T(2)),9);
			BOOST_CHECK_EQUAL(n.pow(T(4)),81);
			BOOST_CHECK_EQUAL(n.pow(T(5)),-243);
			BOOST_CHECK_EQUAL(n.pow(T(13)),-1594323);
			if (std::is_signed<T>::value) {
				BOOST_CHECK_EQUAL(n.pow(T(-1)),0);
			}
			// Random testing.
			const T max_exp = static_cast<T>(std::min(int_cast_t(1000),int_cast_t(std::numeric_limits<T>::max())));
			std::uniform_int_distribution<T> exp_dist(T(0),max_exp);
			std::uniform_int_distribution<int> base_dist(-1000,1000);
			mpz_raii m_base;
			for (int i = 0; i < ntries; ++i) {
				auto base_int = base_dist(rng);
				auto exp_int = exp_dist(rng);
				auto retval = int_type(base_int).pow(exp_int);
				::mpz_set_si(&m_base.m_mpz,static_cast<long>(base_int));
				::mpz_pow_ui(&m_base.m_mpz,&m_base.m_mpz,static_cast<unsigned long>(exp_int));
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(retval),mpz_lexcast(m_base));
				BOOST_CHECK_EQUAL(math::pow(int_type(base_int),exp_int),retval);
			}
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<integral_types>(runner<T>());
	}
};

struct mp_integer_pow_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		BOOST_CHECK((is_exponentiable<int_type,int_type>::value));
		BOOST_CHECK((!is_exponentiable<int_type,float>::value));
		BOOST_CHECK((!is_exponentiable<int_type,double>::value));
		BOOST_CHECK((!is_exponentiable<int_type,long double>::value));
		int_type n;
		BOOST_CHECK((std::is_same<int_type,decltype(math::pow(n,n))>::value));
		BOOST_CHECK_EQUAL(n.pow(int_type(0)),1);
		BOOST_CHECK_THROW(n.pow(int_type(-1)),zero_division_error);
		n = 1;
		BOOST_CHECK_EQUAL(n.pow(int_type(0)),1);
		BOOST_CHECK_EQUAL(n.pow(int_type(-1)),1);
		n = -1;
		BOOST_CHECK_EQUAL(n.pow(int_type(0)),1);
		BOOST_CHECK_EQUAL(n.pow(int_type(-1)),-1);
		n = 2;
		BOOST_CHECK_EQUAL(n.pow(int_type(0)),1);
		BOOST_CHECK_EQUAL(n.pow(int_type(1)),2);
		BOOST_CHECK_EQUAL(n.pow(int_type(2)),4);
		BOOST_CHECK_EQUAL(n.pow(int_type(4)),16);
		BOOST_CHECK_EQUAL(n.pow(int_type(5)),32);
		BOOST_CHECK_EQUAL(n.pow(int_type(-1)),0);
		n = -3;
		BOOST_CHECK_EQUAL(n.pow(int_type(0)),1);
		BOOST_CHECK_EQUAL(n.pow(int_type(1)),-3);
		BOOST_CHECK_EQUAL(n.pow(int_type(2)),9);
		BOOST_CHECK_EQUAL(n.pow(int_type(4)),81);
		BOOST_CHECK_EQUAL(n.pow(int_type(5)),-243);
		BOOST_CHECK_EQUAL(n.pow(int_type(13)),-1594323);
		BOOST_CHECK_EQUAL(n.pow(int_type(-1)),0);
		BOOST_CHECK_THROW(n.pow(int_type(std::numeric_limits<unsigned long>::max()) + 1),std::invalid_argument);
		// Random testing.
		std::uniform_int_distribution<int> exp_dist(0,1000);
		std::uniform_int_distribution<int> base_dist(-1000,1000);
		mpz_raii m_base;
		for (int i = 0; i < ntries; ++i) {
			auto base_int = base_dist(rng);
			auto exp_int = exp_dist(rng);
			auto retval = int_type(base_int).pow(int_type(exp_int));
			::mpz_set_si(&m_base.m_mpz,static_cast<long>(base_int));
			::mpz_pow_ui(&m_base.m_mpz,&m_base.m_mpz,static_cast<unsigned long>(exp_int));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(retval),mpz_lexcast(m_base));
			BOOST_CHECK_EQUAL(math::pow(int_type(base_int),int_type(exp_int)),retval);
		}
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_pow_test)
{
	boost::mpl::for_each<size_types>(int_pow_tester());
	boost::mpl::for_each<size_types>(mp_integer_pow_tester());
}

struct abs_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		int_type n;
		BOOST_CHECK_EQUAL(n.abs(),0);
		n = -5;
		BOOST_CHECK_EQUAL(math::abs(n),5);
		n = 50;
		BOOST_CHECK_EQUAL(math::abs(n),50);
		n = 0;
		int_type m0, m1, m2;
		m0.promote();
		BOOST_CHECK_EQUAL(m0.abs(),0);
		m1 = -5;
		m1.promote();
		BOOST_CHECK_EQUAL(math::abs(m1),5);
		m2 = 50;
		m2.promote();
		BOOST_CHECK_EQUAL(math::abs(m2),50);
		// Random testing.
		std::uniform_int_distribution<int> promote_dist(0,1),
			int_dist(std::numeric_limits<int>::lowest(),std::numeric_limits<int>::max());
		mpz_raii m_tmp;
		for (int i = 0; i < ntries; ++i) {
			int tmp = int_dist(rng);
			int_type n(tmp);
			if (promote_dist(rng) != 0 && n.is_static()) {
				n.promote();
			}
			::mpz_set_si(&m_tmp.m_mpz,static_cast<long>(tmp));
			::mpz_abs(&m_tmp.m_mpz,&m_tmp.m_mpz);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n.abs()),mpz_lexcast(m_tmp));
			BOOST_CHECK_EQUAL(n.abs(),math::abs(n));
		}
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_abs_test)
{
	boost::mpl::for_each<size_types>(abs_tester());
}

using ru_test_types = boost::mpl::vector<std::tuple<unsigned char,unsigned long long>,std::tuple<unsigned long long,unsigned char>,
	std::tuple<unsigned long long,unsigned long long>,std::tuple<unsigned char,unsigned char>>;

struct read_uint_tester
{
	template <typename T>
	void operator()(const T &)
	{
		// Input type.
		using in_type = typename std::tuple_element<0u,T>::type;
		// Output type.
		using out_type = typename std::tuple_element<1u,T>::type;
		// Build a random vector of input types, able to contain a few output types.
		std::uniform_int_distribution<unsigned> bd(0u,1u);
		std::vector<in_type> input_vector;
		using size_type = typename std::vector<in_type>::size_type;
		input_vector.resize(static_cast<size_type>((sizeof(out_type) * 10u) / sizeof(in_type) + 1u));
		std::generate(input_vector.begin(),input_vector.end(),[&bd]() -> in_type {
			in_type retval(0);
			for (unsigned i = 0u; i < static_cast<unsigned>(std::numeric_limits<in_type>::digits); ++i) {
				retval = static_cast<in_type>(retval + (in_type(bd(rng)) << i));
			}
			return retval;
		});
		// Boilerplate to convert the input vector and return value to a sequence of bits.
		std::vector<unsigned char> db_in, db_out;
		auto vec_to_bitset = [&db_in,&input_vector](unsigned ibits) {
			piranha_assert(ibits < static_cast<unsigned>(std::numeric_limits<in_type>::digits));
			db_in.clear();
			for (const auto &el: input_vector) {
				for (unsigned i = 0u; i < static_cast<unsigned>(std::numeric_limits<in_type>::digits) - ibits; ++i) {
					db_in.push_back((el & (in_type(1u) << i)) != 0);
				}
			}
			// Erase the top empty bits.
			while (db_in.size() != 0u && db_in.back() == 0u) {
				db_in.pop_back();
			}
		};
		auto ret_to_bitset = [&db_out](out_type r) {
			db_out.clear();
			for (unsigned i = 0u; i < static_cast<unsigned>(std::numeric_limits<out_type>::digits); ++i) {
				db_out.push_back((r & (out_type(1u) << i)) != 0);
			}
			// Erase the top empty bits.
			while (db_out.size() != 0u && db_out.back() == 0u) {
				db_out.pop_back();
			}
		};
		// Number of rets that can be read from the vector, ignoring ib bits.
		auto n_rets = [&input_vector](unsigned ib) -> unsigned {
			const unsigned tmp1 = static_cast<unsigned>(input_vector.size() * (static_cast<unsigned>(std::numeric_limits<in_type>::digits) - ib)),
				tmp2 = static_cast<unsigned>(std::numeric_limits<out_type>::digits);
			auto q = tmp1 / tmp2, r = tmp1 % tmp2;
			return (r == 0u) ? unsigned(q) : unsigned(q + 1u);
		};
		// Read the first int with different amount of ignored bits.
		// NOTE: the first int can alway be read.
		vec_to_bitset(0u);
		auto r = detail::read_uint<out_type>(&input_vector[0u],input_vector.size(),0u);
		ret_to_bitset(r);
		BOOST_CHECK(db_in.size() >= db_out.size());
		BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),db_in.begin()));
		vec_to_bitset(1u);
		r = detail::read_uint<out_type,1u>(&input_vector[0u],input_vector.size(),0u);
		ret_to_bitset(r);
		BOOST_CHECK(db_in.size() >= db_out.size());
		BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),db_in.begin()));
		vec_to_bitset(2u);
		r = detail::read_uint<out_type,2u>(&input_vector[0u],input_vector.size(),0u);
		ret_to_bitset(r);
		BOOST_CHECK(db_in.size() >= db_out.size());
		BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),db_in.begin()));
		vec_to_bitset(3u);
		r = detail::read_uint<out_type,3u>(&input_vector[0u],input_vector.size(),0u);
		ret_to_bitset(r);
		BOOST_CHECK(db_in.size() >= db_out.size());
		BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),db_in.begin()));
		vec_to_bitset(5u);
		r = detail::read_uint<out_type,5u>(&input_vector[0u],input_vector.size(),0u);
		ret_to_bitset(r);
		BOOST_CHECK(db_in.size() >= db_out.size());
		BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),db_in.begin()));
		vec_to_bitset(7u);
		r = detail::read_uint<out_type,7u>(&input_vector[0u],input_vector.size(),0u);
		ret_to_bitset(r);
		BOOST_CHECK(db_in.size() >= db_out.size());
		BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),db_in.begin()));
		// Second int.
		vec_to_bitset(0u);
		r = detail::read_uint<out_type,0u>(&input_vector[0u],input_vector.size(),1u);
		ret_to_bitset(r);
		BOOST_CHECK(db_in.size() >= db_out.size());
		BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),&db_in[0u] + std::numeric_limits<out_type>::digits));
		if (n_rets(1u) > 1u) {
			vec_to_bitset(1u);
			r = detail::read_uint<out_type,1u>(&input_vector[0u],input_vector.size(),1u);
			ret_to_bitset(r);
			BOOST_CHECK(db_in.size() >= db_out.size());
			BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),&db_in[0u] + std::numeric_limits<out_type>::digits));
		}
		if (n_rets(3u) > 1u) {
			vec_to_bitset(3u);
			r = detail::read_uint<out_type,3u>(&input_vector[0u],input_vector.size(),1u);
			ret_to_bitset(r);
			BOOST_CHECK(db_in.size() >= db_out.size());
			BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),&db_in[0u] + std::numeric_limits<out_type>::digits));
		}
		if (n_rets(5u) > 1u) {
			vec_to_bitset(5u);
			r = detail::read_uint<out_type,5u>(&input_vector[0u],input_vector.size(),1u);
			ret_to_bitset(r);
			BOOST_CHECK(db_in.size() >= db_out.size());
			BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),&db_in[0u] + std::numeric_limits<out_type>::digits));
		}
		if (n_rets(7u) > 1u) {
			vec_to_bitset(7u);
			r = detail::read_uint<out_type,7u>(&input_vector[0u],input_vector.size(),1u);
			ret_to_bitset(r);
			BOOST_CHECK(db_in.size() >= db_out.size());
			BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),&db_in[0u] + std::numeric_limits<out_type>::digits));
		}
		// Third int.
		vec_to_bitset(0u);
		r = detail::read_uint<out_type,0u>(&input_vector[0u],input_vector.size(),2u);
		ret_to_bitset(r);
		BOOST_CHECK(db_in.size() >= db_out.size());
		BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),&db_in[0u] + 2 * std::numeric_limits<out_type>::digits));
		if (n_rets(1u) > 2u) {
			vec_to_bitset(1u);
			r = detail::read_uint<out_type,1u>(&input_vector[0u],input_vector.size(),2u);
			ret_to_bitset(r);
			BOOST_CHECK(db_in.size() >= db_out.size());
			BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),&db_in[0u] + 2 * std::numeric_limits<out_type>::digits));
		}
		if (n_rets(3u) > 2u) {
			vec_to_bitset(3u);
			r = detail::read_uint<out_type,3u>(&input_vector[0u],input_vector.size(),2u);
			ret_to_bitset(r);
			BOOST_CHECK(db_in.size() >= db_out.size());
			BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),&db_in[0u] + 2 * std::numeric_limits<out_type>::digits));
		}
		if (n_rets(5u) > 2u) {
			vec_to_bitset(5u);
			r = detail::read_uint<out_type,5u>(&input_vector[0u],input_vector.size(),2u);
			ret_to_bitset(r);
			BOOST_CHECK(db_in.size() >= db_out.size());
			BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),&db_in[0u] + 2 * std::numeric_limits<out_type>::digits));
		}
		if (n_rets(7u) > 2u) {
			vec_to_bitset(7u);
			r = detail::read_uint<out_type,7u>(&input_vector[0u],input_vector.size(),2u);
			ret_to_bitset(r);
			BOOST_CHECK(db_in.size() >= db_out.size());
			BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),&db_in[0u] + 2 * std::numeric_limits<out_type>::digits));
		}
		// Fifth int.
		vec_to_bitset(0u);
		r = detail::read_uint<out_type,0u>(&input_vector[0u],input_vector.size(),4u);
		ret_to_bitset(r);
		BOOST_CHECK(db_in.size() >= db_out.size());
		BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),&db_in[0u] + 4 * std::numeric_limits<out_type>::digits));
		if (n_rets(1u) > 4u) {
			vec_to_bitset(1u);
			r = detail::read_uint<out_type,1u>(&input_vector[0u],input_vector.size(),4u);
			ret_to_bitset(r);
			BOOST_CHECK(db_in.size() >= db_out.size());
			BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),&db_in[0u] + 4 * std::numeric_limits<out_type>::digits));
		}
		if (n_rets(3u) > 4u) {
			vec_to_bitset(3u);
			r = detail::read_uint<out_type,3u>(&input_vector[0u],input_vector.size(),4u);
			ret_to_bitset(r);
			BOOST_CHECK(db_in.size() >= db_out.size());
			BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),&db_in[0u] + 4 * std::numeric_limits<out_type>::digits));
		}
		if (n_rets(5u) > 4u) {
			vec_to_bitset(5u);
			r = detail::read_uint<out_type,5u>(&input_vector[0u],input_vector.size(),4u);
			ret_to_bitset(r);
			BOOST_CHECK(db_in.size() >= db_out.size());
			BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),&db_in[0u] + 4 * std::numeric_limits<out_type>::digits));
		}
		if (n_rets(7u) > 4u) {
			vec_to_bitset(7u);
			r = detail::read_uint<out_type,7u>(&input_vector[0u],input_vector.size(),4u);
			ret_to_bitset(r);
			BOOST_CHECK(db_in.size() >= db_out.size());
			BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),&db_in[0u] + 4 * std::numeric_limits<out_type>::digits));
		}
		// Seventh int.
		vec_to_bitset(0u);
		r = detail::read_uint<out_type,0u>(&input_vector[0u],input_vector.size(),6u);
		ret_to_bitset(r);
		BOOST_CHECK(db_in.size() >= db_out.size());
		BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),&db_in[0u] + 6 * std::numeric_limits<out_type>::digits));
		if (n_rets(1u) > 6u) {
			vec_to_bitset(1u);
			r = detail::read_uint<out_type,1u>(&input_vector[0u],input_vector.size(),6u);
			ret_to_bitset(r);
			BOOST_CHECK(db_in.size() >= db_out.size());
			BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),&db_in[0u] + 6 * std::numeric_limits<out_type>::digits));
		}
		if (n_rets(3u) > 6u) {
			vec_to_bitset(3u);
			r = detail::read_uint<out_type,3u>(&input_vector[0u],input_vector.size(),6u);
			ret_to_bitset(r);
			BOOST_CHECK(db_in.size() >= db_out.size());
			BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),&db_in[0u] + 6 * std::numeric_limits<out_type>::digits));
		}
		if (n_rets(5u) > 6u) {
			vec_to_bitset(5u);
			r = detail::read_uint<out_type,5u>(&input_vector[0u],input_vector.size(),6u);
			ret_to_bitset(r);
			BOOST_CHECK(db_in.size() >= db_out.size());
			BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),&db_in[0u] + 6 * std::numeric_limits<out_type>::digits));
		}
		if (n_rets(7u) > 6u) {
			vec_to_bitset(7u);
			r = detail::read_uint<out_type,7u>(&input_vector[0u],input_vector.size(),6u);
			ret_to_bitset(r);
			BOOST_CHECK(db_in.size() >= db_out.size());
			BOOST_CHECK(std::equal(db_out.begin(),db_out.end(),&db_in[0u] + 6 * std::numeric_limits<out_type>::digits));
		}
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_read_uint_test)
{
	boost::mpl::for_each<ru_test_types>(read_uint_tester());
}

struct tt_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		BOOST_CHECK(is_cf<int_type>::value);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_tt_test)
{
	boost::mpl::for_each<size_types>(tt_tester());
}

struct ctb_tester
{
	template <typename T>
	void operator()(const T &, typename std::enable_if<std::is_unsigned<T>::value>::type * = nullptr)
	{
		const unsigned nbits = static_cast<unsigned>(std::numeric_limits<T>::digits);
		BOOST_CHECK_EQUAL(detail::clear_top_bits(T(0),0u),T(0));
		BOOST_CHECK_EQUAL(detail::clear_top_bits(T(1),1u),T(1));
		BOOST_CHECK_EQUAL(detail::clear_top_bits(T(2),2u),T(2));
		BOOST_CHECK_EQUAL(detail::clear_top_bits(static_cast<T>(T(1) << (nbits - 1u)),1u),T(0));
		BOOST_CHECK_EQUAL(detail::clear_top_bits(static_cast<T>(T(1) << (nbits - 2u)),1u),static_cast<T>(T(1) << (nbits - 2u)));
		BOOST_CHECK_EQUAL(detail::clear_top_bits(static_cast<T>(T(1) << (nbits - 2u)),2u),T(0));
		BOOST_CHECK_EQUAL(detail::clear_top_bits(static_cast<T>(67),nbits - 1u),T(1));
	}
	template <typename T>
	void operator()(const T &, typename std::enable_if<std::is_signed<T>::value>::type * = nullptr)
	{}
};

BOOST_AUTO_TEST_CASE(mp_integer_clear_top_bits_test)
{
	boost::mpl::for_each<integral_types>(ctb_tester());
}

struct static_hash_tester
{
	template <typename U>
	struct runner
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef detail::static_integer<T::value> int_type1;
			typedef detail::static_integer<U::value> int_type2;
			using lt1 = typename int_type1::limb_t;
			using lt2 = typename int_type2::limb_t;
			const auto lbits1 = int_type1::limb_bits;
			const auto lbits2 = int_type2::limb_bits;
			BOOST_CHECK_EQUAL(int_type1{}.hash(),0u);
			BOOST_CHECK_EQUAL(int_type1{}.hash(),int_type2{}.hash());
			BOOST_CHECK_EQUAL(int_type1{1}.hash(),int_type2{1}.hash());
			BOOST_CHECK_EQUAL(int_type1{-1}.hash(),int_type2{-1}.hash());
			BOOST_CHECK_EQUAL(int_type1{5}.hash(),int_type2{5}.hash());
			BOOST_CHECK_EQUAL(int_type1{-5}.hash(),int_type2{-5}.hash());
			// Random tests.
			std::uniform_int_distribution<int> udist(0,1);
			for (int i = 0; i < ntries; ++i) {
				// Build randomly two identical integers wide as much as the narrowest of the two int types,
				// and compare their hashes.
				int_type1 a(1);
				int_type2 b(1);
				while (a.m_limbs[1u] < (lt1(1) << (lbits1 - 1u)) && b.m_limbs[1u] < (lt2(1) << (lbits2 - 1u))) {
					int tmp = udist(rng);
					a.m_limbs[0u] = static_cast<lt1>(a.m_limbs[0u] + lt1(tmp));
					b.m_limbs[0u] = static_cast<lt2>(b.m_limbs[0u] + lt2(tmp));
					a.lshift1();
					b.lshift1();
				}
				if (udist(rng)) {
					a.negate();
					b.negate();
				}
				BOOST_CHECK_EQUAL(a.hash(),b.hash());
			}
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_static_hash_test)
{
	boost::mpl::for_each<size_types>(static_hash_tester());
}

struct hash_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		BOOST_CHECK(is_hashable<int_type>::value);
		BOOST_CHECK_EQUAL(int_type{}.hash(),0u);
		{
		int_type n;
		n.promote();
		BOOST_CHECK_EQUAL(n.hash(),0u);
		}
		{
		int_type n(1), m(n);
		n.promote();
		BOOST_CHECK_EQUAL(n.hash(),m.hash());
		}
		{
		int_type n(-1), m(n);
		n.promote();
		BOOST_CHECK_EQUAL(n.hash(),m.hash());
		}
		{
		int_type n(2), m(n);
		n.promote();
		BOOST_CHECK_EQUAL(n.hash(),m.hash());
		}
		{
		int_type n(-2), m(n);
		n.promote();
		BOOST_CHECK_EQUAL(n.hash(),m.hash());
		}
		{
		int_type n(-100), m(n);
		n.promote();
		BOOST_CHECK_EQUAL(n.hash(),m.hash());
		}
		// Random tests.
		std::uniform_int_distribution<int> ud(std::numeric_limits<int>::lowest(),std::numeric_limits<int>::max());
		std::uniform_int_distribution<int> promote_dist(0,1);
		for (int i = 0; i < ntries; ++i) {
			auto tmp = ud(rng);
			int_type n(tmp), m(n);
			if (promote_dist(rng) && m.is_static()) {
				m.promote();
			}
			BOOST_CHECK_EQUAL(n.hash(),m.hash());
			BOOST_CHECK_EQUAL(n.hash(),std::hash<int_type>()(m));
		}
		// Try squaring as well for more range.
		for (int i = 0; i < ntries; ++i) {
			auto tmp = ud(rng);
			int_type n(int_type{tmp} * tmp), m(n);
			if (promote_dist(rng)) {
				n.negate();
				m.negate();
			}
			if (promote_dist(rng) && m.is_static()) {
				m.promote();
			}
			BOOST_CHECK_EQUAL(n.hash(),m.hash());
		}
		std::uniform_int_distribution<long long> udll(std::numeric_limits<long long>::lowest(),std::numeric_limits<long long>::max());
		for (int i = 0; i < ntries; ++i) {
			auto tmp = udll(rng);
			int_type n(tmp), m(n);
			if (promote_dist(rng) && m.is_static()) {
				m.promote();
			}
			BOOST_CHECK_EQUAL(n.hash(),m.hash());
			BOOST_CHECK_EQUAL(n.hash(),std::hash<int_type>()(m));
		}
		for (int i = 0; i < ntries; ++i) {
			auto tmp = udll(rng);
			int_type n(int_type{tmp} * tmp), m(n);
			if (promote_dist(rng)) {
				n.negate();
				m.negate();
			}
			if (promote_dist(rng) && m.is_static()) {
				m.promote();
			}
			BOOST_CHECK_EQUAL(n.hash(),m.hash());
		}
		std::uniform_int_distribution<long> udl(std::numeric_limits<long>::lowest(),std::numeric_limits<long>::max());
		for (int i = 0; i < ntries; ++i) {
			auto tmp = udl(rng);
			int_type n(tmp), m(n);
			if (promote_dist(rng) && m.is_static()) {
				m.promote();
			}
			BOOST_CHECK_EQUAL(n.hash(),m.hash());
			BOOST_CHECK_EQUAL(n.hash(),std::hash<int_type>()(m));
		}
		for (int i = 0; i < ntries; ++i) {
			auto tmp = udl(rng);
			int_type n(int_type{tmp} * tmp), m(n);
			if (promote_dist(rng)) {
				n.negate();
				m.negate();
			}
			if (promote_dist(rng) && m.is_static()) {
				m.promote();
			}
			BOOST_CHECK_EQUAL(n.hash(),m.hash());
		}
		std::uniform_int_distribution<unsigned long> udul(std::numeric_limits<unsigned long>::lowest(),std::numeric_limits<unsigned long>::max());
		for (int i = 0; i < ntries; ++i) {
			auto tmp = udul(rng);
			int_type n(tmp), m(n);
			if (promote_dist(rng) && m.is_static()) {
				m.promote();
			}
			BOOST_CHECK_EQUAL(n.hash(),m.hash());
			BOOST_CHECK_EQUAL(n.hash(),std::hash<int_type>()(m));
		}
		for (int i = 0; i < ntries; ++i) {
			auto tmp = udul(rng);
			int_type n(int_type{tmp} * tmp), m(n);
			if (promote_dist(rng)) {
				n.negate();
				m.negate();
			}
			if (promote_dist(rng) && m.is_static()) {
				m.promote();
			}
			BOOST_CHECK_EQUAL(n.hash(),m.hash());
		}
		std::uniform_int_distribution<unsigned long long> udull(std::numeric_limits<unsigned long long>::lowest(),std::numeric_limits<unsigned long long>::max());
		for (int i = 0; i < ntries; ++i) {
			auto tmp = udull(rng);
			int_type n(tmp), m(n);
			if (promote_dist(rng) && m.is_static()) {
				m.promote();
			}
			BOOST_CHECK_EQUAL(n.hash(),m.hash());
			BOOST_CHECK_EQUAL(n.hash(),std::hash<int_type>()(m));
		}
		for (int i = 0; i < ntries; ++i) {
			auto tmp = udull(rng);
			int_type n(int_type{tmp} * tmp), m(n);
			if (promote_dist(rng)) {
				n.negate();
				m.negate();
			}
			if (promote_dist(rng) && m.is_static()) {
				m.promote();
			}
			BOOST_CHECK_EQUAL(n.hash(),m.hash());
		}
		// Try some extremals.
		{
		int_type n(std::numeric_limits<long long>::max()), m(n);
		if (n.is_static()) {
			n.promote();
		}
		BOOST_CHECK_EQUAL(n.hash(),m.hash());
		}
		{
		int_type n(std::numeric_limits<long long>::lowest()), m(n);
		if (n.is_static()) {
			n.promote();
		}
		BOOST_CHECK_EQUAL(n.hash(),m.hash());
		}
		{
		int_type n(std::numeric_limits<double>::max()), m(n);
		if (n.is_static()) {
			n.promote();
		}
		BOOST_CHECK_EQUAL(n.hash(),m.hash());
		}
		{
		int_type n(std::numeric_limits<double>::lowest()), m(n);
		if (n.is_static()) {
			n.promote();
		}
		BOOST_CHECK_EQUAL(n.hash(),m.hash());
		}
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_hash_test)
{
	boost::mpl::for_each<size_types>(hash_tester());
}

struct next_prime_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		int_type n;
		BOOST_CHECK_EQUAL(n.nextprime(),2);
		n = 2;
		BOOST_CHECK_EQUAL(n.nextprime(),3);
		n = 3;
		BOOST_CHECK_EQUAL(n.nextprime(),5);
		n = 7901;
		BOOST_CHECK_EQUAL(n.nextprime(),7907);
		n = -1;
		BOOST_CHECK_THROW(n.nextprime(),std::invalid_argument);
		// Random tests.
		std::uniform_int_distribution<int> ud(std::numeric_limits<int>::lowest(),std::numeric_limits<int>::max());
		std::uniform_int_distribution<int> promote_dist(0,1);
		mpz_raii m;
		for (int i = 0; i < ntries; ++i) {
			auto tmp = ud(rng);
			n = tmp;
			if (promote_dist(rng) && n.is_static()) {
				n.promote();
			}
			if (tmp < 0) {
				BOOST_CHECK_THROW(n.nextprime(),std::invalid_argument);
				continue;
			}
			::mpz_set_si(&m.m_mpz,static_cast<long>(tmp));
			::mpz_nextprime(&m.m_mpz,&m.m_mpz);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n.nextprime()),mpz_lexcast(m));
		}
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_next_prime_test)
{
	boost::mpl::for_each<size_types>(next_prime_tester());
}

struct probab_prime_p_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		int_type n;
		BOOST_CHECK(n.probab_prime_p() == 0);
		n = 1;
		BOOST_CHECK(n.probab_prime_p() == 0);
		n = 2;
		BOOST_CHECK(n.probab_prime_p() != 0);
		n = 3;
		BOOST_CHECK(n.probab_prime_p() != 0);
		n = 5;
		BOOST_CHECK(n.probab_prime_p() != 0);
		n = 11;
		BOOST_CHECK(n.probab_prime_p() != 0);
		n = 16;
		BOOST_CHECK(n.probab_prime_p() != 2);
		n = 7901;
		BOOST_CHECK(n.probab_prime_p() != 0);
		n = 7907;
		BOOST_CHECK(n.probab_prime_p(5) != 0);
		n = -1;
		BOOST_CHECK_THROW(n.probab_prime_p(),std::invalid_argument);
		n = 5;
		BOOST_CHECK_THROW(n.probab_prime_p(0),std::invalid_argument);
		BOOST_CHECK_THROW(n.probab_prime_p(-1),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_probab_prime_p_test)
{
	boost::mpl::for_each<size_types>(probab_prime_p_tester());
}

struct integer_sqrt_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		int_type n;
		BOOST_CHECK(n.sqrt() == 0);
		n = 1;
		BOOST_CHECK(n.sqrt() == 1);
		n = 2;
		BOOST_CHECK(n.sqrt() == 1);
		n = 3;
		BOOST_CHECK(n.sqrt() == 1);
		n = 4;
		BOOST_CHECK(n.sqrt() == 2);
		n = 5;
		BOOST_CHECK(n.sqrt() == 2);
		// Random tests.
		std::uniform_int_distribution<int> ud(std::numeric_limits<int>::lowest(),std::numeric_limits<int>::max());
		std::uniform_int_distribution<int> promote_dist(0,1);
		mpz_raii m;
		for (int i = 0; i < ntries; ++i) {
			auto tmp = ud(rng);
			n = tmp;
			if (promote_dist(rng) && n.is_static()) {
				n.promote();
			}
			if (tmp < 0) {
				BOOST_CHECK_THROW(n.sqrt(),std::invalid_argument);
				continue;
			}
			::mpz_set_si(&m.m_mpz,static_cast<long>(tmp));
			::mpz_sqrt(&m.m_mpz,&m.m_mpz);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n.sqrt()),mpz_lexcast(m));
		}
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_integer_sqrt_test)
{
	boost::mpl::for_each<size_types>(integer_sqrt_tester());
}

struct factorial_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		int_type n;
		BOOST_CHECK(n.factorial() == 1);
		n = 1;
		BOOST_CHECK(n.factorial() == 1);
		n = 2;
		BOOST_CHECK(n.factorial() == 2);
		n = 3;
		BOOST_CHECK(n.factorial() == 6);
		n = 4;
		BOOST_CHECK(n.factorial() == 24);
		n = 5;
		BOOST_CHECK(n.factorial() == 24 * 5);
		// Random tests.
		std::uniform_int_distribution<int> ud(-1000,1000);
		std::uniform_int_distribution<int> promote_dist(0,1);
		mpz_raii m;
		for (int i = 0; i < ntries; ++i) {
			auto tmp = ud(rng);
			n = tmp;
			if (promote_dist(rng) && n.is_static()) {
				n.promote();
			}
			if (tmp < 0) {
				BOOST_CHECK_THROW(n.factorial(),std::invalid_argument);
				continue;
			}
			::mpz_set_si(&m.m_mpz,static_cast<long>(tmp));
			::mpz_fac_ui(&m.m_mpz,static_cast<unsigned long>(tmp));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n.factorial()),mpz_lexcast(m));
		}
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_factorial_test)
{
	boost::mpl::for_each<size_types>(factorial_tester());
}

struct binomial_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		BOOST_CHECK((has_binomial<int_type,int_type>::value));
		BOOST_CHECK((has_binomial<int_type,int>::value));
		BOOST_CHECK((has_binomial<int_type,unsigned>::value));
		BOOST_CHECK((has_binomial<int_type,long>::value));
		BOOST_CHECK((has_binomial<int_type,char>::value));
		int_type n;
		BOOST_CHECK(n.binomial(0) == 1);
		BOOST_CHECK(n.binomial(1) == 0);
		n = 1;
		BOOST_CHECK(n.binomial(1) == 1);
		n = 5;
		BOOST_CHECK(n.binomial(3) == 10);
		n = -5;
		BOOST_CHECK(n.binomial(int_type(4)) == 70);
		// Random tests.
		std::uniform_int_distribution<int> ud(-1000,1000);
		std::uniform_int_distribution<int> promote_dist(0,1);
		mpz_raii m;
		for (int i = 0; i < ntries; ++i) {
			auto tmp1 = ud(rng), tmp2 = ud(rng);
			n = tmp1;
			if (promote_dist(rng) && n.is_static()) {
				n.promote();
			}
			if (tmp2 < 0) {
				BOOST_CHECK_THROW(n.binomial(tmp2),std::invalid_argument);
				continue;
			}
			::mpz_set_si(&m.m_mpz,static_cast<long>(tmp1));
			::mpz_bin_ui(&m.m_mpz,&m.m_mpz,static_cast<unsigned long>(tmp2));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n.binomial(tmp2)),mpz_lexcast(m));
			BOOST_CHECK_EQUAL(n.binomial(tmp2),n.binomial(int_type(tmp2)));
			BOOST_CHECK_EQUAL(n.binomial(long(tmp2)),n.binomial(int_type(tmp2)));
			BOOST_CHECK_EQUAL(n.binomial((long long)(tmp2)),n.binomial(int_type(tmp2)));
			BOOST_CHECK_EQUAL(n.binomial((unsigned long)(tmp2)),n.binomial(int_type(tmp2)));
			BOOST_CHECK_EQUAL(n.binomial((unsigned long long)(tmp2)),n.binomial(int_type(tmp2)));
		}
		BOOST_CHECK_THROW(n.binomial(std::numeric_limits<unsigned long>::max() + int_type(1)),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_binomial_test)
{
	boost::mpl::for_each<size_types>(binomial_tester());
	// Check the ints.
	using int_type = mp_integer<>;
	BOOST_CHECK((has_binomial<int,int>::value));
	BOOST_CHECK_EQUAL(math::binomial(4,2),math::binomial(int_type(4),2));
	BOOST_CHECK((has_binomial<char,unsigned>::value));
	BOOST_CHECK_EQUAL(math::binomial(char(4),2u),math::binomial(int_type(4),2));
	BOOST_CHECK((has_binomial<long long,int>::value));
	BOOST_CHECK_EQUAL(math::binomial(7ll,4),math::binomial(int_type(7),4));
	BOOST_CHECK((std::is_same<decltype(math::binomial(7ll,4)),int_type>::value));
	BOOST_CHECK_EQUAL(math::binomial(-7ll,4u),math::binomial(int_type(-7),4));
}

struct sin_cos_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		BOOST_CHECK(has_sine<int_type>::value);
		BOOST_CHECK(has_cosine<int_type>::value);
		int_type n;
		BOOST_CHECK_EQUAL(math::sin(n),0);
		BOOST_CHECK_EQUAL(math::cos(n),1);
		n = 1;
		BOOST_CHECK_THROW(math::sin(n),std::invalid_argument);
		BOOST_CHECK_THROW(math::cos(n),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_sin_cos_test)
{
	boost::mpl::for_each<size_types>(sin_cos_tester());
}

struct partial_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		BOOST_CHECK(is_differentiable<int_type>::value);
		int_type n;
		BOOST_CHECK_EQUAL(math::partial(n,""),0);
		n = 5;
		BOOST_CHECK_EQUAL(math::partial(n,"abc"),0);
		n = -5;
		BOOST_CHECK_EQUAL(math::partial(n,"def"),0);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_partial_test)
{
	boost::mpl::for_each<size_types>(partial_tester());
}

struct evaluate_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		using d_type = std::unordered_map<std::string,double>;
		BOOST_CHECK((is_evaluable<int_type,int>::value));
		BOOST_CHECK((is_evaluable<int_type,int_type>::value));
		BOOST_CHECK((is_evaluable<int_type,double>::value));
		int_type n;
		BOOST_CHECK_EQUAL(math::evaluate(n,d_type{}),0);
		BOOST_CHECK_EQUAL(math::evaluate(n,d_type{{"foo",5.}}),0);
		n = -1;
		BOOST_CHECK_EQUAL(math::evaluate(n,d_type{{"foo",6.}}),-1);
		n = 101;
		BOOST_CHECK_EQUAL(math::evaluate(n,d_type{{"bar",6.},{"baz",.7}}),101);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_evaluate_test)
{
	boost::mpl::for_each<size_types>(evaluate_tester());
}

struct subs_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		BOOST_CHECK((has_subs<int_type,int_type>::value));
		BOOST_CHECK((has_subs<int_type,double>::value));
		BOOST_CHECK((has_subs<int_type,float>::value));
		int_type n;
		BOOST_CHECK_EQUAL(math::subs(n,"foo",5),0);
		n = -6;
		BOOST_CHECK_EQUAL(math::subs(n,"bar",0),-6);
		n = 1034;
		BOOST_CHECK_EQUAL(math::subs(n,"baz","std::string"),1034);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_subs_test)
{
	boost::mpl::for_each<size_types>(subs_tester());
}

struct integrable_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		BOOST_CHECK(!is_integrable<int_type>::value);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_integrable_test)
{
	boost::mpl::for_each<size_types>(integrable_tester());
}

BOOST_AUTO_TEST_CASE(mp_integer_literal_test)
{
	auto n0 = 12345_z;
	BOOST_CHECK((std::is_same<mp_integer<>,decltype(n0)>::value));
	BOOST_CHECK_EQUAL(n0,12345);
	n0 = -456_z;
	BOOST_CHECK_EQUAL(n0,-456l);
	BOOST_CHECK_THROW((n0 = -1234.5_z),std::invalid_argument);
	BOOST_CHECK_EQUAL(n0,-456l);
}
