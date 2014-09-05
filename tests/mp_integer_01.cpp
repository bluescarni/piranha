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

#define BOOST_TEST_MODULE mp_integer_01_test
#include <boost/test/unit_test.hpp>

#define FUSION_MAX_VECTOR_SIZE 20

#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/integer_traits.hpp>
#include <boost/lexical_cast.hpp>
#include <cstddef>
#include <cstdint>
#include <gmp.h>
#include <iostream>
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

// Constructors and assignments.
struct constructor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef detail::static_integer<T::value> int_type;
		using limbs_type = typename int_type::limbs_type;
		std::cout << "Size of " << T::value << ": " << sizeof(int_type) << '\n';
		std::cout << "Alignment of " << T::value << ": " << alignof(int_type) << '\n';
		int_type n;
		BOOST_CHECK(n._mp_alloc == 0);
		BOOST_CHECK(n._mp_size == 0);
		BOOST_CHECK(n.m_limbs == limbs_type());
		n.m_limbs[0u] = 4;
		n._mp_size = 1;
		int_type m;
		m = n;
		BOOST_CHECK(m._mp_alloc == 0);
		BOOST_CHECK(m._mp_size == 1);
		BOOST_CHECK(m.m_limbs[1u] == 0);
		BOOST_CHECK(m.m_limbs[0u] == 4);
		n.m_limbs[0u] = 5;
		n._mp_size = -1;
		m = std::move(n);
		BOOST_CHECK(m._mp_alloc == 0);
		BOOST_CHECK(m._mp_size == -1);
		BOOST_CHECK(m.m_limbs[1u] == 0);
		BOOST_CHECK(m.m_limbs[0u] == 5);
		int_type o(m);
		BOOST_CHECK(o._mp_alloc == 0);
		BOOST_CHECK(o._mp_size == -1);
		BOOST_CHECK(o.m_limbs[1u] == 0);
		BOOST_CHECK(o.m_limbs[0u] == 5);
		int_type p(std::move(o));
		BOOST_CHECK(p._mp_alloc == 0);
		BOOST_CHECK(p._mp_size == -1);
		BOOST_CHECK(p.m_limbs[1u] == 0);
		BOOST_CHECK(p.m_limbs[0u] == 5);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(4),boost::lexical_cast<std::string>(int_type(4)));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(-4),boost::lexical_cast<std::string>(int_type(-4)));
		std::uniform_int_distribution<short> short_dist(boost::integer_traits<short>::const_min,boost::integer_traits<short>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp = short_dist(rng);
			try {
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(tmp),boost::lexical_cast<std::string>(int_type(tmp)));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned short> ushort_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp = ushort_dist(rng);
			try {
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(tmp),boost::lexical_cast<std::string>(int_type(tmp)));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<int> int_dist(boost::integer_traits<int>::const_min,boost::integer_traits<int>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp = int_dist(rng);
			try {
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(tmp),boost::lexical_cast<std::string>(int_type(tmp)));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned> uint_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp = uint_dist(rng);
			try {
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(tmp),boost::lexical_cast<std::string>(int_type(tmp)));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<long> long_dist(boost::integer_traits<long>::const_min,boost::integer_traits<long>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp = long_dist(rng);
			try {
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(tmp),boost::lexical_cast<std::string>(int_type(tmp)));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned long> ulong_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp = ulong_dist(rng);
			try {
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(tmp),boost::lexical_cast<std::string>(int_type(tmp)));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<long long> llong_dist(boost::integer_traits<long long>::const_min,boost::integer_traits<long long>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp = llong_dist(rng);
			try {
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(tmp),boost::lexical_cast<std::string>(int_type(tmp)));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned long long> ullong_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp = ullong_dist(rng);
			try {
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(tmp),boost::lexical_cast<std::string>(int_type(tmp)));
			} catch (const std::overflow_error &) {}
		}
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_static_integer_constructor_test)
{
	environment env;
	boost::mpl::for_each<size_types>(constructor_tester());
}

static inline std::string mpz_lexcast(const mpz_raii &m)
{
	std::ostringstream os;
	const std::size_t size_base10 = ::mpz_sizeinbase(&m.m_mpz,10);
	if (unlikely(size_base10 > boost::integer_traits<std::size_t>::const_max - static_cast<std::size_t>(2))) {
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

struct set_bit_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef detail::static_integer<T::value> int_type;
		const auto limb_bits = int_type::limb_bits;
		int_type n1;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),boost::lexical_cast<std::string>(0));
		n1.set_bit(0);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),boost::lexical_cast<std::string>(1));
		n1.negate();
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),boost::lexical_cast<std::string>(-1));
		n1.set_bit(1);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),boost::lexical_cast<std::string>(-3));
		n1.negate();
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),boost::lexical_cast<std::string>(3));
		mpz_raii m2;
		int_type n2;
		n2.set_bit(0);
		::mpz_setbit(&m2.m_mpz,static_cast< ::mp_bitcnt_t>(0u));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n2),mpz_lexcast(m2));
		n2.set_bit(3);
		::mpz_setbit(&m2.m_mpz,static_cast< ::mp_bitcnt_t>(3u));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n2),mpz_lexcast(m2));
		n2.negate();
		::mpz_neg(&m2.m_mpz,&m2.m_mpz);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n2),mpz_lexcast(m2));
		n2.negate();
		::mpz_neg(&m2.m_mpz,&m2.m_mpz);
		BOOST_CHECK_EQUAL(n2._mp_size,1);
		n2.set_bit(limb_bits);
		::mpz_setbit(&m2.m_mpz,static_cast< ::mp_bitcnt_t>(limb_bits));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n2),mpz_lexcast(m2));
		BOOST_CHECK_EQUAL(n2._mp_size,2);
		n2.set_bit(limb_bits + 4u);
		::mpz_setbit(&m2.m_mpz,static_cast< ::mp_bitcnt_t>(limb_bits + 4u));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n2),mpz_lexcast(m2));
		n2.set_bit(4u);
		::mpz_setbit(&m2.m_mpz,static_cast< ::mp_bitcnt_t>(4u));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n2),mpz_lexcast(m2));
		BOOST_CHECK_EQUAL(n2._mp_size,2);
		for (typename int_type::limb_t i = 0u; i < int_type::limb_bits * 2u; ++i) {
			n2.set_bit(i);
			::mpz_setbit(&m2.m_mpz,static_cast< ::mp_bitcnt_t>(i));
		}
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n2),mpz_lexcast(m2));
		n2.negate();
		::mpz_neg(&m2.m_mpz,&m2.m_mpz);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n2),mpz_lexcast(m2));
		BOOST_CHECK_EQUAL(n2._mp_size,-2);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_static_integer_set_bit_test)
{
	boost::mpl::for_each<size_types>(set_bit_tester());
}

struct calculate_n_limbs_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using int_type = detail::static_integer<T::value>;
		using mpz_size_t = detail::mpz_size_t;
		const auto limb_bits = int_type::limb_bits;
		int_type n;
		BOOST_CHECK_EQUAL(n.calculate_n_limbs(),mpz_size_t(0));
		n.set_bit(0);
		BOOST_CHECK_EQUAL(n.calculate_n_limbs(),mpz_size_t(1));
		n.set_bit(1);
		BOOST_CHECK_EQUAL(n.calculate_n_limbs(),mpz_size_t(1));
		n.set_bit(limb_bits);
		BOOST_CHECK_EQUAL(n.calculate_n_limbs(),mpz_size_t(2));
		n.set_bit(limb_bits + 1u);
		BOOST_CHECK_EQUAL(n.calculate_n_limbs(),mpz_size_t(2));
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_static_integer_calculate_n_limbs_test)
{
	boost::mpl::for_each<size_types>(calculate_n_limbs_tester());
}

struct static_negate_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using int_type = detail::static_integer<T::value>;
		int_type n;
		n.negate();
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"0");
		n.set_bit(0);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"1");
		n.negate();
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"-1");
		n = int_type(123);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"123");
		n.negate();
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"-123");
		BOOST_CHECK(n._mp_size < 0);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_static_integer_negate_test)
{
	boost::mpl::for_each<size_types>(static_negate_tester());
}

