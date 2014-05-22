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
	}
};

BOOST_AUTO_TEST_CASE(mp_integer_addmul_test)
{
	environment env;
	boost::mpl::for_each<size_types>(addmul_tester());
}
