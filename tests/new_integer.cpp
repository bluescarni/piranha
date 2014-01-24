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

#include "../src/new_integer.hpp"

#define BOOST_TEST_MODULE new_integer_test
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
		using int_type = detail::static_integer<T::value>;
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
		BOOST_CHECK(m.m_limbs[2u] == 0);
		BOOST_CHECK(m.m_limbs[1u] == 0);
		BOOST_CHECK(m.m_limbs[0u] == 4);
		n.m_limbs[0u] = 5;
		n._mp_size = -1;
		m = std::move(n);
		BOOST_CHECK(m._mp_alloc == 0);
		BOOST_CHECK(m._mp_size == -1);
		BOOST_CHECK(m.m_limbs[2u] == 0);
		BOOST_CHECK(m.m_limbs[1u] == 0);
		BOOST_CHECK(m.m_limbs[0u] == 5);
		int_type o(m);
		BOOST_CHECK(o._mp_alloc == 0);
		BOOST_CHECK(o._mp_size == -1);
		BOOST_CHECK(o.m_limbs[2u] == 0);
		BOOST_CHECK(o.m_limbs[1u] == 0);
		BOOST_CHECK(o.m_limbs[0u] == 5);
		int_type p(std::move(o));
		BOOST_CHECK(p._mp_alloc == 0);
		BOOST_CHECK(p._mp_size == -1);
		BOOST_CHECK(p.m_limbs[2u] == 0);
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

BOOST_AUTO_TEST_CASE(new_integer_static_integer_constructor_test)
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
		using int_type = detail::static_integer<T::value>;
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
		n2.set_bit(limb_bits * 2u);
		::mpz_setbit(&m2.m_mpz,static_cast< ::mp_bitcnt_t>(limb_bits * 2u));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n2),mpz_lexcast(m2));
		BOOST_CHECK_EQUAL(n2._mp_size,3);
		n2.set_bit(limb_bits * 2u + 5u);
		::mpz_setbit(&m2.m_mpz,static_cast< ::mp_bitcnt_t>(limb_bits * 2u + 5u));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n2),mpz_lexcast(m2));
		for (typename int_type::limb_t i = 0u; i < int_type::limb_bits * 3u; ++i) {
			n2.set_bit(i);
			::mpz_setbit(&m2.m_mpz,static_cast< ::mp_bitcnt_t>(i));
		}
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n2),mpz_lexcast(m2));
		n2.negate();
		::mpz_neg(&m2.m_mpz,&m2.m_mpz);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n2),mpz_lexcast(m2));
		BOOST_CHECK_EQUAL(n2._mp_size,-3);
	}
};

BOOST_AUTO_TEST_CASE(new_integer_static_integer_set_bit_test)
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
		n.set_bit(limb_bits * 2u);
		BOOST_CHECK_EQUAL(n.calculate_n_limbs(),3u);
		n.set_bit(limb_bits * 2u + 1u);
		BOOST_CHECK_EQUAL(n.calculate_n_limbs(),3u);
	}
};

BOOST_AUTO_TEST_CASE(new_integer_static_integer_calculate_n_limbs_test)
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

BOOST_AUTO_TEST_CASE(new_integer_static_integer_negate_test)
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

BOOST_AUTO_TEST_CASE(new_integer_static_integer_comparison_test)
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

BOOST_AUTO_TEST_CASE(new_integer_static_integer_is_zero_test)
{
	boost::mpl::for_each<size_types>(static_is_zero_tester());
}

struct static_add_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using int_type = detail::static_integer<T::value>;
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
		b = int_type(1);
		c = int_type();
		cmp = c;
		cmp.set_bit(limb_bits * 2u);
		for (typename int_type::limb_t i = 0u; i < limb_bits * 2u; ++i) {
			c.set_bit(i);
		}
		int_type::add(a,b,c);
		BOOST_CHECK_EQUAL(cmp,a);
		b = int_type(-1);
		c = int_type();
		cmp = c;
		cmp.set_bit(limb_bits * 2u);
		cmp.negate();
		for (typename int_type::limb_t i = 0u; i < limb_bits * 2u; ++i) {
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
		// Random testing.
		std::uniform_int_distribution<short> short_dist(boost::integer_traits<short>::const_min / 100,boost::integer_traits<short>::const_max / 100);
		for (int i = 0; i < ntries; ++i) {
			const auto tmp1 = short_dist(rng), tmp2 = short_dist(rng);
			try {
				int_type a(tmp1), b(tmp2), c;
				int_type::add(c,a,b);
				BOOST_CHECK_EQUAL(c,int_type(tmp1 + tmp2));
			} catch (const std::overflow_error &) {}
		}
	}
};

BOOST_AUTO_TEST_CASE(new_integer_static_integer_add_test)
{
	boost::mpl::for_each<size_types>(static_add_tester());
}
