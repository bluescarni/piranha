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

#include "../src/invert.hpp"

#define BOOST_TEST_MODULE invert_test
#include <boost/test/unit_test.hpp>

#include <string>
#include <type_traits>

#include "../src/environment.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/poisson_series.hpp"
#include "../src/polynomial.hpp"
#include "../src/pow.hpp"
#include "../src/real.hpp"

using namespace piranha;
using math::pow;
using math::invert;

BOOST_AUTO_TEST_CASE(invert_test_00)
{
	environment env;
	// Some tests with non-piranha types.
	BOOST_CHECK(is_invertible<float>::value);
	BOOST_CHECK(is_invertible<double>::value);
	BOOST_CHECK(is_invertible<long double>::value);
	BOOST_CHECK_EQUAL(pow(1.5f,-1),invert(1.5f));
	BOOST_CHECK((std::is_same<double,decltype(invert(1.5f))>::value));
	BOOST_CHECK_EQUAL(pow(1.5,-1),invert(1.5));
	BOOST_CHECK((std::is_same<double,decltype(invert(1.5))>::value));
	BOOST_CHECK_EQUAL(pow(1.5l,-1),invert(1.5l));
	BOOST_CHECK((std::is_same<long double,decltype(invert(1.5l))>::value));
	BOOST_CHECK(!is_invertible<std::string>::value);
	// Test with piranha's scalar types.
	BOOST_CHECK(is_invertible<integer>::value);
	BOOST_CHECK(is_invertible<rational>::value);
	BOOST_CHECK(is_invertible<real>::value);
	BOOST_CHECK_EQUAL(invert(1_z),1);
	BOOST_CHECK_EQUAL(invert(-1_z),-1);
	BOOST_CHECK_EQUAL(invert(2_z),0);
	BOOST_CHECK((std::is_same<integer,decltype(invert(1_z))>::value));
	BOOST_CHECK_EQUAL(invert(1_q),1);
	BOOST_CHECK_EQUAL(invert(1/2_q),2);
	BOOST_CHECK_EQUAL(invert(-2/3_q),-3/2_q);
	BOOST_CHECK((std::is_same<rational,decltype(invert(1_q))>::value));
	BOOST_CHECK_EQUAL(invert(1_r),1);
	BOOST_CHECK_EQUAL(invert(1.5_r),pow(1.5_r,-1));
	BOOST_CHECK_EQUAL(invert(-2.5_r),pow(-2.5_r,-1));
	BOOST_CHECK((std::is_same<real,decltype(invert(1_r))>::value));
	// Test with some series types.
	using p_type = polynomial<rational,monomial<short>>;
	BOOST_CHECK(is_invertible<p_type>::value);
	p_type x{"x"};
	BOOST_CHECK_EQUAL(invert(x),pow(x,-1));
	BOOST_CHECK((std::is_same<p_type,decltype(invert(x))>::value));
	using ps_type = poisson_series<p_type>;
	BOOST_CHECK(is_invertible<ps_type>::value);
	ps_type a{"a"};
	BOOST_CHECK_EQUAL(invert(a),pow(a,-1));
	BOOST_CHECK((std::is_same<ps_type,decltype(invert(a))>::value));
}
