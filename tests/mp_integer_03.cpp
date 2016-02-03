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
#include <type_traits>

#include "../src/config.hpp"
#include "../src/environment.hpp"

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
