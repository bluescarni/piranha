/* Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)

This file is part of the Piranha library.

The Piranha library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The Piranha library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the Piranha library.  If not,
see https://www.gnu.org/licenses/. */

#include "../src/mp_integer.hpp"

#define BOOST_TEST_MODULE mp_integer_03_test
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

#include "../src/binomial.hpp"
#include "../src/config.hpp"
#include "../src/environment.hpp"
#include "../src/exceptions.hpp"
#include "../src/math.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

using size_types = boost::mpl::vector<std::integral_constant<int,0>,std::integral_constant<int,8>,std::integral_constant<int,16>,
	std::integral_constant<int,32>
#if defined(PIRANHA_UINT128_T)
	,std::integral_constant<int,64>
#endif
>;

static std::mt19937 rng;
constexpr int ntries = 1000;

using mpz_raii = detail::mpz_raii;

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

struct static_lshift_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using int_type = detail::static_integer<T::value>;
		using limb_t = typename int_type::limb_t;
		constexpr auto limb_bits = int_type::limb_bits;
		int_type n(0);
		auto ret = n.lshift(1u);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n,int_type(0));
		n.lshift(limb_bits);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n,int_type(0));
		n.lshift(2u * limb_bits);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n,int_type(0));
		n.lshift(2u * limb_bits + 1u);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n,int_type(0));
		n = int_type(1);
		ret = n.lshift(0u);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n,int_type(1));
		n = int_type(-1);
		ret = n.lshift(0u);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n,int_type(-1));
		n = int_type(1);
		ret = n.lshift(1u);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n,int_type(2));
		n = int_type(-1);
		ret = n.lshift(1u);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n,int_type(-2));
		n = int_type(3);
		ret = n.lshift(1u);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n,int_type(6));
		n = int_type(-3);
		ret = n.lshift(1u);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n,int_type(-6));
		n = int_type(1);
		ret = n.lshift(limb_bits - 1u);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n.m_limbs[0u],limb_t(1) << (limb_bits - 1u));
		BOOST_CHECK_EQUAL(n.m_limbs[1u],0u);
		n = int_type(1);
		ret = n.lshift(limb_bits - 2u);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n.m_limbs[0u],limb_t(1) << (limb_bits - 2u));
		BOOST_CHECK_EQUAL(n.m_limbs[1u],0u);
		n = int_type(1);
		ret = n.lshift(2u * limb_bits);
		BOOST_CHECK_EQUAL(ret,1);
		BOOST_CHECK_EQUAL(n,int_type(1));
		ret = n.lshift(limb_bits);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n.m_limbs[0u],0u);
		BOOST_CHECK_EQUAL(n.m_limbs[1u],1u);
		n = int_type(-1);
		ret = n.lshift(limb_bits);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n.m_limbs[0u],0u);
		BOOST_CHECK_EQUAL(n.m_limbs[1u],1u);
		n = int_type(1);
		ret = n.lshift(limb_bits + 1u);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n.m_limbs[0u],0u);
		BOOST_CHECK_EQUAL(n.m_limbs[1u],2u);
		n = int_type(-1);
		ret = n.lshift(limb_bits + 1u);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n.m_limbs[0u],0u);
		BOOST_CHECK_EQUAL(n.m_limbs[1u],2u);
		n = int_type(3);
		ret = n.lshift(limb_bits + 2u);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n.m_limbs[0u],0u);
		BOOST_CHECK_EQUAL(n.m_limbs[1u],12u);
		n = int_type(-3);
		ret = n.lshift(limb_bits + 2u);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n.m_limbs[0u],0u);
		BOOST_CHECK_EQUAL(n.m_limbs[1u],12u);
		n = int_type(1);
		ret = n.lshift(limb_bits * 2u - 1u);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n.m_limbs[0u],0u);
		BOOST_CHECK_EQUAL(n.m_limbs[1u],limb_t(1) << (limb_bits - 1u));
		n = int_type(-1);
		ret = n.lshift(limb_bits * 2u - 1u);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n.m_limbs[0u],0u);
		BOOST_CHECK_EQUAL(n.m_limbs[1u],limb_t(1) << (limb_bits - 1u));
		n = int_type(1);
		ret = n.lshift(limb_bits);
		ret = n.lshift(1u);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n.m_limbs[0u],0u);
		BOOST_CHECK_EQUAL(n.m_limbs[1u],2u);
		ret = n.lshift(limb_bits - 1u);
		BOOST_CHECK_EQUAL(ret,1);
		BOOST_CHECK_EQUAL(n.m_limbs[0u],0u);
		BOOST_CHECK_EQUAL(n.m_limbs[1u],2u);
		n = int_type(-1);
		ret = n.lshift(limb_bits);
		ret = n.lshift(1u);
		BOOST_CHECK_EQUAL(ret,0);
		BOOST_CHECK_EQUAL(n.m_limbs[0u],0u);
		BOOST_CHECK_EQUAL(n.m_limbs[1u],2u);
		ret = n.lshift(limb_bits - 1u);
		BOOST_CHECK_EQUAL(ret,1);
		BOOST_CHECK_EQUAL(n.m_limbs[0u],0u);
		BOOST_CHECK_EQUAL(n.m_limbs[1u],2u);
		n = int_type(1);
		ret = n.lshift(limb_bits);
		ret = n.lshift(limb_bits);
		BOOST_CHECK_EQUAL(ret,1);
		BOOST_CHECK_EQUAL(n.m_limbs[0u],0u);
		BOOST_CHECK_EQUAL(n.m_limbs[1u],1u);
		n = int_type(-1);
		ret = n.lshift(limb_bits);
		ret = n.lshift(limb_bits);
		BOOST_CHECK_EQUAL(ret,1);
		BOOST_CHECK_EQUAL(n.m_limbs[0u],0u);
		BOOST_CHECK_EQUAL(n.m_limbs[1u],1u);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_static_integer_lshift_test)
{
	environment env;
	boost::mpl::for_each<size_types>(static_lshift_tester());
}

