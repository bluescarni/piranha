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

#ifndef PIRANHA_SAFE_CAST_HPP
#define PIRANHA_SAFE_CAST_HPP

#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/demangle.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/safe_convert.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

// Exception to signal failure in piranha::safe_cast().
class safe_cast_failure final : public std::invalid_argument
{
public:
    using std::invalid_argument::invalid_argument;
};

// NOTE: here we are checking def-ctible, which corresponds to value initialisation, but
// what we are really doing in the body of the function is a default initialisation. I can't
// come up with an example where a type is value-initable and not default-initable, so
// perhaps the distinction here is only academic. In any case, we can in principle write a
// default_initializable concept/type trait using placement new, if needed.
template <typename From, typename To>
using is_safely_castable
    = conjunction<std::is_default_constructible<To>, is_safely_convertible<From, addlref_t<To>>, is_returnable<To>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename From, typename To>
concept bool SafelyCastable = is_safely_castable<From, To>::value;

#endif

#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename To, SafelyCastable<To> From>
#else
template <typename To, typename From, enable_if_t<is_safely_castable<From, To>::value, int> = 0>
#endif
inline To safe_cast(From &&x)
{
    To retval;
    if (likely(piranha::safe_convert(retval, std::forward<From>(x)))) {
        return retval;
    }
    piranha_throw(safe_cast_failure, "the safe conversion of a value of type '" + piranha::demangle<decltype(x)>()
                                         + "' to the type '" + piranha::demangle<To>() + "' failed");
}

inline namespace impl
{

// This is a small helper to verify that the size() of a container
// can be safely converted to the difference type of its iterator type.
// In a few places we use std::distance to determine the size of a range,
// and that could overflow if the range is a container with an (unsigned)
// size which is too large.
template <typename Container>
inline void check_distance_size(Container &c)
{
    (void)piranha::safe_cast<typename std::iterator_traits<decltype(c.begin())>::difference_type>(c.size());
}

} // namespace impl
} // namespace piranha

#endif
