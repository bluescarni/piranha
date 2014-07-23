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

#include "../src/mp_rational.hpp"

#define BOOST_TEST_MODULE mp_rational_test
#include <boost/test/unit_test.hpp>

#define FUSION_MAX_VECTOR_SIZE 20

#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/lexical_cast.hpp>
#include <cstddef>
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
#include "../src/exceptions.hpp"
#include "../src/environment.hpp"
#include "../src/mp_integer.hpp"

using integral_types = boost::mpl::vector<char,
	signed char,short,int,long,long long,
	unsigned char,unsigned short,unsigned,unsigned long,unsigned long long,
	wchar_t,char16_t,char32_t>;

using floating_point_types = boost::mpl::vector<float,double,long double>;

static std::mt19937 rng;
static const int ntries = 1000;

using namespace piranha;

using size_types = boost::mpl::vector<std::integral_constant<int,0>,std::integral_constant<int,8>,std::integral_constant<int,16>,
	std::integral_constant<int,32>
#if defined(PIRANHA_UINT128_T)
	,std::integral_constant<int,64>
#endif
	>;

using mpq_struct_t = std::remove_extent< ::mpq_t>::type;

// Simple RAII holder for GMP rationals.
struct mpq_raii
{
	mpq_raii()
	{
		::mpq_init(&m_mpq);
		piranha_assert(mpq_numref(&m_mpq)->_mp_alloc > 0);
	}
	mpq_raii(const mpq_raii &) = delete;
	mpq_raii(mpq_raii &&) = delete;
	mpq_raii &operator=(const mpq_raii &) = delete;
	mpq_raii &operator=(mpq_raii &&) = delete;
	~mpq_raii() noexcept
	{
		if (mpq_numref(&m_mpq)->_mp_d != nullptr) {
			::mpq_clear(&m_mpq);
		}
	}
	mpq_struct_t m_mpq;
};

static inline std::string mpq_lexcast(const mpq_raii &m)
{
	std::ostringstream os;
	const std::size_t size_base10_num = ::mpz_sizeinbase(mpq_numref(&m.m_mpq),10),
		size_base10_den = ::mpz_sizeinbase(mpq_denref(&m.m_mpq),10),
		max = std::numeric_limits<std::size_t>::max();
	if (size_base10_den > max - 3u || size_base10_num > max - (size_base10_den + 3u)) {
		piranha_throw(std::overflow_error,"number of digits is too large");
	}
	const auto total_size = size_base10_num + (size_base10_den + 3u);
	std::vector<char> tmp;
	tmp.resize(static_cast<std::vector<char>::size_type>(total_size));
	if (unlikely(tmp.size() != total_size)) {
		piranha_throw(std::overflow_error,"number of digits is too large");
	}
	os << ::mpq_get_str(&tmp[0u],10,&m.m_mpq);
	return os.str();
}

// Constructors and assignments.
struct constructor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using q_type = mp_rational<T::value>;
		std::cout << "NBits,size,align: " << T::value << ',' << sizeof(q_type) << ',' << alignof(q_type) << '\n';
		q_type q;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(q),"0");
		BOOST_CHECK_EQUAL(q.num(),0);
		BOOST_CHECK_EQUAL(q.den(),1);
		BOOST_CHECK(q.is_canonical());
		q_type q0(0,1);
		BOOST_CHECK_EQUAL(q0.num(),0);
		BOOST_CHECK_EQUAL(q0.den(),1);
		BOOST_CHECK(q0.is_canonical());
		q_type q1(0,2);
		BOOST_CHECK_EQUAL(q1.num(),0);
		BOOST_CHECK_EQUAL(q1.den(),1);
		BOOST_CHECK(q1.is_canonical());
		q_type q2(0,-3);
		BOOST_CHECK_EQUAL(q2.num(),0);
		BOOST_CHECK_EQUAL(q2.den(),1);
		BOOST_CHECK(q2.is_canonical());
		BOOST_CHECK_THROW((q_type{1,0}),zero_division_error);
		BOOST_CHECK_THROW((q_type{-1,0}),zero_division_error);
		BOOST_CHECK((std::is_constructible<q_type,int,int>::value));
		BOOST_CHECK((std::is_constructible<q_type,int,unsigned>::value));
		BOOST_CHECK((std::is_constructible<q_type,char,long>::value));
		BOOST_CHECK((std::is_constructible<q_type,typename q_type::int_type,long>::value));
		BOOST_CHECK(!(std::is_constructible<q_type,double,long>::value));
		BOOST_CHECK(!(std::is_constructible<q_type,float,double>::value));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(q_type{2,4}),"1/2");
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(q_type{-2,4}),"-1/2");
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(q_type{-2,-4}),"1/2");
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(q_type{2,-4}),"-1/2");
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(q_type{4,-2}),"-2");
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(q_type{0,-2}),"0");
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(q_type{0,-10}),"0");
		// Random testing.
		std::uniform_int_distribution<int> dist(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
		mpq_raii m;
		detail::mpz_raii z;
		for (int i = 0; i < ntries; ++i) {
			const int a = dist(rng), b = dist(rng);
			if (b == 0) {
				continue;
			}
			// The mpq set function only works with unsigned type at the denom. Bypass using mpz directly.
			if (b > 0) {
				::mpq_set_si(&m.m_mpq,static_cast<long>(a),static_cast<unsigned long>(b));
			} else {
				::mpq_set_si(&m.m_mpq,static_cast<long>(a),static_cast<unsigned long>(1));
				::mpz_set_si(mpq_denref(&m.m_mpq),static_cast<long>(b));
			}
			::mpq_canonicalize(&m.m_mpq);
			q_type tmp_q{a,b};
			BOOST_CHECK_EQUAL(mpq_lexcast(m),boost::lexical_cast<std::string>(tmp_q));
			// Check that it is canonical and that canonicalisation doest not change the value.
			BOOST_CHECK(tmp_q.is_canonical());
			q_type copy = tmp_q;
			copy.canonicalise();
			BOOST_CHECK_EQUAL(mpq_lexcast(m),boost::lexical_cast<std::string>(copy));
		}
		// Copy/move ctor/assignment.
		q_type q3(1,-15);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(q3),boost::lexical_cast<std::string>(q_type(q3)));
		q3._num() *= std::numeric_limits<int>::max();
		q3.canonicalise();
		auto q5(q3);
		q_type q4(std::move(q3));
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(q4),boost::lexical_cast<std::string>(q_type(q5)));
		q_type q6;
		q6 = q5;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(q6),boost::lexical_cast<std::string>(q_type(q5)));
		q6 = std::move(q5);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(q6),boost::lexical_cast<std::string>(q_type(q4)));
	}
};

BOOST_AUTO_TEST_CASE(mp_rational_constructor_test)
{
	environment env;
	boost::mpl::for_each<size_types>(constructor_tester());
}