struct static_comparison_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using int_type = detail::static_integer<T::value>;
		const auto limb_bits = int_type::limb_bits;
		BOOST_CHECK_EQUAL(int_type{},int_type{});
		BOOST_CHECK(!(int_type{} < int_type{}));
		BOOST_CHECK(int_type{} >= int_type{});
		int_type n, m;
		m.negate();
		BOOST_CHECK_EQUAL(n,m);
		BOOST_CHECK(!(n != m));
		BOOST_CHECK(!(n < m));
		BOOST_CHECK(!(n > m));
		BOOST_CHECK(n >= m);
		BOOST_CHECK(n <= m);
		n = int_type(1);
		BOOST_CHECK(m != n);
		BOOST_CHECK(m < n);
		BOOST_CHECK(!(m > n));
		BOOST_CHECK(m <= n);
		BOOST_CHECK(!(m >= n));
		BOOST_CHECK(n > m);
		BOOST_CHECK(!(n < m));
		BOOST_CHECK(n >= m);
		BOOST_CHECK(!(m >= n));
		n = int_type(-1);
		BOOST_CHECK(m != n);
		BOOST_CHECK(n < m);
		BOOST_CHECK(!(n > m));
		BOOST_CHECK(n <= m);
		BOOST_CHECK(!(n >= m));
		BOOST_CHECK(m > n);
		BOOST_CHECK(!(m < n));
		BOOST_CHECK(m >= n);
		BOOST_CHECK(!(n >= m));
		n = int_type(2);
		m = int_type(1);
		BOOST_CHECK(m != n);
		BOOST_CHECK(m < n);
		BOOST_CHECK(!(m > n));
		BOOST_CHECK(m <= n);
		BOOST_CHECK(!(m >= n));
		BOOST_CHECK(n > m);
		BOOST_CHECK(!(n < m));
		BOOST_CHECK(n >= m);
		BOOST_CHECK(!(m >= n));
		BOOST_CHECK(!(n < m));
		BOOST_CHECK(n >= m);
		n = int_type(-1);
		BOOST_CHECK(m != n);
		BOOST_CHECK(n < m);
		BOOST_CHECK(!(n > m));
		BOOST_CHECK(n <= m);
		BOOST_CHECK(!(n >= m));
		BOOST_CHECK(m > n);
		BOOST_CHECK(!(m < n));
		BOOST_CHECK(m >= n);
		BOOST_CHECK(!(n >= m));
		n = int_type(-2);
		m = int_type(-1);
		BOOST_CHECK(m != n);
		BOOST_CHECK(n < m);
		BOOST_CHECK(!(n > m));
		BOOST_CHECK(n <= m);
		BOOST_CHECK(!(n >= m));
		BOOST_CHECK(m > n);
		BOOST_CHECK(!(m < n));
		BOOST_CHECK(m >= n);
		BOOST_CHECK(!(n >= m));
		n = int_type();
		n.set_bit(limb_bits * 1u + 3u);
		m = int_type(1);
		BOOST_CHECK(m != n);
		BOOST_CHECK(m < n);
		BOOST_CHECK(!(m > n));
		BOOST_CHECK(m <= n);
		BOOST_CHECK(!(m >= n));
		BOOST_CHECK(n > m);
		BOOST_CHECK(!(n < m));
		BOOST_CHECK(n >= m);
		BOOST_CHECK(!(m >= n));
		BOOST_CHECK(!(n < m));
		BOOST_CHECK(n >= m);
		m = int_type(-1);
		BOOST_CHECK(m != n);
		BOOST_CHECK(m < n);
		BOOST_CHECK(!(m > n));
		BOOST_CHECK(m <= n);
		BOOST_CHECK(!(m >= n));
		BOOST_CHECK(n > m);
		BOOST_CHECK(!(n < m));
		BOOST_CHECK(n >= m);
		BOOST_CHECK(!(m >= n));
		BOOST_CHECK(!(n < m));
		BOOST_CHECK(n >= m);
		n.negate();
		BOOST_CHECK(m != n);
		BOOST_CHECK(n < m);
		BOOST_CHECK(!(n > m));
		BOOST_CHECK(n <= m);
		BOOST_CHECK(!(n >= m));
		BOOST_CHECK(m > n);
		BOOST_CHECK(!(m < n));
		BOOST_CHECK(m >= n);
		BOOST_CHECK(!(n >= m));
		n = int_type();
		m = n;
		n.set_bit(0);
		n.set_bit(limb_bits);
		m.set_bit(limb_bits);
		BOOST_CHECK(m < n);
		BOOST_CHECK(n > m);
		std::uniform_int_distribution<short> short_dist(boost::integer_traits<short>::const_min,boost::integer_traits<short>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = short_dist(rng), tmp2 = short_dist(rng);
			try {
				BOOST_CHECK_EQUAL((tmp1 > tmp2),(int_type(tmp1) > int_type(tmp2)));
				BOOST_CHECK_EQUAL((tmp2 < tmp1),(int_type(tmp2) < int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp1 >= tmp2),(int_type(tmp1) >= int_type(tmp2)));
				BOOST_CHECK_EQUAL((tmp2 <= tmp1),(int_type(tmp2) <= int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp2 == tmp1),(int_type(tmp2) == int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp1 == tmp1),(int_type(tmp1) == int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp2 != tmp1),(int_type(tmp2) != int_type(tmp1)));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned short> ushort_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = ushort_dist(rng), tmp2 = ushort_dist(rng);
			try {
				BOOST_CHECK_EQUAL((tmp1 > tmp2),(int_type(tmp1) > int_type(tmp2)));
				BOOST_CHECK_EQUAL((tmp2 < tmp1),(int_type(tmp2) < int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp1 >= tmp2),(int_type(tmp1) >= int_type(tmp2)));
				BOOST_CHECK_EQUAL((tmp2 <= tmp1),(int_type(tmp2) <= int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp2 == tmp1),(int_type(tmp2) == int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp1 == tmp1),(int_type(tmp1) == int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp2 != tmp1),(int_type(tmp2) != int_type(tmp1)));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<int> int_dist(boost::integer_traits<int>::const_min,boost::integer_traits<int>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = int_dist(rng), tmp2 = int_dist(rng);
			try {
				BOOST_CHECK_EQUAL((tmp1 > tmp2),(int_type(tmp1) > int_type(tmp2)));
				BOOST_CHECK_EQUAL((tmp2 < tmp1),(int_type(tmp2) < int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp1 >= tmp2),(int_type(tmp1) >= int_type(tmp2)));
				BOOST_CHECK_EQUAL((tmp2 <= tmp1),(int_type(tmp2) <= int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp2 == tmp1),(int_type(tmp2) == int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp1 == tmp1),(int_type(tmp1) == int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp2 != tmp1),(int_type(tmp2) != int_type(tmp1)));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned> uint_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = uint_dist(rng), tmp2 = uint_dist(rng);
			try {
				BOOST_CHECK_EQUAL((tmp1 > tmp2),(int_type(tmp1) > int_type(tmp2)));
				BOOST_CHECK_EQUAL((tmp2 < tmp1),(int_type(tmp2) < int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp1 >= tmp2),(int_type(tmp1) >= int_type(tmp2)));
				BOOST_CHECK_EQUAL((tmp2 <= tmp1),(int_type(tmp2) <= int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp2 == tmp1),(int_type(tmp2) == int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp1 == tmp1),(int_type(tmp1) == int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp2 != tmp1),(int_type(tmp2) != int_type(tmp1)));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<long> long_dist(boost::integer_traits<long>::const_min,boost::integer_traits<long>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = long_dist(rng), tmp2 = long_dist(rng);
			try {
				BOOST_CHECK_EQUAL((tmp1 > tmp2),(int_type(tmp1) > int_type(tmp2)));
				BOOST_CHECK_EQUAL((tmp2 < tmp1),(int_type(tmp2) < int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp1 >= tmp2),(int_type(tmp1) >= int_type(tmp2)));
				BOOST_CHECK_EQUAL((tmp2 <= tmp1),(int_type(tmp2) <= int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp2 == tmp1),(int_type(tmp2) == int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp1 == tmp1),(int_type(tmp1) == int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp2 != tmp1),(int_type(tmp2) != int_type(tmp1)));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned long> ulong_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = ulong_dist(rng), tmp2 = ulong_dist(rng);
			try {
				BOOST_CHECK_EQUAL((tmp1 > tmp2),(int_type(tmp1) > int_type(tmp2)));
				BOOST_CHECK_EQUAL((tmp2 < tmp1),(int_type(tmp2) < int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp1 >= tmp2),(int_type(tmp1) >= int_type(tmp2)));
				BOOST_CHECK_EQUAL((tmp2 <= tmp1),(int_type(tmp2) <= int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp2 == tmp1),(int_type(tmp2) == int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp1 == tmp1),(int_type(tmp1) == int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp2 != tmp1),(int_type(tmp2) != int_type(tmp1)));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<long long> llong_dist(boost::integer_traits<long long>::const_min,boost::integer_traits<long long>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = llong_dist(rng), tmp2 = llong_dist(rng);
			try {
				BOOST_CHECK_EQUAL((tmp1 > tmp2),(int_type(tmp1) > int_type(tmp2)));
				BOOST_CHECK_EQUAL((tmp2 < tmp1),(int_type(tmp2) < int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp1 >= tmp2),(int_type(tmp1) >= int_type(tmp2)));
				BOOST_CHECK_EQUAL((tmp2 <= tmp1),(int_type(tmp2) <= int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp2 == tmp1),(int_type(tmp2) == int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp1 == tmp1),(int_type(tmp1) == int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp2 != tmp1),(int_type(tmp2) != int_type(tmp1)));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned long long> ullong_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = ullong_dist(rng), tmp2 = ullong_dist(rng);
			try {
				BOOST_CHECK_EQUAL((tmp1 > tmp2),(int_type(tmp1) > int_type(tmp2)));
				BOOST_CHECK_EQUAL((tmp2 < tmp1),(int_type(tmp2) < int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp1 >= tmp2),(int_type(tmp1) >= int_type(tmp2)));
				BOOST_CHECK_EQUAL((tmp2 <= tmp1),(int_type(tmp2) <= int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp2 == tmp1),(int_type(tmp2) == int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp1 == tmp1),(int_type(tmp1) == int_type(tmp1)));
				BOOST_CHECK_EQUAL((tmp2 != tmp1),(int_type(tmp2) != int_type(tmp1)));
			} catch (const std::overflow_error &) {}
		}
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_static_integer_comparison_test)
{
	boost::mpl::for_each<size_types>(static_comparison_tester());
}

struct static_is_zero_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using int_type = detail::static_integer<T::value>;
		BOOST_CHECK(int_type{}.is_zero());
		BOOST_CHECK(!int_type{1}.is_zero());
		int_type n;
		n.negate();
		BOOST_CHECK(n.is_zero());
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_static_integer_is_zero_test)
{
	boost::mpl::for_each<size_types>(static_is_zero_tester());
}

struct static_abs_size_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using int_type = detail::static_integer<T::value>;
		BOOST_CHECK_EQUAL(int_type{}.abs_size(),0);
		BOOST_CHECK_EQUAL(int_type{1}.abs_size(),1);
		BOOST_CHECK_EQUAL(int_type{-1}.abs_size(),1);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_static_integer_abs_size_test)
{
	boost::mpl::for_each<size_types>(static_is_zero_tester());
}

struct static_mpz_view_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef detail::static_integer<T::value> int_type;
		using mpz_struct_t = detail::mpz_struct_t;
		const auto limb_bits = int_type::limb_bits;
		// Random testing.
		std::uniform_int_distribution<int> dist(0,1);
		for (int i = 0; i < ntries; ++i) {
			mpz_raii m;
			int_type n;
			for (typename int_type::limb_t i = 0u; i < 2u * limb_bits; ++i) {
				if (dist(rng)) {
					n.set_bit(i);
					::mpz_setbit(&m.m_mpz,static_cast< ::mp_bitcnt_t>(i));
				}
			}
			if (dist(rng)) {
				n.negate();
				::mpz_neg(&m.m_mpz,&m.m_mpz);
			}
			auto v = n.get_mpz_view();
			BOOST_CHECK(::mpz_cmp(v,&m.m_mpz) == 0);
			auto v_ptr = static_cast<mpz_struct_t const *>(v);
			// There must always be something allocated, and the size must be less than or equal
			// to the allocated size.
			BOOST_CHECK(v_ptr->_mp_alloc > 0 && (
				v_ptr->_mp_alloc >= v_ptr->_mp_size ||
				v_ptr->_mp_alloc >= -v_ptr->_mp_size
			));
			check_zero_limbs(n,v_ptr);
		}
		// Check with zero.
		mpz_raii m;
		int_type n;
		auto v = n.get_mpz_view();
		BOOST_CHECK(::mpz_cmp(v,&m.m_mpz) == 0);
		auto v_ptr = static_cast<mpz_struct_t const *>(v);
		BOOST_CHECK(v_ptr->_mp_alloc > 0 && (
			v_ptr->_mp_alloc >= v_ptr->_mp_size ||
			v_ptr->_mp_alloc >= -v_ptr->_mp_size
		));
		// TT checks.
		BOOST_CHECK(!std::is_copy_constructible<decltype(v)>::value);
		BOOST_CHECK(std::is_move_constructible<decltype(v)>::value);
		BOOST_CHECK(!std::is_copy_assignable<typename std::add_lvalue_reference<decltype(v)>::type>::value);
		BOOST_CHECK(!std::is_move_assignable<typename std::add_lvalue_reference<decltype(v)>::type>::value);
	}
	template <typename T, typename V, typename std::enable_if<
		std::is_same< ::mp_limb_t,typename T::limb_t>::value &&
		T::limb_bits == unsigned(GMP_NUMB_BITS),
		int>::type = 0>
	static void check_zero_limbs(const T &, V) {}
	template <typename T, typename V, typename std::enable_if<
		!(std::is_same< ::mp_limb_t,typename T::limb_t>::value &&
		T::limb_bits == unsigned(GMP_NUMB_BITS)),
		int>::type = 0>
	static void check_zero_limbs(const T &, V v_ptr)
	{
		const std::size_t size = static_cast<std::size_t>(v_ptr->_mp_size >= 0 ? v_ptr->_mp_size : -v_ptr->_mp_size);
		const std::size_t alloc = static_cast<std::size_t>(v_ptr->_mp_alloc);
		for (std::size_t i = size; i < alloc; ++i) {
			BOOST_CHECK_EQUAL(v_ptr->_mp_d[i],0u);
		}
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_static_mpz_view_test)
{
	boost::mpl::for_each<size_types>(static_mpz_view_tester());
}

struct static_add_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef detail::static_integer<T::value> int_type;
		const auto limb_bits = int_type::limb_bits;
		int_type a, b, c;
		int_type::add(a,b,c);
		BOOST_CHECK_EQUAL(a,int_type{});
		int_type::add(a,c,b);
		BOOST_CHECK_EQUAL(a,int_type{});
		b = int_type(1);
		c = int_type(2);
		int_type::add(a,b,c);
		BOOST_CHECK_EQUAL(a,int_type{3});
		int_type::add(a,c,b);
		BOOST_CHECK_EQUAL(a,int_type{3});
		b = int_type(-1);
		c = int_type(-2);
		int_type::add(a,b,c);
		BOOST_CHECK_EQUAL(a,int_type{-3});
		b = int_type(1);
		c = int_type();
		int_type cmp;
		cmp.set_bit(limb_bits);
		for (typename int_type::limb_t i = 0u; i < limb_bits; ++i) {
			c.set_bit(i);
		}
		int_type::add(a,b,c);
		BOOST_CHECK_EQUAL(cmp,a);
		b = int_type(-1);
		c = int_type();
		cmp = c;
		cmp.set_bit(limb_bits);
		cmp.negate();
		for (typename int_type::limb_t i = 0u; i < limb_bits; ++i) {
			c.set_bit(i);
		}
		c.negate();
		int_type::add(a,b,c);
		BOOST_CHECK_EQUAL(cmp,a);
		b = int_type(-1);
		c = int_type(1);
		int_type::add(a,b,c);
		BOOST_CHECK_EQUAL(a,int_type(0));
		int_type::add(a,c,b);
		BOOST_CHECK_EQUAL(a,int_type(0));
		b.set_bit(limb_bits);
		c.set_bit(limb_bits);
		int_type::add(a,b,c);
		BOOST_CHECK_EQUAL(a,int_type(0));
		int_type::add(a,c,b);
		BOOST_CHECK_EQUAL(a,int_type(0));
		b = int_type(-1);
		c = int_type();
		cmp = c;
		c.set_bit(limb_bits);
		for (typename int_type::limb_t i = 0u; i < limb_bits; ++i) {
			cmp.set_bit(i);
		}
		int_type::add(a,c,b);
		BOOST_CHECK_EQUAL(a,cmp);
		int_type::add(a,b,c);
		BOOST_CHECK_EQUAL(a,cmp);
		b.negate();
		c.negate();
		cmp.negate();
		int_type::add(a,b,c);
		BOOST_CHECK_EQUAL(a,cmp);
		int_type::add(a,c,b);
		BOOST_CHECK_EQUAL(a,cmp);
		b = int_type(0);
		int_type::add(a,b,c);
		BOOST_CHECK_EQUAL(a,c);
		int_type::add(a,c,b);
		BOOST_CHECK_EQUAL(a,c);
		c.negate();
		int_type::add(a,b,c);
		BOOST_CHECK_EQUAL(a,c);
		int_type::add(a,c,b);
		BOOST_CHECK_EQUAL(a,c);
		b = int_type();
		c = int_type();
		for (typename int_type::limb_t i = 0u; i < limb_bits; ++i) {
			b.set_bit(i);
		}
		c.set_bit(0u);
		cmp = int_type();
		cmp.set_bit(limb_bits);
		int_type::add(a,b,c);
		BOOST_CHECK_EQUAL(a,cmp);
		int_type::add(a,c,b);
		BOOST_CHECK_EQUAL(a,cmp);
		b.negate();
		c.negate();
		cmp.negate();
		int_type::add(a,b,c);
		BOOST_CHECK_EQUAL(a,cmp);
		int_type::add(a,c,b);
		BOOST_CHECK_EQUAL(a,cmp);
		b = int_type();
		c = int_type();
		cmp = int_type();
		b.set_bit(limb_bits);
		c.set_bit(0u);
		c.negate();
		int_type::add(a,b,c);
		for (typename int_type::limb_t i = 0u; i < limb_bits; ++i) {
			cmp.set_bit(i);
		}
		BOOST_CHECK_EQUAL(a,cmp);
		int_type::add(a,c,b);
		BOOST_CHECK_EQUAL(a,cmp);
		b = int_type();
		c = int_type();
		cmp = int_type();
		b.set_bit(0u);
		c.set_bit(0u);
		b.set_bit(limb_bits);
		c.set_bit(limb_bits);
		c.negate();
		int_type::add(a,b,c);
		BOOST_CHECK_EQUAL(a,cmp);
		int_type::add(a,c,b);
		BOOST_CHECK_EQUAL(a,cmp);
		// Check overflow throwing.
		b = int_type();
		c = int_type();
		b.set_bit(2u * limb_bits - 1u);
		c.set_bit(2u * limb_bits - 1u);
		c.set_bit(0u);
		auto old_a(a);
		BOOST_CHECK(int_type::add(a,c,b));
		BOOST_CHECK_EQUAL(old_a,a);
		b = int_type();
		c = int_type();
		b.set_bit(2u * limb_bits - 1u);
		c.set_bit(2u * limb_bits - 1u);
		c.set_bit(0u);
		b.negate();
		c.negate();
		BOOST_CHECK(int_type::add(a,c,b));
		BOOST_CHECK_EQUAL(old_a,a);
		// Random testing.
		mpz_raii mc, ma, mb;
		std::uniform_int_distribution<short> short_dist(boost::integer_traits<short>::const_min,boost::integer_traits<short>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = short_dist(rng), tmp2 = short_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if(int_type::add(c,a,b)) {
					continue;
				}
				::mpz_add(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::add(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::add(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned short> ushort_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = ushort_dist(rng), tmp2 = ushort_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::add(c,a,b)) {
					continue;
				}
				::mpz_add(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::add(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::add(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<int> int_dist(boost::integer_traits<int>::const_min,boost::integer_traits<int>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = int_dist(rng), tmp2 = int_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::add(c,a,b)) {
					continue;
				}
				::mpz_add(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::add(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::add(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned> uint_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = uint_dist(rng), tmp2 = uint_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::add(c,a,b)) {
					continue;
				}
				::mpz_add(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::add(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::add(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<long> long_dist(boost::integer_traits<long>::const_min,boost::integer_traits<long>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = long_dist(rng), tmp2 = long_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::add(c,a,b)) {
					continue;
				}
				::mpz_add(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::add(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::add(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned long> ulong_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = ulong_dist(rng), tmp2 = ulong_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::add(c,a,b)) {
					continue;
				}
				::mpz_add(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::add(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::add(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<long long> llong_dist(boost::integer_traits<long long>::const_min,boost::integer_traits<long long>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = llong_dist(rng), tmp2 = llong_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::add(c,a,b)) {
					continue;
				}
				::mpz_add(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::add(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::add(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned long long> ullong_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = ullong_dist(rng), tmp2 = ullong_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::add(c,a,b)) {
					continue;
				}
				::mpz_add(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::add(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::add(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		// Test the operators.
		a = int_type(1);
		b = int_type(2);
		BOOST_CHECK_EQUAL(a+b,int_type(3));
		a += int_type(-5);
		BOOST_CHECK_EQUAL(a,int_type(-4));
		BOOST_CHECK_EQUAL(+a,int_type(-4));
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_static_integer_add_test)
{
	boost::mpl::for_each<size_types>(static_add_tester());
}

struct static_sub_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef detail::static_integer<T::value> int_type;
		const auto limb_bits = int_type::limb_bits;
		int_type a, b, c;
		int_type::sub(a,b,c);
		BOOST_CHECK_EQUAL(a,int_type{});
		int_type::sub(a,c,b);
		BOOST_CHECK_EQUAL(a,int_type{});
		b = int_type(1);
		c = int_type(2);
		int_type::sub(a,b,c);
		BOOST_CHECK_EQUAL(a,int_type{-1});
		int_type::sub(a,c,b);
		BOOST_CHECK_EQUAL(a,int_type{1});
		b = int_type(-1);
		c = int_type(-2);
		int_type::sub(a,b,c);
		BOOST_CHECK_EQUAL(a,int_type{1});
		b = int_type(1);
		c = int_type();
		int_type cmp;
		for (typename int_type::limb_t i = 0u; i < limb_bits; ++i) {
			if (i != 0u) {
				cmp.set_bit(i);
			}
			c.set_bit(i);
		}
		cmp.negate();
		int_type::sub(a,b,c);
		BOOST_CHECK_EQUAL(cmp,a);
		b = int_type(-1);
		c = int_type(1);
		int_type::sub(a,b,c);
		BOOST_CHECK_EQUAL(a,int_type(-2));
		int_type::sub(a,c,b);
		BOOST_CHECK_EQUAL(a,int_type(2));
		b.set_bit(limb_bits);
		c.set_bit(limb_bits);
		cmp = int_type();
		cmp.set_bit(1u);
		cmp.set_bit(limb_bits + 1u);
		cmp.negate();
		int_type::sub(a,b,c);
		BOOST_CHECK_EQUAL(a,cmp);
		int_type::sub(a,c,b);
		cmp.negate();
		BOOST_CHECK_EQUAL(a,cmp);
		b = int_type(-1);
		c = int_type();
		cmp = c;
		for (typename int_type::limb_t i = 0u; i < limb_bits; ++i) {
			if (i != 0u) {
				cmp.set_bit(i);
			}
			c.set_bit(i);
		}
		c.negate();
		int_type::sub(a,b,c);
		BOOST_CHECK_EQUAL(cmp,a);
		b = int_type(1);
		c = int_type();
		cmp = c;
		for (typename int_type::limb_t i = 0u; i < limb_bits * 2u; ++i) {
			if (i != 0u) {
				cmp.set_bit(i);
			}
			c.set_bit(i);
		}
		cmp.negate();
		int_type::sub(a,b,c);
		BOOST_CHECK_EQUAL(cmp,a);
		b = int_type(-1);
		c = int_type();
		cmp = c;
		c.set_bit(limb_bits);
		cmp.set_bit(0u);
		cmp.set_bit(limb_bits);
		cmp.negate();
		int_type::sub(a,b,c);
		BOOST_CHECK_EQUAL(a,cmp);
		cmp.negate();
		int_type::sub(a,c,b);
		BOOST_CHECK_EQUAL(a,cmp);
		b.negate();
		c.negate();
		cmp.negate();
		int_type::sub(a,c,b);
		BOOST_CHECK_EQUAL(a,cmp);
		cmp.negate();
		int_type::sub(a,b,c);
		BOOST_CHECK_EQUAL(a,cmp);
		b = int_type(0);
		int_type::sub(a,b,c);
		BOOST_CHECK_EQUAL(a,-c);
		int_type::sub(a,c,b);
		BOOST_CHECK_EQUAL(a,c);
		c.negate();
		int_type::sub(a,b,c);
		BOOST_CHECK_EQUAL(a,-c);
		int_type::sub(a,c,b);
		BOOST_CHECK_EQUAL(a,c);
		b = int_type();
		c = int_type();
		cmp = int_type();
		for (typename int_type::limb_t i = limb_bits; i < limb_bits * 2u; ++i) {
			if (i != limb_bits) {
				cmp.set_bit(i);
			}
			b.set_bit(i);
		}
		c.set_bit(limb_bits);
		int_type::sub(a,b,c);
		BOOST_CHECK_EQUAL(a,cmp);
		int_type::sub(a,c,b);
		cmp.negate();
		BOOST_CHECK_EQUAL(a,cmp);
		b.negate();
		c.negate();
		cmp.negate();
		int_type::sub(a,c,b);
		BOOST_CHECK_EQUAL(a,cmp);
		int_type::sub(a,b,c);
		cmp.negate();
		BOOST_CHECK_EQUAL(a,cmp);
		b = int_type();
		c = int_type();
		cmp = int_type();
		for (typename int_type::limb_t i = 0u; i < limb_bits; ++i) {
			if (i != 0u) {
				cmp.set_bit(i);
			}
			b.set_bit(i);
		}
		c.set_bit(0u);
		int_type::sub(a,b,c);
		BOOST_CHECK_EQUAL(a,cmp);
		int_type::sub(a,c,b);
		cmp.negate();
		BOOST_CHECK_EQUAL(a,cmp);
		b.negate();
		c.negate();
		cmp.negate();
		int_type::sub(a,c,b);
		BOOST_CHECK_EQUAL(a,cmp);
		cmp.negate();
		int_type::sub(a,b,c);
		BOOST_CHECK_EQUAL(a,cmp);
		b = int_type();
		c = int_type();
		cmp = int_type();
		b.set_bit(limb_bits);
		c.set_bit(0u);
		c.negate();
		int_type::sub(a,b,c);
		cmp.set_bit(0u);
		cmp.set_bit(limb_bits);
		BOOST_CHECK_EQUAL(a,cmp);
		int_type::sub(a,c,b);
		cmp.negate();
		BOOST_CHECK_EQUAL(a,cmp);
		b = int_type();
		c = int_type();
		cmp = int_type();
		b.set_bit(0u);
		c.set_bit(0u);
		b.set_bit(limb_bits);
		c.set_bit(limb_bits);
		c.negate();
		cmp.set_bit(1u);
		cmp.set_bit(limb_bits + 1u);
		int_type::sub(a,b,c);
		BOOST_CHECK_EQUAL(a,cmp);
		cmp.negate();
		int_type::sub(a,c,b);
		BOOST_CHECK_EQUAL(a,cmp);
		// Check overflow throwing.
		b = int_type();
		c = int_type();
		b.set_bit(2u * limb_bits - 1u);
		c.set_bit(2u * limb_bits - 1u);
		c.set_bit(0u);
		b.negate();
		auto old_a(a);
		BOOST_CHECK(int_type::sub(a,c,b));
		BOOST_CHECK_EQUAL(old_a,a);
		b = int_type();
		c = int_type();
		b.set_bit(2u * limb_bits - 1u);
		c.set_bit(2u * limb_bits - 1u);
		c.set_bit(0u);
		c.negate();
		BOOST_CHECK(int_type::sub(a,b,c));
		BOOST_CHECK_EQUAL(old_a,a);
		// Random testing.
		mpz_raii mc, ma, mb;
		std::uniform_int_distribution<short> short_dist(boost::integer_traits<short>::const_min,boost::integer_traits<short>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = short_dist(rng), tmp2 = short_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::sub(c,a,b)) {
					continue;
				}
				::mpz_sub(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::sub(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::sub(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned short> ushort_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = ushort_dist(rng), tmp2 = ushort_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::sub(c,a,b)) {
					continue;
				}
				::mpz_sub(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::sub(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::sub(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<int> int_dist(boost::integer_traits<int>::const_min,boost::integer_traits<int>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = int_dist(rng), tmp2 = int_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::sub(c,a,b)) {
					continue;
				}
				::mpz_sub(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::sub(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::sub(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned> uint_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = uint_dist(rng), tmp2 = uint_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::sub(c,a,b)) {
					continue;
				}
				::mpz_sub(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::sub(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::sub(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<long> long_dist(boost::integer_traits<long>::const_min,boost::integer_traits<long>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = long_dist(rng), tmp2 = long_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::sub(c,a,b)) {
					continue;
				}
				::mpz_sub(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::sub(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::sub(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned long> ulong_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = ulong_dist(rng), tmp2 = ulong_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::sub(c,a,b)) {
					continue;
				}
				::mpz_sub(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::sub(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::sub(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<long long> llong_dist(boost::integer_traits<long long>::const_min,boost::integer_traits<long long>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = llong_dist(rng), tmp2 = llong_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::sub(c,a,b)) {
					continue;
				}
				::mpz_sub(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::sub(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::sub(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned long long> ullong_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = ullong_dist(rng), tmp2 = ullong_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::sub(c,a,b)) {
					continue;
				}
				::mpz_sub(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::sub(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::sub(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		// Test the operators.
		a = int_type(1);
		b = int_type(2);
		BOOST_CHECK_EQUAL(a-b,int_type(-1));
		a -= int_type(5);
		BOOST_CHECK_EQUAL(a,int_type(-4));
		BOOST_CHECK_EQUAL(-a,int_type(4));
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_static_integer_sub_test)
{
	boost::mpl::for_each<size_types>(static_sub_tester());
}

struct static_mul_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef detail::static_integer<T::value> int_type;
		const auto limb_bits = int_type::limb_bits;
		mpz_raii mc, ma, mb;
		int_type a, b, c;
		int_type::mul(a,b,c);
		BOOST_CHECK_EQUAL(a,int_type{});
		int_type::mul(a,c,b);
		BOOST_CHECK_EQUAL(a,int_type{});
		c = int_type(1);
		int_type::mul(a,b,c);
		BOOST_CHECK_EQUAL(a,int_type{});
		int_type::mul(a,c,b);
		BOOST_CHECK_EQUAL(a,int_type{});
		c = int_type(-1);
		int_type::mul(a,b,c);
		BOOST_CHECK_EQUAL(a,int_type{});
		int_type::mul(a,c,b);
		BOOST_CHECK_EQUAL(a,int_type{});
		b = int_type(1);
		c = b;
		int_type::mul(a,b,c);
		BOOST_CHECK_EQUAL(a,int_type{1});
		int_type::mul(a,c,b);
		BOOST_CHECK_EQUAL(a,int_type{1});
		b = int_type(-1);
		int_type::mul(a,b,c);
		BOOST_CHECK_EQUAL(a,int_type{-1});
		int_type::mul(a,c,b);
		BOOST_CHECK_EQUAL(a,int_type{-1});
		b = int_type(7);
		c = int_type(8);
		int_type::mul(a,b,c);
		BOOST_CHECK_EQUAL(a,int_type{56});
		int_type::mul(a,c,b);
		BOOST_CHECK_EQUAL(a,int_type{56});
		c.negate();
		int_type::mul(a,b,c);
		BOOST_CHECK_EQUAL(a,int_type{-56});
		int_type::mul(a,c,b);
		BOOST_CHECK_EQUAL(a,int_type{-56});
		b = int_type();
		c = b;
		for (typename int_type::limb_t i = 0u; i < limb_bits - 1u; ++i) {
			::mpz_setbit(&mb.m_mpz,i);
			::mpz_setbit(&mc.m_mpz,i);
			b.set_bit(i);
			c.set_bit(i);
		}
		int_type::mul(a,b,c);
		::mpz_mul(&ma.m_mpz,&mb.m_mpz,&mc.m_mpz);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(ma));
		int_type::mul(a,c,b);
		::mpz_mul(&ma.m_mpz,&mc.m_mpz,&mb.m_mpz);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(ma));
		b.negate();
		::mpz_neg(&mb.m_mpz,&mb.m_mpz);
		int_type::mul(a,b,c);
		::mpz_mul(&ma.m_mpz,&mb.m_mpz,&mc.m_mpz);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(ma));
		int_type::mul(a,c,b);
		::mpz_mul(&ma.m_mpz,&mc.m_mpz,&mb.m_mpz);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(ma));
		// Check overflow condition.
		b = int_type();
		c = b;
		a = b;
		c.set_bit(typename int_type::limb_t(2));
		b.set_bit(limb_bits);
		BOOST_CHECK(int_type::mul(a,c,b));
		BOOST_CHECK(int_type::mul(a,b,c));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),"0");
		c.set_bit(limb_bits);
		BOOST_CHECK(int_type::mul(a,c,b));
		BOOST_CHECK(int_type::mul(a,b,c));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),"0");
		// Random testing.
		std::uniform_int_distribution<short> short_dist(boost::integer_traits<short>::const_min,boost::integer_traits<short>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = short_dist(rng), tmp2 = short_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 1 || b.abs_size() > 1) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::mul(c,a,b)) {
					continue;
				}
				::mpz_mul(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::mul(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::mul(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned short> ushort_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = ushort_dist(rng), tmp2 = ushort_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 1 || b.abs_size() > 1) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::mul(c,a,b)) {
					continue;
				}
				::mpz_mul(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::mul(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::mul(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<int> int_dist(boost::integer_traits<int>::const_min,boost::integer_traits<int>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = int_dist(rng), tmp2 = int_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 1 || b.abs_size() > 1) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::mul(c,a,b)) {
					continue;
				}
				::mpz_mul(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::mul(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::mul(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned> uint_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = uint_dist(rng), tmp2 = uint_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 1 || b.abs_size() > 1) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::mul(c,a,b)) {
					continue;
				}
				::mpz_mul(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::mul(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::mul(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<long> long_dist(boost::integer_traits<long>::const_min,boost::integer_traits<long>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = long_dist(rng), tmp2 = long_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 1 || b.abs_size() > 1) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::mul(c,a,b)) {
					continue;
				}
				::mpz_mul(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::mul(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::mul(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned long> ulong_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = ulong_dist(rng), tmp2 = ulong_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 1 || b.abs_size() > 1) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::mul(c,a,b)) {
					continue;
				}
				::mpz_mul(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::mul(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::mul(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<long long> llong_dist(boost::integer_traits<long long>::const_min,boost::integer_traits<long long>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = llong_dist(rng), tmp2 = llong_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 1 || b.abs_size() > 1) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::mul(c,a,b)) {
					continue;
				}
				::mpz_mul(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::mul(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::mul(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned long long> ullong_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = ullong_dist(rng), tmp2 = ullong_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c, old_a(a);
				if (a.abs_size() > 1 || b.abs_size() > 1) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				if (int_type::mul(c,a,b)) {
					continue;
				}
				::mpz_mul(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
				// Try in-place.
				if (int_type::mul(a,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(mc));
				a = old_a;
				if (int_type::mul(b,a,b)) {
					continue;
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(b),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		// Operators.
		b = int_type(4);
		c = int_type(5);
		BOOST_CHECK_EQUAL(b * c,int_type(20));
		b *= -int_type(5);
		BOOST_CHECK_EQUAL(b,int_type(-20));
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_static_integer_mul_test)
{
	boost::mpl::for_each<size_types>(static_mul_tester());
}

struct static_addmul_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef detail::static_integer<T::value> int_type;
		const auto limb_bits = int_type::limb_bits;
		mpz_raii mc, ma, mb;
		int_type a, b, c;
		a.multiply_accumulate(b,c);
		BOOST_CHECK_EQUAL(a,int_type());
		a = int_type(1);
		a.multiply_accumulate(b,c);
		BOOST_CHECK_EQUAL(a,int_type(1));
		a = int_type(-2);
		a.multiply_accumulate(b,c);
		BOOST_CHECK_EQUAL(a,int_type(-2));
		a = int_type(1);
		b = int_type(2);
		c = int_type(3);
		a.multiply_accumulate(b,c);
		BOOST_CHECK_EQUAL(a,int_type(7));
		b = int_type(-2);
		c = int_type(-3);
		a.multiply_accumulate(b,c);
		BOOST_CHECK_EQUAL(a,int_type(13));
		b = int_type(2);
		c = int_type(-3);
		a.multiply_accumulate(b,c);
		BOOST_CHECK_EQUAL(a,int_type(7));
		b = int_type(-2);
		c = int_type(3);
		a.multiply_accumulate(b,c);
		BOOST_CHECK_EQUAL(a,int_type(1));
		a = int_type(-1);
		b = int_type(2);
		c = int_type(3);
		a.multiply_accumulate(b,c);
		BOOST_CHECK_EQUAL(a,int_type(5));
		b = int_type(-2);
		c = int_type(-3);
		a.multiply_accumulate(b,c);
		BOOST_CHECK_EQUAL(a,int_type(11));
		b = int_type(2);
		c = int_type(-3);
		a.multiply_accumulate(b,c);
		BOOST_CHECK_EQUAL(a,int_type(5));
		b = int_type(-2);
		c = int_type(3);
		a.multiply_accumulate(b,c);
		BOOST_CHECK_EQUAL(a,int_type(-1));
		a = int_type(5);
		b = int_type();
		c = b;
		b.set_bit(limb_bits / 2u + 1u);
		c.set_bit(limb_bits / 2u + 2u);
		auto cmp = a + b*c;
		a.multiply_accumulate(b,c);
		BOOST_CHECK_EQUAL(a,cmp);
		a = int_type(5);
		cmp = a + c*b;
		a.multiply_accumulate(c,b);
		BOOST_CHECK_EQUAL(a,cmp);
		a = int_type(-5);
		cmp = a + b*c;
		a.multiply_accumulate(b,c);
		BOOST_CHECK_EQUAL(a,cmp);
		a = int_type(-5);
		cmp = a + c*b;
		a.multiply_accumulate(c,b);
		BOOST_CHECK_EQUAL(a,cmp);
		b.negate();
		a = int_type(-5);
		cmp = a + b*c;
		a.multiply_accumulate(b,c);
		BOOST_CHECK_EQUAL(a,cmp);
		a = int_type(-5);
		cmp = a + c*b;
		a.multiply_accumulate(c,b);
		BOOST_CHECK_EQUAL(a,cmp);
		a = int_type();
		a.set_bit(limb_bits + 2u);
		cmp = a + b*c;
		a.multiply_accumulate(b,c);
		BOOST_CHECK_EQUAL(a,cmp);
		a = int_type();
		a.set_bit(limb_bits + 2u);
		cmp = a + c*b;
		a.multiply_accumulate(c,b);
		BOOST_CHECK_EQUAL(a,cmp);
		a = int_type();
		a.set_bit(limb_bits + 2u);
		a.negate();
		cmp = a + b*c;
		a.multiply_accumulate(b,c);
		BOOST_CHECK_EQUAL(a,cmp);
		a = int_type();
		a.set_bit(limb_bits + 2u);
		a.negate();
		cmp = a + c*b;
		a.multiply_accumulate(c,b);
		BOOST_CHECK_EQUAL(a,cmp);
		a = int_type(2);
		cmp = a + b*c;
		a.multiply_accumulate(b,c);
		BOOST_CHECK_EQUAL(a,cmp);
		a = int_type(2);
		cmp = a + c*b;
		a.multiply_accumulate(c,b);
		BOOST_CHECK_EQUAL(a,cmp);
		a = int_type(2);
		a.negate();
		cmp = a + b*c;
		a.multiply_accumulate(b,c);
		BOOST_CHECK_EQUAL(a,cmp);
		a = int_type(2);
		a.negate();
		cmp = a + c*b;
		a.multiply_accumulate(c,b);
		BOOST_CHECK_EQUAL(a,cmp);
		// This used to be a bug.
		a = int_type();
		b = int_type(2);
		c = int_type(3);
		a.multiply_accumulate(b,c);
		a = int_type();
		b = int_type(2);
		c = int_type(-3);
		a.multiply_accumulate(b,c);
		BOOST_CHECK_EQUAL(a,-int_type(6));
		// Overflow checking.
		a = int_type();
		b = int_type();
		c = int_type();
		for (typename int_type::limb_t i = 0u; i < limb_bits; ++i) {
			b.set_bit(i);
			c.set_bit(i);
		}
		a.set_bit(2u * limb_bits - 1u);
		auto old_a(a);
		BOOST_CHECK(a.multiply_accumulate(b,c));
		BOOST_CHECK_EQUAL(a,old_a);
		a = int_type();
		b = int_type();
		c = int_type();
		for (typename int_type::limb_t i = 0u; i < limb_bits; ++i) {
			b.set_bit(i);
			c.set_bit(i);
		}
		b.negate();
		a.set_bit(2u * limb_bits - 1u);
		a.negate();
		old_a = a;
		BOOST_CHECK(a.multiply_accumulate(b,c));
		BOOST_CHECK_EQUAL(a,old_a);
		// Overflow in the mult part.
		a = int_type();
		b = int_type();
		c = int_type();
		b.set_bit(typename int_type::limb_t(2));
		c.set_bit(limb_bits);
		BOOST_CHECK(a.multiply_accumulate(b,c));
		BOOST_CHECK(a.multiply_accumulate(c,b));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),"0");
		b.set_bit(limb_bits);
		BOOST_CHECK(a.multiply_accumulate(b,c));
		BOOST_CHECK(a.multiply_accumulate(c,b));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),"0");
		// Random tests.
		std::uniform_int_distribution<int> int_dist(0,1);
		// 1 limb for all three operands.
		for (int i = 0; i < ntries; ++i) {
			int_type a, b, c;
			for (typename int_type::limb_t i = 0u; i < limb_bits; ++i) {
				if (int_dist(rng)) {
					a.set_bit(i);
				}
				if (int_dist(rng)) {
					b.set_bit(i);
				}
				if (int_dist(rng)) {
					c.set_bit(i);
				}
			}
			if (int_dist(rng)) {
				a.negate();
			}
			if (int_dist(rng)) {
				b.negate();
			}
			if (int_dist(rng)) {
				c.negate();
			}
			int_type old_a(a);
			::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(a).c_str(),10);
			::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(b).c_str(),10);
			::mpz_set_str(&mc.m_mpz,boost::lexical_cast<std::string>(c).c_str(),10);
			::mpz_addmul(&ma.m_mpz,&mb.m_mpz,&mc.m_mpz);
			int_type cmp = a + b*c;
			a.multiply_accumulate(b,c);
			BOOST_CHECK_EQUAL(a,cmp);
			BOOST_CHECK_EQUAL(a,old_a - (-b * c));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(ma));
			// Test with overlapping.
			a = old_a;
			a.multiply_accumulate(a,a);
			BOOST_CHECK_EQUAL(a,old_a + (old_a * old_a));
		}
		// 2-1-1 limbs.
		for (int i = 0; i < ntries; ++i) {
			int_type a, b, c;
			for (typename int_type::limb_t i = 0u; i < limb_bits; ++i) {
				if (int_dist(rng)) {
					a.set_bit(i);
				}
				if (int_dist(rng)) {
					b.set_bit(i);
				}
				if (int_dist(rng)) {
					c.set_bit(i);
				}
			}
			for (typename int_type::limb_t i = limb_bits; i < 2u * limb_bits; ++i) {
				if (int_dist(rng)) {
					a.set_bit(i);
				}
			}
			if (int_dist(rng)) {
				a.negate();
			}
			if (int_dist(rng)) {
				b.negate();
			}
			if (int_dist(rng)) {
				c.negate();
			}
			int_type old_a(a);
			::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(a).c_str(),10);
			::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(b).c_str(),10);
			::mpz_set_str(&mc.m_mpz,boost::lexical_cast<std::string>(c).c_str(),10);
			::mpz_addmul(&ma.m_mpz,&mb.m_mpz,&mc.m_mpz);
			int_type cmp;
			cmp = a + b*c;
			if (a.multiply_accumulate(b,c)) {
				continue;
			}
			BOOST_CHECK_EQUAL(a,cmp);
			BOOST_CHECK_EQUAL(a,old_a - (-b * c));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(ma));
		}
		// 1-half-half limbs.
		for (int i = 0; i < ntries; ++i) {
			int_type a, b, c;
			for (typename int_type::limb_t i = 0u; i < limb_bits / 2u; ++i) {
				if (int_dist(rng)) {
					a.set_bit(i);
				}
				if (int_dist(rng)) {
					b.set_bit(i);
				}
				if (int_dist(rng)) {
					c.set_bit(i);
				}
			}
			for (typename int_type::limb_t i = limb_bits / 2u; i < limb_bits; ++i) {
				if (int_dist(rng)) {
					a.set_bit(i);
				}
			}
			if (int_dist(rng)) {
				a.negate();
			}
			if (int_dist(rng)) {
				b.negate();
			}
			if (int_dist(rng)) {
				c.negate();
			}
			int_type old_a(a);
			::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(a).c_str(),10);
			::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(b).c_str(),10);
			::mpz_set_str(&mc.m_mpz,boost::lexical_cast<std::string>(c).c_str(),10);
			::mpz_addmul(&ma.m_mpz,&mb.m_mpz,&mc.m_mpz);
			int_type cmp = a + b*c;
			a.multiply_accumulate(b,c);
			BOOST_CHECK_EQUAL(a,cmp);
			BOOST_CHECK_EQUAL(a,old_a - (-b * c));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(ma));
			// Test with overlapping.
			a = old_a;
			a.multiply_accumulate(a,a);
			BOOST_CHECK_EQUAL(a,old_a + (old_a * old_a));
		}
		// 2-half-half limbs.
		for (int i = 0; i < ntries; ++i) {
			int_type a, b, c;
			for (typename int_type::limb_t i = 0u; i < limb_bits / 2u; ++i) {
				if (int_dist(rng)) {
					a.set_bit(i);
				}
				if (int_dist(rng)) {
					b.set_bit(i);
				}
				if (int_dist(rng)) {
					c.set_bit(i);
				}
			}
			for (typename int_type::limb_t i = limb_bits / 2u; i < 2u * limb_bits; ++i) {
				if (int_dist(rng)) {
					a.set_bit(i);
				}
			}
			if (int_dist(rng)) {
				a.negate();
			}
			if (int_dist(rng)) {
				b.negate();
			}
			if (int_dist(rng)) {
				c.negate();
			}
			int_type old_a(a);
			::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(a).c_str(),10);
			::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(b).c_str(),10);
			::mpz_set_str(&mc.m_mpz,boost::lexical_cast<std::string>(c).c_str(),10);
			::mpz_addmul(&ma.m_mpz,&mb.m_mpz,&mc.m_mpz);
			int_type cmp;
			cmp = a + b*c;
			if (a.multiply_accumulate(b,c)) {
				continue;
			}
			BOOST_CHECK_EQUAL(a,cmp);
			BOOST_CHECK_EQUAL(a,old_a - (-b * c));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(ma));
		}
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_static_integer_addmul_test)
{
	boost::mpl::for_each<size_types>(static_addmul_tester());
}

struct static_lshift1_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef detail::static_integer<T::value> int_type;
		const auto limb_bits = int_type::limb_bits;
		mpz_raii mc, ma, mb;
		int_type n;
		n.lshift1();
		BOOST_CHECK_EQUAL(n,int_type());
		n = int_type(1);
		n.lshift1();
		BOOST_CHECK_EQUAL(n,int_type(2));
		n += int_type(1);
		n.lshift1();
		BOOST_CHECK_EQUAL(n,int_type(6));
		for (typename int_type::limb_t i = 2u; i < limb_bits; ++i) {
			n.lshift1();
		}
		int_type m;
		m.set_bit(limb_bits - 1u);
		m.set_bit(limb_bits);
		BOOST_CHECK_EQUAL(n,m);
		BOOST_CHECK_EQUAL(n._mp_size,2);
		// Random tests.
		std::uniform_int_distribution<int> int_dist(0,1);
		// Half limb.
		for (int i = 0; i < ntries; ++i) {
			::mpz_set_si(&ma.m_mpz,0);
			int_type a;
			for (typename int_type::limb_t i = limb_bits / 2u; i < limb_bits; ++i) {
				if (int_dist(rng)) {
					a.set_bit(i);
					::mpz_setbit(&ma.m_mpz,static_cast< ::mp_bitcnt_t>(i));
				}
			}
			if (int_dist(rng)) {
				::mpz_neg(&ma.m_mpz,&ma.m_mpz);
				a.negate();
			}
			a.lshift1();
			::mpz_mul_2exp(&ma.m_mpz,&ma.m_mpz,1u);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(ma));
		}
		// 1 limb.
		for (int i = 0; i < ntries; ++i) {
			::mpz_set_si(&ma.m_mpz,0);
			int_type a;
			for (typename int_type::limb_t i = 0u; i < limb_bits; ++i) {
				if (int_dist(rng)) {
					a.set_bit(i);
					::mpz_setbit(&ma.m_mpz,static_cast< ::mp_bitcnt_t>(i));
				}
			}
			if (int_dist(rng)) {
				::mpz_neg(&ma.m_mpz,&ma.m_mpz);
				a.negate();
			}
			a.lshift1();
			::mpz_mul_2exp(&ma.m_mpz,&ma.m_mpz,1u);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(ma));
		}
		// 2 limbs.
		for (int i = 0; i < ntries; ++i) {
			::mpz_set_si(&ma.m_mpz,0);
			int_type a;
			for (typename int_type::limb_t i = 0u; i < limb_bits * 2u - 1u; ++i) {
				if (int_dist(rng)) {
					a.set_bit(i);
					::mpz_setbit(&ma.m_mpz,static_cast< ::mp_bitcnt_t>(i));
				}
			}
			if (int_dist(rng)) {
				::mpz_neg(&ma.m_mpz,&ma.m_mpz);
				a.negate();
			}
			a.lshift1();
			::mpz_mul_2exp(&ma.m_mpz,&ma.m_mpz,1u);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(ma));
		}
		// half + half limbs.
		for (int i = 0; i < ntries; ++i) {
			::mpz_set_si(&ma.m_mpz,0);
			int_type a;
			for (typename int_type::limb_t i = limb_bits / 2u; i < limb_bits; ++i) {
				if (int_dist(rng)) {
					a.set_bit(i);
					::mpz_setbit(&ma.m_mpz,static_cast< ::mp_bitcnt_t>(i));
					if (i != limb_bits - 1u) {
						a.set_bit(static_cast<typename int_type::limb_t>(i + limb_bits));
						::mpz_setbit(&ma.m_mpz,static_cast< ::mp_bitcnt_t>(i + limb_bits));
					}
				}
			}
			if (int_dist(rng)) {
				::mpz_neg(&ma.m_mpz,&ma.m_mpz);
				a.negate();
			}
			a.lshift1();
			::mpz_mul_2exp(&ma.m_mpz,&ma.m_mpz,1u);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(ma));
		}
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_static_integer_lshift1_test)
{
	boost::mpl::for_each<size_types>(static_lshift1_tester());
}

struct static_bits_size_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef detail::static_integer<T::value> int_type;
		const auto limb_bits = int_type::limb_bits;
		int_type n;
		BOOST_CHECK_EQUAL(n.bits_size(),0u);
		n.set_bit(0);
		BOOST_CHECK_EQUAL(n.bits_size(),1u);
		n.set_bit(3);
		BOOST_CHECK_EQUAL(n.bits_size(),4u);
		n.set_bit(limb_bits);
		BOOST_CHECK_EQUAL(n.bits_size(),limb_bits + 1u);
		n.set_bit(limb_bits + 3u);
		BOOST_CHECK_EQUAL(n.bits_size(),limb_bits + 4u);
		n.set_bit(2u * limb_bits - 1u);
		BOOST_CHECK_EQUAL(n.bits_size(),2u * limb_bits);
		n.set_bit(2u * limb_bits - 2u);
		BOOST_CHECK_EQUAL(n.bits_size(),2u * limb_bits);
		BOOST_CHECK_EQUAL((n - n).bits_size(),0u);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_static_integer_bits_size_test)
{
	boost::mpl::for_each<size_types>(static_bits_size_tester());
}

struct static_test_bit_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef detail::static_integer<T::value> int_type;
		const auto limb_bits = int_type::limb_bits;
		int_type n;
		BOOST_CHECK_EQUAL(n.test_bit(0u),0u);
		n.set_bit(0u);
		BOOST_CHECK_EQUAL(n.test_bit(0u),1u);
		n.set_bit(2u);
		BOOST_CHECK_EQUAL(n.test_bit(1u),0u);
		BOOST_CHECK_EQUAL(n.test_bit(2u),1u);
		n.set_bit(limb_bits - 1u);
		BOOST_CHECK_EQUAL(n.test_bit(limb_bits - 1u),1u);
		n.set_bit(limb_bits);
		BOOST_CHECK_EQUAL(n.test_bit(limb_bits),1u);
		n.set_bit(limb_bits + 1u);
		BOOST_CHECK_EQUAL(n.test_bit(limb_bits + 1u),1u);
		BOOST_CHECK_EQUAL(n.test_bit(limb_bits + 2u),0u);
		BOOST_CHECK_EQUAL(n.test_bit(2u * limb_bits - 1u),0u);
		n.set_bit(2u * limb_bits - 1u);
		BOOST_CHECK_EQUAL(n.test_bit(2u * limb_bits - 1u),1u);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_static_integer_test_bit_test)
{
	boost::mpl::for_each<size_types>(static_test_bit_tester());
}

struct static_test_div_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef detail::static_integer<T::value> int_type;
		const auto limb_bits = int_type::limb_bits;
		int_type n, m(1), q, r;
		mpz_raii mn, mm, mq, mr;
		int_type::div(q,r,n,m);
		BOOST_CHECK_EQUAL(q,int_type(0));
		BOOST_CHECK_EQUAL(r,int_type(0));
		m = int_type(-12);
		int_type::div(q,r,n,m);
		BOOST_CHECK_EQUAL(q,int_type(0));
		BOOST_CHECK_EQUAL(r,int_type(0));
		n = int_type(1);
		m = int_type(2);
		int_type::div(q,r,n,m);
		BOOST_CHECK_EQUAL(q,int_type(0));
		BOOST_CHECK_EQUAL(r,int_type(1));
		n = int_type(2);
		m = int_type(2);
		int_type::div(q,r,n,m);
		BOOST_CHECK_EQUAL(q,int_type(1));
		BOOST_CHECK_EQUAL(r,int_type(0));
		n = int_type(3);
		m = int_type(2);
		int_type::div(q,r,n,m);
		BOOST_CHECK_EQUAL(q,int_type(1));
		BOOST_CHECK_EQUAL(r,int_type(1));
		n = int_type(4);
		m = int_type(2);
		int_type::div(q,r,n,m);
		BOOST_CHECK_EQUAL(q,int_type(2));
		BOOST_CHECK_EQUAL(r,int_type(0));
		n = int_type(-4);
		m = int_type(-2);
		int_type::div(q,r,n,m);
		BOOST_CHECK_EQUAL(q,int_type(2));
		BOOST_CHECK_EQUAL(r,int_type(0));
		n = int_type(-4);
		m = int_type(2);
		int_type::div(q,r,n,m);
		BOOST_CHECK_EQUAL(q,int_type(-2));
		BOOST_CHECK_EQUAL(r,int_type(0));
		n = int_type(4);
		m = int_type(-2);
		int_type::div(q,r,n,m);
		BOOST_CHECK_EQUAL(q,int_type(-2));
		BOOST_CHECK_EQUAL(r,int_type(0));
		n = int_type(0);
		m = int_type(-3);
		int_type::div(q,r,n,m);
		BOOST_CHECK_EQUAL(q,int_type(0));
		BOOST_CHECK_EQUAL(r,int_type(0 % -3));
		n = int_type(1);
		m = int_type(-3);
		int_type::div(q,r,n,m);
		BOOST_CHECK_EQUAL(q,int_type(0));
		BOOST_CHECK_EQUAL(r,int_type(1 % -3));
		n = int_type(-1);
		m = int_type(3);
		int_type::div(q,r,n,m);
		BOOST_CHECK_EQUAL(q,int_type(0));
		BOOST_CHECK_EQUAL(r,int_type((-1) % 3));
		n = int_type(-4);
		m = int_type(3);
		int_type::div(q,r,n,m);
		BOOST_CHECK_EQUAL(q,int_type(-1));
		BOOST_CHECK_EQUAL(r,int_type((-4) % 3));
		n = int_type(4);
		m = int_type(-3);
		int_type::div(q,r,n,m);
		BOOST_CHECK_EQUAL(q,int_type(-1));
		BOOST_CHECK_EQUAL(r,int_type(4 % -3));
		n = int_type(-6);
		m = int_type(3);
		int_type::div(q,r,n,m);
		BOOST_CHECK_EQUAL(q,int_type(-2));
		BOOST_CHECK_EQUAL(r,int_type((-6) % 3));
		n = int_type(6);
		m = int_type(-3);
		int_type::div(q,r,n,m);
		BOOST_CHECK_EQUAL(q,int_type(-2));
		BOOST_CHECK_EQUAL(r,int_type(6 % -3));
		// Random testing.
		std::uniform_int_distribution<int> int_dist(0,1);
		// 1-1 limbs.
		for (int i = 0; i < ntries; ++i) {
			// Clear out the variables.
			n = int_type();
			m = n;
			::mpz_set_si(&mn.m_mpz,0);
			::mpz_set_si(&mm.m_mpz,0);
			for (typename int_type::limb_t i = 0u; i < limb_bits; ++i) {
				if (int_dist(rng)) {
					n.set_bit(i);
					::mpz_setbit(&mn.m_mpz,static_cast< ::mp_bitcnt_t>(i));
				}
				if (int_dist(rng)) {
					m.set_bit(i);
					::mpz_setbit(&mm.m_mpz,static_cast< ::mp_bitcnt_t>(i));
				}
			}
			if (int_dist(rng)) {
				n.negate();
				::mpz_neg(&mn.m_mpz,&mn.m_mpz);
			}
			if (int_dist(rng)) {
				m.negate();
				::mpz_neg(&mm.m_mpz,&mm.m_mpz);
			}
			if (m.is_zero()) {
				continue;
			}
			::mpz_tdiv_qr(&mq.m_mpz,&mr.m_mpz,&mn.m_mpz,&mm.m_mpz);
			int_type::div(q,r,n,m);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(q),mpz_lexcast(mq));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r),mpz_lexcast(mr));
			// Do it with overlapping q and r.
			int_type::div(n,m,n,m);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),mpz_lexcast(mq));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(m),mpz_lexcast(mr));
		}
		// 1-2 limbs.
		for (int i = 0; i < ntries; ++i) {
			// Clear out the variables.
			n = int_type();
			m = n;
			::mpz_set_si(&mn.m_mpz,0);
			::mpz_set_si(&mm.m_mpz,0);
			for (typename int_type::limb_t i = 0u; i < limb_bits; ++i) {
				if (int_dist(rng)) {
					n.set_bit(i);
					::mpz_setbit(&mn.m_mpz,static_cast< ::mp_bitcnt_t>(i));
				}
			}
			for (typename int_type::limb_t i = 0u; i < limb_bits * 2u; ++i) {
				if (int_dist(rng)) {
					m.set_bit(i);
					::mpz_setbit(&mm.m_mpz,static_cast< ::mp_bitcnt_t>(i));
				}
			}
			if (int_dist(rng)) {
				n.negate();
				::mpz_neg(&mn.m_mpz,&mn.m_mpz);
			}
			if (int_dist(rng)) {
				m.negate();
				::mpz_neg(&mm.m_mpz,&mm.m_mpz);
			}
			if (m.is_zero()) {
				continue;
			}
			::mpz_tdiv_qr(&mq.m_mpz,&mr.m_mpz,&mn.m_mpz,&mm.m_mpz);
			int_type::div(q,r,n,m);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(q),mpz_lexcast(mq));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r),mpz_lexcast(mr));
			// Do it with overlapping q and r.
			int_type::div(n,m,n,m);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),mpz_lexcast(mq));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(m),mpz_lexcast(mr));
		}
		// 2-1 limbs.
		for (int i = 0; i < ntries; ++i) {
			// Clear out the variables.
			n = int_type();
			m = n;
			::mpz_set_si(&mn.m_mpz,0);
			::mpz_set_si(&mm.m_mpz,0);
			for (typename int_type::limb_t i = 0u; i < limb_bits * 2u; ++i) {
				if (int_dist(rng)) {
					n.set_bit(i);
					::mpz_setbit(&mn.m_mpz,static_cast< ::mp_bitcnt_t>(i));
				}
			}
			for (typename int_type::limb_t i = 0u; i < limb_bits; ++i) {
				if (int_dist(rng)) {
					m.set_bit(i);
					::mpz_setbit(&mm.m_mpz,static_cast< ::mp_bitcnt_t>(i));
				}
			}
			if (int_dist(rng)) {
				n.negate();
				::mpz_neg(&mn.m_mpz,&mn.m_mpz);
			}
			if (int_dist(rng)) {
				m.negate();
				::mpz_neg(&mm.m_mpz,&mm.m_mpz);
			}
			if (m.is_zero()) {
				continue;
			}
			::mpz_tdiv_qr(&mq.m_mpz,&mr.m_mpz,&mn.m_mpz,&mm.m_mpz);
			int_type::div(q,r,n,m);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(q),mpz_lexcast(mq));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r),mpz_lexcast(mr));
			// Do it with overlapping q and r.
			int_type::div(n,m,n,m);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),mpz_lexcast(mq));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(m),mpz_lexcast(mr));
		}
		// 2-2 limbs.
		for (int i = 0; i < ntries; ++i) {
			// Clear out the variables.
			n = int_type();
			m = n;
			::mpz_set_si(&mn.m_mpz,0);
			::mpz_set_si(&mm.m_mpz,0);
			for (typename int_type::limb_t i = 0u; i < limb_bits * 2u; ++i) {
				if (int_dist(rng)) {
					n.set_bit(i);
					::mpz_setbit(&mn.m_mpz,static_cast< ::mp_bitcnt_t>(i));
				}
				if (int_dist(rng)) {
					m.set_bit(i);
					::mpz_setbit(&mm.m_mpz,static_cast< ::mp_bitcnt_t>(i));
				}
			}
			if (int_dist(rng)) {
				n.negate();
				::mpz_neg(&mn.m_mpz,&mn.m_mpz);
			}
			if (int_dist(rng)) {
				m.negate();
				::mpz_neg(&mm.m_mpz,&mm.m_mpz);
			}
			if (m.is_zero()) {
				continue;
			}
			::mpz_tdiv_qr(&mq.m_mpz,&mr.m_mpz,&mn.m_mpz,&mm.m_mpz);
			int_type::div(q,r,n,m);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(q),mpz_lexcast(mq));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r),mpz_lexcast(mr));
			// Do it with overlapping q and r.
			int_type::div(n,m,n,m);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),mpz_lexcast(mq));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(m),mpz_lexcast(mr));
		}
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_static_integer_division_test)
{
	boost::mpl::for_each<size_types>(static_test_div_tester());
}

struct union_ctor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef detail::integer_union<T::value> int_type;
		const auto limb_bits = int_type::s_storage::limb_bits;
		int_type n;
		BOOST_CHECK(n.is_static());
		n.promote();
		BOOST_CHECK(!n.is_static());
		BOOST_CHECK(n.g_dy()._mp_alloc > 0);
		BOOST_CHECK(n.g_dy()._mp_d != nullptr);
		// Copy ctor tests.
		int_type n1;
		n1.g_st().set_bit(1u);
		BOOST_CHECK(n1.is_static());
		// From S.
		int_type n2(n1);
		BOOST_CHECK(n2.is_static());
		BOOST_CHECK_EQUAL(n2.g_st().test_bit(1u),1u);
		// From D.
		n1.g_st().set_bit(limb_bits);
		n1.promote();
		BOOST_CHECK(!n1.is_static());
		int_type n3(n1);
		BOOST_CHECK(!n3.is_static());
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n3.g_dy(),1u),1);
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n3.g_dy(),limb_bits),1);
		// Move ctor tests.
		int_type n1a;
		n1a.g_st().set_bit(1u);
		BOOST_CHECK(n1a.is_static());
		// From S.
		int_type n2a(std::move(n1a));
		BOOST_CHECK(n2a.is_static());
		BOOST_CHECK_EQUAL(n2a.g_st().test_bit(1u),1u);
		BOOST_CHECK(n1a.is_static());
		BOOST_CHECK_EQUAL(n1a.g_st().test_bit(1u),1u);
		// From D.
		n1a.g_st().set_bit(limb_bits);
		n1a.promote();
		BOOST_CHECK(!n1a.is_static());
		int_type n3a(std::move(n1a));
		BOOST_CHECK(!n3a.is_static());
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n3a.g_dy(),1u),1);
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n3a.g_dy(),limb_bits),1);
		BOOST_CHECK(n1a.is_static());
		BOOST_CHECK_EQUAL(n1a.g_st(),typename int_type::s_storage());
		// Copy assignment tests.
		int_type n4, n5, n6;
		n4.g_st().set_bit(4u);
		// Self assignment.
		n4 = n4;
		BOOST_CHECK(n4.is_static());
		BOOST_CHECK_EQUAL(n4.g_st().test_bit(4u),1u);
		// S vs S.
		n5 = n4;
		BOOST_CHECK(n5.is_static());
		BOOST_CHECK_EQUAL(n5.g_st().test_bit(4u),1u);
		// S vs D.
		n4.g_st().set_bit(limb_bits);
		n4.promote();
		n5 = n4;
		BOOST_CHECK(!n5.is_static());
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n5.g_dy(),4u),1);
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n5.g_dy(),limb_bits),1);
		// D vs S.
		n6.g_st().set_bit(2u);
		n5 = n6;
		BOOST_CHECK(!n5.is_static());
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n5.g_dy(),2u),1);
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n5.g_dy(),4u),0);
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n5.g_dy(),limb_bits),0);
		// D vs D.
		n5 = n4;
		BOOST_CHECK(!n5.is_static());
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n5.g_dy(),2u),0);
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n5.g_dy(),4u),1);
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n5.g_dy(),limb_bits),1);
		// Move assignment tests.
		int_type n4a, n5a, n6a;
		n4a.g_st().set_bit(4u);
		// Self assignment.
		n4a = std::move(n4a);
		BOOST_CHECK(n4a.is_static());
		BOOST_CHECK_EQUAL(n4a.g_st().test_bit(4u),1u);
		// S vs S.
		n5a = std::move(n4a);
		BOOST_CHECK(n5a.is_static());
		BOOST_CHECK_EQUAL(n5a.g_st().test_bit(4u),1u);
		BOOST_CHECK(n4a.is_static());
		BOOST_CHECK_EQUAL(n4a.g_st().test_bit(4u),1u);
		// S vs D.
		n4a.g_st().set_bit(limb_bits);
		n4a.promote();
		n5a = std::move(n4a);
		BOOST_CHECK(!n5a.is_static());
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n5a.g_dy(),4u),1);
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n5a.g_dy(),limb_bits),1);
		BOOST_CHECK(n4a.is_static());
		BOOST_CHECK_EQUAL(n4a.g_st(),typename int_type::s_storage());
		// D vs S.
		n6a.g_st().set_bit(2u);
		n5a = std::move(n6a);
		BOOST_CHECK(!n5a.is_static());
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n5a.g_dy(),2u),1);
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n5a.g_dy(),4u),0);
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n5a.g_dy(),limb_bits),0);
		BOOST_CHECK(!n6a.is_static());
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n6a.g_dy(),4u),1);
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n6a.g_dy(),limb_bits),1);
		// D vs D.
		::mpz_setbit(&n6a.g_dy(),limb_bits + 1u);
		n5a = std::move(n6a);
		BOOST_CHECK(!n5a.is_static());
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n5a.g_dy(),4u),1);
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n5a.g_dy(),limb_bits),1);
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n5a.g_dy(),limb_bits + 1u),1);
		BOOST_CHECK(!n6a.is_static());
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n6a.g_dy(),2u),1);
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n6a.g_dy(),4u),0);
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n6a.g_dy(),limb_bits),0);
		// NOTE: this is here only for historical reasons, moved-from objects
		// are not special any more.
		// Check if reviving moved-from objects works.
		// Need to check only when the first operand is dynamic.
		// Copy-assignment revive.
		BOOST_CHECK(!n5a.is_static());
		int_type n7(std::move(n5a));
		BOOST_CHECK(n5a.is_static());
		n5a = n7;
		BOOST_CHECK(!n5a.is_static());
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n5a.g_dy(),4u),1);
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n5a.g_dy(),limb_bits),1);
		BOOST_CHECK_EQUAL(::mpz_tstbit(&n5a.g_dy(),limb_bits + 1u),1);
		int_type n8;
		n8.g_st().set_bit(3u);
		int_type n7a(std::move(n5a));
		(void)n7a;
		BOOST_CHECK(n5a.is_static());
		n5a = n8;
		BOOST_CHECK(n5a.is_static());
		// Move-assignment revive.
		int_type n7b(std::move(n5a));
		(void)n7b;
		BOOST_CHECK(n8.is_static());
		BOOST_CHECK(n5a.is_static());
		n5a = std::move(n8);
		BOOST_CHECK(n5a.is_static());
		BOOST_CHECK(n8.is_static());
		// n8 now can be revived,
		n8 = std::move(n5a);
		BOOST_CHECK(n8.is_static());
		BOOST_CHECK(n5a.is_static());
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_integer_union_ctor_test)
{
	boost::mpl::for_each<size_types>(union_ctor_tester());
}

