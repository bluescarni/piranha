/* Copyright 2009-2017 Francesco Biscani (bluescarni@gmail.com)

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

#ifndef PIRANHA_GASTINEAU3_HPP
#define PIRANHA_GASTINEAU3_HPP

#include <piranha/polynomial.hpp>

#include "simple_timer.hpp"

namespace piranha
{

template <typename Cf, typename Key>
inline polynomial<Cf, Key> gastineau3()
{
    typedef polynomial<Cf, Key> p_type;
    p_type u("u"), v("v"), w("w"), x("x"), y("y");

    auto f = (1 + u * u + v + w * w + x - y * y);
    auto g = (1 + u + v * v + w + x * x + y * y * y);
    auto tmp_f(f), tmp_g(g);
    for (int i = 1; i < 28; ++i) {
        f *= tmp_f;
        g *= tmp_g;
    }
    g += 1;
    {
        simple_timer t;
        return f * g;
    }
}
}

#endif
