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
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>

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