struct fits_in_static_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef detail::integer_union<T::value> int_type;
		const auto limb_bits = int_type::s_storage::limb_bits;
		mpz_raii mpz;
		::mpz_setbit(&mpz.m_mpz,0);
		BOOST_CHECK(int_type::fits_in_static(mpz.m_mpz));
		::mpz_setbit(&mpz.m_mpz,1);
		BOOST_CHECK(int_type::fits_in_static(mpz.m_mpz));
		::mpz_setbit(&mpz.m_mpz,static_cast< ::mp_bitcnt_t>(limb_bits));
		BOOST_CHECK(int_type::fits_in_static(mpz.m_mpz));
		::mpz_setbit(&mpz.m_mpz,static_cast< ::mp_bitcnt_t>(limb_bits + 1));
		BOOST_CHECK(int_type::fits_in_static(mpz.m_mpz));
		::mpz_setbit(&mpz.m_mpz,static_cast< ::mp_bitcnt_t>(2u * limb_bits - 1u));
		BOOST_CHECK(int_type::fits_in_static(mpz.m_mpz));
		::mpz_setbit(&mpz.m_mpz,static_cast< ::mp_bitcnt_t>(2u * limb_bits));
		BOOST_CHECK(!int_type::fits_in_static(mpz.m_mpz));
		::mpz_setbit(&mpz.m_mpz,static_cast< ::mp_bitcnt_t>(2u * limb_bits - 2u));
		BOOST_CHECK(!int_type::fits_in_static(mpz.m_mpz));
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_integer_union_fits_in_static_test)
{
	boost::mpl::for_each<size_types>(fits_in_static_tester());
}

