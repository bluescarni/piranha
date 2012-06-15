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

#include "../src/real.hpp"

#define BOOST_TEST_MODULE real_test
#include <boost/test/unit_test.hpp>

#define FUSION_MAX_VECTOR_SIZE 20

#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/lexical_cast.hpp>
#include <mpfr.h>
#include <stdexcept>
#include <string>

#include "../src/integer.hpp"
#include "../src/rational.hpp"

using namespace piranha;

const boost::fusion::vector<char,signed char,short,int,long,long long,unsigned char,unsigned short,unsigned,unsigned long,unsigned long long>
	integral_values((char)42,(signed char)42,(short)42,-42,42L,-42LL,(unsigned char)42,
	(unsigned short)42,42U,42UL,42ULL
);

struct check_integral_construction
{
	template <typename T>
	void operator()(const T &value) const
	{
		if (value > T(0)) {
			BOOST_CHECK_EQUAL("4.00e1",boost::lexical_cast<std::string>(real(value,4)));
		} else {
			BOOST_CHECK_EQUAL("-4.00e1",boost::lexical_cast<std::string>(real(value,4)));
		}
	}
};

BOOST_AUTO_TEST_CASE(real_constructors_test)
{
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{}),"0.00000000000000000000000000000000000");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"1.23"}),"1.22999999999999999999999999999999998");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"1.23",4}),"1.25");
	if (MPFR_PREC_MIN > 0) {
		BOOST_CHECK_THROW((real{"1.23",0}),std::invalid_argument);
		BOOST_CHECK_THROW((real{std::string("1.23"),0}),std::invalid_argument);
	}
	BOOST_CHECK_THROW((real{"1a"}),std::invalid_argument);
	BOOST_CHECK_THROW((real{"1.a"}),std::invalid_argument);
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"@NaN@"}),"@NaN@");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"+@NaN@"}),"@NaN@");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"-@NaN@"}),"@NaN@");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"@Inf@"}),"@Inf@");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"+@Inf@"}),"@Inf@");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"-@Inf@"}),"-@Inf@");
	// Copy constructor.
	real r1{"1.23",4};
	real r2{r1};
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r2),"1.25");
	real r3{"-inf"};
	real r4(r3);
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r4),"-@Inf@");
	// Move constructor.
	real r5{std::move(r1)};
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r5),"1.25");
	real r6{std::move(r3)};
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r6),"-@Inf@");
	// Generic constructor.
	if (std::numeric_limits<float>::is_iec559 && std::numeric_limits<float>::radix == 2) {
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{0.f,4}),"0.00");
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{4.f,4}),"4.00");
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{-4.f,4}),"-4.00");
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{.5f,4}),"5.00e-1");
	}
	if (std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::radix == 2) {
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{0.,4}),"0.00");
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{4.,4}),"4.00");
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{-4.,4}),"-4.00");
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{-.5,4}),"-5.00e-1");
	}
	// Construction from integral types.
	boost::fusion::for_each(integral_values,check_integral_construction());
	// Construction from integer and rational.
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{integer(),4}),"0.00");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{integer(2),4}),"2.00");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{integer(-10),4}),"-1.00e1");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{rational(),4}),"0.00");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{rational(2),4}),"2.00");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{rational(-10),4}),"-1.00e1");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{rational(-1,2),4}),"-5.00e-1");
}

BOOST_AUTO_TEST_CASE(real_sign_test)
{
	BOOST_CHECK_EQUAL(real{}.sign(),0);
	BOOST_CHECK_EQUAL(real{"1"}.sign(),1);
	BOOST_CHECK_EQUAL(real{"-10.23"}.sign(),-1);
	BOOST_CHECK_EQUAL(real{"-0."}.sign(),0);
	BOOST_CHECK_EQUAL(real{"-.0"}.sign(),0);
	BOOST_CHECK_EQUAL(real{"1.23e5"}.sign(),1);
	BOOST_CHECK_EQUAL(real{"1.23e-5"}.sign(),1);
	BOOST_CHECK_EQUAL(real{"-1.23e-5"}.sign(),-1);
}

BOOST_AUTO_TEST_CASE(real_stream_test)
{
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"nan"}),"@NaN@");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"+nan"}),"@NaN@");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"-nan"}),"@NaN@");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"inf"}),"@Inf@");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"+inf"}),"@Inf@");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"-inf"}),"-@Inf@");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"3"}),"3.00000000000000000000000000000000000");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"30"}),"3.00000000000000000000000000000000000e1");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"0.5"}),"5.00000000000000000000000000000000000e-1");
}
