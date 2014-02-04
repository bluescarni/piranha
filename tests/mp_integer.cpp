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

#define BOOST_TEST_MODULE mp_integer_test
#include <boost/test/unit_test.hpp>

#define FUSION_MAX_VECTOR_SIZE 20

#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/integer_traits.hpp>
#include <boost/lexical_cast.hpp>
#include <cstddef>
#include <gmp.h>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "../src/config.hpp"
#include "../src/environment.hpp"
#include "../src/exceptions.hpp"
#include "../src/small_vector.hpp"

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
	small_vector<char> tmp;
	tmp.resize(static_cast<small_vector<char>::size_type>(total_size));
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
		const auto limb_bits = int_type::limb_bits;
		int_type n;
		BOOST_CHECK_EQUAL(n.calculate_n_limbs(),0u);
		n.set_bit(0);
		BOOST_CHECK_EQUAL(n.calculate_n_limbs(),1u);
		n.set_bit(1);
		BOOST_CHECK_EQUAL(n.calculate_n_limbs(),1u);
		n.set_bit(limb_bits);
		BOOST_CHECK_EQUAL(n.calculate_n_limbs(),2u);
		n.set_bit(limb_bits + 1u);
		BOOST_CHECK_EQUAL(n.calculate_n_limbs(),2u);
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
		// Random testing.
		detail::mpz_raii mc, ma, mb;
		std::uniform_int_distribution<short> short_dist(boost::integer_traits<short>::const_min,boost::integer_traits<short>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = short_dist(rng), tmp2 = short_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::add(c,a,b);
				::mpz_add(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned short> ushort_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = ushort_dist(rng), tmp2 = ushort_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::add(c,a,b);
				::mpz_add(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<int> int_dist(boost::integer_traits<int>::const_min,boost::integer_traits<int>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = int_dist(rng), tmp2 = int_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::add(c,a,b);
				::mpz_add(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned> uint_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = uint_dist(rng), tmp2 = uint_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::add(c,a,b);
				::mpz_add(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<long> long_dist(boost::integer_traits<long>::const_min,boost::integer_traits<long>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = long_dist(rng), tmp2 = long_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::add(c,a,b);
				::mpz_add(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned long> ulong_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = ulong_dist(rng), tmp2 = ulong_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::add(c,a,b);
				::mpz_add(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<long long> llong_dist(boost::integer_traits<long long>::const_min,boost::integer_traits<long long>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = llong_dist(rng), tmp2 = llong_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::add(c,a,b);
				::mpz_add(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned long long> ullong_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = ullong_dist(rng), tmp2 = ullong_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::add(c,a,b);
				::mpz_add(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
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
		// Random testing.
		detail::mpz_raii mc, ma, mb;
		std::uniform_int_distribution<short> short_dist(boost::integer_traits<short>::const_min,boost::integer_traits<short>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = short_dist(rng), tmp2 = short_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::sub(c,a,b);
				::mpz_sub(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned short> ushort_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = ushort_dist(rng), tmp2 = ushort_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::sub(c,a,b);
				::mpz_sub(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<int> int_dist(boost::integer_traits<int>::const_min,boost::integer_traits<int>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = int_dist(rng), tmp2 = int_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::sub(c,a,b);
				::mpz_sub(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned> uint_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = uint_dist(rng), tmp2 = uint_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::sub(c,a,b);
				::mpz_sub(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<long> long_dist(boost::integer_traits<long>::const_min,boost::integer_traits<long>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = long_dist(rng), tmp2 = long_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::sub(c,a,b);
				::mpz_sub(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned long> ulong_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = ulong_dist(rng), tmp2 = ulong_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::sub(c,a,b);
				::mpz_sub(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<long long> llong_dist(boost::integer_traits<long long>::const_min,boost::integer_traits<long long>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = llong_dist(rng), tmp2 = llong_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::sub(c,a,b);
				::mpz_sub(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned long long> ullong_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = ullong_dist(rng), tmp2 = ullong_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 2 || b.abs_size() > 2) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::sub(c,a,b);
				::mpz_sub(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
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
		detail::mpz_raii mc, ma, mb;
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
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
		int_type::mul(a,c,b);
		::mpz_mul(&ma.m_mpz,&mc.m_mpz,&mb.m_mpz);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
		b.negate();
		::mpz_neg(&mb.m_mpz,&mb.m_mpz);
		int_type::mul(a,b,c);
		::mpz_mul(&ma.m_mpz,&mb.m_mpz,&mc.m_mpz);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
		int_type::mul(a,c,b);
		::mpz_mul(&ma.m_mpz,&mc.m_mpz,&mb.m_mpz);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
		// Random testing.
		std::uniform_int_distribution<short> short_dist(boost::integer_traits<short>::const_min,boost::integer_traits<short>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = short_dist(rng), tmp2 = short_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 1 || b.abs_size() > 1) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::mul(c,a,b);
				::mpz_mul(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned short> ushort_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = ushort_dist(rng), tmp2 = ushort_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 1 || b.abs_size() > 1) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::mul(c,a,b);
				::mpz_mul(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<int> int_dist(boost::integer_traits<int>::const_min,boost::integer_traits<int>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = int_dist(rng), tmp2 = int_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 1 || b.abs_size() > 1) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::mul(c,a,b);
				::mpz_mul(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned> uint_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = uint_dist(rng), tmp2 = uint_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 1 || b.abs_size() > 1) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::mul(c,a,b);
				::mpz_mul(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<long> long_dist(boost::integer_traits<long>::const_min,boost::integer_traits<long>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = long_dist(rng), tmp2 = long_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 1 || b.abs_size() > 1) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::mul(c,a,b);
				::mpz_mul(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned long> ulong_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = ulong_dist(rng), tmp2 = ulong_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 1 || b.abs_size() > 1) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::mul(c,a,b);
				::mpz_mul(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<long long> llong_dist(boost::integer_traits<long long>::const_min,boost::integer_traits<long long>::const_max);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = llong_dist(rng), tmp2 = llong_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 1 || b.abs_size() > 1) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::mul(c,a,b);
				::mpz_mul(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
			} catch (const std::overflow_error &) {}
		}
		std::uniform_int_distribution<unsigned long long> ullong_dist;
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = ullong_dist(rng), tmp2 = ullong_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				if (a.abs_size() > 1 || b.abs_size() > 1) {
					continue;
				}
				::mpz_set_str(&ma.m_mpz,boost::lexical_cast<std::string>(tmp1).c_str(),10);
				::mpz_set_str(&mb.m_mpz,boost::lexical_cast<std::string>(tmp2).c_str(),10);
				int_type::mul(c,a,b);
				::mpz_mul(&mc.m_mpz,&ma.m_mpz,&mb.m_mpz);
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(c),mpz_lexcast(mc));
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
		detail::mpz_raii mc, ma, mb;
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
			try {
				cmp = a + b*c;
				a.multiply_accumulate(b,c);
			} catch (const std::overflow_error &) {
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
			try {
				cmp = a + b*c;
				a.multiply_accumulate(b,c);
			} catch (const std::overflow_error &) {
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
		detail::mpz_raii mc, ma, mb;
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
