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

#ifndef PIRANHA_SFINAE_TYPES_HPP
#define PIRANHA_SFINAE_TYPES_HPP

#include <array>

namespace piranha
{

namespace detail
{

// Types for SFINAE-based type traits.
// Guidelines for usage:
// - decltype-based SFINAE,
// - use std::is_same in the value of the type trait,
// - use (...,void(),yes/no()) in the decltype in order to avoid problems with
//   overloads of the comma operator.
struct sfinae_types
{
	struct yes {};
	struct no {};
};

}

}

#endif
