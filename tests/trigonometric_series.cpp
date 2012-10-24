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

#include "../src/trigonometric_series.hpp"

#define BOOST_TEST_MODULE trigonometric_series_test
#include <boost/test/unit_test.hpp>

#include "../src/environment.hpp"
#include "../src/math.hpp"
#include "../src/poisson_series.hpp"
#include "../src/rational.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(trigonometric_series_degree_order_test)
{
	environment env;
	using math::sin;
	using math::cos;
	typedef poisson_series<rational> p_type1;
	p_type1 x{"x"}, y{"y"};
	BOOST_CHECK_EQUAL(x.t_degree(),0);
	BOOST_CHECK_EQUAL(cos(3 * x).t_degree(),3);
	BOOST_CHECK_EQUAL(cos(3 * x - 4 * y).t_degree(),-1);
	BOOST_CHECK_EQUAL((cos(3 * x - 4 * y) + sin(x + y)).t_degree(),2);
	BOOST_CHECK_EQUAL((cos(-3 * x - 4 * y) + sin(-x - y)).t_degree(),7);
	BOOST_CHECK_EQUAL(math::t_degree(cos(-3 * x - 4 * y) + sin(-x - y)),7);
	BOOST_CHECK_EQUAL((cos(-3 * x - 2 * y) + sin(-x + y)).t_degree(),5);
	BOOST_CHECK_EQUAL(cos(2 * x).t_degree({"x"}),2);
	BOOST_CHECK_EQUAL(cos(2 * x).t_degree({"y"}),0);
	BOOST_CHECK_EQUAL((cos(2 * x) + cos(3 * x + y)).t_degree({"x"}),3);
	BOOST_CHECK_EQUAL(math::t_degree((cos(2 * x) + cos(3 * x + y)),{"x"}),3);
	BOOST_CHECK_EQUAL((cos(2 * x) + cos(x + y)).t_degree({"x"}),2);
	BOOST_CHECK_EQUAL((x * cos(2 * x) - y * cos(x + y)).t_degree({"y"}),1);
	BOOST_CHECK_EQUAL((y * cos(x - y)).t_degree({"y"}),-1);
	BOOST_CHECK_EQUAL((y * cos(x - y) + x).t_degree({"y"}),0);
	BOOST_CHECK_EQUAL((y * cos(x - y) + x).t_degree({"y","x","y"}),0);
	BOOST_CHECK_EQUAL((y * cos(x - y) + cos(x + y)).t_degree({"y","x","y","z"}),2);
	BOOST_CHECK_EQUAL((y * cos(x - y) + cos(x + y)).t_degree({"x"}),1);
	BOOST_CHECK_EQUAL((y * sin(x - y) + cos(x + y)).t_degree({}),0);
	BOOST_CHECK_EQUAL(p_type1{}.t_degree({}),0);
	BOOST_CHECK_EQUAL(p_type1{}.t_degree({"x"}),0);
	BOOST_CHECK_EQUAL(p_type1{}.t_degree(),0);
	BOOST_CHECK_EQUAL(p_type1{2}.t_degree(),0);
	// Low trigonometric degree.
	BOOST_CHECK_EQUAL(x.t_ldegree(),0);
	BOOST_CHECK_EQUAL(cos(3 * x).t_ldegree(),3);
	BOOST_CHECK_EQUAL(cos(3 * x - 4 * y).t_ldegree(),-1);
	BOOST_CHECK_EQUAL((cos(3 * x - 4 * y) + sin(x + y)).t_ldegree(),-1);
	BOOST_CHECK_EQUAL((cos(-3 * x - 4 * y) + sin(-x - y)).t_ldegree(),2);
	BOOST_CHECK_EQUAL((cos(-3 * x - 2 * y) + sin(-x + y)).t_ldegree(),0);
	BOOST_CHECK_EQUAL(math::t_ldegree(cos(-3 * x - 2 * y) + sin(-x + y)),0);
	BOOST_CHECK_EQUAL(cos(2 * x).t_ldegree({"x"}),2);
	BOOST_CHECK_EQUAL(cos(2 * x).t_ldegree({"y"}),0);
	BOOST_CHECK_EQUAL((cos(2 * x) + cos(3 * x + y)).t_ldegree({"x"}),2);
	BOOST_CHECK_EQUAL((cos(2 * x) + cos(x + y)).t_ldegree({"x"}),1);
	BOOST_CHECK_EQUAL(math::t_ldegree((x * cos(2 * x) - y * cos(x + y)),{"y"}),0);
	BOOST_CHECK_EQUAL((y * cos(x - y)).t_ldegree({"y"}),-1);
	BOOST_CHECK_EQUAL((y * cos(x - y) + x).t_ldegree({"y"}),-1);
	BOOST_CHECK_EQUAL((y * cos(x - y) + x).t_ldegree({"y","x","y"}),0);
	BOOST_CHECK_EQUAL((y * cos(x - y) + cos(x + y)).t_ldegree({"y","x","y","z"}),0);
	BOOST_CHECK_EQUAL((y * cos(x - y) + cos(x + y)).t_ldegree({"x"}),1);
	BOOST_CHECK_EQUAL((y * sin(x - y) + cos(x + y)).t_ldegree({}),0);
	BOOST_CHECK_EQUAL(p_type1{}.t_ldegree({}),0);
	BOOST_CHECK_EQUAL(p_type1{}.t_ldegree({"x"}),0);
	BOOST_CHECK_EQUAL(p_type1{}.t_ldegree(),0);
	BOOST_CHECK_EQUAL(p_type1{2}.t_ldegree(),0);
	// Order tests.
	BOOST_CHECK_EQUAL(x.t_order(),0);
	BOOST_CHECK_EQUAL(cos(3 * x).t_order(),3);
	BOOST_CHECK_EQUAL(cos(3 * x - 4 * y).t_order(),7);
	BOOST_CHECK_EQUAL((cos(3 * x - 4 * y) + sin(x + y)).t_order(),7);
	BOOST_CHECK_EQUAL((cos(-3 * x - 4 * y) + sin(-x - y)).t_order(),7);
	BOOST_CHECK_EQUAL((cos(-3 * x - 2 * y) + sin(-x + y)).t_order(),5);
	BOOST_CHECK_EQUAL(math::t_order(cos(-3 * x - 2 * y) + sin(-x + y)),5);
	BOOST_CHECK_EQUAL(cos(2 * x).t_order({"x"}),2);
	BOOST_CHECK_EQUAL(cos(2 * x).t_order({"y"}),0);
	BOOST_CHECK_EQUAL((cos(2 * x) + cos(3 * x + y)).t_order({"x"}),3);
	BOOST_CHECK_EQUAL(math::t_order((cos(2 * x) + cos(3 * x + y)),{"x"}),3);
	BOOST_CHECK_EQUAL((cos(2 * x) + cos(x + y)).t_order({"x"}),2);
	BOOST_CHECK_EQUAL((x * cos(2 * x) - y * cos(x + y)).t_order({"y"}),1);
	BOOST_CHECK_EQUAL((y * cos(x - y)).t_order({"y"}),1);
	BOOST_CHECK_EQUAL((y * cos(x - y) + x).t_order({"y"}),1);
	BOOST_CHECK_EQUAL((y * cos(x - y) + x).t_order({"y","x","y"}),2);
	BOOST_CHECK_EQUAL((y * cos(x - y) + cos(x + y)).t_order({"y","x","y","z"}),2);
	BOOST_CHECK_EQUAL((y * cos(x - y) + cos(x + y)).t_order({"x"}),1);
	BOOST_CHECK_EQUAL((y * sin(x - y) + cos(x + y)).t_order({}),0);
	BOOST_CHECK_EQUAL(p_type1{}.t_order({}),0);
	BOOST_CHECK_EQUAL(p_type1{}.t_order({"x"}),0);
	BOOST_CHECK_EQUAL(p_type1{}.t_order(),0);
	BOOST_CHECK_EQUAL(p_type1{2}.t_order(),0);
	// Low trigonometric order.
	BOOST_CHECK_EQUAL(x.t_lorder(),0);
	BOOST_CHECK_EQUAL(cos(3 * x).t_lorder(),3);
	BOOST_CHECK_EQUAL(cos(3 * x - 4 * y).t_lorder(),7);
	BOOST_CHECK_EQUAL((cos(3 * x - 4 * y) + sin(x + y)).t_lorder(),2);
	BOOST_CHECK_EQUAL((cos(-3 * x - 4 * y) + sin(-x - y)).t_lorder(),2);
	BOOST_CHECK_EQUAL((cos(-3 * x - 2 * y) + sin(-x + y)).t_lorder(),2);
	BOOST_CHECK_EQUAL(math::t_lorder(cos(-3 * x - 2 * y) + sin(-x + y)),2);
	BOOST_CHECK_EQUAL(cos(2 * x).t_lorder({"x"}),2);
	BOOST_CHECK_EQUAL(cos(2 * x).t_lorder({"y"}),0);
	BOOST_CHECK_EQUAL((cos(2 * x) + cos(3 * x + y)).t_lorder({"x"}),2);
	BOOST_CHECK_EQUAL((cos(2 * x) + cos(x + y)).t_lorder({"x"}),1);
	BOOST_CHECK_EQUAL((x * cos(2 * x) - y * cos(x + y)).t_lorder({"y"}),0);
	BOOST_CHECK_EQUAL((y * cos(x - y)).t_lorder({"y"}),1);
	BOOST_CHECK_EQUAL((y * cos(x - y) + x).t_lorder({"y"}),0);
	BOOST_CHECK_EQUAL((y * cos(x - y) + x).t_lorder({"y","x","y"}),0);
	BOOST_CHECK_EQUAL((y * cos(x - y) + cos(x + y)).t_lorder({"y","x","y","z"}),2);
	BOOST_CHECK_EQUAL(math::t_lorder((y * cos(x - y) + cos(x + y)),{"y","x","y","z"}),2);
	BOOST_CHECK_EQUAL((y * cos(x - y) + cos(x + y)).t_lorder({"x"}),1);
	BOOST_CHECK_EQUAL((y * sin(x - y) + cos(x + y)).t_lorder({}),0);
	BOOST_CHECK_EQUAL(p_type1{}.t_lorder({}),0);
	BOOST_CHECK_EQUAL(p_type1{}.t_lorder({"x"}),0);
	BOOST_CHECK_EQUAL(p_type1{}.t_lorder(),0);
	BOOST_CHECK_EQUAL(p_type1{2}.t_lorder(),0);
	// Type traits checks.
	BOOST_CHECK(has_t_degree<p_type1>::value);
	BOOST_CHECK(has_t_ldegree<p_type1>::value);
	BOOST_CHECK(has_t_order<p_type1>::value);
	BOOST_CHECK(has_t_lorder<p_type1>::value);
}
