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

#include "../src/polynomial.hpp"

#define BOOST_TEST_MODULE kronecker_polynomial_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include "../src/degree_truncator_settings.hpp"
#include "../src/integer.hpp"
#include "../src/kronecker_array.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/settings.hpp"

using namespace piranha;

typedef boost::mpl::vector<double,integer> cf_types;

// Tests specific for the polynomial multiplier specialisation for Kronecker monomials.
struct multiplication_tester
{
	template <typename Cf>
	void operator()(const Cf &)
	{
		degree_truncator_settings::unset();
		typedef kronecker_array<std::int_least8_t> ka1;
		typedef polynomial<Cf,kronecker_monomial<std::int_least8_t>> p_type1;
		// Test for empty series.
		BOOST_CHECK((p_type1{} * p_type1{}).empty());
		p_type1 tmp = p_type1("x") * p_type1("y"), xy(tmp);
		BOOST_CHECK((p_type1{} * xy).empty());
		BOOST_CHECK((xy * p_type1{}).empty());
		// Check for correct throwing on overflow.
		for (std::int_least8_t i = 2; tmp.degree({"x"}) < std::get<0u>(ka1::get_limits()[2u])[0u]; ++i) {
			tmp *= p_type1("x");
			BOOST_CHECK_EQUAL(i,tmp.degree({"x"}));
			BOOST_CHECK_EQUAL(1,tmp.degree({"y"}));
		}
		BOOST_CHECK_THROW(tmp * xy,std::overflow_error);
		BOOST_CHECK_THROW(xy * tmp,std::overflow_error);
		typedef polynomial<Cf,kronecker_monomial<std::int_least32_t>> p_type2;
		p_type2 y("y"), z("z"), t("t"), u("u");
		auto f = 1 + p_type2("x") + y + z + t;
		auto tmp2 = f;
		for (int i = 1; i < 10; ++i) {
			f *= tmp2;
		}
		auto g = f + 1;
		auto retval = f * g;
		auto r_no_trunc = retval;
		BOOST_CHECK_EQUAL(retval.size(),10626u);
		degree_truncator_settings::set(0);
		f = 1 + p_type2("x") + y + z + t;
		tmp2 = f;
		for (int i = 1; i < 10; ++i) {
			f *= tmp2;
		}
		g = f + 1;
		retval = f * g;
		BOOST_CHECK_EQUAL(retval.size(),0u);
		degree_truncator_settings::set(1);
		f = 1 + p_type2("x") + y + z + t;
		tmp2 = f;
		for (int i = 1; i < 10; ++i) {
			f *= tmp2;
		}
		g = f + 1;
		retval = f * g;
		BOOST_CHECK_EQUAL(retval.size(),1u);
		BOOST_CHECK(retval.degree() == 0);
		degree_truncator_settings::set(10);
		f = 1 + p_type2("x") + y + z + t;
		tmp2 = f;
		for (int i = 1; i < 10; ++i) {
			f *= tmp2;
		}
		g = f + 1;
		retval = f * g;
		BOOST_CHECK(retval.degree() == 9);
		BOOST_CHECK((r_no_trunc - retval).degree() >= 9);
		degree_truncator_settings::unset();
		// NOTE: this test is going to be exact in case of coefficients cancellations with double
		// precision coefficients only if the platform has ieee 754 format (integer exactly representable
		// as doubles up to 2 ** 53).
		if (std::is_same<Cf,double>::value && (!std::numeric_limits<double>::is_iec559 ||
			std::numeric_limits<double>::digits < 53))
		{
			return;
		}
		// Dense case with cancellations, default setup.
		auto h = 1 - p_type2("x") + y + z + t;
		f = 1 + p_type2("x") + y + z + t;
		tmp2 = h;
		auto tmp3 = f;
		for (int i = 1; i < 10; ++i) {
			h *= tmp2;
			f *= tmp3;
		}
		retval = f * h;
		BOOST_CHECK_EQUAL(retval.size(),5786u);
	}
};

BOOST_AUTO_TEST_CASE(kronecker_polynomial_multiplier_test)
{
	boost::mpl::for_each<cf_types>(multiplication_tester());
	for (unsigned i = 1u; i <= 4u; ++i) {
		settings::set_n_threads(i);
		boost::mpl::for_each<cf_types>(multiplication_tester());
	}
}
