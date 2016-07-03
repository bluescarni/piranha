/* Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)

This file is part of the Piranha library.

The Piranha library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The Piranha library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the Piranha library.  If not,
see https://www.gnu.org/licenses/. */

#ifndef PIRANHA_GASTINEAU4_HPP
#define PIRANHA_GASTINEAU4_HPP

#include <boost/timer/timer.hpp>

#include "../src/polynomial.hpp"

namespace piranha
{

template <typename Cf,typename Key>
inline polynomial<Cf,Key> gastineau4()
{
	typedef polynomial<Cf,Key> p_type;
	p_type z("z"), t("t"), u("u"), x("x"), y("y");

	auto f = (1 + x + y + 2*z*z + 3*t*t*t + 5*u*u*u*u*u);
	auto g = (1 + u + t + 2*z*z + 3*y*y*y + 5*x*x*x*x*x);
	auto tmp_f(f), tmp_g(g);
	for (int i = 1; i < 20; ++i) {
		f *= tmp_f;
		g *= tmp_g;
	}
	{
	boost::timer::auto_cpu_timer t;
	return f * g;
	}
}

}

#endif
