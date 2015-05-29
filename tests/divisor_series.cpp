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

#include "../src/divisor_series.hpp"

#define BOOST_TEST_MODULE divisor_series_test
#include <boost/test/unit_test.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "../src/divisor.hpp"
#include "../src/environment.hpp"
#include "../src/exceptions.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/math.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/poisson_series.hpp"
#include "../src/polynomial.hpp"
#include "../src/real.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

using cf_types = boost::mpl::vector<double,integer,real,rational,polynomial<rational,monomial<int>>>;
using expo_types = boost::mpl::vector<short,int,long,integer>;

struct test_00_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using s_type = divisor_series<T,divisor<short>>;
		s_type s0{3};
		// Just test some math operations and common functionalities.
		BOOST_CHECK_EQUAL(s0 + s0,6);
		BOOST_CHECK_EQUAL(s0 * s0,9);
		BOOST_CHECK_EQUAL(s0 * 4,12);
		BOOST_CHECK_EQUAL(4 * s0,12);
		BOOST_CHECK_EQUAL(math::pow(s0,3),27);
		BOOST_CHECK_EQUAL(math::cos(s_type{0}),1);
		BOOST_CHECK_EQUAL(math::sin(s_type{0}),0);
		BOOST_CHECK_EQUAL(math::evaluate<int>(math::pow(s0,3),{{"x",4}}),27);
		BOOST_CHECK(is_differentiable<s_type>::value);
		BOOST_CHECK_EQUAL(s_type{1}.partial("x"),0);
		if (std::is_base_of<detail::polynomial_tag,T>::value) {
			BOOST_CHECK((has_subs<s_type,s_type>::value));
			BOOST_CHECK((has_subs<s_type,int>::value));
			BOOST_CHECK((has_subs<s_type,integer>::value));
		}
		BOOST_CHECK((!has_subs<s_type,std::string>::value));
		if (std::is_base_of<detail::polynomial_tag,T>::value) {
			BOOST_CHECK((has_ipow_subs<s_type,s_type>::value));
			BOOST_CHECK((has_ipow_subs<s_type,int>::value));
			BOOST_CHECK((has_ipow_subs<s_type,integer>::value));
		}
		BOOST_CHECK((!has_ipow_subs<s_type,std::string>::value));
	}
};

BOOST_AUTO_TEST_CASE(divisor_series_test_00)
{
	environment env;
	boost::mpl::for_each<cf_types>(test_00_tester());
}

