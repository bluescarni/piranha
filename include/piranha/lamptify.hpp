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

#ifndef PIRANHA_LAMPTIFY_HPP
#define PIRANHA_LAMPTIFY_HPP

#include <functional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <piranha/math.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace math
{

template <typename T, typename U, typename = void>
class lamptify_impl
{
    // Type resulting from the evaluation of T1 with U.
    template <typename T1>
    using eval_t = decltype(
        math::evaluate(std::declval<const T1 &>(), std::declval<const std::unordered_map<std::string, U> &>()));
    // Default lamptified implementation: convert input T1 to its eval type, store the result internally
    // and return it upon call operator invocation.
    template <typename T1>
    struct default_lamptified {
        eval_t<T1> operator()(const std::vector<U> &) const
        {
            return m_x;
        }
        eval_t<T1> m_x;
    };
    // The return type is the default lamptified, iff:
    // - T1 is evaluable with U,
    // - the evaluation is made by the default implementation of evaluate_impl.
    // The fact that T1 is evaluable already ensures that the lamptified can be returned, as it just contains
    // the result of the default evaluation.
    // NOTE: we need to check both conditions because the default evaluate_impl can always be instantiated,
    // and we thus need to make sure T1 is actually evaluable.
    template <typename T1>
    using return_t = enable_if_t<conjunction<is_detected<eval_t, T1>,
                                             std::is_base_of<default_math_evaluate_tag, evaluate_impl<T1, U>>>::value,
                                 default_lamptified<T1>>;

public:
    template <typename T1>
    return_t<T1> operator()(const T1 &x, const std::vector<std::string> &,
                            const std::unordered_map<std::string, std::function<U(const std::vector<U> &)>> &) const
    {
        return default_lamptified<T1>{x};
    }
};
}

inline namespace impl
{

// Candidate retval for lamptify().
template <typename T, typename U>
using math_lamptify_t_ = decltype(math::lamptify_impl<T, U>{}(
    std::declval<const T &>(), std::declval<const std::vector<std::string> &>(),
    std::declval<const std::unordered_map<std::string, std::function<U(const std::vector<U> &)>> &>()));

// Result of the call operator of the lamptified object.
template <typename T, typename U>
using math_lamptify_t_call_t
    = decltype(std::declval<const math_lamptify_t_<T, U> &>()(std::declval<const std::vector<U> &>()));

// Final typedef:
// - U must be mappable,
// - the output of math::lamptify_impl must be returnable,
// - the output of math::lamptify_impl must have a call operator with the expected argument type.
template <typename T, typename U>
using math_lamptify_t = enable_if_t<conjunction<is_mappable<U>, is_returnable<math_lamptify_t_<T, U>>,
                                                is_detected<math_lamptify_t_call_t, T, U>>::value,
                                    math_lamptify_t_<T, U>>;
}

namespace math
{

template <typename U, typename T>
math_lamptify_t<T, U>
lamptify(const T &x, const std::vector<std::string> &names,
         const std::unordered_map<std::string, std::function<U(const std::vector<U> &)>> &implicit_deps = {})
{
    return lamptify_impl<T, U>{}(x, names, implicit_deps);
}
}

inline namespace impl
{

/// Detect the presence of piranha::math::lamptify().
/**
 * This type trait will be \p true if piranha::math::lamptify() can be called with a first argument of type
 * \p T and evaluation type \p U, \p false otherwise.
 */
template <typename T, typename U>
class has_lamptify
{
    template <typename T1, typename U1>
    using lam_t = decltype(math::lamptify(
        std::declval<const T1 &>(), std::declval<const std::vector<std::string> &>(),
        std::declval<const std::unordered_map<std::string, std::function<U(const std::vector<U> &)>> &>()));
    static const bool implementation_defined = is_detected<lam_t, T, U>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T, typename U>
const bool has_lamptify<T, U>::value;

/// Type trait to detect if a key type provides a <tt>lamptify()</tt> method.
/**
 * This type trait will be \p true if \p Key is a key type providing a const method <tt>partial()</tt> taking a const
 * instance of
 * piranha::symbol_set::positions and a const instance of piranha::symbol_set as input, and returning an
 * <tt>std::pair</tt> of
 * any type and \p Key. Otherwise, the type trait will be \p false.
 * If \p Key does not satisfy piranha::is_key, a compilation error will be produced.
 *
 * The decay type of \p Key is considered in this type trait.
 */
template <typename T>
class key_is_differentiable : detail::sfinae_types
{
    using Td = typename std::decay<T>::type;
    PIRANHA_TT_CHECK(is_key, Td);
    template <typename U>
    static auto test(const U &u)
        -> decltype(u.partial(std::declval<const symbol_set::positions &>(), std::declval<const symbol_set &>()));
    static no test(...);

public:
    /// Value of the type trait.
    static const bool value = detail::is_differential_key_pair<Td, decltype(test(std::declval<Td>()))>::value;
};

template <typename T>
const bool key_is_differentiable<T>::value;
}
}

#endif
