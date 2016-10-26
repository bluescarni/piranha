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

#ifndef PIRANHA_SAFE_CAST_HPP
#define PIRANHA_SAFE_CAST_HPP

#include <boost/numeric/conversion/cast.hpp>
#include <cmath>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "detail/demangle.hpp"
#include "exceptions.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Exception to signal failure in piranha::safe_cast().
/**
 * This exception should be thrown by specialisations of piranha::safe_cast() in case a value-preserving type
 * conversion cannot be performed.
 *
 * This exception inherits the constructors from \p std::invalid_argument.
 */
struct safe_cast_failure final : std::invalid_argument {
    using std::invalid_argument::invalid_argument;
};

/// Default implementation of piranha::safe_cast().
/**
 * The default implementation of piranha::safe_cast() is activated only when the source type coincides with the target
 * type. A copy of the input value will be returned.
 */
template <typename To, typename From, typename = void>
struct safe_cast_impl {
private:
    template <typename T>
    using enabler = enable_if_t<conjunction<std::is_same<To, T>, std::is_same<From, T>, std::is_copy_constructible<T>>::value, int>;

public:
    /// Call operator.
    /**
     * \note
     * This call operator is enabled only if:
     * - \p T, \p To and \p From are the same type,
     * - \p To is copy-constructible.
     *
     * @param f conversion argument.
     *
     * @return a copy of \p f.
     *
     * @throws unspecified any exception thrown by the copy/move constructor of \p From.
     */
    template <typename T, enabler<T> = 0>
    To operator()(const T &f) const
    {
        return f;
    }
};

inline namespace impl
{

template <typename To, typename From>
using sc_int_int_enabler = enable_if_t<conjunction<std::is_integral<To>, std::is_integral<From>>::value>;
}

/// Specialisation of piranha::safe_cast() for C++ integral types.
/**
 * This specialisation is enabled when both \p To and \p From are integral types.
 */
template <typename To, typename From>
struct safe_cast_impl<To, From, sc_int_int_enabler<To, From>> {
    /// Call operator.
    /**
     * The call operator uses \p boost::numeric_cast() to perform a safe conversion
     * between integral types.
     *
     * @param f conversion argument.
     *
     * @return a copy of \p f cast safely to \p To.
     *
     * @throws piranha::safe_cast_failure if \p boost::numeric_cast() raises an error.
     */
    To operator()(const From &f) const
    {
        try {
            return boost::numeric_cast<To>(f);
        } catch (...) {
            piranha_throw(safe_cast_failure, "the integral value " + std::to_string(f)
                                                 + " cannot be converted to the type '" + detail::demangle<To>()
                                                 + "', as the conversion cannot preserve the original value");
        }
    }
};

inline namespace impl
{

template <typename To, typename From>
using sc_float_to_int_enabler = enable_if_t<conjunction<std::is_integral<To>, std::is_floating_point<From>>::value>;
}

/// Specialisation of piranha::safe_cast() for C++ floating-point to C++ integral conversions.
/**
 * \note
 * This specialisation is enabled if \p From is a C++ floating-point type and \p To is a C++ integral type.
 */
template <typename To, typename From>
struct safe_cast_impl<To, From, sc_float_to_int_enabler<To, From>> {
    /// Call operator.
    /**
     * The call operator will first check that \p f is a finite integral value, and it will then invoke
     * \p boost::numeric_cast() to perform the conversion.
     *
     * @param f conversion argument.
     *
     * @return \p f safely converted to \p To.
     *
     * @throws piranha::safe_cast_failure if:
     * - \p f is not finite or it has a nonzero fractional part,
     * - \p boost::numeric_cast() raises an error.
     */
    To operator()(const From &f) const
    {
        if (unlikely(!std::isfinite(f))) {
            piranha_throw(safe_cast_failure, "the non-finite floating-point value " + std::to_string(f)
                                                 + " cannot be converted to the integral type '"
                                                 + detail::demangle<To>() + "'");
        }
        if (std::trunc(f) != f) {
            piranha_throw(safe_cast_failure, "the floating-point value with nonzero fractional part "
                                                 + std::to_string(f) + " cannot be converted to the integral type '"
                                                 + detail::demangle<To>()
                                                 + "', as the conversion cannot preserve the original value");
        }
        try {
            return boost::numeric_cast<To>(f);
        } catch (...) {
            piranha_throw(safe_cast_failure, "the floating-point value " + std::to_string(f)
                                                 + " cannot be converted to the integral type '"
                                                 + detail::demangle<To>()
                                                 + "', as the conversion cannot preserve the original value");
        }
    }
};

inline namespace impl
{

template <typename To, typename From>
using safe_cast_t_ = decltype(safe_cast_impl<uncvref_t<To>, From>{}(std::declval<const From &>()));

template <typename To, typename From>
using safe_cast_type
    = enable_if_t<conjunction<std::is_same<safe_cast_t_<To, From>, uncvref_t<To>>, is_returnable<uncvref_t<To>>>::value,
                  uncvref_t<To>>;
}

/// Safe cast.
/**
 * This function is meant to be used when it is necessary to convert between two types while making
 * sure that the value is preserved after the conversion. For instance, a safe cast between
 * integral types will check that the input value is representable by the return type, otherwise
 * an error will be raised.
 *
 * The actual implementation of this function is in the piranha::safe_cast_impl functor's
 * call operator. \p To is passed as first template parameter of piranha::safe_cast_impl after the removal
 * of cv/reference qualifiers, whereas \p From is passed as-is. The body of this function is thus equivalent to:
 * @code
 * return safe_cast_impl<uncvref_t<To>, From>{}(x);
 * @endcode
 * (where \p uncvref_t<To> refers to \p To without cv/reference qualifiers).
 *
 * Any specialisation of piranha::safe_cast_impl must have a call operator returning
 * an instance of type \p uncvref_t<To>, otherwise this function will be disabled. The function will also be disabled
 * if \p uncvref_t<To> does not satisfy piranha::is_returnable.
 *
 * Specialisations of piranha::safe_cast_impl are encouraged to raise an exception of type piranha::safe_cast_failure
 * in case the type conversion fails.
 *
 * @param x argument for the conversion.
 *
 * @return \p x converted to \p To.
 *
 * @throws unspecified any exception throw by the call operator of piranha::safe_cast_impl.
 */
template <typename To, typename From>
inline safe_cast_type<To, From> safe_cast(const From &x)
{
    return safe_cast_impl<uncvref_t<To>, From>{}(x);
}

inline namespace impl
{

template <typename To, typename From>
using safe_cast_t = decltype(safe_cast<To>(std::declval<From>()));
}

/// Type trait to detect piranha::safe_cast().
/**
 * The type trait will be \p true if piranha::safe_cast() can be called with \p To and \p From
 * as template arguments, \p false otherwise.
 */
template <typename To, typename From>
class has_safe_cast
{
    static const bool implementation_defined = is_detected<safe_cast_t, To, From>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

// Static init.
template <typename To, typename From>
const bool has_safe_cast<To, From>::value;
}

#endif
