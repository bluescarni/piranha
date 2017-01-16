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

#ifndef PIRANHA_PEARCE1_HPP
#define PIRANHA_PEARCE1_HPP

#include <piranha/polynomial.hpp>

#include "simple_timer.hpp"

namespace piranha
{

template <typename Cf, typename Key>
inline polynomial<Cf, Key> pearce1(unsigned long long factor = 1u)
{
    typedef polynomial<Cf, Key> p_type;
    p_type x("x"), y("y"), z("z"), t("t"), u("u");

    auto f = (x + y + z * z * 2 + t * t * t * 3 + u * u * u * u * u * 5 + 1);
    auto tmp_f(f);
    auto g = (u + t + z * z * 2 + y * y * y * 3 + x * x * x * x * x * 5 + 1);
    auto tmp_g(g);
    for (int i = 1; i < 12; ++i) {
        f *= tmp_f;
        g *= tmp_g;
    }
    if (factor > 1u) {
        f *= factor;
        g *= factor;
    }
    {
        simple_timer t;
        return f * g;
    }
}
}

#endif