// Maximum exponent n such that radix**n is representable by long long.
static inline int get_max_exp(int radix)
{
	int retval(0);
	long long tmp(1);
	while (tmp < boost::integer_traits<long long>::const_max / radix) {
		tmp *= radix;
		++retval;
	}
	return retval;
}

struct float_ctor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		mpz_raii m;
		int_type n1;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(0),boost::lexical_cast<std::string>(n1));
		n1.promote();
		BOOST_CHECK_THROW(n1.promote(),std::invalid_argument);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(0),boost::lexical_cast<std::string>(n1));
		std::uniform_int_distribution<int> sign_dist(0,1);
		// NOTE: the idea here is that we are going to multiply by radix**n, and we need to make sure both long long and the FP
		// type can represent the result.
		std::uniform_int_distribution<int> exp_dist_d(0,std::min(get_max_exp(std::numeric_limits<double>::radix),
			std::numeric_limits<double>::max_exponent));
		std::uniform_real_distribution<double> urd_d(0,1);
		for (int i = 0; i < ntries; ++i) {
			auto tmp = urd_d(rng);
			if (sign_dist(rng)) {
				tmp = -tmp;
			}
			tmp = std::scalbn(tmp,exp_dist_d(rng));
			::mpz_set_d(&m.m_mpz,tmp);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(int_type(tmp)),boost::lexical_cast<std::string>(static_cast<long long>(tmp)));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(int_type(tmp)),mpz_lexcast(m));
		}
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(int_type(0.)),boost::lexical_cast<std::string>(0));
		BOOST_CHECK(int_type{0.}.is_static());
		if (std::numeric_limits<double>::has_infinity && std::numeric_limits<double>::has_quiet_NaN) {
			BOOST_CHECK_THROW(int_type{std::numeric_limits<double>::infinity()},std::invalid_argument);
			BOOST_CHECK_THROW(int_type{std::numeric_limits<double>::quiet_NaN()},std::invalid_argument);
		}
		// Long double.
		std::uniform_int_distribution<int> exp_dist_ld(0,std::min(get_max_exp(std::numeric_limits<long double>::radix),
			std::numeric_limits<long double>::max_exponent));
		std::uniform_real_distribution<long double> urd_ld(0,1);
		for (int i = 0; i < ntries; ++i) {
			auto tmp = urd_ld(rng);
			if (sign_dist(rng)) {
				tmp = -tmp;
			}
			tmp = std::scalbn(tmp,exp_dist_ld(rng));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(int_type(tmp)),boost::lexical_cast<std::string>(static_cast<long long>(tmp)));
		}
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(int_type(0.l)),boost::lexical_cast<std::string>(0));
		BOOST_CHECK(int_type{0.l}.is_static());
		if (std::numeric_limits<long double>::has_infinity && std::numeric_limits<long double>::has_quiet_NaN) {
			BOOST_CHECK_THROW(int_type{std::numeric_limits<long double>::infinity()},std::invalid_argument);
			BOOST_CHECK_THROW(int_type{std::numeric_limits<long double>::quiet_NaN()},std::invalid_argument);
		}
		// Float.
		std::uniform_int_distribution<int> exp_dist_f(0,std::min(get_max_exp(std::numeric_limits<float>::radix),
			std::numeric_limits<float>::max_exponent));
		std::uniform_real_distribution<float> urd_f(0,1);
		for (int i = 0; i < ntries; ++i) {
			auto tmp = urd_f(rng);
			if (sign_dist(rng)) {
				tmp = -tmp;
			}
			tmp = std::scalbn(tmp,exp_dist_f(rng));
			::mpz_set_d(&m.m_mpz,tmp);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(int_type(tmp)),boost::lexical_cast<std::string>(static_cast<long long>(tmp)));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(int_type(tmp)),mpz_lexcast(m));
		}
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(int_type(0.f)),boost::lexical_cast<std::string>(0));
		BOOST_CHECK(int_type{0.f}.is_static());
		if (std::numeric_limits<float>::has_infinity && std::numeric_limits<float>::has_quiet_NaN) {
			BOOST_CHECK_THROW(int_type{std::numeric_limits<float>::infinity()},std::invalid_argument);
			BOOST_CHECK_THROW(int_type{std::numeric_limits<float>::quiet_NaN()},std::invalid_argument);
		}
		// Test with some exact integers.
		BOOST_CHECK_EQUAL(int_type{41.},41);
		BOOST_CHECK_EQUAL(int_type{-42.},-42);
		BOOST_CHECK_EQUAL(int_type{43.},43);
		BOOST_CHECK_EQUAL(int_type{41.f},41);
		BOOST_CHECK_EQUAL(int_type{-42.f},-42);
		BOOST_CHECK_EQUAL(int_type{43.f},43);
		BOOST_CHECK_EQUAL(int_type{41.l},41);
		BOOST_CHECK_EQUAL(int_type{-42.l},-42);
		BOOST_CHECK_EQUAL(int_type{43.l},43);
	}
};

