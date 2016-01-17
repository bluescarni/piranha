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

#ifndef PIRANHA_DETAIL_CF_MULT_IMPL_HPP
#define PIRANHA_DETAIL_CF_MULT_IMPL_HPP

#include <type_traits>
#include <utility>

#include "../is_cf.hpp"
#include "../mp_rational.hpp"
#include "../type_traits.hpp"
#include "series_fwd.hpp"

namespace piranha
{

namespace detail
{

// Overload if the coefficient is a series.
template <typename Cf, typename std::enable_if<std::is_base_of<detail::series_tag,Cf>::value,int>::type = 0>
inline void cf_mult_impl(Cf &out_cf, const Cf &cf1, const Cf &cf2)
{
	out_cf = cf1 * cf2;
}

// Overload if the coefficient is a rational.
template <typename Cf, typename std::enable_if<detail::is_mp_rational<Cf>::value,int>::type = 0>
inline void cf_mult_impl(Cf &out_cf, const Cf &cf1, const Cf &cf2)
{
	out_cf._num() = cf1.num();
	out_cf._num() *= cf2.num();
}

// Overload if the coefficient is not a series and not a rational.
template <typename Cf, typename std::enable_if<!std::is_base_of<detail::series_tag,Cf>::value && !detail::is_mp_rational<Cf>::value,int>::type = 0>
inline void cf_mult_impl(Cf &out_cf, const Cf &cf1, const Cf &cf2)
{
	out_cf = cf1;
	out_cf *= cf2;
}

// Enabler for the functions above.
template <typename Cf>
using cf_mult_enabler = typename std::enable_if<std::is_same<decltype(std::declval<const Cf &>() * std::declval<const Cf &>()),Cf>::value &&
	is_multipliable_in_place<Cf>::value && is_cf<Cf>::value && std::is_copy_assignable<Cf>::value>::type;

}

}

#endif
