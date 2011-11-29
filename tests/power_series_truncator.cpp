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

#include "../src/power_series_truncator.hpp"

#define BOOST_TEST_MODULE power_series_truncator_test
#include <boost/test/unit_test.hpp>

#include "../src/degree_truncator_settings.hpp"
#include "../src/integer.hpp"
#include "../src/polynomial_term.hpp"
#include "../src/symbol.hpp"
#include "../src/symbol_set.hpp"

using namespace piranha;

struct g_truncator: public power_series_truncator
{
	g_truncator(): power_series_truncator()
	{
		if (!is_active()) {
			return;
		}
		degree_truncator_settings dts;
		typedef polynomial_term<double,int> term_type1;
		typedef typename term_type1::key_type key_type1;
		typedef polynomial_term<integer,long> term_type2;
		typedef typename term_type2::key_type key_type2;
		term_type1 t1a(3,key_type1{1}), t1b(1,key_type1{2});
		term_type2 t2a(3,key_type2{1}), t2b(1,key_type2{2});
		BOOST_CHECK(compare_ldegree(t1a,t1b,symbol_set{symbol("x")}));
		BOOST_CHECK(!compare_ldegree(t1b,t1a,symbol_set{symbol("x")}));
		BOOST_CHECK(compare_ldegree(t2a,t2b,symbol_set{symbol("x")}));
		BOOST_CHECK(!compare_ldegree(t2b,t2a,symbol_set{symbol("x")}));
		if (dts.get_args().size()) {
			term_type1 t1a(3,key_type1{1,2}), t1b(1,key_type1{2,0});
			term_type2 t2a(3,key_type2{1,2}), t2b(1,key_type2{2,0});
			BOOST_CHECK(compare_pldegree(t1a,t1b,symbol_set{symbol("x"),symbol("y")}));
			BOOST_CHECK(!compare_pldegree(t1b,t1a,symbol_set{symbol("x"),symbol("y")}));
			BOOST_CHECK(compare_pldegree(t2a,t2b,symbol_set{symbol("x"),symbol("y")}));
			BOOST_CHECK(!compare_pldegree(t2b,t2a,symbol_set{symbol("x"),symbol("y")}));
		}
	}
};

BOOST_AUTO_TEST_CASE(power_series_truncator_test)
{
	g_truncator gt0;
	BOOST_CHECK(!gt0.is_active());
	degree_truncator_settings dts;
	dts.set(3);
	g_truncator gt1;
	BOOST_CHECK(gt1.is_active());
	dts.set("x",3);
	g_truncator gt2;
	BOOST_CHECK(gt2.is_active());
}