BOOST_AUTO_TEST_CASE(divisor_series_pow_test)
{
	using s_type0 = divisor_series<int,divisor<short>>;
	BOOST_CHECK_EQUAL(math::pow(s_type0{2},-1),0);
	BOOST_CHECK_EQUAL(math::pow(s_type0{2},2),4);
	using s_type1 = divisor_series<rational,divisor<short>>;
	BOOST_CHECK_EQUAL(math::pow(s_type1{2},-1),1/2_q);
	BOOST_CHECK_EQUAL(math::pow(s_type1{2/3_q},2),4/9_q);
	{
	using s_type = divisor_series<polynomial<rational,monomial<short>>,divisor<short>>;
	s_type x{"x"}, y{"y"}, z{"z"}, null;
	BOOST_CHECK_EQUAL(math::pow(x,2),x*x);
	BOOST_CHECK_EQUAL(math::pow(x,0),1);
	BOOST_CHECK_EQUAL(math::pow(null,1),0);
	BOOST_CHECK_THROW(math::pow(null,-1),zero_division_error);
	BOOST_CHECK_EQUAL(math::pow(null,0),1);
	BOOST_CHECK((std::is_same<decltype(x.pow(-1)),s_type>::value));
	BOOST_CHECK((std::is_same<decltype(math::pow(x,-1)),s_type>::value));
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x,-1)),"1/[(x)]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x,-2)),"1/[(x)**2]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x,-10)),"1/[(x)**10]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x-y,-1)),"1/[(x-y)]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x-y,-2)),"1/[(x-y)**2]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x-y,-10)),"1/[(x-y)**10]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(2*x-4*y,-1)),"1/2*1/[(x-2*y)]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(2*x-4*y,-2)),"1/4*1/[(x-2*y)**2]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(2*x-4*y,-10)),"1/1024*1/[(x-2*y)**10]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(-2*x+4*y,-10)),"1/1024*1/[(x-2*y)**10]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(-2*x+4*y,-11)),"-1/2048*1/[(x-2*y)**11]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x+y+z,-1)),"1/[(x+y+z)]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x+y+z-z,-1)),"1/[(x+y)]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x+y+z,-2)),"1/[(x+y+z)**2]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x+y+z-z,-2)),"1/[(x+y)**2]");
	BOOST_CHECK_EQUAL(math::pow(s_type{1/2_q},1.5),math::pow(1/2_q,1.5));
	BOOST_CHECK_THROW(math::pow(x-1,-1),std::invalid_argument);
	BOOST_CHECK_THROW(math::pow(x-y/2,-1),std::invalid_argument);
	BOOST_CHECK_THROW(math::pow(x-x,-2),zero_division_error);
	BOOST_CHECK_EQUAL(math::pow(s_type{1/2_q},2),1/4_q);
	BOOST_CHECK_EQUAL(math::pow(s_type{1/2_q},0),1);
	BOOST_CHECK_EQUAL(math::pow(s_type{0_q},0),1);
	BOOST_CHECK_EQUAL(math::pow(s_type{1/2_q},-2),4);
	// Out of bounds for short.
	BOOST_CHECK_THROW(math::pow((std::numeric_limits<short>::max()+1_q)*x+y,-1),std::invalid_argument);
	// Check, if appropriate, construction from outside the bounds defined in divisor.
	if (detail::safe_abs_sint<short>::value < std::numeric_limits<short>::max()) {
		BOOST_CHECK_THROW(math::pow((detail::safe_abs_sint<short>::value+1_q)*x+y,-1),std::invalid_argument);
	}
	if (-detail::safe_abs_sint<short>::value > std::numeric_limits<short>::min()) {
		BOOST_CHECK_THROW(math::pow((-detail::safe_abs_sint<short>::value-1_q)*x+y,-1),std::invalid_argument);
	}
	}
	{
	using s_type = divisor_series<polynomial<rational,k_monomial>,divisor<short>>;
	s_type x{"x"}, y{"y"}, z{"z"}, null;
	BOOST_CHECK_EQUAL(math::pow(x,2),x*x);
	BOOST_CHECK_EQUAL(math::pow(x,0),1);
	BOOST_CHECK_EQUAL(math::pow(null,1),0);
	BOOST_CHECK_THROW(math::pow(null,-1),zero_division_error);
	BOOST_CHECK_EQUAL(math::pow(null,0),1);
	BOOST_CHECK((std::is_same<decltype(x.pow(-1)),s_type>::value));
	BOOST_CHECK((std::is_same<decltype(math::pow(x,-1)),s_type>::value));
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x,-1)),"1/[(x)]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x,-2)),"1/[(x)**2]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x,-10)),"1/[(x)**10]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x-y,-1)),"1/[(x-y)]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x-y,-2)),"1/[(x-y)**2]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x-y,-10)),"1/[(x-y)**10]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(2*x-4*y,-1)),"1/2*1/[(x-2*y)]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(2*x-4*y,-2)),"1/4*1/[(x-2*y)**2]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(2*x-4*y,-10)),"1/1024*1/[(x-2*y)**10]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(-2*x+4*y,-10)),"1/1024*1/[(x-2*y)**10]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(-2*x+4*y,-11)),"-1/2048*1/[(x-2*y)**11]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x+y+z,-1)),"1/[(x+y+z)]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x+y+z-z,-1)),"1/[(x+y)]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x+y+z,-2)),"1/[(x+y+z)**2]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x+y+z-z,-2)),"1/[(x+y)**2]");
	BOOST_CHECK_EQUAL(math::pow(s_type{1/2_q},1.5),math::pow(1/2_q,1.5));
	BOOST_CHECK_THROW(math::pow(x-1,-1),std::invalid_argument);
	BOOST_CHECK_THROW(math::pow(x-y/2,-1),std::invalid_argument);
	BOOST_CHECK_THROW(math::pow(x-x,-2),zero_division_error);
	BOOST_CHECK_EQUAL(math::pow(s_type{1/2_q},2),1/4_q);
	BOOST_CHECK_EQUAL(math::pow(s_type{1/2_q},0),1);
	BOOST_CHECK_EQUAL(math::pow(s_type{0_q},0),1);
	BOOST_CHECK_EQUAL(math::pow(s_type{1/2_q},-2),4);
	BOOST_CHECK_THROW(math::pow((std::numeric_limits<short>::max()+1_q)*x+y,-1),std::invalid_argument);
	if (detail::safe_abs_sint<short>::value < std::numeric_limits<short>::max()) {
		BOOST_CHECK_THROW(math::pow((detail::safe_abs_sint<short>::value+1_q)*x+y,-1),std::invalid_argument);
	}
	if (-detail::safe_abs_sint<short>::value > std::numeric_limits<short>::min()) {
		BOOST_CHECK_THROW(math::pow((-detail::safe_abs_sint<short>::value-1_q)*x+y,-1),std::invalid_argument);
	}
	}
	{
	using s_type = divisor_series<polynomial<rational,monomial<rational>>,divisor<short>>;
	s_type x{"x"}, y{"y"}, z{"z"}, null;
	BOOST_CHECK_EQUAL(math::pow(x,2),x*x);
	BOOST_CHECK_EQUAL(math::pow(x,0),1);
	BOOST_CHECK_EQUAL(math::pow(null,1),0);
	BOOST_CHECK_THROW(math::pow(null,-1),zero_division_error);
	BOOST_CHECK_EQUAL(math::pow(null,0),1);
	BOOST_CHECK((std::is_same<decltype(x.pow(-1)),s_type>::value));
	BOOST_CHECK((std::is_same<decltype(math::pow(x,-1)),s_type>::value));
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x,-1)),"1/[(x)]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x,-2)),"1/[(x)**2]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x,-10)),"1/[(x)**10]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x-y,-1)),"1/[(x-y)]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x-y,-2)),"1/[(x-y)**2]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x-y,-10)),"1/[(x-y)**10]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(2*x-4*y,-1)),"1/2*1/[(x-2*y)]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(2*x-4*y,-2)),"1/4*1/[(x-2*y)**2]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(2*x-4*y,-10)),"1/1024*1/[(x-2*y)**10]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(-2*x+4*y,-10)),"1/1024*1/[(x-2*y)**10]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(-2*x+4*y,-11)),"-1/2048*1/[(x-2*y)**11]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x+y+z,-1)),"1/[(x+y+z)]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x+y+z-z,-1)),"1/[(x+y)]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x+y+z,-2)),"1/[(x+y+z)**2]");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::pow(x+y+z-z,-2)),"1/[(x+y)**2]");
	BOOST_CHECK_EQUAL(math::pow(s_type{1/2_q},1.5),math::pow(1/2_q,1.5));
	BOOST_CHECK_THROW(math::pow(x-1,-1),std::invalid_argument);
	BOOST_CHECK_THROW(math::pow(x-y/2,-1),std::invalid_argument);
	BOOST_CHECK_THROW(math::pow(x-x,-2),zero_division_error);
	BOOST_CHECK_EQUAL(math::pow(s_type{1/2_q},2),1/4_q);
	BOOST_CHECK_EQUAL(math::pow(s_type{1/2_q},0),1);
	BOOST_CHECK_EQUAL(math::pow(s_type{0_q},0),1);
	BOOST_CHECK_EQUAL(math::pow(s_type{1/2_q},-2),4);
	BOOST_CHECK_THROW(math::pow((std::numeric_limits<short>::max()+1_q)*x+y,-1),std::invalid_argument);
	if (detail::safe_abs_sint<short>::value < std::numeric_limits<short>::max()) {
		BOOST_CHECK_THROW(math::pow((detail::safe_abs_sint<short>::value+1_q)*x+y,-1),std::invalid_argument);
	}
	if (-detail::safe_abs_sint<short>::value > std::numeric_limits<short>::min()) {
		BOOST_CHECK_THROW(math::pow((-detail::safe_abs_sint<short>::value-1_q)*x+y,-1),std::invalid_argument);
	}
	}
}

