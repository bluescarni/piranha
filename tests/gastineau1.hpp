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

#ifndef PIRANHA_GASTINEAU1_HPP
#define PIRANHA_GASTINEAU1_HPP

#include "../src/polynomial.hpp"
#include "simple_timer.hpp"

namespace piranha
{

template <typename Cf, typename Key>
inline polynomial<Cf, Key> gastineau1()
{
    typedef polynomial<Cf, Key> p_type;
    p_type x("x"), y("y"), z("z"), t("t");
    auto f = x + y + z + t + 1;
    auto tmp(f);
    for (auto i = 1; i < 40; ++i) {
        f *= tmp;
    }
    {
        simple_timer t;
        return f * (f + 1);
    }
}
}

#endif
