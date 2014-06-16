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
#include <iostream>
#include <random>
#include <string>
#include <type_traits>

#include "../src/config.hpp"
#include "../src/environment.hpp"

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
		q_type q3(-3);
		q_type q4(123.456);
		std::cout << q4 << '\n';
		q_type q5(123.456l);
		std::cout << q5 << '\n';
		q_type q6(1.7);
		std::cout << q6 << '\n';
		std::cout << (long double)(q6) << '\n';
	}
};

BOOST_AUTO_TEST_CASE(mp_rational_constructor_test)
{
	environment env;
	boost::mpl::for_each<size_types>(constructor_tester());
}