struct integral_ctor_tester
{
	template <typename U>
	struct runner
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			std::uniform_int_distribution<T> int_dist(std::numeric_limits<T>::lowest(),std::numeric_limits<T>::max());
			std::ostringstream oss;
			for (int i = 0; i < ntries; ++i) {
				auto tmp = int_dist(rng);
				if (std::is_signed<T>::value) {
					oss << static_cast<long long>(tmp);
				} else {
					oss << static_cast<unsigned long long>(tmp);
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(int_type(tmp)),oss.str());
				oss.str("");
			}
			oss << static_cast<typename std::conditional<std::is_signed<T>::value,long long, unsigned long long>::type>
				(std::numeric_limits<T>::lowest());
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(int_type(std::numeric_limits<T>::lowest())),oss.str());
			oss.str("");
			oss << static_cast<typename std::conditional<std::is_signed<T>::value,long long, unsigned long long>::type>
				(std::numeric_limits<T>::max());
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(int_type(std::numeric_limits<T>::max())),oss.str());
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		boost::mpl::for_each<integral_types>(runner<T>());
		// Special casing for bool.
		int_type t(true);
		BOOST_CHECK(t.is_static());
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(t),"1");
		int_type f(false);
		BOOST_CHECK(f.is_static());
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(f),"0");
	}
};

