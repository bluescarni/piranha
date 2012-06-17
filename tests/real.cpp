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

static_assert(MPFR_PREC_MIN <= 4 && MPFR_PREC_MAX >= 4,"The unit tests for piranha::real assume that 4 is a valid value for significand precision.");

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
	// Copy ctor with different precision.
	if (MPFR_PREC_MIN > 0) {
		BOOST_CHECK_THROW((real{real{"1.23"},0}),std::invalid_argument);
	}
	BOOST_CHECK_EQUAL((real{real{"1.23"},4}.get_prec()),::mpfr_prec_t(4));
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{real{"1.23"},4}),"1.25");
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

BOOST_AUTO_TEST_CASE(real_precision_test)
{
	BOOST_CHECK_EQUAL(real{}.get_prec(),real::default_prec);
	BOOST_CHECK_EQUAL((real{0.1,MPFR_PREC_MIN + 1}.get_prec()),MPFR_PREC_MIN + 1);
	real r{1};
	r.set_prec(4);
	BOOST_CHECK_EQUAL(r.get_prec(),::mpfr_prec_t(4));
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r),"@NaN@");
	if (MPFR_PREC_MIN > 0) {
		BOOST_CHECK_THROW(r.set_prec(0),std::invalid_argument);
	}
}

BOOST_AUTO_TEST_CASE(real_swap_test)
{
	real r1{-1,4}, r2{0};
	r1.swap(r1);
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"-1.00");
	BOOST_CHECK_EQUAL(r1.get_prec(),::mpfr_prec_t(4));
	r1.swap(r2);
	BOOST_CHECK_EQUAL(r1.get_prec(),real::default_prec);
	BOOST_CHECK_EQUAL(r2.get_prec(),::mpfr_prec_t(4));
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"0.00000000000000000000000000000000000");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r2),"-1.00");
}

struct check_integral_assignment
{
	template <typename T>
	void operator()(const T &value) const
	{
		real r{0.,4};
		r = value;
		if (value > T(0)) {
			BOOST_CHECK_EQUAL("4.00e1",boost::lexical_cast<std::string>(r));
		} else {
			BOOST_CHECK_EQUAL("-4.00e1",boost::lexical_cast<std::string>(r));
		}
		real tmp(std::move(r));
		r = value;
		if (value > T(0)) {
			BOOST_CHECK_EQUAL("4.20000000000000000000000000000000000e1",boost::lexical_cast<std::string>(r));
		} else {
			BOOST_CHECK_EQUAL("-4.20000000000000000000000000000000000e1",boost::lexical_cast<std::string>(r));
		}
	}
};

BOOST_AUTO_TEST_CASE(real_assignment_test)
{
	real r1{-1,4}, r2;
	r1 = r1;
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"-1.00");
	BOOST_CHECK_EQUAL(r1.get_prec(),::mpfr_prec_t(4));
	r2 = r1;
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r2),"-1.00");
	BOOST_CHECK_EQUAL(r2.get_prec(),::mpfr_prec_t(4));
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"-1.00");
	BOOST_CHECK_EQUAL(r1.get_prec(),::mpfr_prec_t(4));
	real r3;
	r3 = std::move(r2);
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r3),"-1.00");
	BOOST_CHECK_EQUAL(r3.get_prec(),::mpfr_prec_t(4));
	// Revive r2.
	r2 = std::move(r1);
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r2),"-1.00");
	BOOST_CHECK_EQUAL(r2.get_prec(),::mpfr_prec_t(4));
	// Revive r1.
	r1 = r2;
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"-1.00");
	BOOST_CHECK_EQUAL(r1.get_prec(),::mpfr_prec_t(4));
	// Assignment from string.
	r1 = "1.2300000000001";
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"1.25");
	BOOST_CHECK_EQUAL(r1.get_prec(),::mpfr_prec_t(4));
	r1 = std::string("1.2300000000001");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"1.25");
	BOOST_CHECK_EQUAL(r1.get_prec(),::mpfr_prec_t(4));
	// Move r1 away.
	real r4{std::move(r1)};
	r1 = std::string("1.23");
	BOOST_CHECK_EQUAL(r1.get_prec(),real::default_prec);
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"1.22999999999999999999999999999999998");
	BOOST_CHECK_THROW(r1 = "foo_the_bar",std::invalid_argument);
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"0.00000000000000000000000000000000000");
	// Assignment to floating-point.
	if (std::numeric_limits<float>::is_iec559 && std::numeric_limits<float>::radix == 2) {
		r1 = 0.f;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"0.00000000000000000000000000000000000");
		r1 = 4.f;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"4.00000000000000000000000000000000000");
		r1 = -.5f;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"-5.00000000000000000000000000000000000e-1");
		real tmp(std::move(r1));
		r1 = -.5f;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"-5.00000000000000000000000000000000000e-1");
	}
	r1 = real{0.,4};
	if (std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::radix == 2) {
		r1 = 0.;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"0.00");
		r1 = 4.;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"4.00");
		r1 = -.5;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"-5.00e-1");
		real tmp(std::move(r1));
		r1 = -.5;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"-5.00000000000000000000000000000000000e-1");
	}
	// Assignment from integrals.
	boost::fusion::for_each(integral_values,check_integral_assignment());
	// Assignment from integer and rational.
	r1.set_prec(4);
	r1 = integer(1);
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"1.00");
	real{std::move(r1)};
	r1 = integer(2);
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"2.00000000000000000000000000000000000");
	r1.set_prec(4);
	r1 = rational(1,2);
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"5.00e-1");
	real{std::move(r1)};
	r1 = -rational(1,2);
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"-5.00000000000000000000000000000000000e-1");
}

BOOST_AUTO_TEST_CASE(real_is_inf_nan_test)
{
	BOOST_CHECK(!real{}.is_nan());
	BOOST_CHECK(!real{}.is_inf());
	BOOST_CHECK(!real{1}.is_nan());
	BOOST_CHECK(!real{1}.is_inf());
	BOOST_CHECK(real{"nan"}.is_nan());
	BOOST_CHECK(real{"-nan"}.is_nan());
	BOOST_CHECK(!real{"nan"}.is_inf());
	BOOST_CHECK(!real{"-nan"}.is_inf());
	BOOST_CHECK(real{"inf"}.is_inf());
	BOOST_CHECK(real{"-inf"}.is_inf());
	BOOST_CHECK(!real{"inf"}.is_nan());
	BOOST_CHECK(!real{"-inf"}.is_nan());
}
