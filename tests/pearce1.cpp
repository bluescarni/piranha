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

#define BOOST_TEST_MODULE pearce1_test
#include <boost/test/unit_test.hpp>

#include "../src/kronecker_monomial.hpp"

using namespace piranha;

// Pearce's polynomial multiplication test number 1. Calculate:
// f * g
// where
// f = (1 + x + y + 2*z**2 + 3*t**3 + 5*u**5)**12
// g = (1 + u + t + 2*z**2 + 3*y**3 + 5*x**5)**12

BOOST_AUTO_TEST_CASE(pearce1_test)
{
	typedef polynomial<double,kronecker_monomial<>> p_type;
	p_type x("x"), y("y"), z("z"), t("t"), u("u");

	auto f = (x + y + z*z*2 + t*t*t*3 + u*u*u*u*u*5 + 1);
	auto tmp_f(f);
	auto g = (u + t + z*z*2 + y*y*y*3 + x*x*x*x*x*5 + 1);
	auto tmp_g(g);
	for (int i = 1; i < 12; ++i) {
		f *= tmp_f;
		g *= tmp_g;
	}
	auto retval = f * g;
	BOOST_CHECK_EQUAL(retval.size(),5821335u);
}
