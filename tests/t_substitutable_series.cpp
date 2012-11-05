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

#include "../src/t_substitutable_series.hpp"

#define BOOST_TEST_MODULE t_substitutable_series_test
#include <boost/test/unit_test.hpp>

#include <type_traits>

#include "../src/environment.hpp"
#include "../src/math.hpp"
#include "../src/polynomial.hpp"
#include "../src/poisson_series.hpp"
#include "../src/rational.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(t_subs_series_t_subs_test)
{
	environment env;
	typedef poisson_series<polynomial<rational>> p_type1;
	p_type1 x{"x"}, y{"y"};
	BOOST_CHECK((has_t_subs<p_type1>::value));
	BOOST_CHECK((has_t_subs<p_type1,rational>::value));
	BOOST_CHECK((has_t_subs<p_type1,rational,rational>::value));
	BOOST_CHECK((has_t_subs<p_type1,double>::value));
	BOOST_CHECK((!has_t_subs<p_type1,int,double>::value));
	BOOST_CHECK_EQUAL(p_type1{}.t_subs("a",2,3),0);
	BOOST_CHECK_EQUAL(math::t_subs(p_type1{},"a",2,3),0);
	BOOST_CHECK_EQUAL(p_type1{"x"}.t_subs("a",2,3),p_type1{"x"});
	BOOST_CHECK_EQUAL(math::t_subs(p_type1{"x"},"a",2,3),p_type1{"x"});
	BOOST_CHECK_EQUAL(math::cos(p_type1{"x"}).t_subs("a",2,3),p_type1{"x"}.cos());
	BOOST_CHECK_EQUAL(math::t_subs(math::cos(p_type1{"x"}),"a",2,3),p_type1{"x"}.cos());
	BOOST_CHECK_EQUAL(math::t_subs(math::cos(p_type1{"x"}),"x",2,3),2);
	BOOST_CHECK_EQUAL(math::t_subs(math::sin(p_type1{"x"}),"x",2,3),3);
	BOOST_CHECK_EQUAL(math::t_subs(math::cos(p_type1{"x"})+math::sin(p_type1{"x"}),"x",2,3),5);
	auto tmp1 = math::t_subs(4 * math::cos(p_type1{"x"}+p_type1{"y"}) +
		3 * math::sin(p_type1{"x"}+p_type1{"y"}),"x",2,3);
	BOOST_CHECK((std::is_same<decltype(tmp1),p_type1>::value));
	BOOST_CHECK_EQUAL(tmp1,4*2*math::cos(y)-4*3*math::sin(y)+3*3*math::cos(y)+3*2*math::sin(y));
	auto tmp2 = math::t_subs(4 * math::cos(p_type1{"x"}-p_type1{"y"}) +
		3 * math::sin(p_type1{"x"}-p_type1{"y"}),"x",2,3);
	BOOST_CHECK((std::is_same<decltype(tmp2),p_type1>::value));
	BOOST_CHECK_EQUAL(tmp2,4*2*math::cos(y)+4*3*math::sin(y)+3*3*math::cos(y)-3*2*math::sin(y));
	auto tmp3 = math::t_subs(4 * math::cos(-p_type1{"x"}-p_type1{"y"}) +
		3 * math::sin(-p_type1{"x"}-p_type1{"y"}),"x",2,3);
	BOOST_CHECK((std::is_same<decltype(tmp3),p_type1>::value));
	BOOST_CHECK_EQUAL(tmp3,4*2*math::cos(y)-4*3*math::sin(y)-3*3*math::cos(y)-3*2*math::sin(y));
}
