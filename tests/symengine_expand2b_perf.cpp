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

#define BOOST_TEST_MODULE symengine_expand2b_test
#include <boost/test/unit_test.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/timer/timer.hpp>

#include "../src/environment.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/math.hpp"
#include "../src/mp_integer.hpp"
#include "../src/settings.hpp"

using namespace piranha;
using math::pow;

// Test taken from:
// https://github.com/sympy/symengine/blob/master/benchmarks/expand2b.cpp

BOOST_AUTO_TEST_CASE(symengine_expand2b_test)
{
	environment env;
	if (boost::unit_test::framework::master_test_suite().argc > 1) {
		settings::set_n_threads(boost::lexical_cast<unsigned>(boost::unit_test::framework::master_test_suite().argv[1u]));
	}
	using p_type = polynomial<integer,k_monomial>;
	auto x = p_type{"x"}, y = p_type{"y"}, z = p_type{"z"}, w = p_type{"w"};
	auto f = pow(x+y+z+w,15);
	auto g = f + w;
	{
	boost::timer::auto_cpu_timer t;
	BOOST_CHECK_EQUAL((f * g).size(),6272u);
	}
}