struct lshift_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using int_type = mp_integer<T::value>;
		using limb_t = typename detail::static_integer<T::value>::limb_t;
		constexpr auto limb_bits = detail::static_integer<T::value>::limb_bits;
		// Test the type traits.
		BOOST_CHECK((has_left_shift<int_type>::value));
		BOOST_CHECK((has_left_shift<int_type,int>::value));
		BOOST_CHECK((has_left_shift<int_type,short>::value));
		BOOST_CHECK((has_left_shift<long,int_type>::value));
		BOOST_CHECK((has_left_shift<short,int_type>::value));
		BOOST_CHECK((!has_left_shift<int_type,std::string>::value));
		BOOST_CHECK((!has_left_shift<int_type,double>::value));
		BOOST_CHECK((!has_left_shift<double,int>::value));
		BOOST_CHECK((has_left_shift_in_place<int_type>::value));
		BOOST_CHECK((has_left_shift_in_place<int_type,int>::value));
		BOOST_CHECK((has_left_shift_in_place<int_type,short>::value));
		BOOST_CHECK((has_left_shift_in_place<long,int_type>::value));
		BOOST_CHECK((has_left_shift_in_place<short,int_type>::value));
		BOOST_CHECK((!has_left_shift_in_place<double,int_type>::value));
		BOOST_CHECK((!has_left_shift_in_place<const short,int_type>::value));
		BOOST_CHECK((!has_left_shift_in_place<std::string,int_type>::value));
		// Random testing.
		std::uniform_int_distribution<int> int_dist(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
		std::uniform_int_distribution<limb_t> sdist(limb_t(0u),limb_t(limb_bits * 2u));
		for (int i = 0; i < ntries; ++i) {
			const auto int_n = int_dist(rng);
			int_type n(int_n);
			auto s = sdist(rng);
			auto ns = n << s;
			BOOST_CHECK_EQUAL(ns,n * int_type(2).pow(s));
			auto ns2 = n << int_type(s);
			BOOST_CHECK_EQUAL(ns2,ns);
			n <<= s;
			BOOST_CHECK_EQUAL(ns,n);
			BOOST_CHECK_EQUAL(int_n << int_type(s),ns);
		}
		// Throwing conditions.
		BOOST_CHECK_THROW(int_type{1} << (int_type(std::numeric_limits< ::mp_bitcnt_t>::max()) + 1),std::invalid_argument);
		BOOST_CHECK_THROW(int_type{1} << int_type(-1),std::invalid_argument);
		BOOST_CHECK_THROW(int_type{1} << -1,std::invalid_argument);
		// A couple of tests with C++ integral on the left.
		BOOST_CHECK_EQUAL(1 << int_type(1),2);
		BOOST_CHECK_THROW(1 << int_type(-1),std::invalid_argument);
		unsigned n = 1;
		BOOST_CHECK_THROW(n <<= (int_type(std::numeric_limits<unsigned>::digits) + 10),std::overflow_error);
		BOOST_CHECK_EQUAL(n,1u);
		n <<= int_type(1);
		BOOST_CHECK_EQUAL(n,2u);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_lshift_test)
{
	boost::mpl::for_each<size_types>(lshift_tester());
}