struct partial_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using p_type = polynomial<rational,monomial<int>>;
		using s_type = divisor_series<p_type,divisor<T>>;
		// Tests using the special pow(-1) for polynomial coefficients..
		s_type x{"x"}, y{"y"}, z{"z"};
		// First with variables only in the divisors.
		auto s0 = math::pow(x+y-2*z,-1);
		BOOST_CHECK((std::is_same<s_type,decltype(s0.partial("x"))>::value));
		BOOST_CHECK((std::is_same<s_type,decltype(math::partial(s0,"x"))>::value));
		BOOST_CHECK_EQUAL(s0.partial("x"),-s0*s0);
		BOOST_CHECK_EQUAL(math::partial(s0,"x"),-s0*s0);
		BOOST_CHECK_EQUAL(s0.partial("z"),2*s0*s0);
		auto s1 = s0 * s0;
		BOOST_CHECK_EQUAL(s1.partial("x"),-2*s0*s1);
		BOOST_CHECK_EQUAL(s1.partial("z"),4*s0*s1);
		auto s2 = math::pow(x-y,-1);
		auto s3 = s0*s2;
		BOOST_CHECK_EQUAL(s3.partial("x"),-s0*s0*s2-s0*s2*s2);
		auto s4 = math::pow(x,-1);
		auto s5 = s0*s2*s4;
		BOOST_CHECK_EQUAL(s5.partial("x"),-s0*s0*s2*s4-s0*s2*s2*s4-s0*s2*s4*s4);
		BOOST_CHECK_EQUAL(s5.partial("z"),2*s0*s0*s2*s4);
		auto s6 = s0*s0*s2*s4;
		BOOST_CHECK_EQUAL(s6.partial("x"),-2*s0*s0*s0*s2*s4-s0*s0*s2*s2*s4-s0*s0*s2*s4*s4);
		// Variables only in the coefficients.
		auto s7 = s2*s4 * (x*x/5+y-3*z);
		BOOST_CHECK_EQUAL(s7.partial("z"),s2*s4*-3);
		auto s8 = s2*s4 * (x*x/5+y-3*z) + z*s2*s4*y;
		BOOST_CHECK_EQUAL(s8.partial("z"),s2*s4*-3+s2*s4*y);
		BOOST_CHECK_EQUAL((x*x*math::pow(z,-1)).partial("x"),2*x*math::pow(z,-1));
		// This excercises the presense of an additional divisor variable with a zero multiplier.
		BOOST_CHECK_EQUAL((x*x*math::pow(z,-1)+s4-s4).partial("x"),2*x*math::pow(z,-1));
		// Variables both in the coefficients and in the divisors.
		auto s9 = x * s2;
		BOOST_CHECK_EQUAL(s9.partial("x"),s2-x*s2*s2);
		BOOST_CHECK_EQUAL(math::partial(s9,"x"),s2-x*s2*s2);
		auto s10 = x*s2*s4;
		BOOST_CHECK_EQUAL(s10.partial("x"),s2*s4+x*(-s2*s2*s4-s2*s4*s4));
		auto s11 = math::pow(-3*x-y,-1);
		auto s12 = math::pow(z,-1);
		auto s13 = x*s11*s4+x*y*z*s2*s2*s2*s12;
		BOOST_CHECK_EQUAL(s13.partial("x"),s11*s4+x*(3*s11*s11*s4-s11*s4*s4)+y*z*s2*s2*s2*s12+x*y*z*(-3*s2*s2*s2*s2*s12));
		BOOST_CHECK_EQUAL(math::partial(s13,"x"),s11*s4+x*(3*s11*s11*s4-s11*s4*s4)+y*z*s2*s2*s2*s12+x*y*z*(-3*s2*s2*s2*s2*s12));
		auto s15 = x*s11*s4+x*y*z*s2*s2*s2*s12+s4*s12;
		BOOST_CHECK_EQUAL(s15.partial("x"),s11*s4+x*(3*s11*s11*s4-s11*s4*s4)+y*z*s2*s2*s2*s12+x*y*z*(-3*s2*s2*s2*s2*s12)-s4*s4*s12);
		// Overflow in an exponent.
		overflow_check<T>();
		auto s16 = math::pow(x-4*y,-1);
		auto s17 = s2*s2*s2*s2*s2*s16*s16*s16*s12;
		BOOST_CHECK_EQUAL(s17.partial("x"),-5*s2*s2*s2*s2*s2*s2*s16*s16*s16*s12-3*s2*s2*s2*s2*s2*s16*s16*s16*s16*s12);
		// Excercise the chain rule.
		auto s18 = x*x*3/4_q*y*z*z;
		auto s19 = -y*y*x*z*z;
		auto s20 = y*x*x*4;
		auto s21 = s18*s17+s19*s2*s11*s12+s20*s16*s2*s3;
		BOOST_CHECK_EQUAL(s21.partial("x"),s18.partial("x")*s17+s18*s17.partial("x")+s19.partial("x")*s2*s11*s12+s19*(s2*s11*s12).partial("x")
			+s20.partial("x")*s16*s2*s3+s20*(s16*s2*s3).partial("x"));
		BOOST_CHECK_EQUAL(s21.partial("y"),s18.partial("y")*s17+s18*s17.partial("y")+s19.partial("y")*s2*s11*s12+s19*(s2*s11*s12).partial("y")
			+s20.partial("y")*s16*s2*s3+s20*(s16*s2*s3).partial("y"));
		BOOST_CHECK_EQUAL(s21.partial("z"),s18.partial("z")*s17+s18*s17.partial("z")+s19.partial("z")*s2*s11*s12+s19*(s2*s11*s12).partial("z")
			+s20.partial("z")*s16*s2*s3+s20*(s16*s2*s3).partial("z"));
		BOOST_CHECK_EQUAL(s21.partial("v"),0);
		BOOST_CHECK_EQUAL(s_type{1}.partial("x"),0);
	}
	template <typename T, typename U = T, typename std::enable_if<std::is_integral<U>::value,int>::type = 0>
	static void overflow_check()
	{
		using p_type = polynomial<rational,monomial<int>>;
		using s_type = divisor_series<p_type,divisor<T>>;
		s_type s14;
		symbol_set ss;
		ss.add("x");
		s14.set_symbol_set(ss);
		typename s_type::term_type::key_type k0;
		std::vector<T> vs;
		vs.push_back(T(1));
		T expo = std::numeric_limits<T>::max();
		k0.insert(vs.begin(),vs.end(),expo);
		s14.insert(typename s_type::term_type(typename s_type::term_type::cf_type(1),k0));
		BOOST_CHECK_THROW(s14.partial("x"),std::overflow_error);
		// Skip this overflow test if T is short, as short * short will become int * int and it will
		// not overflow.
		if (std::is_same<T,short>::value) {
			return;
		}
		s_type s15;
		ss.add("y");
		s15.set_symbol_set(ss);
		vs[0] = static_cast<T>(std::numeric_limits<T>::max() / T(4));
		vs.push_back(T(1));
		expo = static_cast<T>(std::numeric_limits<T>::max() - 1);
		typename s_type::term_type::key_type k1;
		k1.insert(vs.begin(),vs.end(),expo);
		s15.insert(typename s_type::term_type(typename s_type::term_type::cf_type(1),k1));
		BOOST_CHECK_THROW(s15.partial("x"),std::overflow_error);
	}
	template <typename T, typename U = T, typename std::enable_if<!std::is_integral<U>::value,int>::type = 0>
	static void overflow_check()
	{}
};

