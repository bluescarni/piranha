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

#ifndef PIRANHA_POLYNOMIAL_FWD_HPP
#define PIRANHA_POLYNOMIAL_FWD_HPP

#include <type_traits>

#include "../series.hpp"

namespace piranha
{

namespace detail
{

#if !defined(PIRANHA_DOXYGEN_INVOKED)

// Polynomial tag struct to work around is_instace_of bug in GCC.
struct polynomial_tag;

// Test if a series type has at least one polynomial
// in the coefficient hierarchy.
template <typename T, typename = void>
struct poly_in_cf
{
	static const bool value = false;
};

template <typename T>
struct poly_in_cf<T,typename std::enable_if<(series_recursion_index<T>::value > 0u) &&
	std::is_base_of<polynomial_tag,typename T::term_type::cf_type>::value>::type>
{
	static const bool value = true;
};

template <typename T>
struct poly_in_cf<T,typename std::enable_if<(series_recursion_index<T>::value > 0u) &&
	!std::is_base_of<polynomial_tag,typename T::term_type::cf_type>::value>::type>
{
	static const bool value = poly_in_cf<typename T::term_type::cf_type>::value;
};

}

// Forward declaration of polynomial class.
template <typename, typename>
class polynomial;

#endif

}

#endif