struct static_rshift_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using int_type = detail::static_integer<T::value>;
		using limb_t = typename int_type::limb_t;
		constexpr auto limb_bits = int_type::limb_bits;
		int_type n(0);
		n.rshift(1u);
		BOOST_CHECK_EQUAL(n,int_type(0));
		n.rshift(limb_bits);
		BOOST_CHECK_EQUAL(n,int_type(0));
		n.rshift(2u * limb_bits);
		BOOST_CHECK_EQUAL(n,int_type(0));
		n.rshift(2u * limb_bits + 1u);
		BOOST_CHECK_EQUAL(n,int_type(0));
		n = int_type(1);
		n.rshift(0u);
		BOOST_CHECK_EQUAL(n,int_type(1));
		n = int_type(-1);
		n.rshift(0u);
		BOOST_CHECK_EQUAL(n,int_type(-1));
		n.rshift(2u * limb_bits);
		BOOST_CHECK_EQUAL(n,int_type(0));
		n = int_type(-1);
		n.rshift(2u * limb_bits + 1u);
		BOOST_CHECK_EQUAL(n,int_type(0));
		n = int_type(1);
		n.rshift(1u);
		BOOST_CHECK_EQUAL(n,int_type(0));
		n = int_type(-1);
		n.rshift(1u);
		BOOST_CHECK_EQUAL(n,int_type(0));
		// Testing with limb_bits and higher shifting.
		n = int_type(1);
		n.rshift(limb_bits);
		BOOST_CHECK_EQUAL(n,int_type(0));
		n = int_type(-1);
		n.rshift(limb_bits);
		BOOST_CHECK_EQUAL(n,int_type(0));
		n = int_type(1);
		n.rshift(limb_bits + 1u);
		BOOST_CHECK_EQUAL(n,int_type(0));
		n = int_type(-1);
		n.rshift(limb_bits + 1u);
		BOOST_CHECK_EQUAL(n,int_type(0));
		n = int_type(1);
		n.lshift(limb_bits - 1u);
		n.rshift(limb_bits);
		BOOST_CHECK_EQUAL(n,int_type(0));
		n = int_type(-1);
		n.lshift(limb_bits - 1u);
		n.rshift(limb_bits);
		BOOST_CHECK_EQUAL(n,int_type(0));
		n = int_type(1);
		n.lshift(limb_bits);
		n.rshift(limb_bits);
		BOOST_CHECK_EQUAL(n,int_type(1));
		n = int_type(-1);
		n.lshift(limb_bits);
		n.rshift(limb_bits);
		BOOST_CHECK_EQUAL(n,int_type(-1));
		n = int_type(1);
		n.lshift(limb_bits + 1u);
		n.rshift(limb_bits);
		BOOST_CHECK_EQUAL(n,int_type(2));
		n = int_type(-1);
		n.lshift(limb_bits + 1u);
		n.rshift(limb_bits);
		BOOST_CHECK_EQUAL(n,int_type(-2));
		n = int_type(1);
		n.lshift(limb_bits + 2u);
		n.rshift(limb_bits);
		BOOST_CHECK_EQUAL(n,int_type(4));
		n = int_type(-1);
		n.lshift(limb_bits + 2u);
		n.rshift(limb_bits);
		BOOST_CHECK_EQUAL(n,int_type(-4));
		n = int_type(1);
		n.lshift(limb_bits + 2u);
		n.m_limbs[0u] = limb_t(1u);
		n.rshift(limb_bits);
		BOOST_CHECK_EQUAL(n,int_type(4));
		n = int_type(-1);
		n.lshift(limb_bits + 2u);
		n.m_limbs[0u] = limb_t(1u);
		n.rshift(limb_bits);
		BOOST_CHECK_EQUAL(n,int_type(-4));
		// Testing with shift in ]0,limb_bits[.
		n = int_type(1);
		n.lshift(limb_bits);
		n.m_limbs[0u] = limb_t(1u);
		n.rshift(1u);
		BOOST_CHECK_EQUAL(n,int_type(limb_t(1u) << (limb_bits - 1u)));
		n = int_type(-1);
		n.lshift(limb_bits);
		n.m_limbs[0u] = limb_t(1u);
		n.rshift(1u);
		BOOST_CHECK_EQUAL(n,-int_type(limb_t(1u) << (limb_bits - 1u)));
		n = int_type(1);
		n.lshift(limb_bits + 1u);
		n.m_limbs[0u] = limb_t(2u);
		n.rshift(1u);
		BOOST_CHECK_EQUAL(n.m_limbs[0u],1u);
		BOOST_CHECK_EQUAL(n.m_limbs[1u],1u);
		n = int_type(-1);
		n.lshift(limb_bits + 1u);
		n.m_limbs[0u] = limb_t(2u);
		n.rshift(1u);
		BOOST_CHECK_EQUAL(n.m_limbs[0u],1u);
		BOOST_CHECK_EQUAL(n.m_limbs[1u],1u);
		n = int_type(1);
		n.lshift(limb_bits + 1u);
		n.m_limbs[0u] = limb_t(4u);
		n.rshift(3u);
		BOOST_CHECK_EQUAL(n.m_limbs[0u],limb_t(1u) << (limb_bits - 2u));
		BOOST_CHECK_EQUAL(n.m_limbs[1u],0u);
		n = int_type(1);
		n.lshift(limb_bits + 1u);
		n.m_limbs[0u] = limb_t(8u);
		n.rshift(3u);
		BOOST_CHECK_EQUAL(n.m_limbs[0u],(limb_t(1u) << (limb_bits - 2u)) + 1u);
		BOOST_CHECK_EQUAL(n.m_limbs[1u],0u);
		n = int_type(1);
		n.lshift(limb_bits + 1u);
		n.m_limbs[0u] = limb_t(8u);
		n.m_limbs[1u] = limb_t(n.m_limbs[1u] + 8u);
		n.rshift(3u);
		BOOST_CHECK_EQUAL(n.m_limbs[0u],(limb_t(1u) << (limb_bits - 2u)) + 1u);
		BOOST_CHECK_EQUAL(n.m_limbs[1u],1u);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_static_integer_rshift_test)
{
	boost::mpl::for_each<size_types>(static_rshift_tester());
}

struct rshift_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using int_type = mp_integer<T::value>;
		using limb_t = typename detail::static_integer<T::value>::limb_t;
		constexpr auto limb_bits = detail::static_integer<T::value>::limb_bits;
		// Test the type traits.
		BOOST_CHECK((has_right_shift<int_type>::value));
		BOOST_CHECK((has_right_shift<int_type,int>::value));
		BOOST_CHECK((has_right_shift<int_type,short>::value));
		BOOST_CHECK((has_right_shift<long,int_type>::value));
		BOOST_CHECK((has_right_shift<short,int_type>::value));
		BOOST_CHECK((!has_right_shift<int_type,double>::value));
		BOOST_CHECK((!has_right_shift<int_type,std::string>::value));
		BOOST_CHECK((!has_right_shift<double,int>::value));
		BOOST_CHECK((has_right_shift_in_place<int_type>::value));
		BOOST_CHECK((has_right_shift_in_place<int_type,int>::value));
		BOOST_CHECK((has_right_shift_in_place<int_type,short>::value));
		BOOST_CHECK((has_right_shift_in_place<long,int_type>::value));
		BOOST_CHECK((has_right_shift_in_place<short,int_type>::value));
		BOOST_CHECK((!has_right_shift_in_place<double,int_type>::value));
		BOOST_CHECK((!has_right_shift_in_place<const short,int_type>::value));
		BOOST_CHECK((!has_right_shift_in_place<std::string,int_type>::value));
		// Random testing.
		std::uniform_int_distribution<int> int_dist(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
		std::uniform_int_distribution<limb_t> sdist(limb_t(0u),limb_t(limb_bits * 2u));
		for (int i = 0; i < ntries; ++i) {
			const auto int_n = int_dist(rng);
			int_type n(int_n);
			auto s = sdist(rng);
			auto ns = n >> s;
			BOOST_CHECK_EQUAL(ns,n / int_type(2).pow(s));
			auto ns2 = n >> int_type(s);
			BOOST_CHECK_EQUAL(ns2,ns);
			n >>= s;
			BOOST_CHECK_EQUAL(ns,n);
			BOOST_CHECK_EQUAL(int_n >> int_type(s),ns);
			// Check that left shift followed by right shift preserves the value.
			s = sdist(rng);
			BOOST_CHECK_EQUAL((n << s) >> s,n);
		}
		// Throwing conditions.
		BOOST_CHECK_THROW(int_type{1} >> (int_type(std::numeric_limits< ::mp_bitcnt_t>::max()) + 1),std::invalid_argument);
		BOOST_CHECK_THROW(int_type{1} >> int_type(-1),std::invalid_argument);
		BOOST_CHECK_THROW(int_type{1} >> -1,std::invalid_argument);
		// A couple of tests with C++ integral on the left.
		BOOST_CHECK_EQUAL(2 >> int_type(1),1);
		BOOST_CHECK_THROW(1 >> int_type(-1),std::invalid_argument);
		unsigned n = 1;
		n >>= int_type(1);
		BOOST_CHECK_EQUAL(n,0u);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_rshift_test)
{
	boost::mpl::for_each<size_types>(rshift_tester());
}

