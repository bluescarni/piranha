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

#ifndef PIRANHA_DETAIL_CF_MULT_IMPL_HPP
#define PIRANHA_DETAIL_CF_MULT_IMPL_HPP

#include <type_traits>
#include <utility>

#include <piranha/is_cf.hpp>
#include <piranha/math.hpp>
#include <piranha/mp_rational.hpp>

namespace piranha
{

namespace detail
{

// Overload if the coefficient is a rational.
template <typename Cf, typename std::enable_if<detail::is_mp_rational<Cf>::value, int>::type = 0>
inline void cf_mult_impl(Cf &out_cf, const Cf &cf1, const Cf &cf2)
{
    math::mul3(out_cf._num(), cf1.num(), cf2.num());
}

// Overload if the coefficient is not a rational.
template <typename Cf, typename std::enable_if<!detail::is_mp_rational<Cf>::value, int>::type = 0>
inline void cf_mult_impl(Cf &out_cf, const Cf &cf1, const Cf &cf2)
{
    math::mul3(out_cf, cf1, cf2);
}

// Enabler for the functions above.
template <typename Cf>
using cf_mult_enabler = typename std::enable_if<is_cf<Cf>::value && has_mul3<Cf>::value>::type;
}
}

#endif