struct str_ctor_tester
{
	template <typename U>
	struct runner
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			std::uniform_int_distribution<T> int_dist(std::numeric_limits<T>::lowest(),std::numeric_limits<T>::max());
			std::ostringstream oss;
			for (int i = 0; i < ntries; ++i) {
				auto tmp = int_dist(rng);
				if (std::is_signed<T>::value) {
					oss << static_cast<long long>(tmp);
				} else {
					oss << static_cast<unsigned long long>(tmp);
				}
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(int_type(oss.str().c_str())),oss.str());
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(int_type(oss.str())),oss.str());
				oss.str("");
			}
			oss << static_cast<typename std::conditional<std::is_signed<T>::value,long long, unsigned long long>::type>
				(std::numeric_limits<T>::lowest());
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(int_type(oss.str().c_str())),oss.str());
			oss.str("");
			oss << static_cast<typename std::conditional<std::is_signed<T>::value,long long, unsigned long long>::type>
				(std::numeric_limits<T>::max());
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(int_type(oss.str().c_str())),oss.str());
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		// Random testing.
		boost::mpl::for_each<integral_types>(runner<T>());
		// Well- and mal- formed strings.
		BOOST_CHECK_EQUAL("123",boost::lexical_cast<std::string>(int_type("123")));
		BOOST_CHECK_EQUAL("-123",boost::lexical_cast<std::string>(int_type("-123")));
		const std::vector<std::string> invalid_strings{"-0","+0","01","+1","+01","-01","123f"," 123","123 ","123.56","-","+",
			""," +0"," -0","-123 ","12a","-12a"};
		for (const std::string &s: invalid_strings) {
			BOOST_CHECK_THROW(int_type{s},std::invalid_argument);
		}
	}
};

struct generic_assignment_tester
{
	template <typename U>
	struct runner
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			std::uniform_int_distribution<T> int_dist(std::numeric_limits<T>::lowest(),std::numeric_limits<T>::max());
			std::ostringstream oss;
			int_type n;
			for (int i = 0; i < ntries; ++i) {
				auto tmp = int_dist(rng);
				if (std::is_signed<T>::value) {
					oss << static_cast<long long>(tmp);
				} else {
					oss << static_cast<unsigned long long>(tmp);
				}
				n = tmp;
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),oss.str());
				oss.str("");
			}
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		boost::mpl::for_each<integral_types>(runner<T>());
		int_type n;
		// Special casing for bool.
		n = true;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"1");
		n = false;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"0");
		// Some tests for floats.
		n = 1.0f;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"1");
		if (std::numeric_limits<float>::has_infinity && std::numeric_limits<float>::has_quiet_NaN) {
			BOOST_CHECK_THROW((n = std::numeric_limits<float>::infinity()),std::invalid_argument);
			BOOST_CHECK_THROW((n = std::numeric_limits<float>::quiet_NaN()),std::invalid_argument);
		}
		n = -2.0;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"-2");
		if (std::numeric_limits<double>::has_infinity && std::numeric_limits<double>::has_quiet_NaN) {
			BOOST_CHECK_THROW((n = std::numeric_limits<double>::infinity()),std::invalid_argument);
			BOOST_CHECK_THROW((n = std::numeric_limits<double>::quiet_NaN()),std::invalid_argument);
		}
		n = 3.0L;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"3");
		if (std::numeric_limits<long double>::has_infinity && std::numeric_limits<long double>::has_quiet_NaN) {
			BOOST_CHECK_THROW((n = std::numeric_limits<long double>::infinity()),std::invalid_argument);
			BOOST_CHECK_THROW((n = std::numeric_limits<long double>::quiet_NaN()),std::invalid_argument);
		}
	}
};

struct str_assignment_tester
{
	template <typename U>
	struct runner
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			std::uniform_int_distribution<T> int_dist(std::numeric_limits<T>::lowest(),std::numeric_limits<T>::max());
			std::ostringstream oss;
			int_type n;
			for (int i = 0; i < ntries; ++i) {
				auto tmp = int_dist(rng);
				if (std::is_signed<T>::value) {
					oss << static_cast<long long>(tmp);
				} else {
					oss << static_cast<unsigned long long>(tmp);
				}
				n = oss.str();
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),oss.str());
				n = oss.str().c_str();
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),oss.str());
				oss.str("");
			}
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		// Random testing.
		boost::mpl::for_each<integral_types>(runner<T>());
		// Well- and mal- formed strings.
		int_type n;
		n = "123";
		BOOST_CHECK_EQUAL("123",boost::lexical_cast<std::string>(n));
		n = "-123";
		BOOST_CHECK_EQUAL("-123",boost::lexical_cast<std::string>(n));
		const std::vector<std::string> invalid_strings{"-0","+0","01","+1","+01","-01","123f"," 123","123 ","123.56","-","+",
			""," +0"," -0","-123 ","12a","-12a"};
		for (const std::string &s: invalid_strings) {
			BOOST_CHECK_THROW((n = s),std::invalid_argument);
		}
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_ctor_assign_test)
{
	boost::mpl::for_each<size_types>(float_ctor_tester());
	boost::mpl::for_each<size_types>(integral_ctor_tester());
	boost::mpl::for_each<size_types>(str_ctor_tester());
	boost::mpl::for_each<size_types>(generic_assignment_tester());
	boost::mpl::for_each<size_types>(str_assignment_tester());
}

struct integral_conversion_tester
{
	template <typename U>
	struct runner
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			BOOST_CHECK_EQUAL(T(0),static_cast<T>(int_type{}));
			BOOST_CHECK_EQUAL(std::numeric_limits<T>::max(),static_cast<T>(int_type{std::numeric_limits<T>::max()}));
			BOOST_CHECK_EQUAL(std::numeric_limits<T>::min(),static_cast<T>(int_type{std::numeric_limits<T>::min()}));
			BOOST_CHECK_EQUAL(std::numeric_limits<T>::max() - T(1),static_cast<T>(int_type{std::numeric_limits<T>::max() - T(1)}));
			BOOST_CHECK_EQUAL(std::numeric_limits<T>::min() + T(1),static_cast<T>(int_type{std::numeric_limits<T>::min() + T(1)}));
			BOOST_CHECK_EQUAL(std::numeric_limits<T>::max() - T(2),static_cast<T>(int_type{std::numeric_limits<T>::max() - T(2)}));
			BOOST_CHECK_EQUAL(std::numeric_limits<T>::min() + T(2),static_cast<T>(int_type{std::numeric_limits<T>::min() + T(2)}));
			mpz_raii tmp;
			std::ostringstream oss;
			oss << static_cast<typename std::conditional<std::is_signed<T>::value,long long, unsigned long long>::type>
				(std::numeric_limits<T>::max());
			::mpz_set_str(&tmp.m_mpz,oss.str().c_str(),10);
			::mpz_add_ui(&tmp.m_mpz,&tmp.m_mpz,1ul);
			BOOST_CHECK_THROW((void)static_cast<T>(int_type{mpz_lexcast(tmp)}),std::overflow_error);
			::mpz_add_ui(&tmp.m_mpz,&tmp.m_mpz,1ul);
			BOOST_CHECK_THROW((void)static_cast<T>(int_type{mpz_lexcast(tmp)}),std::overflow_error);
			::mpz_sub_ui(&tmp.m_mpz,&tmp.m_mpz,2ul);
			BOOST_CHECK_EQUAL(static_cast<T>(int_type{mpz_lexcast(tmp)}),std::numeric_limits<T>::max());
			oss.str("");
			oss << static_cast<typename std::conditional<std::is_signed<T>::value,long long, unsigned long long>::type>
				(std::numeric_limits<T>::min());
			::mpz_set_str(&tmp.m_mpz,oss.str().c_str(),10);
			::mpz_sub_ui(&tmp.m_mpz,&tmp.m_mpz,1ul);
			BOOST_CHECK_THROW((void)static_cast<T>(int_type{mpz_lexcast(tmp)}),std::overflow_error);
			::mpz_sub_ui(&tmp.m_mpz,&tmp.m_mpz,1ul);
			BOOST_CHECK_THROW((void)static_cast<T>(int_type{mpz_lexcast(tmp)}),std::overflow_error);
			::mpz_add_ui(&tmp.m_mpz,&tmp.m_mpz,2ul);
			BOOST_CHECK_EQUAL(static_cast<T>(int_type{mpz_lexcast(tmp)}),std::numeric_limits<T>::min());
			// Random testing.
			std::uniform_int_distribution<T> int_dist(std::numeric_limits<T>::lowest(),std::numeric_limits<T>::max());
			for (int i = 0; i < ntries; ++i) {
				auto tmp = int_dist(rng);
				BOOST_CHECK_EQUAL(tmp,static_cast<T>(int_type{tmp}));
			}
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		boost::mpl::for_each<integral_types>(runner<T>());
		// Special casing for bool.
		BOOST_CHECK_EQUAL(true,static_cast<bool>(int_type{1}));
		BOOST_CHECK_EQUAL(true,static_cast<bool>(int_type{-1}));
		BOOST_CHECK_EQUAL(true,static_cast<bool>(int_type{-2}));
		BOOST_CHECK_EQUAL(false,static_cast<bool>(int_type{0}));
	}
};

struct float_conversion_tester
{
	template <typename U>
	struct runner
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			const auto radix = std::numeric_limits<T>::radix;
			BOOST_CHECK_EQUAL(T(0),static_cast<T>(int_type{}));
			BOOST_CHECK_EQUAL(T(1),static_cast<T>(int_type{1}));
			BOOST_CHECK_EQUAL(T(-1),static_cast<T>(int_type{-1}));
			BOOST_CHECK_EQUAL(T(2),static_cast<T>(int_type{2}));
			BOOST_CHECK_EQUAL(T(-2),static_cast<T>(int_type{-2}));
			// NOTE: hopefully the radix value is not insane here and we can take negative and +-1.
			BOOST_CHECK_EQUAL(T(radix),static_cast<T>(int_type{radix}));
			BOOST_CHECK_EQUAL(T(-radix),static_cast<T>(int_type{-radix}));
			BOOST_CHECK_EQUAL(T(radix + 1),static_cast<T>(int_type{radix + 1}));
			BOOST_CHECK_EQUAL(T(-radix - 1),static_cast<T>(int_type{-radix - 1}));
			BOOST_CHECK_EQUAL(std::numeric_limits<T>::max(),static_cast<T>(int_type{std::numeric_limits<T>::max()}));
			BOOST_CHECK_EQUAL(std::numeric_limits<T>::lowest(),static_cast<T>(int_type{std::numeric_limits<T>::lowest()}));
			// Random testing.
			std::uniform_int_distribution<int> sign_dist(0,1);
			std::uniform_int_distribution<int> exp_dist(0,std::min(get_max_exp(std::numeric_limits<T>::radix),
				std::numeric_limits<T>::max_exponent));
			std::uniform_real_distribution<T> urd(0,1);
			for (int i = 0; i < ntries; ++i) {
				auto tmp = urd(rng);
				if (sign_dist(rng)) {
					tmp = -tmp;
				}
				tmp = std::scalbn(tmp,exp_dist(rng));
				BOOST_CHECK_EQUAL(std::trunc(tmp),static_cast<T>(int_type{tmp}));
			}
			if (std::numeric_limits<T>::has_infinity) {
				mpz_raii tmp;
				::mpz_set_str(&tmp.m_mpz,boost::lexical_cast<std::string>(int_type{std::numeric_limits<T>::max()}).c_str(),10);
				::mpz_mul_si(&tmp.m_mpz,&tmp.m_mpz,radix);
				BOOST_CHECK_EQUAL(std::numeric_limits<T>::infinity(),static_cast<T>(int_type{mpz_lexcast(tmp)}));
				::mpz_neg(&tmp.m_mpz,&tmp.m_mpz);
				BOOST_CHECK_EQUAL(std::copysign(std::numeric_limits<T>::infinity(),std::numeric_limits<T>::lowest()),
					static_cast<T>(int_type{mpz_lexcast(tmp)}));
			}
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<floating_point_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_conversion_test)
{
	boost::mpl::for_each<size_types>(integral_conversion_tester());
	boost::mpl::for_each<size_types>(float_conversion_tester());
}

struct negate_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		int_type n;
		BOOST_CHECK(n.is_static());
		n.negate();
		BOOST_CHECK(n.is_static());
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"0");
		n = 11;
		n.negate();
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"-11");
		n.negate();
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"11");
		// Random testing.
		std::uniform_int_distribution<int> promote_dist(0,1);
		std::uniform_int_distribution<int> int_dist(boost::integer_traits<int>::const_min,boost::integer_traits<int>::const_max);
		for (int i = 0; i < ntries; ++i) {
			auto tmp = int_dist(rng);
			int_type tmp_int(tmp);
			if (promote_dist(rng) == 1 && tmp_int.is_static()) {
				tmp_int.promote();
			}
			if (tmp < 0) {
				tmp_int.negate();
				std::string tmp_str = boost::lexical_cast<std::string>(tmp);
				tmp_str = std::string(tmp_str.begin() + 1,tmp_str.end());
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(tmp_int),tmp_str);
				tmp_int.negate();
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(tmp_int),boost::lexical_cast<std::string>(tmp));
			} else if (tmp > 0) {
				tmp_int.negate();
				std::string tmp_str = boost::lexical_cast<std::string>(tmp);
				tmp_str = std::string("-") + tmp_str;
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(tmp_int),tmp_str);
				tmp_int.negate();
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(tmp_int),boost::lexical_cast<std::string>(tmp));
			}
		}
		// Function overload.
		BOOST_CHECK(has_negate<int_type>::value);
		BOOST_CHECK(has_negate<int_type &>::value);
		BOOST_CHECK(has_negate<int_type &&>::value);
		BOOST_CHECK(!has_negate<const int_type>::value);
		BOOST_CHECK(!has_negate<const int_type &>::value);
		math::negate(n);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"-11");
		n = 0;
		math::negate(n);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"0");
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_negate_test)
{
	boost::mpl::for_each<size_types>(negate_tester());
}

struct sign_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		int_type n;
		BOOST_CHECK_EQUAL(n.sign(),0);
		n = 1;
		BOOST_CHECK_EQUAL(n.sign(),1);
		n = 101;
		BOOST_CHECK_EQUAL(n.sign(),1);
		n = -1;
		BOOST_CHECK_EQUAL(n.sign(),-1);
		n = -101;
		BOOST_CHECK_EQUAL(n.sign(),-1);
		n.promote();
		n = 0;
		BOOST_CHECK_EQUAL(n.sign(),0);
		n = 1;
		BOOST_CHECK_EQUAL(n.sign(),1);
		n = 101;
		BOOST_CHECK_EQUAL(n.sign(),1);
		n = -1;
		BOOST_CHECK_EQUAL(n.sign(),-1);
		n = -101;
		BOOST_CHECK_EQUAL(n.sign(),-1);
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_sign_test)
{
	boost::mpl::for_each<size_types>(sign_tester());
}