struct ternary_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using int_type = mp_integer<T::value>;
		constexpr auto limb_bits = detail::static_integer<T::value>::limb_bits;
		{
		// Addition.
		BOOST_CHECK(has_add3<int_type>::value);
		int_type a, b, c;
		a.add(b,c);
		BOOST_CHECK_EQUAL(a,0);
		BOOST_CHECK(a.is_static());
		a = 1;
		b = -4;
		c = 2;
		a.add(b,c);
		BOOST_CHECK_EQUAL(a,-2);
		BOOST_CHECK(a.is_static());
		// Try with promotion trigger.
		b = int_type(1) << (2u * limb_bits - 1u);
		c = b;
		a.add(b,c);
		BOOST_CHECK_EQUAL(a,int_type(1) << (2u * limb_bits));
		BOOST_CHECK(!a.is_static());
		// Same with overlapping operands.
		a = int_type(1) << (2u * limb_bits - 1u);
		BOOST_CHECK(a.is_static());
		a.add(a,a);
		BOOST_CHECK_EQUAL(a,int_type(1) << (2u * limb_bits));
		BOOST_CHECK(!a.is_static());
		// Random testing.
		std::uniform_int_distribution<int> int_dist(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
		std::uniform_int_distribution<int> p_dist(0,1);
		for (int i = 0; i < ntries; ++i) {
			int_type n1(int_dist(rng)), n2(int_dist(rng)), out;
			// Promote randomly.
			if (p_dist(rng) && n1.is_static()) {
				n1.promote();
			}
			if (p_dist(rng) && n2.is_static()) {
				n2.promote();
			}
			if (p_dist(rng)) {
				out.promote();
			}
			out.add(n1,n2);
			BOOST_CHECK_EQUAL(out,n1 + n2);
			// Try the math:: counterpart.
			math::add3(out,n1,n2);
			BOOST_CHECK_EQUAL(out,n1 + n2);
			// Try with overlapping operands.
			out = n1;
			out.add(out,n2);
			BOOST_CHECK_EQUAL(out,n1 + n2);
			out = n2;
			out.add(n1,out);
			BOOST_CHECK_EQUAL(out,n1 + n2);
			out = n1;
			out.add(out,out);
			BOOST_CHECK_EQUAL(out,n1 * 2);
		}
		}
		{
		// Subtraction.
		BOOST_CHECK(has_sub3<int_type>::value);
		int_type a, b, c;
		a.sub(b,c);
		BOOST_CHECK_EQUAL(a,0);
		BOOST_CHECK(a.is_static());
		a = 1;
		b = -4;
		c = 2;
		a.sub(b,c);
		BOOST_CHECK_EQUAL(a,-6);
		BOOST_CHECK(a.is_static());
		// Try with promotion trigger.
		b = int_type(1) << (2u * limb_bits - 1u);
		c = -b;
		a.sub(b,c);
		BOOST_CHECK_EQUAL(a,int_type(1) << (2u * limb_bits));
		BOOST_CHECK(!a.is_static());
		// Same with overlapping operands.
		a = int_type(1) << (2u * limb_bits - 1u);
		BOOST_CHECK(a.is_static());
		a.sub(a,-a);
		BOOST_CHECK_EQUAL(a,int_type(1) << (2u * limb_bits));
		BOOST_CHECK(!a.is_static());
		// Random testing.
		std::uniform_int_distribution<int> int_dist(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
		std::uniform_int_distribution<int> p_dist(0,1);
		for (int i = 0; i < ntries; ++i) {
			int_type n1(int_dist(rng)), n2(int_dist(rng)), out;
			// Promote randomly.
			if (p_dist(rng) && n1.is_static()) {
				n1.promote();
			}
			if (p_dist(rng) && n2.is_static()) {
				n2.promote();
			}
			if (p_dist(rng)) {
				out.promote();
			}
			out.sub(n1,n2);
			BOOST_CHECK_EQUAL(out,n1 - n2);
			// Try the math:: counterpart.
			math::sub3(out,n1,n2);
			BOOST_CHECK_EQUAL(out,n1 - n2);
			// Try with overlapping operands.
			out = n1;
			out.sub(out,n2);
			BOOST_CHECK_EQUAL(out,n1 - n2);
			out = n2;
			out.sub(n1,out);
			BOOST_CHECK_EQUAL(out,n1 - n2);
			out = n1;
			out.sub(out,out);
			BOOST_CHECK_EQUAL(out,0);
		}
		}
		{
		// Multiplication.
		BOOST_CHECK(has_mul3<int_type>::value);
		int_type a, b, c;
		a.mul(b,c);
		BOOST_CHECK_EQUAL(a,0);
		BOOST_CHECK(a.is_static());
		a = 1;
		b = -4;
		c = 2;
		a.mul(b,c);
		BOOST_CHECK_EQUAL(a,-8);
		BOOST_CHECK(a.is_static());
		// Try with promotion trigger.
		b = int_type(1) << (2u * limb_bits - 1u);
		c = 2;
		a.mul(b,c);
		BOOST_CHECK_EQUAL(a,int_type(1) << (2u * limb_bits));
		BOOST_CHECK(!a.is_static());
		// Same with overlapping operands.
		a = int_type(1) << (2u * limb_bits - 1u);
		BOOST_CHECK(a.is_static());
		a.mul(a,int_type(2));
		BOOST_CHECK_EQUAL(a,int_type(1) << (2u * limb_bits));
		BOOST_CHECK(!a.is_static());
		// Random testing.
		std::uniform_int_distribution<int> int_dist(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
		std::uniform_int_distribution<int> p_dist(0,1);
		for (int i = 0; i < ntries; ++i) {
			int_type n1(int_dist(rng)), n2(int_dist(rng)), out;
			// Promote randomly.
			if (p_dist(rng) && n1.is_static()) {
				n1.promote();
			}
			if (p_dist(rng) && n2.is_static()) {
				n2.promote();
			}
			if (p_dist(rng)) {
				out.promote();
			}
			out.mul(n1,n2);
			BOOST_CHECK_EQUAL(out,n1 * n2);
			// Try the math:: counterpart.
			math::mul3(out,n1,n2);
			BOOST_CHECK_EQUAL(out,n1 * n2);
			// Try with overlapping operands.
			out = n1;
			out.mul(out,n2);
			BOOST_CHECK_EQUAL(out,n1 * n2);
			out = n2;
			out.mul(n1,out);
			BOOST_CHECK_EQUAL(out,n1 * n2);
			out = n1;
			out.mul(out,out);
			BOOST_CHECK_EQUAL(out,n1 * n1);
		}
		}
		{
		// Division.
		BOOST_CHECK(has_div3<int_type>::value);
		int_type a, b, c;
		BOOST_CHECK_THROW(a.div(b,c),zero_division_error);
		BOOST_CHECK_EQUAL(a,0);
		BOOST_CHECK(a.is_static());
		a = 1;
		b = -4;
		c = 2;
		a.div(b,c);
		BOOST_CHECK_EQUAL(a,-2);
		BOOST_CHECK(a.is_static());
		// Try with promotion.
		b = int_type(1) << (2u * limb_bits);
		c = 2;
		a.div(b,c);
		BOOST_CHECK_EQUAL(a,int_type(1) << (2u * limb_bits - 1u));
		BOOST_CHECK(!a.is_static());
		// Same with overlapping operands.
		a = int_type(1) << (2u * limb_bits);
		BOOST_CHECK(!a.is_static());
		a.div(a,int_type(2));
		BOOST_CHECK_EQUAL(a,int_type(1) << (2u * limb_bits - 1u));
		BOOST_CHECK(!a.is_static());
		// Random testing.
		std::uniform_int_distribution<int> int_dist(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
		std::uniform_int_distribution<int> p_dist(0,1);
		for (int i = 0; i < ntries; ++i) {
			int_type n1(int_dist(rng)), n2(int_dist(rng)), out;
			if (n1 == 0 || n2 == 0) {
				continue;
			}
			// Promote randomly.
			if (p_dist(rng) && n1.is_static()) {
				n1.promote();
			}
			if (p_dist(rng) && n2.is_static()) {
				n2.promote();
			}
			if (p_dist(rng)) {
				out.promote();
			}
			out.div(n1,n2);
			BOOST_CHECK_EQUAL(out,n1 / n2);
			// Try the math:: counterpart.
			math::div3(out,n1,n2);
			BOOST_CHECK_EQUAL(out,n1 / n2);
			// Try with overlapping operands.
			out = n1;
			out.div(out,n2);
			BOOST_CHECK_EQUAL(out,n1 / n2);
			out = n2;
			out.div(n1,out);
			BOOST_CHECK_EQUAL(out,n1 / n2);
			out = n1;
			out.div(out,out);
			BOOST_CHECK_EQUAL(out,n1 / n1);
		}
		}
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_ternary_test)
{
	boost::mpl::for_each<size_types>(ternary_tester());
}

struct divexact_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using int_type = mp_integer<T::value>;
		{
		// A few simple checks.
		int_type out, n1{1}, n2{1};
		int_type::_divexact(out,n1,n2);
		BOOST_CHECK(out.is_static());
		BOOST_CHECK_EQUAL(out,1);
		n1 = 6;
		n2 = -3;
		int_type::_divexact(out,n1,n2);
		BOOST_CHECK(out.is_static());
		BOOST_CHECK_EQUAL(out,-2);
		// Overlapping n1, n2.
		int_type::_divexact(out,n1,n1);
		BOOST_CHECK(out.is_static());
		BOOST_CHECK_EQUAL(out,1);
		// Overlapping out, n1.
		int_type::_divexact(n1,n1,n2);
		BOOST_CHECK(n1.is_static());
		BOOST_CHECK_EQUAL(n1,-2);
		// Overlapping out, n2.
		n1 = 6;
		int_type::_divexact(n2,n1,n2);
		BOOST_CHECK(n2.is_static());
		BOOST_CHECK_EQUAL(n2,-2);
		// All overlap.
		int_type::_divexact(n1,n1,n1);
		BOOST_CHECK(n1.is_static());
		BOOST_CHECK_EQUAL(n1,1);
		// Try the exception.
		BOOST_CHECK_THROW(int_type::_divexact(n1,n1,int_type{}),zero_division_error);
		}
		// Random testing.
		std::uniform_int_distribution<int> int_dist(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
		std::uniform_int_distribution<int> p_dist(0,1);
		for (int i = 0; i < ntries; ++i) {
			int_type n1(int_dist(rng)), n2(int_dist(rng)), out, n1n2(n1*n2);
			if (math::is_zero(n1) || math::is_zero(n2)) {
				continue;
			}
			// Promote randomly.
			if (p_dist(rng) && n1.is_static()) {
				n1.promote();
			}
			if (p_dist(rng) && n2.is_static()) {
				n2.promote();
			}
			if (p_dist(rng) && n1n2.is_static()) {
				n1n2.promote();
			}
			if (p_dist(rng)) {
				out.promote();
			}
			int_type::_divexact(out,n1n2,n2);
			BOOST_CHECK_EQUAL(out,n1);
			// Try overlapping.
			int_type::_divexact(out,n1,n1);
			BOOST_CHECK_EQUAL(out,1);
			int_type::_divexact(n1,n1n2,n1);
			BOOST_CHECK_EQUAL(n1,n2);
			int_type::_divexact(n1,-2*n1,n1);
			BOOST_CHECK_EQUAL(n1,-2);
			int_type::_divexact(n1,n1,n1);
			BOOST_CHECK_EQUAL(n1,1);
			// Try with zero.
			int_type::_divexact(out,int_type{},n1n2);
			BOOST_CHECK_EQUAL(out,0);
		}
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_divexact_test)
{
	boost::mpl::for_each<size_types>(divexact_tester());
}

struct gcd_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		{
		int_type a, b, out;
		// Check with two zeroes.
		int_type::gcd(out,a,b);
		BOOST_CHECK_EQUAL(out,0);
		BOOST_CHECK(out.is_static());
		a.promote();
		int_type::gcd(out,a,b);
		BOOST_CHECK_EQUAL(out,0);
		int_type::gcd(out,b,a);
		BOOST_CHECK_EQUAL(out,0);
		BOOST_CHECK(!out.is_static());
		b.promote();
		a = 0;
		out = 2;
		int_type::gcd(out,a,b);
		BOOST_CHECK_EQUAL(out,0);
		int_type::gcd(out,b,a);
		BOOST_CHECK_EQUAL(out,0);
		BOOST_CHECK(!out.is_static());
		a.promote();
		out = 1;
		int_type::gcd(out,a,b);
		BOOST_CHECK_EQUAL(out,0);
		int_type::gcd(out,b,a);
		BOOST_CHECK_EQUAL(out,0);
		BOOST_CHECK(!out.is_static());
		// Only one zero.
		a = 0;
		b = 1;
		out = 2;
		int_type::gcd(out,a,b);
		BOOST_CHECK_EQUAL(out,1);
		int_type::gcd(out,b,a);
		BOOST_CHECK_EQUAL(out,1);
		BOOST_CHECK(out.is_static());
		a.promote();
		int_type::gcd(out,a,b);
		BOOST_CHECK_EQUAL(out,1);
		int_type::gcd(out,b,a);
		BOOST_CHECK_EQUAL(out,1);
		BOOST_CHECK(!out.is_static());
		b.promote();
		a = 0;
		out = 0;
		int_type::gcd(out,a,b);
		BOOST_CHECK_EQUAL(out,1);
		int_type::gcd(out,b,a);
		BOOST_CHECK_EQUAL(out,1);
		BOOST_CHECK(!out.is_static());
		a.promote();
		out = 0;
		int_type::gcd(out,a,b);
		BOOST_CHECK_EQUAL(out,1);
		int_type::gcd(out,b,a);
		BOOST_CHECK_EQUAL(out,1);
		BOOST_CHECK(!out.is_static());
		}
		// Randomised testing.
		std::uniform_int_distribution<int> pdist(0,1);
		std::uniform_int_distribution<int> ndist(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
		for (int i = 0; i < ntries; ++i) {
			auto aint = ndist(rng);
			auto bint = ndist(rng);
			int_type a(aint);
			int_type b(bint);
			int_type out;
			if (pdist(rng) && a.is_static()) {
				a.promote();
			}
			if (pdist(rng) && b.is_static()) {
				b.promote();
			}
			if (pdist(rng)) {
				out.promote();
			}
			int_type::gcd(out,a,b);
			if (out == 0) {
				continue;
			}
			BOOST_CHECK_EQUAL(a % out.abs(),0);
			BOOST_CHECK_EQUAL(b % out.abs(),0);
			const auto out_copy(out);
			int_type::gcd(out,b,a);
			BOOST_CHECK_EQUAL(out,out_copy);
			// Check the math overload.
			math::gcd3(out,a,b);
			BOOST_CHECK_EQUAL(out,out_copy);
			math::gcd3(out,out,out);
			BOOST_CHECK_EQUAL(out,out_copy);
			// Some tests with overlapping arguments.
			int_type old_a(a), old_b(b);
			int_type::gcd(a,a,b);
			BOOST_CHECK_EQUAL(a,out_copy);
			a = old_a;
			math::gcd3(a,a,b);
			BOOST_CHECK_EQUAL(a,out_copy);
			a = old_a;
			int_type::gcd(b,a,b);
			BOOST_CHECK_EQUAL(b,out_copy);
			b = old_b;
			math::gcd3(b,a,b);
			BOOST_CHECK_EQUAL(b,out_copy);
			b = old_b;
			int_type::gcd(a,a,a);
			BOOST_CHECK_EQUAL(a.abs(),old_a.abs());
			a = old_a;
			math::gcd3(a,a,a);
			BOOST_CHECK_EQUAL(a.abs(),old_a.abs());
			a = old_a;
			// Check the math overloads.
			BOOST_CHECK((std::is_same<int_type,decltype(math::gcd(a,b))>::value));
			BOOST_CHECK((std::is_same<int_type,decltype(math::gcd(a,1))>::value));
			BOOST_CHECK((std::is_same<int_type,decltype(math::gcd(1,a))>::value));
			BOOST_CHECK_EQUAL(math::gcd(a,b).abs(),out.abs());
			BOOST_CHECK_EQUAL(math::gcd(aint,b).abs(),out.abs());
			BOOST_CHECK_EQUAL(math::gcd(a,bint).abs(),out.abs());
		}
		// Check the math type traits.
		BOOST_CHECK((has_gcd<int_type>::value));
		BOOST_CHECK((has_gcd<int_type,int>::value));
		BOOST_CHECK((has_gcd<short &,int_type &&>::value));
		BOOST_CHECK((!has_gcd<double,int_type>::value));
		BOOST_CHECK((!has_gcd<int_type,double>::value));
		BOOST_CHECK((has_gcd3<int_type>::value));
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_gcd_test)
{
	boost::mpl::for_each<size_types>(gcd_tester());
}

struct divrem_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		{
		// Check throwing conditions
		int_type q, r;
		BOOST_CHECK_THROW(int_type::divrem(q,r,int_type(1),int_type(0)),zero_division_error);
		BOOST_CHECK_THROW(int_type::divrem(q,q,int_type(1),int_type(1)),std::invalid_argument);
		BOOST_CHECK_THROW(int_type::divrem(r,r,int_type(1),int_type(1)),std::invalid_argument);
		}
		// Randomised testing.
		std::uniform_int_distribution<int> pdist(0,1);
		std::uniform_int_distribution<int> ndist(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
		for (int i = 0; i < ntries; ++i) {
			int_type a(ndist(rng)), b(ndist(rng));
			int_type q, r;
			if (b == 0) {
				continue;
			}
			if (pdist(rng) && a.is_static()) {
				a.promote();
			}
			if (pdist(rng) && b.is_static()) {
				b.promote();
			}
			if (pdist(rng)) {
				q.promote();
			}
			if (pdist(rng)) {
				r.promote();
			}
			int_type::divrem(q,r,a,b);
			BOOST_CHECK_EQUAL(q,a / b);
			BOOST_CHECK_EQUAL(r,a % b);
			BOOST_CHECK(r.sign() == a.sign() || r.sign() == 0);
			int_type::divrem(q,r,a*b,b);
			BOOST_CHECK_EQUAL(q,a);
			BOOST_CHECK_EQUAL(r,0);
			int_type::divrem(q,r,a*a*b,b);
			BOOST_CHECK_EQUAL(q,a*a);
			BOOST_CHECK_EQUAL(r,0);
			int_type::divrem(q,r,a*b + 1,b);
			BOOST_CHECK_EQUAL(q,(a*b + 1) / b);
			BOOST_CHECK_EQUAL(r,(a*b + 1) % b);
			BOOST_CHECK(r.sign() == (a*b + 1).sign() || r.sign() == 0);
			int_type::divrem(q,r,a*a*b + 1,b);
			BOOST_CHECK_EQUAL(q,(a*a*b + 1) / b);
			BOOST_CHECK_EQUAL(r,(a*a*b + 1) % b);
			BOOST_CHECK(r.sign() == (a*a*b + 1).sign() || r.sign() == 0);
			int_type::divrem(q,r,a,int_type(1));
			BOOST_CHECK_EQUAL(q,a);
			BOOST_CHECK_EQUAL(r,0);
			int_type::divrem(q,r,a,int_type(-1));
			BOOST_CHECK_EQUAL(q,-a);
			BOOST_CHECK_EQUAL(r,0);
			// Tests with overlapping arguments.
			auto old_a(a);
			int_type::divrem(a,r,a,b);
			BOOST_CHECK_EQUAL(a,old_a / b);
			BOOST_CHECK_EQUAL(r,old_a % b);
			a = old_a;
			auto old_b(b);
			int_type::divrem(b,r,a,b);
			BOOST_CHECK_EQUAL(b,a / old_b);
			BOOST_CHECK_EQUAL(r,a % old_b);
			b = old_b;
			old_a = a;
			int_type::divrem(q,a,a,b);
			BOOST_CHECK_EQUAL(q,old_a / b);
			BOOST_CHECK_EQUAL(a,old_a % b);
			a = old_a;
			old_b = b;
			int_type::divrem(q,b,a,b);
			BOOST_CHECK_EQUAL(q,a / old_b);
			BOOST_CHECK_EQUAL(b,a % old_b);
			b = old_b;
			old_a = a;
			int_type::divrem(q,a,a,a);
			BOOST_CHECK_EQUAL(q,1);
			BOOST_CHECK_EQUAL(a,0);
			a = old_a;
			old_b = b;
			int_type::divrem(q,b,b,b);
			BOOST_CHECK_EQUAL(q,1);
			BOOST_CHECK_EQUAL(b,0);
			b = old_b;
			old_a = a;
			int_type::divrem(a,r,a,a);
			BOOST_CHECK_EQUAL(a,1);
			BOOST_CHECK_EQUAL(r,0);
			a = old_a;
			old_b = b;
			int_type::divrem(b,r,b,b);
			BOOST_CHECK_EQUAL(b,1);
			BOOST_CHECK_EQUAL(r,0);
			b = old_b;
		}
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_divrem_test)
{
	boost::mpl::for_each<size_types>(divrem_tester());
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
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::factorial(n)),mpz_lexcast(m));
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
		BOOST_CHECK((has_binomial<int_type,int>::value));
		BOOST_CHECK((has_binomial<int,int_type>::value));
		BOOST_CHECK((std::is_same<int_type,decltype(math::binomial(int_type{},0))>::value));
		BOOST_CHECK((std::is_same<decltype(math::binomial(int_type{},0)),int_type>::value));
		BOOST_CHECK((has_binomial<int_type,double>::value));
		BOOST_CHECK((has_binomial<double,int_type>::value));
		BOOST_CHECK((std::is_same<double,decltype(math::binomial(int_type{},0.))>::value));
		BOOST_CHECK((std::is_same<decltype(math::binomial(int_type{},0.)),double>::value));
		BOOST_CHECK((has_binomial<int_type,int_type>::value));
		BOOST_CHECK((std::is_same<int_type,decltype(math::binomial(int_type{},int_type{}))>::value));
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
				// NOTE: we cannot check this case with GMP, defer to some tests below.
				BOOST_CHECK_NO_THROW(n.binomial(tmp2));
				continue;
			}
			::mpz_set_si(&m.m_mpz,static_cast<long>(tmp1));
			::mpz_bin_ui(&m.m_mpz,&m.m_mpz,static_cast<unsigned long>(tmp2));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n.binomial(tmp2)),mpz_lexcast(m));
			// int -- integral.
			BOOST_CHECK_EQUAL(math::binomial(n,tmp2),n.binomial(tmp2));
			// integral -- int.
			BOOST_CHECK_EQUAL(math::binomial(tmp2,n),int_type(tmp2).binomial(n));
			// integral -- integral.
			BOOST_CHECK_EQUAL(math::binomial(tmp2,tmp1),integer(tmp2).binomial(tmp1));
			// int -- double.
			BOOST_CHECK_EQUAL(math::binomial(n,static_cast<double>(tmp2)),math::binomial(double(n),static_cast<double>(tmp2)));
			// double -- int.
			BOOST_CHECK_EQUAL(math::binomial(static_cast<double>(tmp2),n),math::binomial(static_cast<double>(tmp2),double(n)));
			BOOST_CHECK_EQUAL(n.binomial(tmp2),n.binomial(int_type(tmp2)));
			BOOST_CHECK_EQUAL(n.binomial(long(tmp2)),n.binomial(int_type(tmp2)));
			BOOST_CHECK_EQUAL(n.binomial((long long)(tmp2)),n.binomial(int_type(tmp2)));
			BOOST_CHECK_EQUAL(n.binomial((unsigned long)(tmp2)),n.binomial(int_type(tmp2)));
			BOOST_CHECK_EQUAL(n.binomial((unsigned long long)(tmp2)),n.binomial(int_type(tmp2)));
		}
		BOOST_CHECK_THROW(n.binomial(std::numeric_limits<unsigned long>::max() + int_type(1)),std::invalid_argument);
		// Negative k.
		BOOST_CHECK_EQUAL(int_type{-3}.binomial(-4),-3);
		BOOST_CHECK_EQUAL(int_type{-3}.binomial(-10),-36);
		BOOST_CHECK_EQUAL(int_type{-3}.binomial(-1),0);
		BOOST_CHECK_EQUAL(int_type{3}.binomial(-1),0);
		BOOST_CHECK_EQUAL(int_type{10}.binomial(-1),0);
		BOOST_CHECK_EQUAL(int_type{-3}.binomial(-3),1);
		BOOST_CHECK_EQUAL(int_type{-1}.binomial(-1),1);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_binomial_test)
{
	boost::mpl::for_each<size_types>(binomial_tester());
	// Check the ints.
	using int_type = integer;
	BOOST_CHECK((has_binomial<int,int>::value));
	BOOST_CHECK_EQUAL(math::binomial(4,2),math::binomial(int_type(4),2));
	BOOST_CHECK((has_binomial<char,unsigned>::value));
	BOOST_CHECK_EQUAL(math::binomial(char(4),2u),math::binomial(int_type(4),2));
	BOOST_CHECK((has_binomial<long long,int>::value));
	BOOST_CHECK_EQUAL(math::binomial(7ll,4),math::binomial(int_type(7),4));
	BOOST_CHECK((std::is_same<decltype(math::binomial(7ll,4)),int_type>::value));
	BOOST_CHECK_EQUAL(math::binomial(-7ll,4u),math::binomial(int_type(-7),4));
	// Different bits sizes.
	BOOST_CHECK((!has_binomial<mp_integer<16>,mp_integer<32>>::value));
	BOOST_CHECK((!has_binomial<mp_integer<32>,mp_integer<16>>::value));
}

