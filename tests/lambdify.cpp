/* Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)

This file is part of the Piranha library.

The Piranha library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The Piranha library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the Piranha library.  If not,
see https://www.gnu.org/licenses/. */

#include "../src/lambdify.hpp"

#define BOOST_TEST_MODULE lambdify_test
#include <boost/test/unit_test.hpp>

#include <stdexcept>
#include <string>
#include <type_traits>

#include "../src/environment.hpp"
#include "../src/exceptions.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/real.hpp"
#include "../src/polynomial.hpp"
#include "../src/rational_function.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(lambdify_test_00)
{
	environment env;
	{
	using p_type = polynomial<integer,k_monomial>;
	p_type x{"x"}, y{"y"}, z{"z"};
	BOOST_CHECK((has_lambdify<p_type,integer>::value));
	BOOST_CHECK((!has_lambdify<p_type,std::string>::value));
	auto l0 = lambdify<integer>(x+y+z,{"x","y","z"});
	BOOST_CHECK((std::is_same<decltype(l0({})),integer>::value));
	BOOST_CHECK_EQUAL(l0({1_z,2_z,3_z}),6);
	auto l1 = lambdify<integer>(x+2*y+3*z,{"y","z","x"});
	BOOST_CHECK_EQUAL(l1({1_z,2_z,3_z}),2*1+3*2+3);
	BOOST_CHECK_THROW(lambdify<integer>(x+2*y+3*z,{"y","z","x","x"}),std::invalid_argument);
	BOOST_CHECK((has_lambdify<p_type,rational>::value));
	auto l2 = lambdify<rational>(x*x-2*y+3*z*z*z,{"x","y","z","a"});
	BOOST_CHECK((std::is_same<decltype(l2({})),rational>::value));
	BOOST_CHECK_THROW(l2({1_q,2_q,3_q}),std::invalid_argument);
	BOOST_CHECK_THROW(l2({1_q,2_q,3_q,4_q,5_q}),std::invalid_argument);
	BOOST_CHECK_EQUAL(l2({1/7_q,-2/5_q,2/3_q,15_q}),1/7_q*1/7_q-2*-2/5_q+3*2/3_q*2/3_q*2/3_q);
	BOOST_CHECK((has_lambdify<p_type,double>::value));
	auto l3 = lambdify<double>(x*x-2*y+3*z*z*z,{"x","y","z"});
	BOOST_CHECK((std::is_same<decltype(l3({})),double>::value));
	BOOST_CHECK((has_lambdify<p_type,p_type>::value));
	auto l4 = lambdify<p_type>(x*x-2*y+3*z*z*z,{"x","y","z"});
	BOOST_CHECK((std::is_same<decltype(l4({})),p_type>::value));
	}
	{
	using r_type = rational_function<k_monomial>;
	r_type x{"x"}, y{"y"}, z{"z"};
	BOOST_CHECK((has_lambdify<r_type,integer>::value));
	BOOST_CHECK((!has_lambdify<r_type,std::string>::value));
	auto l0 = lambdify<rational>((x+y)/(y-z),{"x","y","z"});
	BOOST_CHECK((std::is_same<decltype(l0({})),rational>::value));
	BOOST_CHECK_EQUAL(l0({1_q,3/5_q,9_q}),(1+3/5_q)/(3/5_q-9));
	BOOST_CHECK_THROW(l0({1_q,3/5_q,3/5_q}),zero_division_error);
	BOOST_CHECK((has_lambdify<r_type,real>::value));
	auto l1 = lambdify<real>((x+y)/(y-z),{"x","y","z"});
	BOOST_CHECK((std::is_same<decltype(l1({})),real>::value));
	}
	{
	BOOST_CHECK((has_lambdify<double,integer>::value));
	BOOST_CHECK((has_lambdify<double,std::string>::value));
	BOOST_CHECK((has_lambdify<double,rational>::value));
	auto l0 = lambdify<integer>(3.4,{});
	BOOST_CHECK((std::is_same<double,decltype(l0({}))>::value));
	BOOST_CHECK_EQUAL(l0({}),3.4);
	BOOST_CHECK_THROW(l0({1_z,2_z,3_z}),std::invalid_argument);
	}
}