BOOST_AUTO_TEST_CASE(divisor_series_partial_test)
{
	// A couple of general tests to start.
	using p_type = polynomial<rational,monomial<int>>;
	using s_type = divisor_series<p_type,divisor<short>>;
	{
	BOOST_CHECK_EQUAL(s_type{}.partial("x"),0);
	s_type s0{3};
	BOOST_CHECK_EQUAL(s0.partial("x"),0);
	s_type x{"x"};
	BOOST_CHECK_EQUAL((x * 3).partial("x"),3);
	BOOST_CHECK_EQUAL((x * 3).partial("y"),0);
	// Define an EPS.
	using ps_type = poisson_series<s_type>;
	ps_type a{"a"}, b{"b"}, c{"c"};
	auto p1 = 3*a*b*math::cos(3*c);
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p1.t_integrate()),"a*b*1/[(\\nu_{c})]*sin(3*c)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p1.t_integrate().partial("a")),"b*1/[(\\nu_{c})]*sin(3*c)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p1.t_integrate().partial("b")),"a*1/[(\\nu_{c})]*sin(3*c)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p1.t_integrate().partial("c")),"3*a*b*1/[(\\nu_{c})]*cos(3*c)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p1.t_integrate().partial("\\nu_{c}")),"-a*b*1/[(\\nu_{c})**2]*sin(3*c)");
	}
	// Test with various exponent types.
	boost::mpl::for_each<expo_types>(partial_tester());
	// Test custom derivatives.
	s_type x{"x"}, y{"y"};
	s_type::register_custom_derivative("x",[x](const s_type &s) -> s_type {
		return s.partial("x") + math::partial(s,"y") * 2 * x;
	});
	BOOST_CHECK_EQUAL(math::partial(math::pow(x+y,-1),"x"),(-1-2*x)*math::pow(x+y,-1).pow(2));
	s_type::register_custom_derivative("x",[y](const s_type &s) -> s_type {
		return s.partial("x") + math::partial(s,"y") * math::pow(y,-1) / 2;
	});
	BOOST_CHECK_EQUAL(math::partial(math::pow(x+2*y,-1),"x"),(-1-1*y.pow(-1))*math::pow(x+2*y,-1).pow(2));
	s_type::register_custom_derivative("x",[y](const s_type &s) -> s_type {
		return s.partial("x") + math::partial(s,"y") * math::pow(y,-1) / 2;
	});
	BOOST_CHECK_EQUAL(math::partial(math::pow(x+y,-1),"x"),-math::pow(x+y,-1).pow(2)
		-1/2_q*math::pow(x+y,-1).pow(2)*math::pow(y,-1));
	// Implicit variable dependency both in the poly and in the divisor.
	s_type::register_custom_derivative("x",[x](const s_type &s) -> s_type {
		return s.partial("x") + math::partial(s,"y") * 2 * x;
	});
	BOOST_CHECK_EQUAL(math::partial(y*math::pow(x+y,-1),"x"),2*x*math::pow(x+y,-1)-y*(2*x+1)*math::pow(x+y,-1).pow(2));
}