struct is_zero_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		BOOST_CHECK(has_is_zero<int_type>::value);
		BOOST_CHECK(has_is_zero<const int_type>::value);
		BOOST_CHECK(has_is_zero<int_type &>::value);
		BOOST_CHECK(has_is_zero<const int_type &>::value);
		int_type n;
		BOOST_CHECK(math::is_zero(n));
		n = 1;
		BOOST_CHECK(!math::is_zero(n));
		n = 101;
		BOOST_CHECK(!math::is_zero(n));
		n = -1;
		BOOST_CHECK(!math::is_zero(n));
		n = -101;
		BOOST_CHECK(!math::is_zero(n));
		n = 0;
		n.promote();
		BOOST_CHECK(math::is_zero(n));
		n = 1;
		BOOST_CHECK(!math::is_zero(n));
		n = 101;
		BOOST_CHECK(!math::is_zero(n));
		n = -1;
		BOOST_CHECK(!math::is_zero(n));
		n = -101;
		BOOST_CHECK(!math::is_zero(n));
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_is_zero_test)
{
	boost::mpl::for_each<size_types>(is_zero_tester());
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

struct in_place_mp_integer_add_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		BOOST_CHECK(is_addable_in_place<int_type>::value);
		BOOST_CHECK((!is_addable_in_place<const int_type,int_type>::value));
		int_type a, b;
		a += b;
		BOOST_CHECK((std::is_same<decltype(a += b),int_type &>::value));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),boost::lexical_cast<std::string>(int_type{0}));
		BOOST_CHECK(a.is_static());
		a = int_type{"1"};
		b = a;
		a += b;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),boost::lexical_cast<std::string>(int_type{2}));
		BOOST_CHECK(a.is_static());
		a = int_type{"1"};
		b = int_type{"-1"};
		a += b;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),boost::lexical_cast<std::string>(int_type{0}));
		BOOST_CHECK(a.is_static());
		a = int_type{"-1"};
		b = int_type{"-1"};
		a += b;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),boost::lexical_cast<std::string>(int_type{-2}));
		BOOST_CHECK(a.is_static());
		// Random testing.
		std::uniform_int_distribution<int> promote_dist(0,1);
		std::uniform_int_distribution<int> int_dist(boost::integer_traits<int>::const_min,boost::integer_traits<int>::const_max);
		mpz_raii m_a, m_b;
		for (int i = 0; i < ntries; ++i) {
			auto tmp1 = int_dist(rng), tmp2 = int_dist(rng);
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
			a += b;
			::mpz_add(&m_a.m_mpz,&m_a.m_mpz,&m_b.m_mpz);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(m_a));
		}
		// Check when static add fails.
		a = int_type{"67"};
		b = int_type{"15"};
		BOOST_CHECK(a.is_static());
		BOOST_CHECK(b.is_static());
		using limb_t = typename detail::integer_union<T::value>::s_storage::limb_t;
		auto &st_a = get_m(a).g_st(), &st_b = get_m(b).g_st();
		st_a.set_bit(static_cast<limb_t>(st_a.limb_bits * 2u - 1u));
		st_b.set_bit(static_cast<limb_t>(st_a.limb_bits * 2u - 1u));
		::mpz_set_str(&m_a.m_mpz,boost::lexical_cast<std::string>(a).c_str(),10);
		::mpz_set_str(&m_b.m_mpz,boost::lexical_cast<std::string>(b).c_str(),10);
		a += b;
		::mpz_add(&m_a.m_mpz,&m_a.m_mpz,&m_b.m_mpz);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(m_a));
		// Promotion bug.
		int_type c;
		mpz_raii m_c;
		::mpz_setbit(&m_c.m_mpz,static_cast< ::mp_bitcnt_t>(get_m(c).g_st().limb_bits * 2u - 1u));
		BOOST_CHECK(c.is_static());
		get_m(c).g_st().set_bit(static_cast<limb_t>(get_m(c).g_st().limb_bits * 2u - 1u));
		c += c;
		::mpz_add(&m_c.m_mpz,&m_c.m_mpz,&m_c.m_mpz);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(m_c));
	}
};

struct in_place_int_add_tester
{
	template <typename U>
	struct runner
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			BOOST_CHECK((is_addable_in_place<int_type,T>::value));
			BOOST_CHECK((!is_addable_in_place<const int_type,T>::value));
			using int_cast_t = typename std::conditional<std::is_signed<T>::value,long long,unsigned long long>::type;
			int_type n1;
			n1 += T(1);
			BOOST_CHECK((std::is_same<decltype(n1 += T(1)),int_type &>::value));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),"1");
			n1 += T(100);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),"101");
			// Random testing.
			std::uniform_int_distribution<T> int_dist(std::numeric_limits<T>::min(),std::numeric_limits<T>::max());
			mpz_raii m1, m2;
			for (int i = 0; i < ntries; ++i) {
				T tmp1 = int_dist(rng), tmp2 = int_dist(rng);
				int_type n(tmp1);
				n += tmp2;
				::mpz_set_str(&m1.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp1)).c_str(),10);
				::mpz_set_str(&m2.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp2)).c_str(),10);
				::mpz_add(&m1.m_mpz,&m1.m_mpz,&m2.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),mpz_lexcast(m1));
			}
			// int += mp_integer.
			BOOST_CHECK((is_addable_in_place<T,int_type>::value));
			BOOST_CHECK((!is_addable_in_place<const T,int_type>::value));
			T n2(0);
			n2 += int_type(1);
			BOOST_CHECK((std::is_same<T &,decltype(n2 += int_type(1))>::value));
			BOOST_CHECK_EQUAL(n2,T(1));
			BOOST_CHECK_THROW(n2 += int_type(std::numeric_limits<T>::max()),std::overflow_error);
			BOOST_CHECK_EQUAL(n2,T(1));
			for (int i = 0; i < ntries; ++i) {
				T tmp1 = int_dist(rng), tmp2 = int_dist(rng);
				::mpz_set_str(&m1.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp1)).c_str(),10);
				::mpz_set_str(&m2.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp2)).c_str(),10);
				try {
					tmp1 += int_type(tmp2);
				} catch (const std::overflow_error &) {
					// Catch overflow errors and move on.
					continue;
				}
				::mpz_add(&m1.m_mpz,&m1.m_mpz,&m2.m_mpz);
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

struct in_place_float_add_tester
{
	template <typename U>
	struct runner
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			BOOST_CHECK((is_addable_in_place<int_type,T>::value));
			BOOST_CHECK((!is_addable_in_place<const int_type,T>::value));
			int_type n1;
			n1 += T(1);
			BOOST_CHECK((std::is_same<decltype(n1 += T(1)),int_type &>::value));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),"1");
			// Random testing
			std::uniform_real_distribution<T> urd1(T(0),std::numeric_limits<T>::max()), urd2(std::numeric_limits<T>::lowest(),T(0));
			for (int i = 0; i < ntries; ++i) {
				int_type n(0);
				const T tmp1 = urd1(rng);
				n += tmp1;
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),boost::lexical_cast<std::string>(int_type{tmp1}));
				n = 0;
				const T tmp2 = urd2(rng);
				n += tmp2;
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),boost::lexical_cast<std::string>(int_type{tmp2}));
			}
			// float += mp_integer.
			BOOST_CHECK((is_addable_in_place<T,int_type>::value));
			BOOST_CHECK((!is_addable_in_place<const T,int_type>::value));
			T x1(0);
			x1 += int_type(1);
			BOOST_CHECK((std::is_same<T &,decltype(x1 += int_type(1))>::value));
			BOOST_CHECK_EQUAL(x1,T(1));
			// NOTE: limit the number of times we run this test, as the conversion from int to float
			// is slow as the random values are taken from the entire float range and thus use a lot of digits.
			for (int i = 0; i < ntries / 100; ++i) {
				T tmp1(0), tmp2 = urd1(rng);
				tmp1 += int_type{tmp2};
				BOOST_CHECK_EQUAL(tmp1,static_cast<T>(int_type{tmp2}));
				tmp1 = T(0);
				tmp2 = urd2(rng);
				tmp1 += int_type{tmp2};
				BOOST_CHECK_EQUAL(tmp1,static_cast<T>(int_type{tmp2}));
			}
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<floating_point_types>(runner<T>());
	}
};

struct binary_add_tester
{
	template <typename U>
	struct runner1
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			using int_cast_t = typename std::conditional<std::is_signed<T>::value,long long,unsigned long long>::type;
			BOOST_CHECK((is_addable<int_type,T>::value));
			BOOST_CHECK((is_addable<T,int_type>::value));
			int_type n;
			T m;
			BOOST_CHECK((std::is_same<decltype(n + m),int_type>::value));
			// Random testing.
			std::uniform_int_distribution<T> int_dist(std::numeric_limits<T>::min(),std::numeric_limits<T>::max());
			mpz_raii m1, m2;
			for (int i = 0; i < ntries; ++i) {
				T tmp1 = int_dist(rng), tmp2 = int_dist(rng);
				int_type n(tmp1);
				::mpz_set_str(&m1.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp1)).c_str(),10);
				::mpz_set_str(&m2.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp2)).c_str(),10);
				::mpz_add(&m1.m_mpz,&m1.m_mpz,&m2.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n + tmp2),mpz_lexcast(m1));
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(tmp2 + n),mpz_lexcast(m1));
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
			BOOST_CHECK((is_addable<int_type,T>::value));
			BOOST_CHECK((is_addable<T,int_type>::value));
			int_type n;
			T m;
			BOOST_CHECK((std::is_same<decltype(n + m),T>::value));
			// Random testing
			std::uniform_real_distribution<T> urd1(T(0),std::numeric_limits<T>::max()), urd2(std::numeric_limits<T>::lowest(),T(0));
			for (int i = 0; i < ntries; ++i) {
				int_type n(0);
				const T tmp1 = urd1(rng);
				BOOST_CHECK_EQUAL(n + tmp1,tmp1);
				BOOST_CHECK_EQUAL(tmp1 + n,tmp1);
				const T tmp2 = urd2(rng);
				BOOST_CHECK_EQUAL(n + tmp2,tmp2);
				BOOST_CHECK_EQUAL(tmp2 + n,tmp2);
			}
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		BOOST_CHECK(is_addable<int_type>::value);
		int_type n1, n2;
		BOOST_CHECK((std::is_same<decltype(n1 + n2),int_type>::value));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1+n2),"0");
		n1 = 1;
		n2 = 4;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1+n2),"5");
		n2 = -6;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1+n2),"-5");
		// Random testing.
		std::uniform_int_distribution<int> promote_dist(0,1);
		std::uniform_int_distribution<int> int_dist(boost::integer_traits<int>::const_min,boost::integer_traits<int>::const_max);
		mpz_raii m_a, m_b;
		for (int i = 0; i < ntries; ++i) {
			auto tmp1 = int_dist(rng), tmp2 = int_dist(rng);
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
			::mpz_add(&m_a.m_mpz,&m_a.m_mpz,&m_b.m_mpz);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a + b),mpz_lexcast(m_a));
		}
		// Test vs hardware int and float types.
		boost::mpl::for_each<integral_types>(runner1<T>());
		boost::mpl::for_each<floating_point_types>(runner2<T>());
	}
};

struct plus_ops_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		int_type n;
		++n;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"1");
		n++;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"2");
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(+n),"2");
		// Random testing.
		std::uniform_int_distribution<int> promote_dist(0,1);
		std::uniform_int_distribution<int> int_dist(boost::integer_traits<int>::const_min,boost::integer_traits<int>::const_max);
		mpz_raii m_a;
		for (int i = 0; i < ntries; ++i) {
			auto tmp = int_dist(rng);
			int_type a{tmp};
			::mpz_set_si(&m_a.m_mpz,static_cast<long>(tmp));
			// Promote randomly.
			if (promote_dist(rng) == 1 && a.is_static()) {
				a.promote();
			}
			::mpz_add_ui(&m_a.m_mpz,&m_a.m_mpz,1ul);
			++a;
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(m_a));
			::mpz_add_ui(&m_a.m_mpz,&m_a.m_mpz,1ul);
			a++;
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(m_a));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(+a),mpz_lexcast(m_a));
		}
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_add_test)
{
	boost::mpl::for_each<size_types>(in_place_mp_integer_add_tester());
	boost::mpl::for_each<size_types>(in_place_int_add_tester());
	boost::mpl::for_each<size_types>(in_place_float_add_tester());
	boost::mpl::for_each<size_types>(binary_add_tester());
	boost::mpl::for_each<size_types>(plus_ops_tester());
}

struct in_place_mp_integer_sub_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		BOOST_CHECK(is_subtractable_in_place<int_type>::value);
		BOOST_CHECK((!is_subtractable_in_place<const int_type,int_type>::value));
		int_type a, b;
		a -= b;
		BOOST_CHECK((std::is_same<decltype(a -= b),int_type &>::value));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),boost::lexical_cast<std::string>(int_type{0}));
		BOOST_CHECK(a.is_static());
		a = int_type{"1"};
		b = a;
		a -= b;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),boost::lexical_cast<std::string>(int_type{0}));
		BOOST_CHECK(a.is_static());
		a = int_type{"1"};
		b = int_type{"-1"};
		a -= b;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),boost::lexical_cast<std::string>(int_type{2}));
		BOOST_CHECK(a.is_static());
		a = int_type{"-1"};
		b = int_type{"1"};
		a -= b;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),boost::lexical_cast<std::string>(int_type{-2}));
		BOOST_CHECK(a.is_static());
		// Random testing.
		std::uniform_int_distribution<int> promote_dist(0,1);
		std::uniform_int_distribution<int> int_dist(boost::integer_traits<int>::const_min,boost::integer_traits<int>::const_max);
		mpz_raii m_a, m_b;
		for (int i = 0; i < ntries; ++i) {
			auto tmp1 = int_dist(rng), tmp2 = int_dist(rng);
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
			a -= b;
			::mpz_sub(&m_a.m_mpz,&m_a.m_mpz,&m_b.m_mpz);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(m_a));
		}
		// Check when static sub fails.
		a = int_type{"-67"};
		b = int_type{"15"};
		BOOST_CHECK(a.is_static());
		BOOST_CHECK(b.is_static());
		using limb_t = typename detail::integer_union<T::value>::s_storage::limb_t;
		auto &st_a = get_m(a).g_st(), &st_b = get_m(b).g_st();
		st_a.set_bit(static_cast<limb_t>(st_a.limb_bits * 2u - 1u));
		st_b.set_bit(static_cast<limb_t>(st_a.limb_bits * 2u - 1u));
		::mpz_set_str(&m_a.m_mpz,boost::lexical_cast<std::string>(a).c_str(),10);
		::mpz_set_str(&m_b.m_mpz,boost::lexical_cast<std::string>(b).c_str(),10);
		a -= b;
		::mpz_sub(&m_a.m_mpz,&m_a.m_mpz,&m_b.m_mpz);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(m_a));
	}
};

struct in_place_int_sub_tester
{
	template <typename U>
	struct runner
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			BOOST_CHECK((is_subtractable_in_place<int_type,T>::value));
			BOOST_CHECK((!is_subtractable_in_place<const int_type,T>::value));
			using int_cast_t = typename std::conditional<std::is_signed<T>::value,long long,unsigned long long>::type;
			int_type n1;
			n1 -= T(1);
			BOOST_CHECK((std::is_same<decltype(n1 -= T(1)),int_type &>::value));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),"-1");
			n1 -= T(100);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),"-101");
			// Random testing.
			std::uniform_int_distribution<T> int_dist(std::numeric_limits<T>::min(),std::numeric_limits<T>::max());
			mpz_raii m1, m2;
			for (int i = 0; i < ntries; ++i) {
				T tmp1 = int_dist(rng), tmp2 = int_dist(rng);
				int_type n(tmp1);
				n -= tmp2;
				::mpz_set_str(&m1.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp1)).c_str(),10);
				::mpz_set_str(&m2.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp2)).c_str(),10);
				::mpz_sub(&m1.m_mpz,&m1.m_mpz,&m2.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),mpz_lexcast(m1));
			}
			// int += mp_integer.
			BOOST_CHECK((is_subtractable_in_place<T,int_type>::value));
			BOOST_CHECK((!is_subtractable_in_place<const T,int_type>::value));
			T n2(1);
			n2 -= int_type(1);
			BOOST_CHECK((std::is_same<T &,decltype(n2 -= int_type(1))>::value));
			BOOST_CHECK_EQUAL(n2,T(0));
			if (std::is_signed<T>::value) {
				int_type tmp_min(std::numeric_limits<T>::min());
				tmp_min.negate();
				n2 = T(-1);
				BOOST_CHECK_THROW(n2 -= tmp_min,std::overflow_error);
				BOOST_CHECK_EQUAL(n2,T(-1));
			} else {
				BOOST_CHECK_THROW(n2 -= int_type(1),std::overflow_error);
			}
			for (int i = 0; i < ntries; ++i) {
				T tmp1 = int_dist(rng), tmp2 = int_dist(rng);
				::mpz_set_str(&m1.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp1)).c_str(),10);
				::mpz_set_str(&m2.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp2)).c_str(),10);
				try {
					tmp1 -= int_type(tmp2);
				} catch (const std::overflow_error &) {
					continue;
				}
				::mpz_sub(&m1.m_mpz,&m1.m_mpz,&m2.m_mpz);
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

