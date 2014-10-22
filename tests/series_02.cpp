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

#include "../src/series.hpp"

#define BOOST_TEST_MODULE series_02_test
#include <boost/test/unit_test.hpp>

#include <type_traits>

#include "../src/environment.hpp"
#include "../src/forwarding.hpp"
#include "../src/math.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/polynomial_term.hpp"

using namespace piranha;

template <typename Cf, typename Expo>
class g_series_type: public series<polynomial_term<Cf,Expo>,g_series_type<Cf,Expo>>
{
	public:
		typedef series<polynomial_term<Cf,Expo>,g_series_type<Cf,Expo>> base;
		g_series_type() = default;
		g_series_type(const g_series_type &) = default;
		g_series_type(g_series_type &&) = default;
		explicit g_series_type(const char *name):base()
		{
			typedef typename base::term_type term_type;
			// Insert the symbol.
			this->m_symbol_set.add(name);
			// Construct and insert the term.
			this->insert(term_type(Cf(1),typename term_type::key_type{Expo(1)}));
		}
		g_series_type &operator=(const g_series_type &) = default;
		g_series_type &operator=(g_series_type &&) = default;
		PIRANHA_FORWARDING_CTOR(g_series_type,base)
		PIRANHA_FORWARDING_ASSIGNMENT(g_series_type,base)
};

BOOST_AUTO_TEST_CASE(series_partial_test)
{
	typedef g_series_type<rational,int> p_type1;
	p_type1 x1{"x"};
	BOOST_CHECK(is_differentiable<p_type1>::value);
	BOOST_CHECK((std::is_same<decltype(x1.partial("foo")),p_type1>::value));
	p_type1 x{"x"}, y{"y"};
	BOOST_CHECK_EQUAL(math::partial(x,"x"),1);
	BOOST_CHECK_EQUAL(math::partial(x,"y"),0);
	BOOST_CHECK_EQUAL(math::partial(-4 * x.pow(2),"x"),-8 * x);
	BOOST_CHECK_EQUAL(math::partial(-4 * x.pow(2) + y * x,"y"),x);
	BOOST_CHECK_EQUAL(math::partial(math::partial(-4 * x.pow(2),"x"),"x"),-8);
	BOOST_CHECK_EQUAL(math::partial(math::partial(math::partial(-4 * x.pow(2),"x"),"x"),"x"),0);
	BOOST_CHECK_EQUAL(math::partial(-x + 1,"x"),-1);
	BOOST_CHECK_EQUAL(math::partial((1 + 2 * x).pow(10),"x"),20 * (1 + 2 * x).pow(9));
	BOOST_CHECK_EQUAL(math::partial((1 + 2 * x + y).pow(10),"x"),20 * (1 + 2 * x + y).pow(9));
	BOOST_CHECK_EQUAL(math::partial(x * (1 + 2 * x + y).pow(10),"x"),20 * x * (1 + 2 * x + y).pow(9) + (1 + 2 * x + y).pow(10));
	BOOST_CHECK(math::partial((1 + 2 * x + y).pow(0),"x").empty());
	// Custom derivatives.
	p_type1::register_custom_derivative("x",[](const p_type1 &) {return p_type1{rational(1,314)};});
	BOOST_CHECK_EQUAL(math::partial(x,"x"),rational(1,314));
	p_type1::register_custom_derivative("x",[](const p_type1 &) {return p_type1{rational(1,315)};});
	BOOST_CHECK_EQUAL(math::partial(x,"x"),rational(1,315));
	p_type1::unregister_custom_derivative("x");
	p_type1::unregister_custom_derivative("x");
	BOOST_CHECK_EQUAL(math::partial(x,"x"),1);
	// y as implicit function of x: y = x**2.
	p_type1::register_custom_derivative("x",[x](const p_type1 &p) -> p_type1 {
		return p.partial("x") + math::partial(p,"y") * 2 * x;
	});
	BOOST_CHECK_EQUAL(math::partial(x + y,"x"),1 + 2 * x);
	p_type1::unregister_custom_derivative("y");
	p_type1::unregister_custom_derivative("x");
	BOOST_CHECK_EQUAL(math::partial(x + y,"x"),1);
	BOOST_CHECK_EQUAL(math::partial(x + 2 * y,"y"),2);
	p_type1::register_custom_derivative("x",[](const p_type1 &p) {return p.partial("x");});
	BOOST_CHECK_EQUAL(math::partial(x + y,"x"),1);
	BOOST_CHECK_EQUAL(math::partial(x + y * x,"x"),y + 1);
	p_type1::register_custom_derivative("x",[x](const p_type1 &p) -> p_type1 {
		return p.partial("x") + math::partial(p,"y") * 2 * x;
	});
	p_type1::register_custom_derivative("y",[](const p_type1 &p) -> p_type1 {
		return 2 * p;
	});
	BOOST_CHECK_EQUAL(math::partial(x + y,"x"),1 + 4 * x * (x + y));
	BOOST_CHECK_EQUAL(math::partial(x + y,"y"),2 * (x + y));
	p_type1::unregister_all_custom_derivatives();
	BOOST_CHECK_EQUAL(math::partial(x + y,"x"),1);
	BOOST_CHECK_EQUAL(math::partial(x + 3 * y,"y"),3);
	typedef g_series_type<integer,rational> p_type2;
	p_type2 x2{"x"};
	BOOST_CHECK(is_differentiable<p_type2>::value);
	// NOTE: this will have to be changed once we rework series arithmetics with mixed
	// types. The correct type then will be not p_type2 but g_series_type<rational,int>.
	// This should anyway trigger the second algorithm, the non-optimised one.
	BOOST_CHECK((std::is_same<decltype(x2.partial("foo")),p_type2>::value));
}
