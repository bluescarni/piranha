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
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"@NaN@"}),"nan");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"+@NaN@"}),"nan");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"-@NaN@"}),"nan");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"@Inf@"}),"inf");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"+@Inf@"}),"inf");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"-@Inf@"}),"-inf");
	// Copy constructor.
	real r1{"1.23",4};
	real r2{r1};
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r2),"1.25");
	real r3{"-inf"};
	real r4(r3);
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r4),"-inf");
	// Move constructor.
	real r5{std::move(r1)};
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r5),"1.25");
	real r6{std::move(r3)};
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r6),"-inf");
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
	BOOST_CHECK_EQUAL(real{"inf"}.sign(),1);
	BOOST_CHECK_EQUAL(real{"-inf"}.sign(),-1);
	BOOST_CHECK_EQUAL(real{"nan"}.sign(),0);
	BOOST_CHECK_EQUAL(real{"-nan"}.sign(),0);
}

BOOST_AUTO_TEST_CASE(real_stream_test)
{
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"nan"}),"nan");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"+nan"}),"nan");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"-nan"}),"nan");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"inf"}),"inf");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"+inf"}),"inf");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(real{"-inf"}),"-inf");
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
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r),"nan");
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

struct check_integral_conversion
{
	template <typename T>
	void operator()(const T &value) const
	{
		BOOST_CHECK_THROW(static_cast<T>(real{"inf"}),std::overflow_error);
		BOOST_CHECK_THROW(static_cast<T>(real{"-inf"}),std::overflow_error);
		BOOST_CHECK_THROW(static_cast<T>(real{"nan"}),std::overflow_error);
		BOOST_CHECK_THROW(static_cast<T>(real{"-nan"}),std::overflow_error);
		BOOST_CHECK_EQUAL(static_cast<T>(real{value}),value);
		if (std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::radix == 2) {
			if (value > T(0)) {
				BOOST_CHECK_EQUAL(static_cast<T>(real{value + 0.5}),value);
			} else {
				BOOST_CHECK_EQUAL(static_cast<T>(real{value - 0.5}),value);
			}
		}
		BOOST_CHECK_THROW(static_cast<T>(real{integer(boost::integer_traits<T>::const_max) * 2}),std::overflow_error);
		if (std::is_signed<T>::value) {
			BOOST_CHECK_THROW(static_cast<T>(real{integer(boost::integer_traits<T>::const_min) * 2}),std::overflow_error);
		}
	}
};

