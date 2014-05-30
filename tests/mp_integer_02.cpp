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
#include <type_traits>
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
		auto &st_a = get_m(a).st, &st_b = get_m(b).st, &st_c = get_m(c).st;
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
		auto &st_a = get_m(a).st, &st_b = get_m(b).st, &st_c = get_m(c).st;
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
		get_m(a).st.set_bit(static_cast<limb_t>(get_m(a).st.limb_bits * 2u - 1u));
		::mpz_setbit(&m_a.m_mpz,static_cast< ::mp_bitcnt_t>(get_m(a).st.limb_bits * 2u - 1u));
		a.multiply_accumulate(a,b);
		::mpz_addmul(&m_a.m_mpz,&m_a.m_mpz,&m_b.m_mpz);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(m_a));
		}
		{
		// Promotion bug.
		using limb_t = typename detail::integer_union<T::value>::s_storage::limb_t;
		int_type a;
		mpz_raii m_a;
		get_m(a).st.set_bit(static_cast<limb_t>(get_m(a).st.limb_bits * 2u - 1u));
		::mpz_setbit(&m_a.m_mpz,static_cast< ::mp_bitcnt_t>(get_m(a).st.limb_bits * 2u - 1u));
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
			BOOST_CHECK_THROW(n1 /= T(0),std::invalid_argument);
			// Random testing
			std::uniform_real_distribution<T> urd1(T(0),std::numeric_limits<T>::max()), urd2(std::numeric_limits<T>::lowest(),T(0));
			for (int i = 0; i < ntries / 100; ++i) {
				const T tmp1 = urd1(rng);
				int_type n(tmp1);
				n /= tmp1;
				BOOST_CHECK(boost::lexical_cast<std::string>(n) == "0" || boost::lexical_cast<std::string>(n) == "1");
				const T tmp2 = urd2(rng);
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
			// NOTE: limit the number of times we run this test, as the conversion from int to float
			// is slow as the random values are taken from the entire float range and thus use a lot of digits.
			for (int i = 0; i < ntries / 100; ++i) {
				T tmp1(1), tmp2 = urd1(rng);
				tmp1 /= int_type{tmp2};
				BOOST_CHECK_EQUAL(tmp1,T(1)/static_cast<T>(int_type{tmp2}));
				tmp1 = T(1);
				tmp2 = urd2(rng);
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
			BOOST_CHECK_EQUAL(n.pow(0),1);
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

BOOST_AUTO_TEST_CASE(mp_integer_pow_test)
{
	boost::mpl::for_each<size_types>(int_pow_tester());
}
