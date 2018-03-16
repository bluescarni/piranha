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

#ifndef PIRANHA_KRONECKER_ARRAY_HPP
#define PIRANHA_KRONECKER_ARRAY_HPP

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <mp++/integer.hpp>

#include <piranha/config.hpp>
#include <piranha/detail/demangle.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/integer.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

inline namespace impl
{

template <typename T>
struct kronecker_array_statics {
    // Determine limits for the codification of m-dimensional arrays.
    // The returned value is a 4-tuple built as follows:
    // 0. vector of absolute values of the upper/lower limit for each component,
    // 1. h_min,
    // 2. h_max,
    // 3. h_max - h_min.
    //
    // NOTE: when reasoning about this, keep in mind that this is not a completely generic
    // codification: min/max vectors are negative/positive and symmetric. This makes it easy
    // to reason about overflows during (de)codification of vectors, representability of the
    // quantities involved, etc.
    //
    // NOTE: here we should not have problems when interoperating with libraries that
    // modify the GMP allocation functions, as we do not store any static piranha::integer:
    // the creation and destruction of integer objects is confined to the determine_limit() function.
    static std::tuple<std::vector<T>, T, T, T> determine_limit(const std::size_t &m)
    {
        piranha_assert(m >= 1u);
        // Init the engine with a different seed for each size m.
        std::mt19937 engine(static_cast<std::mt19937::result_type>(m));
        // Perturb an integer value: add random quantity (hard-coded to +-5%)
        // and then take the next prime.
        std::uniform_int_distribution<int> dist(-5, 5);
        auto perturb = [&engine, &dist](integer &arg) {
            arg += (dist(engine) * arg) / 100;
            arg.nextprime();
        };
        // Build the initial minmax and coding vectors: all elements in the [-1,1] range.
        std::vector<integer> m_vec, M_vec, c_vec, prev_m_vec, prev_M_vec, prev_c_vec;
        c_vec.emplace_back(1);
        m_vec.emplace_back(-1);
        M_vec.emplace_back(1);
        for (std::size_t i = 1; i < m; ++i) {
            m_vec.emplace_back(-1);
            M_vec.emplace_back(1);
            c_vec.emplace_back(c_vec.back() * 3);
        }
        // Functor for the scalar product of two vectors.
        auto dot_prod = [](const std::vector<integer> &v1, const std::vector<integer> &v2) -> integer {
            piranha_assert(v1.size() && v1.size() == v2.size());
            return std::inner_product(v1.begin(), v1.end(), v2.begin(), integer{});
        };
        while (true) {
            // Compute the current h_min/max and diff.
            integer h_min = dot_prod(c_vec, m_vec);
            integer h_max = dot_prod(c_vec, M_vec);
            integer diff = h_max - h_min;
            piranha_assert(diff.sgn() >= 0);
            // Try to cast everything to hardware integers.
            T tmp_int;
            // NOTE: here it is diff + 1 because h_max - h_min must be strictly less than the maximum value
            // of T. In the paper, in eq. (7), the Delta_i product appearing in the
            // decoding of the last component of a vector is equal to (h_max - h_min + 1) so we need
            // to be able to represent it.
            const bool fits_int_type
                = mppp::get(tmp_int, h_min) && mppp::get(tmp_int, h_max) && mppp::get(tmp_int, diff + 1);
            // NOTE: we do not need to cast the individual elements of m/M vecs, as the representability
            // of h_min/max ensures the representability of m/M as well.
            if (!fits_int_type) {
                std::vector<T> tmp;
                if (prev_c_vec.size()) {
                    // We are not at the first iteration. Return
                    // the codification limits from the previous iteration.
                    h_min = dot_prod(prev_c_vec, prev_m_vec);
                    h_max = dot_prod(prev_c_vec, prev_M_vec);
                    std::transform(prev_M_vec.begin(), prev_M_vec.end(), std::back_inserter(tmp),
                                   [](const integer &n) { return static_cast<T>(n); });
                    return std::make_tuple(std::move(tmp), static_cast<T>(h_min), static_cast<T>(h_max),
                                           static_cast<T>(h_max - h_min));
                } else {
                    // We are the first iteration: this means that m variables are too many,
                    // and we signal that with a tuple filled with zeroes.
                    return std::make_tuple(std::move(tmp), T(0), T(0), T(0));
                }
            }
            // Store the old vectors.
            prev_c_vec = c_vec;
            prev_m_vec = m_vec;
            prev_M_vec = M_vec;
            // Generate the new coding vector for next iteration.
            auto it = c_vec.begin() + 1, prev_it = prev_c_vec.begin();
            for (; it != c_vec.end(); ++it, ++prev_it) {
                // Recover the original delta.
                *it /= *prev_it;
                // Multiply by two and perturb.
                *it *= 2;
                perturb(*it);
                // Multiply by the new accumulated delta product.
                *it *= *(it - 1);
            }
            // Fill in the minmax vectors, apart from the last component.
            it = c_vec.begin() + 1;
            piranha_assert(M_vec.size() && M_vec.size() == m_vec.size());
            for (decltype(M_vec.size()) i = 0; i < M_vec.size() - 1u; ++i, ++it) {
                M_vec[i] = ((*it) / *(it - 1) - 1) / 2;
                m_vec[i] = -M_vec[i];
            }
            // We need to generate the last interval, which does not appear in the coding vector.
            // Take the previous interval and enlarge it so that the corresponding delta is increased by a
            // perturbed factor of 2.
            M_vec.back() = (4 * M_vec.back() + 1) / 2;
            perturb(M_vec.back());
            m_vec.back() = -M_vec.back();
        }
    }
    static std::vector<std::tuple<std::vector<T>, T, T, T>> determine_limits()
    {
        std::vector<std::tuple<std::vector<T>, T, T, T>> retval;
        retval.emplace_back(std::vector<T>{}, T(0), T(0), T(0));
        for (std::size_t i = 1;; ++i) {
            auto tmp = determine_limit(i);
            if (std::get<0>(tmp).empty()) {
                break;
            }
            retval.emplace_back(std::move(tmp));
        }
        return retval;
    }
    // Static vector of limits built at startup.
    static const std::vector<std::tuple<std::vector<T>, T, T, T>> s_limits;
};

// Static init.
template <typename T>
const std::vector<std::tuple<std::vector<T>, T, T, T>> kronecker_array_statics<T>::s_limits
    = kronecker_array_statics<T>::determine_limits();

// Handy getter for the limits.
template <typename T>
inline const std::vector<std::tuple<std::vector<T>, T, T, T>> &k_limits()
{
    return kronecker_array_statics<T>::s_limits;
}
}