struct in_place_float_sub_tester
{
	template <typename U>
	struct runner
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			BOOST_CHECK((is_subtractable_in_place<int_type,T>::value));
			BOOST_CHECK((!is_subtractable_in_place<const int_type,T>::value));
			int_type n1;
			n1 -= T(1);
			BOOST_CHECK((std::is_same<decltype(n1 -= T(1)),int_type &>::value));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),"-1");
			// Random testing
			std::uniform_real_distribution<T> urd1(T(0),std::numeric_limits<T>::max()), urd2(std::numeric_limits<T>::lowest(),T(0));
			for (int i = 0; i < ntries; ++i) {
				int_type n(0);
				const T tmp1 = urd1(rng);
				n -= tmp1;
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),boost::lexical_cast<std::string>(int_type{-tmp1}));
				n = 0;
				const T tmp2 = urd2(rng);
				n -= tmp2;
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),boost::lexical_cast<std::string>(int_type{-tmp2}));
			}
			// float -= mp_integer.
			BOOST_CHECK((is_subtractable_in_place<T,int_type>::value));
			BOOST_CHECK((!is_subtractable_in_place<const T,int_type>::value));
			T x1(0);
			x1 -= int_type(1);
			BOOST_CHECK((std::is_same<T &,decltype(x1 -= int_type(1))>::value));
			BOOST_CHECK_EQUAL(x1,T(-1));
			for (int i = 0; i < ntries / 100; ++i) {
				T tmp1(0), tmp2 = urd1(rng);
				tmp1 -= int_type{tmp2};
				BOOST_CHECK_EQUAL(tmp1,static_cast<T>(int_type{-tmp2}));
				tmp1 = T(0);
				tmp2 = urd2(rng);
				tmp1 -= int_type{tmp2};
				BOOST_CHECK_EQUAL(tmp1,static_cast<T>(int_type{-tmp2}));
			}
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<floating_point_types>(runner<T>());
	}
};

struct binary_sub_tester
{
	template <typename U>
	struct runner1
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			using int_cast_t = typename std::conditional<std::is_signed<T>::value,long long,unsigned long long>::type;
			BOOST_CHECK((is_subtractable<int_type,T>::value));
			BOOST_CHECK((is_subtractable<T,int_type>::value));
			int_type n;
			T m;
			BOOST_CHECK((std::is_same<decltype(n - m),int_type>::value));
			// Random testing.
			std::uniform_int_distribution<T> int_dist(std::numeric_limits<T>::min(),std::numeric_limits<T>::max());
			mpz_raii m1, m2;
			for (int i = 0; i < ntries; ++i) {
				T tmp1 = int_dist(rng), tmp2 = int_dist(rng);
				int_type n(tmp1);
				::mpz_set_str(&m1.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp1)).c_str(),10);
				::mpz_set_str(&m2.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp2)).c_str(),10);
				::mpz_sub(&m1.m_mpz,&m1.m_mpz,&m2.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n - tmp2),mpz_lexcast(m1));
				::mpz_neg(&m1.m_mpz,&m1.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(tmp2 - n),mpz_lexcast(m1));
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
			BOOST_CHECK((is_subtractable<int_type,T>::value));
			BOOST_CHECK((is_subtractable<T,int_type>::value));
			int_type n;
			T m;
			BOOST_CHECK((std::is_same<decltype(n - m),T>::value));
			// Random testing
			std::uniform_real_distribution<T> urd1(T(0),std::numeric_limits<T>::max()), urd2(std::numeric_limits<T>::lowest(),T(0));
			for (int i = 0; i < ntries; ++i) {
				int_type n(0);
				const T tmp1 = urd1(rng);
				BOOST_CHECK_EQUAL(n - tmp1,-tmp1);
				BOOST_CHECK_EQUAL(tmp1 - n,tmp1);
				const T tmp2 = urd2(rng);
				BOOST_CHECK_EQUAL(n - tmp2,-tmp2);
				BOOST_CHECK_EQUAL(tmp2 - n,tmp2);
			}
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		BOOST_CHECK(is_subtractable<int_type>::value);
		int_type n1, n2;
		BOOST_CHECK((std::is_same<decltype(n1 - n2),int_type>::value));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1-n2),"0");
		n1 = 1;
		n2 = 4;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1-n2),"-3");
		n2 = -6;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1-n2),"7");
		// Random testing.
		std::uniform_int_distribution<int> promote_dist(0,1);
		std::uniform_int_distribution<int> int_dist(boost::integer_traits<int>::const_min,boost::integer_traits<int>::const_max);
		mpz_raii m_a, m_b;
		for (int i = 0; i < ntries; ++i) {
			auto tmp1 = int_dist(rng), tmp2 = int_dist(rng);
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
			::mpz_sub(&m_a.m_mpz,&m_a.m_mpz,&m_b.m_mpz);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a - b),mpz_lexcast(m_a));
		}
		// Test vs hardware int and float types.
		boost::mpl::for_each<integral_types>(runner1<T>());
		boost::mpl::for_each<floating_point_types>(runner2<T>());
	}
};

struct minus_ops_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		int_type n;
		--n;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"-1");
		n--;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"-2");
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(-n),"2");
		// Random testing.
		std::uniform_int_distribution<int> promote_dist(0,1);
		std::uniform_int_distribution<int> int_dist(boost::integer_traits<int>::const_min,boost::integer_traits<int>::const_max);
		mpz_raii m_a;
		for (int i = 0; i < ntries; ++i) {
			auto tmp = int_dist(rng);
			int_type a{tmp};
			::mpz_set_si(&m_a.m_mpz,static_cast<long>(tmp));
			// Promote randomly.
			if (promote_dist(rng) == 1 && a.is_static()) {
				a.promote();
			}
			::mpz_sub_ui(&m_a.m_mpz,&m_a.m_mpz,1ul);
			--a;
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(m_a));
			::mpz_sub_ui(&m_a.m_mpz,&m_a.m_mpz,1ul);
			a--;
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(m_a));
			::mpz_neg(&m_a.m_mpz,&m_a.m_mpz);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(-a),mpz_lexcast(m_a));
		}
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_sub_test)
{
	boost::mpl::for_each<size_types>(in_place_mp_integer_sub_tester());
	boost::mpl::for_each<size_types>(in_place_int_sub_tester());
	boost::mpl::for_each<size_types>(in_place_float_sub_tester());
	boost::mpl::for_each<size_types>(binary_sub_tester());
	boost::mpl::for_each<size_types>(minus_ops_tester());
}

struct in_place_mp_integer_mul_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		BOOST_CHECK(is_multipliable_in_place<int_type>::value);
		BOOST_CHECK((!is_multipliable_in_place<const int_type,int_type>::value));
		int_type a, b;
		a *= b;
		BOOST_CHECK((std::is_same<decltype(a *= b),int_type &>::value));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),boost::lexical_cast<std::string>(int_type{0}));
		BOOST_CHECK(a.is_static());
		a = int_type{"1"};
		b = a;
		a *= b;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),boost::lexical_cast<std::string>(int_type{1}));
		BOOST_CHECK(a.is_static());
		a = int_type{"1"};
		b = int_type{"-1"};
		a *= b;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),boost::lexical_cast<std::string>(int_type{-1}));
		BOOST_CHECK(a.is_static());
		a = int_type{"-1"};
		b = int_type{"-1"};
		a *= b;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),boost::lexical_cast<std::string>(int_type{1}));
		BOOST_CHECK(a.is_static());
		// Random testing.
		std::uniform_int_distribution<int> promote_dist(0,1);
		std::uniform_int_distribution<int> int_dist(boost::integer_traits<int>::const_min,boost::integer_traits<int>::const_max);
		mpz_raii m_a, m_b;
		for (int i = 0; i < ntries; ++i) {
			auto tmp1 = int_dist(rng), tmp2 = int_dist(rng);
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
			a *= b;
			::mpz_mul(&m_a.m_mpz,&m_a.m_mpz,&m_b.m_mpz);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(m_a));
		}
		// Check when static mul fails.
		a = int_type{"67"};
		b = int_type{"15"};
		BOOST_CHECK(a.is_static());
		BOOST_CHECK(b.is_static());
		using limb_t = typename detail::integer_union<T::value>::s_storage::limb_t;
		auto &st_a = get_m(a).g_st(), &st_b = get_m(b).g_st();
		st_a.set_bit(static_cast<limb_t>(st_a.limb_bits - 1u));
		st_b.set_bit(static_cast<limb_t>(st_a.limb_bits - 1u));
		::mpz_set_str(&m_a.m_mpz,boost::lexical_cast<std::string>(a).c_str(),10);
		::mpz_set_str(&m_b.m_mpz,boost::lexical_cast<std::string>(b).c_str(),10);
		a *= b;
		::mpz_mul(&m_a.m_mpz,&m_a.m_mpz,&m_b.m_mpz);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a),mpz_lexcast(m_a));
		// Test the bug when promoting both operands which are the same underlying
		// object.
		int_type c(2);
		mpz_raii m_c;
		::mpz_set_si(&m_c.m_mpz,2);
		while (c.is_static()) {
			c *= c;
			::mpz_mul(&m_c.m_mpz,&m_c.m_mpz,&m_c.m_mpz);
		}
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(m_c));
	}
};

struct in_place_int_mul_tester
{
	template <typename U>
	struct runner
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			BOOST_CHECK((is_multipliable_in_place<int_type,T>::value));
			BOOST_CHECK((!is_multipliable_in_place<const int_type,T>::value));
			using int_cast_t = typename std::conditional<std::is_signed<T>::value,long long,unsigned long long>::type;
			int_type n1;
			n1 *= T(1);
			BOOST_CHECK((std::is_same<decltype(n1 *= T(1)),int_type &>::value));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),"0");
			n1 = T(2);
			n1 *= T(50);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),"100");
			// Random testing.
			std::uniform_int_distribution<T> int_dist(std::numeric_limits<T>::min(),std::numeric_limits<T>::max());
			mpz_raii m1, m2;
			for (int i = 0; i < ntries; ++i) {
				T tmp1 = int_dist(rng), tmp2 = int_dist(rng);
				int_type n(tmp1);
				n *= tmp2;
				::mpz_set_str(&m1.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp1)).c_str(),10);
				::mpz_set_str(&m2.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp2)).c_str(),10);
				::mpz_mul(&m1.m_mpz,&m1.m_mpz,&m2.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),mpz_lexcast(m1));
			}
			// int *= mp_integer.
			BOOST_CHECK((is_multipliable_in_place<T,int_type>::value));
			BOOST_CHECK((!is_multipliable_in_place<const T,int_type>::value));
			T n2(2);
			n2 *= int_type(2);
			BOOST_CHECK((std::is_same<T &,decltype(n2 *= int_type(2))>::value));
			BOOST_CHECK_EQUAL(n2,T(4));
			BOOST_CHECK_THROW(n2 *= int_type(std::numeric_limits<T>::max()),std::overflow_error);
			BOOST_CHECK_EQUAL(n2,T(4));
			for (int i = 0; i < ntries; ++i) {
				T tmp1 = int_dist(rng), tmp2 = int_dist(rng);
				::mpz_set_str(&m1.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp1)).c_str(),10);
				::mpz_set_str(&m2.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp2)).c_str(),10);
				try {
					tmp1 *= int_type(tmp2);
				} catch (const std::overflow_error &) {
					// Catch overflow errors and move on.
					continue;
				}
				::mpz_mul(&m1.m_mpz,&m1.m_mpz,&m2.m_mpz);
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


struct in_place_float_mul_tester
{
	template <typename U>
	struct runner
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			BOOST_CHECK((is_multipliable_in_place<int_type,T>::value));
			BOOST_CHECK((!is_multipliable_in_place<const int_type,T>::value));
			int_type n1(2);
			n1 *= T(2);
			BOOST_CHECK((std::is_same<decltype(n1 *= T(2)),int_type &>::value));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1),"4");
			// Random testing
			std::uniform_real_distribution<T> urd1(T(0),std::numeric_limits<T>::max()), urd2(std::numeric_limits<T>::lowest(),T(0));
			for (int i = 0; i < ntries; ++i) {
				int_type n(1);
				const T tmp1 = urd1(rng);
				n *= tmp1;
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),boost::lexical_cast<std::string>(int_type{tmp1}));
				n = 1;
				const T tmp2 = urd2(rng);
				n *= tmp2;
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),boost::lexical_cast<std::string>(int_type{tmp2}));
			}
			// float *= mp_integer.
			BOOST_CHECK((is_multipliable_in_place<T,int_type>::value));
			BOOST_CHECK((!is_multipliable_in_place<const T,int_type>::value));
			T x1(2);
			x1 *= int_type(3);
			BOOST_CHECK((std::is_same<T &,decltype(x1 *= int_type(3))>::value));
			BOOST_CHECK_EQUAL(x1,T(6));
			// NOTE: limit the number of times we run this test, as the conversion from int to float
			// is slow as the random values are taken from the entire float range and thus use a lot of digits.
			for (int i = 0; i < ntries / 100; ++i) {
				T tmp1(1), tmp2 = urd1(rng);
				tmp1 *= int_type{tmp2};
				BOOST_CHECK_EQUAL(tmp1,static_cast<T>(int_type{tmp2}));
				tmp1 = T(1);
				tmp2 = urd2(rng);
				tmp1 *= int_type{tmp2};
				BOOST_CHECK_EQUAL(tmp1,static_cast<T>(int_type{tmp2}));
			}
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<floating_point_types>(runner<T>());
	}
};

struct binary_mul_tester
{
	template <typename U>
	struct runner1
	{
		template <typename T>
		void operator()(const T &)
		{
			typedef mp_integer<U::value> int_type;
			using int_cast_t = typename std::conditional<std::is_signed<T>::value,long long,unsigned long long>::type;
			BOOST_CHECK((is_multipliable<int_type,T>::value));
			BOOST_CHECK((is_multipliable<T,int_type>::value));
			int_type n;
			T m;
			BOOST_CHECK((std::is_same<decltype(n * m),int_type>::value));
			// Random testing.
			std::uniform_int_distribution<T> int_dist(std::numeric_limits<T>::min(),std::numeric_limits<T>::max());
			mpz_raii m1, m2;
			for (int i = 0; i < ntries; ++i) {
				T tmp1 = int_dist(rng), tmp2 = int_dist(rng);
				int_type n(tmp1);
				::mpz_set_str(&m1.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp1)).c_str(),10);
				::mpz_set_str(&m2.m_mpz,boost::lexical_cast<std::string>(static_cast<int_cast_t>(tmp2)).c_str(),10);
				::mpz_mul(&m1.m_mpz,&m1.m_mpz,&m2.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n * tmp2),mpz_lexcast(m1));
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(tmp2 * n),mpz_lexcast(m1));
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
			BOOST_CHECK((is_multipliable<int_type,T>::value));
			BOOST_CHECK((is_multipliable<T,int_type>::value));
			int_type n;
			T m;
			BOOST_CHECK((std::is_same<decltype(n * m),T>::value));
			// Random testing
			std::uniform_real_distribution<T> urd1(T(0),std::numeric_limits<T>::max()), urd2(std::numeric_limits<T>::lowest(),T(0));
			for (int i = 0; i < ntries; ++i) {
				int_type n(1);
				const T tmp1 = urd1(rng);
				BOOST_CHECK_EQUAL(n * tmp1,tmp1);
				BOOST_CHECK_EQUAL(tmp1 * n,tmp1);
				const T tmp2 = urd2(rng);
				BOOST_CHECK_EQUAL(n * tmp2,tmp2);
				BOOST_CHECK_EQUAL(tmp2 * n,tmp2);
			}
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		typedef mp_integer<T::value> int_type;
		BOOST_CHECK(is_multipliable<int_type>::value);
		int_type n1, n2;
		BOOST_CHECK((std::is_same<decltype(n1 * n2),int_type>::value));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1*n2),"0");
		n1 = 2;
		n2 = 4;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1*n2),"8");
		n2 = -6;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n1*n2),"-12");
		// Random testing.
		std::uniform_int_distribution<int> promote_dist(0,1);
		std::uniform_int_distribution<int> int_dist(boost::integer_traits<int>::const_min,boost::integer_traits<int>::const_max);
		mpz_raii m_a, m_b;
		for (int i = 0; i < ntries; ++i) {
			auto tmp1 = int_dist(rng), tmp2 = int_dist(rng);
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
			::mpz_mul(&m_a.m_mpz,&m_a.m_mpz,&m_b.m_mpz);
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(a * b),mpz_lexcast(m_a));
		}
		// Test vs hardware int and float types.
		boost::mpl::for_each<integral_types>(runner1<T>());
		boost::mpl::for_each<floating_point_types>(runner2<T>());
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_mul_test)
{
	boost::mpl::for_each<size_types>(in_place_mp_integer_mul_tester());
	boost::mpl::for_each<size_types>(in_place_int_mul_tester());
	boost::mpl::for_each<size_types>(in_place_float_mul_tester());
	boost::mpl::for_each<size_types>(binary_mul_tester());
}
