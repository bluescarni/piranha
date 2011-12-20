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

#define BOOST_TEST_MODULE fateman1_test
#include <boost/test/unit_test.hpp>

#include "../src/kronecker_monomial.hpp"

using namespace piranha;

// Fateman's polynomial multiplication test number 1. Calculate:
// f * (f+1)
// where f = (1+x+y+z+t)**20

BOOST_AUTO_TEST_CASE(fateman1_test)
{
	typedef polynomial<double,kronecker_monomial<>> p_type;
	p_type x("x"), y("y"), z("z"), t("t");
	auto f = x + y + z + t + 1, tmp(f);
	for (auto i = 1; i < 20; ++i) {
		f *= tmp;
	}
	auto retval = f * (f + 1);
	BOOST_CHECK_EQUAL(retval.size(),135751u);
}