// Signed C++ integral, without cv qualifications.
template <typename T>
using is_uncv_cpp_signed_integral
    = conjunction<negation<std::is_const<T>>, negation<std::is_volatile<T>>, std::is_integral<T>, std::is_signed<T>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool UncvCppSignedIntegral = is_uncv_cpp_signed_integral<T>::value;

#endif

// Codification.

// NOTE: the way this is currently written we are in the situation in which:
// - the iterator being dereferenced is an lvalue (see definition of det_deref_t),
// - we are checking the expression safe_cast<To>(*it), that is, we are applying
//   safe_cast() directly to the rvalue result of the dereferencing (rather than, say,
//   storing the dereference somewhere and casting it later as an lvalue),
// - we are checking that an rvalue of the difference type is castable safely to std::size_t.
template <typename It, typename T>
using is_k_encodable_iterator = conjunction<is_forward_iterator<It>, is_safely_castable<det_deref_t<It>, T>,
                                            is_safely_castable<detected_t<it_traits_difference_type, It>, std::size_t>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename It, typename T>
concept bool KEncodableIterator = is_k_encodable_iterator<It, T>::value;

#endif

// Encodable range.
// NOTE: this is fine written as this, as we will have functions which accept ranges as
// forwarding references, and R will thus resolve appropriately to rvalue/lvalue while
// being perfectly forwarded to begin()/end().
template <typename R, typename T>
using is_k_encodable_range = conjunction<is_forward_range<R>, is_k_encodable_iterator<range_begin_t<R>, T>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename R, typename T>
concept bool KEncodableRange = is_k_encodable_range<R, T>::value;

