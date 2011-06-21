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

#include "../src/math.hpp"

#define BOOST_TEST_MODULE math_test
#include <boost/test/unit_test.hpp>

#define FUSION_MAX_VECTOR_SIZE 20

#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/sequence.hpp>
#include <type_traits>

using namespace piranha;

const boost::fusion::vector<char,short,int,long,long long,unsigned char,unsigned short,unsigned,unsigned long,unsigned long long,float,double,long double> arithmetic_values(
	(char)-42,(short)42,-42,42L,-42LL,
	(unsigned char)42,(unsigned short)42,42U,42UL,42ULL,
	23.456f,-23.456,23.456L
);

struct check_negate
{
	template <typename T>
	void operator()(const T &value) const
	{
		if (std::is_signed<T>::value) {
			T negation(value);
			math::negate(negation);
			BOOST_CHECK_EQUAL(negation,-value);
		}
	}
};

BOOST_AUTO_TEST_CASE(negate_test)
{
	boost::fusion::for_each(arithmetic_values,check_negate());
}

const boost::fusion::vector<char,short,int,long,long long,unsigned char,unsigned short,unsigned,unsigned long,unsigned long long,float,double,long double> zeroes(
	(char)0,(short)0,0,0L,0LL,
	(unsigned char)0,(unsigned short)0,0U,0UL,0ULL,
	0.f,-0.,0.L
);

struct check_is_zero_01
{
	template <typename T>
	void operator()(const T &value) const
	{
		BOOST_CHECK(math::is_zero(value));
		BOOST_CHECK(math::is_zero(std::complex<T>(value)));
	}
};

struct check_is_zero_02
{
	template <typename T>
	void operator()(const T &value) const
	{
		BOOST_CHECK(!math::is_zero(value));
		BOOST_CHECK(!math::is_zero(std::complex<T>(value)));
	}
};

BOOST_AUTO_TEST_CASE(is_zero_test)
{
	boost::fusion::for_each(zeroes,check_is_zero_01());
	boost::fusion::for_each(arithmetic_values,check_is_zero_02());
}

struct check_multiply_accumulate
{
	template <typename T>
	void operator()(const T &) const
	{
		T x(2);
		math::multiply_accumulate(x,T(4),T(6));
		BOOST_CHECK_EQUAL(x,T(2) + T(4) * T(6));
		if (std::is_signed<T>::value || std::is_floating_point<T>::value) {
			x = T(-2);
			math::multiply_accumulate(x,T(5),T(-7));
			BOOST_CHECK_EQUAL(x,T(-2) + T(5) * T(-7));
		}
	}
};

BOOST_AUTO_TEST_CASE(multiply_accumulate_test)
{
	boost::fusion::for_each(arithmetic_values,check_multiply_accumulate());
}

