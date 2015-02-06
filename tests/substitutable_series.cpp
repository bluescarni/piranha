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

#include "../src/substitutable_series.hpp"

#define BOOST_TEST_MODULE substitutable_series_test
#include <boost/test/unit_test.hpp>

#include <string>

#include "../src/environment.hpp"
#include "../src/forwarding.hpp"
#include "../src/math.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/real.hpp"
#include "../src/serialization.hpp"
#include "../src/series.hpp"

using namespace piranha;

template <typename Cf, typename Expo>
class g_series_type: public substitutable_series<series<Cf,monomial<Expo>,g_series_type<Cf,Expo>>,g_series_type<Cf,Expo>>
{
		using base = substitutable_series<series<Cf,monomial<Expo>,g_series_type<Cf,Expo>>,g_series_type<Cf,Expo>>;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
	public:
		template <typename Cf2>
		using rebind = g_series_type<Cf2,Expo>;
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

BOOST_AUTO_TEST_CASE(subs_series_subs_test)
{
	environment env;
	// Substitution on key only.
	using stype0 = g_series_type<rational,int>;
	// Type trait checks.
	BOOST_CHECK((has_subs<stype0,int>::value));
	BOOST_CHECK((has_subs<stype0,double>::value));
	BOOST_CHECK((has_subs<stype0,integer>::value));
	BOOST_CHECK((has_subs<stype0,rational>::value));
	BOOST_CHECK((has_subs<stype0,real>::value));
	BOOST_CHECK((!has_subs<stype0,std::string>::value));
	stype0 x{"x"}, y{"y"}, z{"z"};
	auto tmp = (x + y).subs("x",2);
	BOOST_CHECK_EQUAL(tmp,y + 2);
	BOOST_CHECK(tmp.is_identical(math::subs(x+y,"x",2)));
	BOOST_CHECK(tmp.is_identical(y + 2 + x - x));
	BOOST_CHECK((std::is_same<decltype(tmp),stype0>::value));
	auto tmp2 = (x + y).subs("x",2.);
	BOOST_CHECK_EQUAL(tmp2,y + 2.);
	BOOST_CHECK(tmp2.is_identical(math::subs(x+y,"x",2.)));
	BOOST_CHECK((std::is_same<decltype(tmp2),g_series_type<double,int>>::value));
	auto tmp3 = (3*x + y*y/7).subs("y",2/5_q);
	BOOST_CHECK(tmp3.is_identical(math::subs(3*x + y*y/7,"y",2/5_q)));
	BOOST_CHECK((std::is_same<decltype(tmp3),stype0>::value));
	BOOST_CHECK_EQUAL(tmp3,3*x + 2/5_q * 2/5_q / 7);
	auto tmp4 = (3*x + y*y/7).subs("y",2.123_r);
	BOOST_CHECK(tmp4.is_identical(math::subs(3*x + y*y/7,"y",2.123_r)));
	BOOST_CHECK((std::is_same<decltype(tmp4),g_series_type<real,int>>::value));
	BOOST_CHECK_EQUAL(tmp4,3*x + math::pow(2.123_r,2) / 7);
	auto tmp5 = (3*x + y*y/7).subs("y",-2_z);
	BOOST_CHECK(tmp5.is_identical(math::subs(3*x + y*y/7,"y",-2_z)));
	BOOST_CHECK((std::is_same<decltype(tmp5),stype0>::value));
	BOOST_CHECK_EQUAL(tmp5,3*x + math::pow(-2_z,2) / 7_q);
}