#endif

inline namespace impl
{

// The actual implementation of the k encoding. Make it a non-constrained
// function, we'll do the concept checking from the outside.
template <typename T, typename It>
inline T k_encode_impl(It begin, It end)
{
    const auto size = piranha::safe_cast<std::size_t>(std::distance(begin, end));
    const auto &limits = k_limits<T>();
    // NOTE: here the check is >= because indices in the limits vector correspond to the
    // sizes of the ranges to be encoded.
    if (unlikely(size >= limits.size())) {
        piranha_throw(std::invalid_argument, "cannot Kronecker-encode a range of size " + std::to_string(size)
                                                 + ": the size must be less than " + std::to_string(limits.size()));
    }
    // Special case for zero size.
    if (!size) {
        return T(0);
    }
    // Cache quantities.
    const auto &limit = limits[static_cast<decltype(limits.size())>(size)];
    const auto &minmax_vec = std::get<0>(limit);
    piranha_assert(minmax_vec[0] > T(0));
    // Small helper to check that the value val in the input range
    // is within the allowed bounds (from minmax_vec).
    auto range_checker = [](T val, T minmax) {
        if (unlikely(val < -minmax || val > minmax)) {
            piranha_throw(std::invalid_argument, "one of the elements of a range to be Kronecker-encoded is out of "
                                                 "bounds: the value of the element is "
                                                     + std::to_string(val) + ", while the bounds are ["
                                                     + std::to_string(-minmax) + ", " + std::to_string(minmax) + "]");
        }
    };
    // Start of the iteration.
    auto retval = piranha::safe_cast<T>(*begin);
    range_checker(retval, minmax_vec[0]);
    retval = static_cast<T>(retval + minmax_vec[0]);
    auto cur_c = static_cast<T>(2 * minmax_vec[0] + 1);
    piranha_assert(retval >= T(0));
    // Do the rest.
    ++begin;
    for (decltype(minmax_vec.size()) i = 1; begin != end; ++i, ++begin) {
        piranha_assert(i < minmax_vec.size());
        const auto tmp = piranha::safe_cast<T>(*begin);
        range_checker(tmp, minmax_vec[i]);
        retval = static_cast<T>(retval + (tmp + minmax_vec[i]) * cur_c);
        piranha_assert(minmax_vec[i] > 0);
        cur_c = static_cast<T>(cur_c * (2 * minmax_vec[i] + 1));
    }
    return static_cast<T>(retval + std::get<1>(limit));
}
}

// Encode from iterators.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <UncvCppSignedIntegral T, KEncodableIterator<T> It>
#else
template <typename T, typename It,
          enable_if_t<conjunction<is_uncv_cpp_signed_integral<T>, is_k_encodable_iterator<It, T>>::value, int> = 0>
#endif
inline T k_encode(It begin, It end)
{
    return k_encode_impl<T>(begin, end);
}

// Encode range.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <UncvCppSignedIntegral T, KEncodableRange<T> R>
#else
template <typename T, typename R,
          enable_if_t<conjunction<is_uncv_cpp_signed_integral<T>, is_k_encodable_range<R, T>>::value, int> = 0>
#endif
inline T k_encode(R &&r)
{
    using std::begin;
    using std::end;
    return k_encode_impl<T>(begin(std::forward<R>(r)), end(std::forward<R>(r)));
}

