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

#ifndef PIRANHA_UTILS_KRONECKER_ENCDEC_HPP
#define PIRANHA_UTILS_KRONECKER_ENCDEC_HPP

#include <algorithm>
#include <cstddef>
#include <iterator>
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
#include <piranha/detail/safe_integral_arith.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/integer.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

inline namespace impl
{

template <typename T>
struct kronecker_statics {
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
    // NOTE: ideally we would want to do this at compile time with constexpr computations. This
    // requires constexpr rng and prime number generation, amongst others, and of course
    // C++14/C++17 (but probably it's better to limit it to 17 due to constexpr bugs in early
    // C++14 compiler implementations - and probably constexpr if can be really helpful).
    // A constexpr rng idea:
    // https://gist.github.com/bluescarni/7a164293894b29cda8480f06e06b724c
    // Some links about computing the next prime number and primality tests:
    // https://stackoverflow.com/questions/4475996/given-prime-number-n-compute-the-next-prime
    // https://en.wikipedia.org/wiki/Miller%E2%80%93Rabin_primality_test
    // https://en.wikipedia.org/wiki/Prime_number_theorem
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
        for (std::size_t i = 1;; i = piranha::safe_int_add(i, std::size_t(1))) {
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
const std::vector<std::tuple<std::vector<T>, T, T, T>> kronecker_statics<T>::s_limits
    = kronecker_statics<T>::determine_limits();

// Handy getter for the limits.
template <typename T>
inline const std::vector<std::tuple<std::vector<T>, T, T, T>> &k_limits()
{
    return kronecker_statics<T>::s_limits;
}
} // namespace impl

// Signed C++ integral, without cv qualifications.
template <typename T>
using is_uncv_cpp_signed_integral
    = conjunction<negation<std::is_const<T>>, negation<std::is_volatile<T>>, std::is_integral<T>, std::is_signed<T>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool UncvCppSignedIntegral = is_uncv_cpp_signed_integral<T>::value;

#endif

// The streaming Kronecker encoder.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <UncvCppSignedIntegral T>
#else
template <typename T, enable_if_t<is_uncv_cpp_signed_integral<T>::value, int> = 0>
#endif
class k_encoder
{
public:
    // Constructor from sequence size.
    explicit k_encoder(std::size_t size) : m_index(0), m_size(size), m_value(0), m_cur_c(1)
    {
        // NOTE: here the check is >= because indices in the limits vector correspond to the
        // sizes of the ranges to be encoded.
        if (unlikely(size >= piranha::k_limits<T>().size())) {
            piranha_throw(std::overflow_error, "cannot Kronecker-encode a sequence of size " + std::to_string(size)
                                                   + " to the signed integral type '" + piranha::demangle<T>()
                                                   + "': the maximum allowed size for this signed integral type is "
                                                   // NOTE: limits.size() of at least 1 should always
                                                   // be guaranteed by the limits construction process.
                                                   + std::to_string(piranha::k_limits<T>().size() - 1u));
        }
    }
    // Push a value to the encoder.
    k_encoder &operator<<(T n)
    {
        if (unlikely(m_index == m_size)) {
            piranha_throw(std::out_of_range,
                          "cannot push any more values to this Kronecker encoder: the number of "
                          "values already pushed to the encoder is equal to the size used for construction ("
                              + std::to_string(m_size) + ")");
        }
        // Cache quantities.
        const auto &limits = piranha::k_limits<T>();
        const auto &limit = limits[static_cast<decltype(limits.size())>(m_size)];
        const auto &minmax_vec = std::get<0>(limit);
        // NOTE: we make sure in determine_limits() that std::size_t can represent the size
        // of the minmax vector.
        const auto minmax = minmax_vec[static_cast<decltype(minmax_vec.size())>(m_index)];
        piranha_assert(minmax > T(0));
        // Check n against the bounds.
        if (unlikely(n < -minmax || n > minmax)) {
            piranha_throw(std::overflow_error, "one of the elements of a sequence to be Kronecker-encoded is out of "
                                               "bounds: the value of the element is "
                                                   + std::to_string(n) + ", while the bounds are ["
                                                   + std::to_string(-minmax) + ", " + std::to_string(minmax) + "]");
        }
        // Compute the next value.
        m_value = static_cast<T>(m_value + (n + minmax) * m_cur_c);
        // Update cur_c.
        m_cur_c = static_cast<T>(m_cur_c * (2 * minmax + 1));
        // Bump up the index.
        ++m_index;
        return *this;
    }
    // Get the encoded value.
    T get() const
    {
        if (unlikely(m_index < m_size)) {
            piranha_throw(std::out_of_range,
                          "cannot fetch the Kronecker-encoded value from this Kronecker encoder: the number of "
                          "values pushed to the encoder ("
                              + std::to_string(m_index) + ") is less than the size used for construction ("
                              + std::to_string(m_size) + ")");
        }
        const auto &limits = piranha::k_limits<T>();
        const auto &limit = limits[static_cast<decltype(limits.size())>(m_size)];
        return static_cast<T>(m_value + std::get<1>(limit));
    }

private:
    std::size_t m_index;
    std::size_t m_size;
    T m_value;
    T m_cur_c;
};

// Input iterator type pointing to values that can be Kronecker-encoded to the signed integral type T.
// NOTE: written like this, we are checking the safe-castability of rvalues for the reference type and the
// difference type. This mirrors the usage below in k_encode_impl().
template <typename It, typename T>
using is_k_encodable_iterator = conjunction<is_input_iterator<It>, is_uncv_cpp_signed_integral<T>,
                                            is_safely_castable<detected_t<it_traits_reference, It>, T>,
                                            is_safely_castable<detected_t<it_traits_difference_type, It>, std::size_t>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename It, typename T>
concept bool KEncodableIterator = is_k_encodable_iterator<It, T>::value;

#endif

// Same concepts as above, plus we check that It is a forward iterator.
template <typename It, typename T>
using is_k_encodable_forward_iterator = conjunction<is_forward_iterator<It>, is_k_encodable_iterator<It, T>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename It, typename T>
concept bool KEncodableForwardIterator = is_k_encodable_forward_iterator<It, T>::value;

#endif

template <typename R, typename T>
using is_k_encodable_forward_range
    = conjunction<is_forward_range<R>, is_k_encodable_forward_iterator<range_begin_t<R>, T>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename R, typename T>
concept bool KEncodableForwardRange = is_k_encodable_forward_range<R, T>::value;

#endif

inline namespace impl
{

// Implementation of k encoding for a [begin, end) iterator range.
template <typename T, typename It>
inline T k_encode_impl(It begin, It end)
{
    k_encoder<T> enc(piranha::safe_cast<std::size_t>(std::distance(begin, end)));
    for (; begin != end; ++begin) {
        enc << piranha::safe_cast<T>(*begin);
    }
    return enc.get();
}

} // namespace impl

// Encode from input iterator + size.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename T, KEncodableIterator<T> It>
#else
template <typename T, typename It, enable_if_t<is_k_encodable_iterator<It, T>::value, int> = 0>
#endif
inline T k_encode(It begin, std::size_t size)
{
    k_encoder<T> enc(size);
    for (std::size_t i = 0; i != size; ++i, ++begin) {
        enc << piranha::safe_cast<T>(*begin);
    }
    return enc.get();
}

// Encode from forward iterators.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename T, KEncodableForwardIterator<T> It>
#else
template <typename T, typename It, enable_if_t<is_k_encodable_forward_iterator<It, T>::value, int> = 0>
#endif
inline T k_encode(It begin, It end)
{
    return piranha::k_encode_impl<T>(begin, end);
}

// Encode from forward range.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename T, KEncodableForwardRange<T> R>
#else
template <typename T, typename R, enable_if_t<is_k_encodable_forward_range<R, T>::value, int> = 0>
#endif
inline T k_encode(R &&r)
{
    using std::begin;
    using std::end;
    return piranha::k_encode<T>(begin(std::forward<R>(r)), end(std::forward<R>(r)));
}

// The streaming Kronecker decoder.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <UncvCppSignedIntegral T>
#else
template <typename T, enable_if_t<is_uncv_cpp_signed_integral<T>::value, int> = 0>
#endif
class k_decoder
{
public:
    // Constructor from a value to be decoded and the size of the range into which n
    // will be decoded.
    explicit k_decoder(T n, std::size_t size) : m_index(0), m_size(size), m_code(0), m_mod_arg(1)
    {
        const auto &limits = piranha::k_limits<T>();
        if (unlikely(m_size >= limits.size())) {
            piranha_throw(std::overflow_error,
                          "cannot Kronecker-decode the signed integer " + std::to_string(n) + " of type '"
                              + piranha::demangle<T>() + "' into an output range of size " + std::to_string(m_size)
                              + ": the maximum allowed size for the range is " + std::to_string(limits.size() - 1u));
        }
        if (m_size) {
            const auto &limit = limits[static_cast<decltype(limits.size())>(m_size)];
            const auto hmin = std::get<1>(limit), hmax = std::get<2>(limit);
            if (unlikely(n < hmin || n > hmax)) {
                piranha_throw(std::overflow_error, "cannot Kronecker-decode the signed integer " + std::to_string(n)
                                                       + " of type '" + piranha::demangle<T>()
                                                       + "' into a range of size " + std::to_string(m_size)
                                                       + ": the value of the integer is outside the allowed bounds ["
                                                       + std::to_string(hmin) + ", " + std::to_string(hmax) + "]");
            }
            m_code = static_cast<T>(n - hmin);
        } else {
            if (unlikely(n != T(0))) {
                piranha_throw(std::invalid_argument,
                              "only zero can be Kronecker-decoded into an empty output range, but a value of "
                                  + std::to_string(n) + " was provided instead");
            }
        }
    }
    k_decoder &operator>>(T &out)
    {
        if (unlikely(m_index == m_size)) {
            piranha_throw(std::out_of_range, "cannot decode any more values from this Kronecker decoder: the number of "
                                             "values already decoded is equal to the size used for construction ("
                                                 + std::to_string(m_size) + ")");
        }
        // Cache quantities.
        const auto &limits = piranha::k_limits<T>();
        const auto &limit = limits[static_cast<decltype(limits.size())>(m_size)];
        const auto &minmax_vec = std::get<0>(limit);
        // NOTE: we make sure in determine_limits() that std::size_t can represent the size
        // of the minmax vector.
        const auto minmax = minmax_vec[static_cast<decltype(minmax_vec.size())>(m_index)];
        piranha_assert(minmax > T(0));
        // Compute the next mod_arg.
        const auto next_mod_arg = static_cast<T>(m_mod_arg * (2 * minmax + 1));
        // Compute and write the return value.
        out = static_cast<T>((m_code % next_mod_arg) / m_mod_arg - minmax);
        // Update m_mod_arg.
        m_mod_arg = next_mod_arg;
        // Update the index.
        ++m_index;
        return *this;
    }

private:
    std::size_t m_index;
    std::size_t m_size;
    T m_code;
    T m_mod_arg;
};

// Forward iterator type into which a code of type T can be decoded.
template <typename It, typename T>
using is_k_decodable_forward_iterator
    = conjunction<is_mutable_forward_iterator<It>, is_uncv_cpp_signed_integral<T>,
                  is_safely_castable<T, detected_t<it_traits_value_type, It>>,
                  std::is_move_assignable<detected_t<it_traits_value_type, It>>,
                  is_safely_castable<detected_t<it_traits_difference_type, It>, std::size_t>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename It, typename T>
concept bool KDecodableForwardIterator = is_k_decodable_forward_iterator<It, T>::value;

#endif

// Forward range type into which a code of type T can be decoded.
template <typename R, typename T>
using is_k_decodable_forward_range
    = conjunction<is_mutable_forward_range<R>, is_k_decodable_forward_iterator<range_begin_t<R>, T>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename R, typename T>
concept bool KDecodableForwardRange = is_k_decodable_forward_range<R, T>::value;

#endif

inline namespace impl
{

// Decodification into mutable forward iterators.
template <typename T, typename It>
inline void k_decode_impl(T n, It begin, It end)
{
    k_decoder<T> dec(n, piranha::safe_cast<std::size_t>(std::distance(begin, end)));
    T tmp;
    for (; begin != end; ++begin) {
        dec >> tmp;
        // Safe cast to the value type of the iterator before assigning.
        *begin = piranha::safe_cast<it_traits_value_type<It>>(tmp);
    }
}
} // namespace impl

// Decode into output iterator + size.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <UncvCppSignedIntegral T, OutputIterator<T> It>
#else
template <typename T, typename It,
          enable_if_t<conjunction<is_uncv_cpp_signed_integral<T>, is_output_iterator<It, T>>::value, int> = 0>
#endif
inline void k_decode(T n, It begin, std::size_t size)
{
    k_decoder<T> dec(n, size);
    T tmp;
    for (std::size_t i = 0; i != size; ++i, ++begin) {
        dec >> tmp;
        *begin = tmp;
    }
}

// Decodification into mutable forward iterators.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename T, KDecodableForwardIterator<T> It>
#else
template <typename T, typename It, enable_if_t<is_k_decodable_forward_iterator<It, T>::value, int> = 0>
#endif
inline void k_decode(T n, It begin, It end)
{
    piranha::k_decode_impl(n, begin, end);
}

// Decodification into a mutable forward range.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename T, KDecodableForwardRange<T> R>
#else
template <typename T, typename R, enable_if_t<is_k_decodable_forward_range<R, T>::value, int> = 0>
#endif
inline void k_decode(T n, R &&r)
{
    using std::begin;
    using std::end;
    return piranha::k_decode(n, begin(std::forward<R>(r)), end(std::forward<R>(r)));
}
} // namespace piranha

#endif