BOOST_AUTO_TEST_CASE(divisor_series_integrate_test)
{
	using s_type = divisor_series<polynomial<rational,monomial<short>>,divisor<short>>;
	s_type x{"x"}, y{"y"}, z{"z"};
	BOOST_CHECK((is_integrable<s_type>::value));
	// A few cases with the variables only in the polynomial part.
	BOOST_CHECK_EQUAL(x.integrate("x"),x*x/2);
	BOOST_CHECK_EQUAL(math::integrate(x,"x"),x*x/2);
	BOOST_CHECK((std::is_same<s_type,decltype(math::integrate(x,"x"))>::value));
	BOOST_CHECK_EQUAL(math::integrate(x,"y"),x*y);
	BOOST_CHECK_EQUAL(math::integrate(x+y,"x"),x*y+x*x/2);
	BOOST_CHECK_EQUAL(math::integrate(x+y,"y"),x*y+y*y/2);
	BOOST_CHECK_EQUAL(math::integrate(s_type{1},"y"),y);
	BOOST_CHECK_EQUAL(math::integrate(s_type{1},"x"),x);
	BOOST_CHECK_EQUAL(math::integrate(s_type{0},"x"),0);
	// Put variables in the divisors as well.
	BOOST_CHECK_EQUAL(math::integrate(x+y.pow(-1),"x"),x*x/2+x*y.pow(-1));
	BOOST_CHECK_THROW(math::integrate(x+y.pow(-1)+x.pow(-1),"x"),std::invalid_argument);
	BOOST_CHECK_EQUAL(math::integrate(x+y.pow(-1)+x.pow(-1)-x.pow(-1),"x"),x*x/2+x*y.pow(-1));
}