// Encode initializer list.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <UncvCppSignedIntegral T, SafelyCastable<T> U>
#else
template <typename T, typename U,
          enable_if_t<conjunction<is_uncv_cpp_signed_integral<T>, is_safely_castable<U, T>>::value, int> = 0>
#endif
inline T k_encode(std::initializer_list<U> l)
{
    return k_encode_impl<T>(l.begin(), l.end());
}

// Decodification.

// NOTE: here we are checking that:
// - an rvalue of T is safely castable to the value type,
// - the value type is move assignable,
// - an rvalue of the difference type is safely castable to std::size_t.
// That is, we are testing an expression like *it = safe_cast<value_type>(T &&);
template <typename It, typename T>
using is_k_decodable_iterator
    = conjunction<is_mutable_forward_iterator<It>, is_safely_castable<T, detected_t<it_traits_value_type, It>>,
                  std::is_move_assignable<detected_t<it_traits_value_type, It>>,
                  is_safely_castable<detected_t<it_traits_difference_type, It>, std::size_t>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename It, typename T>
concept bool KDecodableIterator = is_k_decodable_iterator<It, T>::value;

#endif

template <typename R, typename T>
using is_k_decodable_range = conjunction<is_mutable_forward_range<R>, is_k_decodable_iterator<range_begin_t<R>, T>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename R, typename T>
concept bool KDecodableRange = is_k_decodable_range<R, T>::value;

#endif

inline namespace impl
{

template <typename T, typename It>
inline void k_decode_impl(T n, It begin, It end)
{
    using v_type = typename std::iterator_traits<It>::value_type;
    const auto m = piranha::safe_cast<std::size_t>(std::distance(begin, end));
    const auto &limits = k_limits<T>();
    if (unlikely(m >= limits.size())) {
        piranha_throw(std::invalid_argument, "cannot Kronecker-decode the signed integer " + std::to_string(n)
                                                 + " of type '" + demangle<T>() + "' into an output range of size "
                                                 + std::to_string(m) + ": the size of the range must be less than "
                                                 + std::to_string(limits.size()));
    }
    if (!m) {
        if (unlikely(n != T(0))) {
            piranha_throw(std::invalid_argument,
                          "only a value of zero can be Kronecker-decoded into an empty output range, but a value of "
                              + std::to_string(n) + " was provided instead");
        }
        return;
    }
    // Cache values.
    const auto &limit = limits[static_cast<decltype(limits.size())>(m)];
    const auto &minmax_vec = std::get<0>(limit);
    const auto hmin = std::get<1>(limit), hmax = std::get<2>(limit);
    if (unlikely(n < hmin || n > hmax)) {
        piranha_throw(std::invalid_argument, "cannot Kronecker-decode the signed integer " + std::to_string(n)
                                                 + " of type '" + demangle<T>() + "' into a range of size "
                                                 + std::to_string(m) + ": its value is outside the allowed range ["
                                                 + std::to_string(hmin) + ", " + std::to_string(hmax) + "]");
    }
    // NOTE: the static_cast here is useful when working with short integral types. In that case,
    // the binary operation on the RHS produces an int (due to integer promotion rules), which gets
    // assigned back to the short integral causing the compiler to complain about potentially lossy conversion.
    const auto code = static_cast<T>(n - hmin);
    piranha_assert(code >= T(0));
    piranha_assert(minmax_vec[0] > T(0));
    auto mod_arg = static_cast<T>(2 * minmax_vec[0] + 1);
    // Do the first value manually.
    *begin = piranha::safe_cast<v_type>(static_cast<T>((code % mod_arg) - minmax_vec[0]));
    // Do the rest.
    ++begin;
    for (decltype(minmax_vec.size()) i = 1; i < m; ++i, ++begin) {
        piranha_assert(minmax_vec[i] > T(0));
        *begin = piranha::safe_cast<v_type>(
            static_cast<T>((code % (mod_arg * (2 * minmax_vec[i] + 1))) / mod_arg - minmax_vec[i]));
        mod_arg = static_cast<T>(mod_arg * (2 * minmax_vec[i] + 1));
    }
}
}