struct sin_cos_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		BOOST_CHECK_EQUAL(math::sin(int_type()),0);
		BOOST_CHECK_EQUAL(math::cos(int_type()),1);
		BOOST_CHECK_THROW(math::sin(int_type(1)),std::invalid_argument);
		BOOST_CHECK_THROW(math::cos(int_type(1)),std::invalid_argument);
		BOOST_CHECK((std::is_same<int_type,decltype(math::cos(int_type{}))>::value));
		BOOST_CHECK((std::is_same<int_type,decltype(math::sin(int_type{}))>::value));
		BOOST_CHECK(has_sine<int_type>::value);
		BOOST_CHECK(has_cosine<int_type>::value);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_sin_cos_test)
{
	boost::mpl::for_each<size_types>(sin_cos_tester());
}

struct math_divexact_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		BOOST_CHECK(has_exact_division<int_type>::value);
		int_type out;
		math::divexact(out,int_type(4),int_type(-2));
		BOOST_CHECK_EQUAL(out,-2);
		math::divexact(out,int_type(0),int_type(-2));
		BOOST_CHECK_EQUAL(out,0);
		BOOST_CHECK_THROW(math::divexact(out,int_type(0),int_type(0)),zero_division_error);
		BOOST_CHECK_THROW(math::divexact(out,int_type(3),int_type(2)),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_math_divexact_test)
{
	boost::mpl::for_each<size_types>(math_divexact_tester());
}

struct ero_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using z_type = mp_integer<T::value>;
		BOOST_CHECK(has_exact_ring_operations<z_type>::value);
		BOOST_CHECK(has_exact_ring_operations<const z_type>::value);
		BOOST_CHECK(has_exact_ring_operations<z_type &&>::value);
		BOOST_CHECK(has_exact_ring_operations<volatile z_type &&>::value);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_ero_test)
{
	boost::mpl::for_each<size_types>(ero_tester());
}
