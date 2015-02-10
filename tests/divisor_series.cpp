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

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <string>
#include <type_traits>

#include "../src/divisor.hpp"
#include "../src/environment.hpp"
#include "../src/math.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/polynomial.hpp"
#include "../src/real.hpp"

using namespace piranha;

typedef boost::mpl::vector<double,integer,real,rational,polynomial<rational,monomial<int>>> cf_types;

struct test_00_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using s_type = divisor_series<T,divisor<short>>;
		s_type s0{3};
		// Just test some math operations and common functionalities. The class does
		// not support much.
		BOOST_CHECK_EQUAL(s0 + s0,6);
		BOOST_CHECK_EQUAL(s0 * s0,9);
		BOOST_CHECK_EQUAL(s0 * 4,12);
		BOOST_CHECK_EQUAL(4 * s0,12);
		BOOST_CHECK_EQUAL(math::pow(s0,3),27);
		BOOST_CHECK_EQUAL(math::cos(s_type{0}),1);
		BOOST_CHECK_EQUAL(math::sin(s_type{0}),0);
		BOOST_CHECK_EQUAL(math::evaluate<int>(math::pow(s0,3),{{"x",4}}),27);
		BOOST_CHECK(!is_differentiable<s_type>::value);
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