BOOST_AUTO_TEST_CASE(real_conversion_test)
{
	// Test boolean conversion.
	BOOST_CHECK(!real{});
	BOOST_CHECK(real{"0.5"});
	BOOST_CHECK(real{1});
	// Integer conversion.
	BOOST_CHECK_EQUAL(static_cast<integer>(real{}),0);
	BOOST_CHECK_EQUAL(static_cast<integer>(real{"1.43111e4"}),14311);
	BOOST_CHECK_EQUAL(static_cast<integer>(real{"-1.43111e4"}),-14311);
	BOOST_CHECK_EQUAL(static_cast<integer>(real{"1.43119e4"}),14311);
	BOOST_CHECK_EQUAL(static_cast<integer>(real{"-1.43119e4"}),-14311);
	BOOST_CHECK_THROW(static_cast<integer>(real{"inf"}),std::overflow_error);
	BOOST_CHECK_THROW(static_cast<integer>(real{"-inf"}),std::overflow_error);
	BOOST_CHECK_THROW(static_cast<integer>(real{"nan"}),std::overflow_error);
	BOOST_CHECK_THROW(static_cast<integer>(real{"-nan"}),std::overflow_error);
	// Integral conversions.
	boost::fusion::for_each(integral_values,check_integral_conversion());
	// FP conversions. Double.
	if (std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::radix == 2) {
		BOOST_CHECK_EQUAL(static_cast<double>(real{}),0.);
		BOOST_CHECK_EQUAL(static_cast<double>(real{-10.}),-10.);
		BOOST_CHECK_EQUAL(static_cast<double>(real{0.5}),.5);
	}
	if (std::numeric_limits<double>::has_quiet_NaN) {
		BOOST_CHECK(static_cast<double>(real{"nan"}) != std::numeric_limits<double>::quiet_NaN());
	} else {
		BOOST_CHECK_THROW(static_cast<double>(real{"nan"}),std::overflow_error);
	}
	if (std::numeric_limits<double>::has_infinity) {
		BOOST_CHECK_EQUAL(static_cast<double>(real{"inf"}),std::numeric_limits<double>::infinity());
		BOOST_CHECK_EQUAL(static_cast<double>(real{"-inf"}),-std::numeric_limits<double>::infinity());
	} else {
		BOOST_CHECK_THROW(static_cast<double>(real{"inf"}),std::overflow_error);
		BOOST_CHECK_THROW(static_cast<double>(real{"-inf"}),std::overflow_error);
	}
	// Float.
	if (std::numeric_limits<float>::is_iec559 && std::numeric_limits<float>::radix == 2) {
		BOOST_CHECK_EQUAL(static_cast<float>(real{}),0.f);
		BOOST_CHECK_EQUAL(static_cast<float>(real{-10.f}),-10.f);
		BOOST_CHECK_EQUAL(static_cast<float>(real{0.5f}),.5f);
	}
	if (std::numeric_limits<float>::has_quiet_NaN) {
		BOOST_CHECK(static_cast<float>(real{"nan"}) != std::numeric_limits<float>::quiet_NaN());
	} else {
		BOOST_CHECK_THROW(static_cast<float>(real{"nan"}),std::overflow_error);
	}
	if (std::numeric_limits<float>::has_infinity) {
		BOOST_CHECK_EQUAL(static_cast<float>(real{"inf"}),std::numeric_limits<float>::infinity());
		BOOST_CHECK_EQUAL(static_cast<float>(real{"-inf"}),-std::numeric_limits<float>::infinity());
	} else {
		BOOST_CHECK_THROW(static_cast<float>(real{"inf"}),std::overflow_error);
		BOOST_CHECK_THROW(static_cast<float>(real{"-inf"}),std::overflow_error);
	}
	// Rational.
	BOOST_CHECK_EQUAL(static_cast<rational>(real{}),0);
	BOOST_CHECK_EQUAL(static_cast<rational>(real{1}),1);
	BOOST_CHECK_EQUAL(static_cast<rational>(real{12}),12);
	BOOST_CHECK_EQUAL(static_cast<rational>(real{-1234}),-1234);
	BOOST_CHECK_EQUAL(static_cast<rational>(real{"-0.5"}),rational(-1,2));
	BOOST_CHECK_EQUAL(static_cast<rational>(real{"0.03125"}),rational(1,32));
	BOOST_CHECK_EQUAL(static_cast<rational>(real{"-7.59375"}),rational(243,-32));
	BOOST_CHECK_THROW(static_cast<rational>(real{"nan"}),std::overflow_error);
	BOOST_CHECK_THROW(static_cast<rational>(real{"inf"}),std::overflow_error);
	BOOST_CHECK_THROW(static_cast<rational>(real{"-inf"}),std::overflow_error);
}

struct check_in_place_add_integral
{
	template <typename T>
	void operator()(const T &value) const
	{
		real r{0.,4};
		r += value;
		BOOST_CHECK_EQUAL(r.get_prec(),::mpfr_prec_t(4));
		if (value > T(0)) {
			BOOST_CHECK_EQUAL("4.00e1",boost::lexical_cast<std::string>(r));
		} else {
			BOOST_CHECK_EQUAL("-4.00e1",boost::lexical_cast<std::string>(r));
		}
	}
};

BOOST_AUTO_TEST_CASE(real_addition_test)
{
	real r1{1.,4}, r2{2.,4};
	r1 += r2;
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"3.00");
	r1 += real{1.};
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"4.00000000000000000000000000000000000");
	BOOST_CHECK_EQUAL(r1.get_prec(),real::default_prec);
	r1 += rational(1,2);
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"4.50000000000000000000000000000000000");
	r1 += integer(2);
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"6.50000000000000000000000000000000000");
	if (std::numeric_limits<float>::is_iec559 && std::numeric_limits<float>::radix == 2) {
		r1 += 2.f;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"8.50000000000000000000000000000000000");
		r1 += 2.5f;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"1.10000000000000000000000000000000000e1");
		r1 += -4.5f;
	}
	if (std::numeric_limits<float>::has_infinity) {
		real r;
		r += std::numeric_limits<float>::infinity();
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r),"inf");
		r = 0;
		r += -std::numeric_limits<float>::infinity();
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r),"-inf");
		r += std::numeric_limits<float>::infinity();
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r),"nan");
	}
	if (std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::radix == 2) {
		r1 += 2.;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"8.50000000000000000000000000000000000");
		r1 += 2.5;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r1),"1.10000000000000000000000000000000000e1");
		r1 += -4.5;
	}
	if (std::numeric_limits<double>::has_infinity) {
		real r;
		r += std::numeric_limits<double>::infinity();
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r),"inf");
		r = 0;
		r += -std::numeric_limits<double>::infinity();
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r),"-inf");
		r += std::numeric_limits<double>::infinity();
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(r),"nan");
	}
	boost::fusion::for_each(integral_values,check_in_place_add_integral());
}

BOOST_AUTO_TEST_CASE(real_identity_operator)
{
	real r{"1.5"};
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(+r),"1.50000000000000000000000000000000000");
}