#if defined(PIRANHA_HAVE_CONCEPTS)
template <UncvCppSignedIntegral T, KDecodableIterator<T> It>
#else
template <typename T, typename It,
          enable_if_t<conjunction<is_uncv_cpp_signed_integral<T>, is_k_decodable_iterator<It, T>>::value, int> = 0>
#endif
inline void k_decode(T n, It begin, It end)
{
    k_decode_impl(n, begin, end);
}

#if defined(PIRANHA_HAVE_CONCEPTS)
template <UncvCppSignedIntegral T, KDecodableRange<T> R>
#else
template <typename T, typename R,
          enable_if_t<conjunction<is_uncv_cpp_signed_integral<T>, is_k_decodable_range<R, T>>::value, int> = 0>
#endif
inline void k_decode(T n, R &&r)
{
    using std::begin;
    using std::end;
    return k_decode_impl(n, begin(std::forward<R>(r)), end(std::forward<R>(r)));
}

inline namespace impl
{

// Type requirement for Kronecker array.
template <typename T>
using ka_type_reqs = conjunction<std::is_integral<T>, std::is_signed<T>>;
}

/// Kronecker array.
/**
 * This class offers static methods to encode (and decode) arrays of integral values as instances of \p SignedInteger
 * type,
 * using a technique known as "Kronecker substitution".
 *
 * Depending on the bit width and numerical limits of \p SignedInteger, the class will be able to operate on vectors of
 * integers up to a certain
 * dimension and within certain bounds on the vector's components. Such limits can be queried with the get_limits()
 * static method.
 *
 * ## Type requirements ##
 *
 * \p SignedInteger must be a C++ signed integral type.
 *
 * ## Exception safety guarantee ##
 *
 * Unless otherwise specified, this class provides the strong exception safety guarantee for all operations.
 *
 * ## Move semantics ##
 *
 * This class does not have any non-static data members, hence it has trivial move semantics.
 */
