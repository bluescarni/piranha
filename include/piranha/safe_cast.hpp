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
    piranha_throw(safe_cast_failure, "the safe conversion of a value of type '" + demangle<decltype(x)>()
                                         + "' to the type '" + demangle<To>() + "' failed");
}

// Input iterator whose ref type is safely castable to To.
// NOTE: the way this is currently written we are in the situation in which:
// - the iterator being dereferenced is an lvalue (see definition of det_deref_t),
// - we are checking the expression safe_cast<To>(*it), that is, we are applying
//   safe_cast() directly to the rvalue result of the dereferencing (rather than, say,
//   storing the dereference somewhere and casting it later as an lvalue).
template <typename T, typename To>
using is_safely_castable_input_iterator = conjunction<is_input_iterator<T>, is_safely_castable<det_deref_t<T>, To>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename To>
concept bool SafelyCastableInputIterator = is_safely_castable_input_iterator<T, To>::value;

#endif

// Forward iterator whose ref type is safely castable to To.
template <typename T, typename To>
using is_safely_castable_forward_iterator = conjunction<is_forward_iterator<T>, is_safely_castable<det_deref_t<T>, To>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename To>
concept bool SafelyCastableForwardIterator = is_safely_castable_forward_iterator<T, To>::value;

#endif

// Mutable forward iterator whose ref type is safely castable to To.
template <typename T, typename To>
using is_safely_castable_mutable_forward_iterator
    = conjunction<is_mutable_forward_iterator<T>, is_safely_castable<det_deref_t<T>, To>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename To>
concept bool SafelyCastableMutableForwardIterator = is_safely_castable_mutable_forward_iterator<T, To>::value;

#endif

// Input range whose ref type is safely castable to To.
template <typename T, typename To>
using is_safely_castable_input_range
    // NOTE: we avoid re-using is_safely_castable_input_iterator in the implementation:
    // we already know from is_input_range that the iterator type of T is an input iterator,
    // we just need to check for the safe castability.
    = conjunction<is_input_range<T>, is_safely_castable<det_deref_t<detected_t<begin_adl::type, T>>, To>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename To>
concept bool SafelyCastableInputRange = is_safely_castable_input_range<T, To>::value;

#endif

// Forward range whose ref type is safely castable to To.
template <typename T, typename To>
using is_safely_castable_forward_range
    = conjunction<is_forward_range<T>, is_safely_castable<det_deref_t<detected_t<begin_adl::type, T>>, To>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename To>
concept bool SafelyCastableForwardRange = is_safely_castable_forward_range<T, To>::value;

#endif

// Mutable forward range whose ref type is safely castable to To.
template <typename T, typename To>
using is_safely_castable_mutable_forward_range
    = conjunction<is_mutable_forward_range<T>, is_safely_castable<det_deref_t<detected_t<begin_adl::type, T>>, To>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename To>
concept bool SafelyCastableMutableForwardRange = is_safely_castable_mutable_forward_range<T, To>::value;

#endif
} // namespace piranha

#endif
