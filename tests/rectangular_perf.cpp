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

#define BOOST_TEST_MODULE rectangular_test
#include <boost/test/unit_test.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/timer/timer.hpp>

#include "../src/environment.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/settings.hpp"

using namespace piranha;

// Test taken from:
// http://groups.google.com/group/sage-devel/browse_thread/thread/f5b976c979a3b784/1263afcc6f9d09da
// Meant to test sparse multiplication where series have very different sizes.

BOOST_AUTO_TEST_CASE(rectangular_test)
{
	environment env;
	if (boost::unit_test::framework::master_test_suite().argc > 1) {
		settings::set_n_threads(boost::lexical_cast<unsigned>(boost::unit_test::framework::master_test_suite().argv[1u]));
	}
	typedef polynomial<double,kronecker_monomial<>> p_type;

	auto func = []() -> p_type {
		p_type x("x"), y("y"), z("z");
		auto f = x*y*y*y*z*z + x*x*y*y*z + x*y*y*y*z + x*y*y*z*z + y*y*y*z*z + y*y*y*z +
			2*y*y*z*z + 2*x*y*z + y*y*z + y*z*z + y*y + 2*y*z + z;
		p_type curr(1);
		for (auto i = 1; i <= 70; ++i) {
			curr *= f;
		}
		BOOST_CHECK_EQUAL(curr.size(),1284816u);
		return curr;
	};
	{
	boost::timer::auto_cpu_timer t;
	auto tmp = func();
	}
}