// NOTE: we should optimise the decodification with only one element, there should be no need to do divisions
// and modulo operations.
template <typename SignedInteger>
class kronecker_array
{
public:
    /// Signed integer type used for encoding.
    using int_type = SignedInteger;

private:
    static_assert(ka_type_reqs<int_type>::value, "This class can be used only with signed integers.");
    // This is a 4-tuple of int_type built as follows:
    // 0. vector of absolute values of the upper/lower limit for each component,
    // 1. h_min,
    // 2. h_max,
    // 3. h_max - h_min.
    using limit_type = std::tuple<std::vector<int_type>, int_type, int_type, int_type>;
    // Vector of limits.
    using limits_type = std::vector<limit_type>;

public:
    /// Size type.
    /**
     * Equivalent to \p std::size_t, used to represent the
     * dimension of the vectors on which the class can operate.
     */
    using size_type = std::size_t;

private:
    // Static vector of limits built at startup.
    // NOTE: here we should not have problems when interoperating with libraries that modify the GMP allocation
    // functions,
    // as we do not store any static piranha::integer: the creation and destruction of integer objects is confined to
    // the determine_limit()
    // function.
    static const limits_type m_limits;
    // Determine limits for m-dimensional vectors.
    // NOTE: when reasoning about this, keep in mind that this is not a completely generic
    // codification: min/max vectors are negative/positive and symmetric. This makes it easy
    // to reason about overflows during (de)codification of vectors, representability of the
    // quantities involved, etc.
    static limit_type determine_limit(const size_type &m)
    {
        piranha_assert(m >= 1u);
        std::mt19937 engine(static_cast<std::mt19937::result_type>(m));
        std::uniform_int_distribution<int> dist(-5, 5);
        // Perturb integer value: add random quantity and then take next prime.
        auto perturb = [&engine, &dist](integer &arg) {
            arg += (dist(engine) * (arg)) / 100;
            arg.nextprime();
        };
        // Build initial minmax and coding vectors: all elements in the [-1,1] range.
        std::vector<integer> m_vec, M_vec, c_vec, prev_c_vec, prev_m_vec, prev_M_vec;
        c_vec.emplace_back(1);
        m_vec.emplace_back(-1);
        M_vec.emplace_back(1);
        for (size_type i = 1u; i < m; ++i) {
            m_vec.emplace_back(-1);
            M_vec.emplace_back(1);
            c_vec.emplace_back(c_vec.back() * 3);
        }
        // Functor for scalar product of two vectors.
        auto dot_prod = [](const std::vector<integer> &v1, const std::vector<integer> &v2) -> integer {
            piranha_assert(v1.size() && v1.size() == v2.size());
            return std::inner_product(v1.begin(), v1.end(), v2.begin(), integer(0));
        };
        while (true) {
            // Compute the current h_min/max and diff.
            integer h_min = dot_prod(c_vec, m_vec);
            integer h_max = dot_prod(c_vec, M_vec);
            integer diff = h_max - h_min;
            piranha_assert(diff.sgn() >= 0);
            // Try to cast everything to hardware integers.
            int_type tmp_int;
            bool fits_int_type = mppp::get(tmp_int, h_min);
            fits_int_type = fits_int_type && mppp::get(tmp_int, h_max);
            // NOTE: here it is +1 because h_max - h_min must be strictly less than the maximum value
            // of int_type. In the paper, in eq. (7), the Delta_i product appearing in the
            // decoding of the last component of a vector is equal to (h_max - h_min + 1) so we need
            // to be able to represent it.
            fits_int_type = fits_int_type && mppp::get(tmp_int, diff + 1);
            // NOTE: we do not need to cast the individual elements of m/M vecs, as the representability
            // of h_min/max ensures the representability of m/M as well.
            if (!fits_int_type) {
                std::vector<int_type> tmp;
                // Check if we are at the first iteration.
                if (prev_c_vec.size()) {
                    h_min = dot_prod(prev_c_vec, prev_m_vec);
                    h_max = dot_prod(prev_c_vec, prev_M_vec);
                    std::transform(prev_M_vec.begin(), prev_M_vec.end(), std::back_inserter(tmp),
                                   [](const integer &n) { return static_cast<int_type>(n); });
                    return std::make_tuple(std::move(tmp), static_cast<int_type>(h_min), static_cast<int_type>(h_max),
                                           static_cast<int_type>(h_max - h_min));
                } else {
                    // Here it means m variables are too many, and we stopped at the first iteration
                    // of the cycle. Return tuple filled with zeroes.
                    return std::make_tuple(std::move(tmp), int_type(0), int_type(0), int_type(0));
                }
            }
            // Store old vectors.
            prev_c_vec = c_vec;
            prev_m_vec = m_vec;
            prev_M_vec = M_vec;
            // Generate new coding vector for next iteration.
            auto it = c_vec.begin() + 1, prev_it = prev_c_vec.begin();
            for (; it != c_vec.end(); ++it, ++prev_it) {
                // Recover original delta.
                *it /= *prev_it;
                // Multiply by two and perturb.
                *it *= 2;
                perturb(*it);
                // Multiply by the new accumulated delta product.
                *it *= *(it - 1);
            }
            // Fill in the minmax vectors, apart from the last component.
            it = c_vec.begin() + 1;
            piranha_assert(M_vec.size() && M_vec.size() == m_vec.size());
            for (size_type i = 0u; i < M_vec.size() - 1u; ++i, ++it) {
                M_vec[i] = ((*it) / *(it - 1) - 1) / 2;
                m_vec[i] = -M_vec[i];
            }
            // We need to generate the last interval, which does not appear in the coding vector.
            // Take the previous interval and enlarge it so that the corresponding delta is increased by a
            // perturbed factor of 2.
            M_vec.back() = (4 * M_vec.back() + 1) / 2;
            perturb(M_vec.back());
            m_vec.back() = -M_vec.back();
        }
    }
    static limits_type determine_limits()
    {
        limits_type retval;
        retval.emplace_back(std::vector<int_type>{}, int_type(0), int_type(0), int_type(0));
        for (size_type i = 1u;; ++i) {
            auto tmp = determine_limit(i);
            if (std::get<0u>(tmp).empty()) {
                break;
            } else {
                retval.emplace_back(std::move(tmp));
            }
        }
        return retval;
    }

public:
    /// Get the limits of the Kronecker codification.
    /**
     * Will return a const reference to an \p std::vector of tuples describing the limits for the Kronecker
     * codification of arrays of integer. The indices in this vector correspond to the dimension of the array to be
     * encoded, so that the object at index \f$i\f$ in the returned vector describes the limits for the codification of
     * \f$i\f$-dimensional arrays of integers.
     *
     * Each element of the returned vector is an \p std::tuple of 4 elements built as follows:
     *
     * - position 0: a vector containing the absolute value of the lower/upper bounds for each component,
     * - position 1: \f$h_\textnormal{min}\f$, the minimum value for the integer encoding the array,
     * - position 2: \f$h_\textnormal{max}\f$, the maximum value for the integer encoding the array,
     * - position 3: \f$h_\textnormal{max}-h_\textnormal{min}\f$.
     *
     * The tuple at index 0 of the returned vector is filled with zeroes. The size of the returned vector determines the
     * maximum dimension of the vectors to be encoded.
     *
     * @return const reference to an \p std::vector of limits for the Kronecker codification of arrays of integers.
     */
    static const limits_type &get_limits()
    {
        return m_limits;
    }
    /// Encode vector.
    /**
     * \note
     * This method can be called only if \p Vector is a type with a vector-like interface.
     * Specifically, it must have a <tt>size()</tt> method and overloaded const index operator.
     *
     * Encode input vector \p v into an instance of \p SignedInteger. If the value type of \p Vector
     * is not \p SignedInteger, the values of \p v will be converted to \p SignedInteger using
     * piranha::safe_cast(). A vector of size 0 is always encoded as 0.
     *
     * @param v vector to be encoded.
     *
     * @return \p v encoded as a \p SignedInteger using Kronecker substitution.
     *
     * @throws std::invalid_argument if any of these conditions hold:
     * - the size of \p v is equal to or greater than the size of the output of get_limits(),
     * - one of the components of \p v is outside the bounds reported by get_limits().
     * @throws unspecified any exception thrown by piranha::safe_cast().
     */
    template <typename Vector>
    static int_type encode(const Vector &v)
    {
        const auto size = v.size();
        // NOTE: here the check is >= because indices in the limits vector correspond to the sizes of the vectors to be
        // encoded.
        if (unlikely(size >= m_limits.size())) {
            piranha_throw(std::invalid_argument, "size of vector to be encoded is too large");
        }
        if (unlikely(!size)) {
            return int_type(0);
        }
        // Cache quantities.
        const auto &limit = m_limits[size];
        const auto &minmax_vec = std::get<0u>(limit);
        // Check that the vector's components are compatible with the limits.
        // NOTE: here size is not greater than m_limits.size(), which in turn is compatible with the minmax vectors.
        for (min_int<decltype(v.size()), decltype(minmax_vec.size())> i = 0u; i < size; ++i) {
            if (unlikely(piranha::safe_cast<int_type>(v[i]) < -minmax_vec[i]
                         || piranha::safe_cast<int_type>(v[i]) > minmax_vec[i])) {
                piranha_throw(std::invalid_argument, "a component of the vector to be encoded is out of bounds");
            }
        }
        piranha_assert(minmax_vec[0u] > 0);
        int_type retval = static_cast<int_type>(piranha::safe_cast<int_type>(v[0u]) + minmax_vec[0u]),
                 cur_c = static_cast<int_type>(2 * minmax_vec[0u] + 1);
        piranha_assert(retval >= 0);
        for (decltype(v.size()) i = 1u; i < size; ++i) {
            retval = static_cast<int_type>(retval + ((piranha::safe_cast<int_type>(v[i]) + minmax_vec[i]) * cur_c));
            piranha_assert(minmax_vec[i] > 0);
            cur_c = static_cast<int_type>(cur_c * (2 * minmax_vec[i] + 1));
        }
        return static_cast<int_type>(retval + std::get<1u>(limit));
    }
    /// Decode into vector.
    /**
     * \note
     * This method can be called only if \p Vector is a type with a vector-like interface.
     * Specifically, it must have a <tt>size()</tt> method and overloaded mutable index operator.
     *
     * Decode input code \p n into \p retval. If the value type of \p Vector
     * is not \p SignedInteger, the components decoded from \p n will be converted to the value type of \p Vector
     * using piranha::safe_cast().
     *
     * In case of exceptions, \p retval will be left in a valid but undefined state.
     *
     * @param retval object that will store the decoded vector.
     * @param n code to be decoded.
     *
     * @throws std::invalid_argument if any of these conditions hold:
     * - the size of \p retval is equal to or greater than the size of the output of get_limits(),
     * - the size of \p retval is zero and \p n is not zero,
     * - \p n is out of the allowed bounds reported by get_limits().
     * @throws unspecified any exception thrown by piranha::safe_cast().
     */
    template <typename Vector>
    static void decode(Vector &retval, const int_type &n)
    {
        typedef typename Vector::value_type v_type;
        const auto m = retval.size();
        if (unlikely(m >= m_limits.size())) {
            piranha_throw(std::invalid_argument, "size of vector to be decoded is too large");
        }
        if (unlikely(!m)) {
            if (unlikely(n != 0)) {
                piranha_throw(std::invalid_argument, "a vector of size 0 must always be encoded as 0");
            }
            return;
        }
        // Cache values.
        const auto &limit = m_limits[m];
        const auto &minmax_vec = std::get<0u>(limit);
        const auto hmin = std::get<1u>(limit), hmax = std::get<2u>(limit);
        if (unlikely(n < hmin || n > hmax)) {
            piranha_throw(std::invalid_argument, "the integer to be decoded is out of bounds");
        }
        // NOTE: the static_cast here is useful when working with int_type == char. In that case,
        // the binary operation on the RHS produces an int (due to integer promotion rules), which gets
        // assigned back to char causing the compiler to complain about potentially lossy conversion.
        const int_type code = static_cast<int_type>(n - hmin);
        piranha_assert(code >= 0);
        piranha_assert(minmax_vec[0u] > 0);
        int_type mod_arg = static_cast<int_type>(2 * minmax_vec[0u] + 1);
        // Do the first value manually.
        retval[0u] = piranha::safe_cast<v_type>((code % mod_arg) - minmax_vec[0u]);
        for (min_int<typename Vector::size_type, decltype(minmax_vec.size())> i = 1u; i < m; ++i) {
            piranha_assert(minmax_vec[i] > 0);
            retval[i]
                = piranha::safe_cast<v_type>((code % (mod_arg * (2 * minmax_vec[i] + 1))) / mod_arg - minmax_vec[i]);
            mod_arg = static_cast<int_type>(mod_arg * (2 * minmax_vec[i] + 1));
        }
    }
};

// Static initialization.
template <typename SignedInteger>
const typename kronecker_array<SignedInteger>::limits_type kronecker_array<SignedInteger>::m_limits
    = kronecker_array<SignedInteger>::determine_limits();
}

#endif
