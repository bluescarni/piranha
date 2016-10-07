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

#ifndef PIRANHA_REAL_TRIGONOMETRIC_KRONECKER_MONOMIAL_HPP
#define PIRANHA_REAL_TRIGONOMETRIC_KRONECKER_MONOMIAL_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "binomial.hpp"
#include "config.hpp"
#include "detail/cf_mult_impl.hpp"
#include "detail/km_commons.hpp"
#include "detail/prepare_for_print.hpp"
#include "detail/safe_integral_adder.hpp"
#include "exceptions.hpp"
#include "is_cf.hpp"
#include "is_key.hpp"
#include "kronecker_array.hpp"
#include "math.hpp"
#include "mp_integer.hpp"
#include "s11n.hpp"
#include "safe_cast.hpp"
#include "serialization.hpp"
#include "static_vector.hpp"
#include "symbol.hpp"
#include "symbol_set.hpp"
#include "term.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Real trigonometric Kronecker monomial class.
/**
 * This class represents a multivariate real trigonometric monomial, i.e., functions of the form
 * \f[
 * \begin{array}{c}
 * \sin \\
 * \cos
 * \end{array}
 * \left(n_0x_0 + n_1x_1 + \ldots + n_mx_m\right),
 * \f]
 * where the integers \f$ n_i \f$ are called <em>multipliers</em>. The values of the multipliers
 * are packed in a signed integer using Kronecker substitution, using the facilities provided
 * by piranha::kronecker_array. The boolean <em>flavour</em> of the monomial indicates whether it represents
 * a cosine (<tt>true</tt>) or sine (<tt>false</tt>).
 *
 * Similarly to an ordinary monomial, this class provides methods to query the <em>trigonometric</em> (partial) (low)
 * degree, defined
 * as if the multipliers were exponents of a regular monomial (e.g., the total trigonometric degree is the sum of the
 * multipliers).
 * Closely related is the concept of trigonometric order, calculated by adding the absolute values of the multipliers.
 *
 * This class satisfies the piranha::is_key, piranha::key_has_t_degree, piranha::key_has_t_ldegree,
 * piranha::key_has_t_order, piranha::key_has_t_lorder and piranha::key_is_differentiable type traits.
 *
 * ## Type requirements ##
 *
 * \p T must be suitable for use in piranha::kronecker_array. The default type for \p T is the signed counterpart of \p
 * std::size_t.
 *
 * ## Exception safety guarantee ##
 *
 * Unless otherwise specified, this class provides the strong exception safety guarantee for all operations.
 *
 * ## Move semantics ##
 *
 * The move semantics of this class are equivalent to the move semantics of C++ signed integral types.
 *
 * ## Serialization ##
 *
 * This class supports serialization.
 */
// NOTES:
// - it might make sense, for canonicalisation and is_compatible(), to provide a method in kronecker_array to get only
//   the first element of the array. This should be quite fast, and it will provide enough information for the
//   canon/compatibility.
// - related to the above: we can embed the flavour as the first element of the kronecker array - at that point checking
//   the flavour is just determining if the int value is even or odd.
// - need to do some reasoning on the impact of the codification in a specialised fast poisson series multiplier: how do
// we deal
//   with the canonical form without going through code/decode? if we require the last multiplier to be always positive
//   (instead
//   of the first), can we just check/flip the sign of the coded value?
template <typename T = std::make_signed<std::size_t>::type>
class real_trigonometric_kronecker_monomial
{
public:
    /// Alias for \p T.
    typedef T value_type;

private:
    typedef kronecker_array<value_type> ka;

public:
    /// Arity of the multiply() method.
    static const std::size_t multiply_arity = 2u;
    /// Size type.
    /**
     * Used to represent the number of variables in the monomial. Equivalent to the size type of
     * piranha::kronecker_array.
     */
    typedef typename ka::size_type size_type;
    /// Maximum monomial size.
    static const size_type max_size = 255u;
    /// Vector type used for temporary packing/unpacking.
    typedef static_vector<value_type, max_size> v_type;

private:
// Doxygen gets terribly confused by the following.
#if !defined(PIRANHA_DOXYGEN_INVOKED)
    static_assert(max_size <= std::numeric_limits<static_vector<int, 1u>::size_type>::max(), "Invalid max size.");
    // Eval and subs type definition.
    template <typename U, typename = void>
    struct eval_type_ {
    };
    template <typename U>
    using e_type = decltype(std::declval<U const &>() * std::declval<value_type const &>());
    template <typename U>
    struct eval_type_<U,
                      typename std::enable_if<is_addable_in_place<e_type<U>>::value
                                              && std::is_constructible<e_type<U>, int>::value
                                              && std::is_same<decltype(math::cos(std::declval<e_type<U>>())),
                                                              decltype(math::sin(std::declval<e_type<U>>()))>::value
                                              && std::is_constructible<decltype(math::cos(std::declval<e_type<U>>())),
                                                                       int>::value
                                              && detail::is_pmappable<U>::value>::type> {
        using type = decltype(math::cos(std::declval<e_type<U>>()));
    };
    // Final typedef for the eval type.
    template <typename U>
    using eval_type = typename eval_type_<U>::type;
    // Substitution utils.
    template <typename U>
    using subs_cos_type = decltype(math::cos(std::declval<const value_type &>() * std::declval<const U &>()));
    template <typename U>
    using subs_sin_type = decltype(math::sin(std::declval<const value_type &>() * std::declval<const U &>()));
    template <typename U, typename = void>
    struct subs_type_ {
    };
    template <typename U>
    struct subs_type_<U, typename std::enable_if<std::is_same<subs_cos_type<U>, subs_sin_type<U>>::value
                                                 && std::is_constructible<subs_cos_type<U>, int>::value
                                                 && std::is_assignable<subs_cos_type<U> &, subs_cos_type<U>>::value
                                                 && has_negate<subs_cos_type<U>>::value>::type> {
        using type = subs_cos_type<U>;
    };
    template <typename U>
    using subs_type = typename subs_type_<U>::type;
// Trig subs utils.
#define PIRANHA_TMP_TYPE                                                                                               \
    decltype((std::declval<value_type const &>()                                                                       \
              * math::binomial(std::declval<value_type const &>(), std::declval<value_type const &>()))                \
             * (std::declval<const U &>() * std::declval<const U &>()))
    template <typename U>
    using t_subs_type = typename std::
        enable_if<std::is_constructible<U, int>::value && std::is_default_constructible<U>::value
                      && std::is_assignable<U &, U>::value
                      && std::is_assignable<U &, decltype(std::declval<const U &>() * std::declval<const U &>())>::value
                      && is_addable_in_place<PIRANHA_TMP_TYPE,
                                             decltype(std::declval<const value_type &>()
                                                      * std::declval<PIRANHA_TMP_TYPE const &>())>::value
                      && has_negate<PIRANHA_TMP_TYPE>::value,
                  PIRANHA_TMP_TYPE>::type;
#undef PIRANHA_TMP_TYPE
    // Implementation of canonicalisation.
    static bool canonicalise_impl(v_type &unpacked)
    {
        const auto size = unpacked.size();
        bool sign_change = false;
        for (decltype(unpacked.size()) i = 0u; i < size; ++i) {
            if (sign_change || unpacked[i] < value_type(0)) {
                unpacked[i] = static_cast<value_type>(-unpacked[i]);
                sign_change = true;
            } else if (unpacked[i] > value_type(0)) {
                break;
            }
        }
        return sign_change;
    }
    // Couple of helper functions for Vieta's formulae.
    static value_type cos_phase(const value_type &n)
    {
        piranha_assert(n >= value_type(0));
        const value_type v[4] = {1, 0, -1, 0};
        return v[n % value_type(4)];
    }
    static value_type sin_phase(const value_type &n)
    {
        piranha_assert(n >= value_type(0));
        const value_type v[4] = {0, 1, 0, -1};
        return v[n % value_type(4)];
    }
    // Enabler for ctor from init list.
    template <typename U>
    using init_list_enabler = typename std::enable_if<has_safe_cast<value_type, U>::value, int>::type;
    // Enabler for ctor from iterator.
    template <typename Iterator>
    using it_ctor_enabler =
        typename std::enable_if<is_input_iterator<Iterator>::value
                                    && has_safe_cast<value_type, decltype(*std::declval<const Iterator &>())>::value,
                                int>::type;
    // Serialization support.
    // NOTE: split for exception safety.
    friend class boost::serialization::access;
    template <class Archive>
    void save(Archive &ar, unsigned int) const
    {
        ar &m_value;
        ar &m_flavour;
    }
    template <class Archive>
    void load(Archive &ar, unsigned int)
    {
        value_type value;
        bool flavour;
        ar &value;
        ar &flavour;
        m_value = value;
        m_flavour = flavour;
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
    // Enabler for multiplication.
    template <typename Cf>
    using multiply_enabler = typename std::enable_if<is_divisible_in_place<Cf, int>::value && has_negate<Cf>::value
                                                         && detail::true_tt<detail::cf_mult_enabler<Cf>>::value
                                                         && std::is_copy_assignable<Cf>::value,
                                                     int>::type;
    // Degree utils.
    using degree_type = decltype(std::declval<const T &>() + std::declval<const T &>());
    // Order utils.
    using order_type = decltype(math::abs(std::declval<const T &>()) + math::abs(std::declval<const T &>()));
#endif
public:
    /// Default constructor.
    /**
     * After construction all multipliers in the monomial will be zero, and the flavour will be set to \p true.
     */
    real_trigonometric_kronecker_monomial() : m_value(0), m_flavour(true)
    {
    }
    /// Defaulted copy constructor.
    real_trigonometric_kronecker_monomial(const real_trigonometric_kronecker_monomial &) = default;
    /// Defaulted move constructor.
    real_trigonometric_kronecker_monomial(real_trigonometric_kronecker_monomial &&) = default;
    /// Constructor from initalizer list.
    /**
     * \note
     * This constructor is enabled if \p U is safely convertible to \p T.
     *
     * The values in the initializer list are intended to represent the multipliers of the monomial:
     * they will be safely converted to type \p T (if \p T and \p U are not the same type),
     * encoded using piranha::kronecker_array::encode() and the result assigned to the internal integer instance.
     * The flavour will be set to \p true.
     *
     * @param[in] list initializer list representing the multipliers.
     *
     * @throws unspecified any exception thrown by:
     * - piranha::kronecker_array::encode(),
     * - piranha::safe_cast(),
     * - piranha::static_vector::push_back().
     */
    template <typename U, init_list_enabler<U> = 0>
    explicit real_trigonometric_kronecker_monomial(std::initializer_list<U> list) : m_value(0), m_flavour(true)
    {
        v_type tmp;
        for (const auto &x : list) {
            tmp.push_back(safe_cast<value_type>(x));
        }
        m_value = ka::encode(tmp);
    }
    /// Constructor from range.
    /**
     * \note
     * This constructor is enabled if \p Iterator is an input iterator whose \p value_type is safely convertible
     * to \p T.
     *
     * Will build internally a vector of values from the input iterators, encode it and assign the result
     * to the internal integer instance. The value type of the iterator is converted to \p T using
     * piranha::safe_cast(). The flavour will be set to \p true.
     *
     * @param[in] start beginning of the range.
     * @param[in] end end of the range.
     *
     * @throws unspecified any exception thrown by:
     * - piranha::kronecker_array::encode(),
     * - piranha::safe_cast(),
     * - piranha::static_vector::push_back().
     */
    template <typename Iterator, it_ctor_enabler<Iterator> = 0>
    explicit real_trigonometric_kronecker_monomial(const Iterator &start, const Iterator &end)
        : m_value(0), m_flavour(true)
    {
        typedef typename std::iterator_traits<Iterator>::value_type it_v_type;
        v_type tmp;
        std::transform(start, end, std::back_inserter(tmp),
                       [](const it_v_type &v) { return safe_cast<value_type>(v); });
        m_value = ka::encode(tmp);
    }
    /// Constructor from set of symbols.
    /**
     * After construction all multipliers will be zero and the flavour will be set to \p true.
     *
     * @param[in] args reference set of piranha::symbol.
     *
     * @throws unspecified any exception thrown by:
     * - piranha::kronecker_array::encode(),
     * - piranha::static_vector::push_back().
     */
    explicit real_trigonometric_kronecker_monomial(const symbol_set &args) : m_flavour(true)
    {
        v_type tmp;
        for (auto it = args.begin(); it != args.end(); ++it) {
            tmp.push_back(value_type(0));
        }
        m_value = ka::encode(tmp);
    }
    /// Constructor from \p value_type and flavour.
    /**
     * This constructor will initialise the internal integer instance
     * to \p n and the flavour to \p f.
     *
     * @param[in] n initializer for the internal integer instance.
     * @param[in] f desired flavour.
     */
    explicit real_trigonometric_kronecker_monomial(const value_type &n, bool f) : m_value(n), m_flavour(f)
    {
    }
    /// Converting constructor.
    /**
     * This constructor is for use when converting from one term type to another in piranha::series. It will
     * set the internal integer instance and flavour to the same value of \p other, after having checked that
     * \p other is compatible with \p args.
     *
     * @param[in] other construction argument.
     * @param[in] args reference set of piranha::symbol.
     *
     * @throws std::invalid_argument if \p other is not compatible with \p args.
     */
    explicit real_trigonometric_kronecker_monomial(const real_trigonometric_kronecker_monomial &other,
                                                   const symbol_set &args)
        : m_value(other.m_value), m_flavour(other.m_flavour)
    {
        if (unlikely(!other.is_compatible(args))) {
            piranha_throw(std::invalid_argument, "incompatible arguments");
        }
    }
    /// Trivial destructor.
    ~real_trigonometric_kronecker_monomial()
    {
        PIRANHA_TT_CHECK(is_key, real_trigonometric_kronecker_monomial);
        PIRANHA_TT_CHECK(key_has_t_degree, real_trigonometric_kronecker_monomial);
        PIRANHA_TT_CHECK(key_has_t_ldegree, real_trigonometric_kronecker_monomial);
        PIRANHA_TT_CHECK(key_has_t_order, real_trigonometric_kronecker_monomial);
        PIRANHA_TT_CHECK(key_has_t_lorder, real_trigonometric_kronecker_monomial);
        PIRANHA_TT_CHECK(key_is_differentiable, real_trigonometric_kronecker_monomial);
    }
    /// Defaulted copy assignment operator.
    real_trigonometric_kronecker_monomial &operator=(const real_trigonometric_kronecker_monomial &) = default;
    /// Defaulted move assignment operator.
    real_trigonometric_kronecker_monomial &operator=(real_trigonometric_kronecker_monomial &&) = default;
    /// Set the internal integer instance.
    /**
     * @param[in] n value to which the internal integer instance will be set.
     */
    void set_int(const value_type &n)
    {
        m_value = n;
    }
    /// Get internal instance.
    /**
     * @return value of the internal integer instance.
     */
    value_type get_int() const
    {
        return m_value;
    }
    /// Get flavour.
    /**
     * @return flavour of the monomial.
     */
    bool get_flavour() const
    {
        return m_flavour;
    }
    /// Set flavour.
    /**
     * @param[in] f value to which the flavour will be set.
     */
    void set_flavour(bool f)
    {
        m_flavour = f;
    }
    /// Canonicalise.
    /**
     * A monomial is considered to be in canonical form when the first nonzero multiplier is positive.
     * If \p this is not in canonical form, the method will canonicalise \p this by switching
     * the signs of all multipliers and return \p true.
     * Otherwise, \p this will not be modified and \p false will be returned.
     *
     * @param[in] args reference set of piranha::symbol.
     *
     * @return \p true if the monomial was canonicalised, \p false otherwise.
     *
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - piranha::kronecker_array::encode().
     */
    bool canonicalise(const symbol_set &args)
    {
        auto unpacked = unpack(args);
        const bool retval = canonicalise_impl(unpacked);
        if (retval) {
            m_value = ka::encode(unpacked);
        }
        return retval;
    }
    /// Compatibility check.
    /**
     * A monomial is considered incompatible if any of these conditions holds:
     *
     * - the size of \p args is zero and the internal integer is not zero,
     * - the size of \p args is equal to or larger than the size of the output of
     * piranha::kronecker_array::get_limits(),
     * - the internal integer is not within the limits reported by piranha::kronecker_array::get_limits(),
     * - the first nonzero element of the vector of multipliers represented by the internal integer is negative.
     *
     * Otherwise, the monomial is considered to be compatible for insertion.
     *
     * @param[in] args reference set of piranha::symbol.
     *
     * @return compatibility flag for the monomial.
     */
    bool is_compatible(const symbol_set &args) const noexcept
    {
        const auto s = args.size();
        // No args means the value must also be zero.
        if (s == 0u) {
            return !m_value;
        }
        const auto &limits = ka::get_limits();
        // If we overflow the maximum size available, we cannot use this object as key in series.
        if (s >= limits.size()) {
            return false;
        }
        const auto &l = limits[s];
        // Value is compatible if it is within the bounds for the given size.
        if (m_value < std::get<1u>(l) || m_value > std::get<2u>(l)) {
            return false;
        }
        // Now check for the first multiplier.
        // NOTE: here we have already checked all the conditions that could lead to unpack() throwing, so
        // we do not need to put @throw specifications in the doc.
        const auto unpacked = unpack(args);
        // We know that s != 0.
        piranha_assert(unpacked.size() > 0u);
        for (typename v_type::size_type i = 0u; i < s; ++i) {
            if (unpacked[i] < value_type(0)) {
                return false;
            } else if (unpacked[i] > value_type(0)) {
                break;
            }
        }
        return true;
    }
    /// Ignorability check.
    /**
     * A monomial is considered ignorable if all multipliers are zero and the flavour is \p false.
     *
     * @return ignorability flag.
     */
    bool is_ignorable(const symbol_set &) const noexcept
    {
        if (m_value == value_type(0) && !m_flavour) {
            return true;
        }
        return false;
    }
    /// Merge arguments.
    /**
     * Merge the new arguments set \p new_args into \p this, given the current reference arguments set
     * \p orig_args.
     *
     * @param[in] orig_args original arguments set.
     * @param[in] new_args new arguments set.
     *
     * @return monomial with merged arguments.
     *
     * @throws std::invalid_argument if at least one of these conditions is true:
     * - the size of \p new_args is not greater than the size of \p orig_args,
     * - not all elements of \p orig_args are included in \p new_args.
     * @throws unspecified any exception thrown by:
     * - piranha::kronecker_array::encode(),
     * - piranha::static_vector::push_back(),
     * - unpack().
     */
    real_trigonometric_kronecker_monomial merge_args(const symbol_set &orig_args, const symbol_set &new_args) const
    {
        return real_trigonometric_kronecker_monomial(detail::km_merge_args<v_type, ka>(orig_args, new_args, m_value),
                                                     m_flavour);
    }
    /// Check if monomial is unitary.
    /**
     * @param[in] args reference set of piranha::symbol.
     *
     * @return \p true if all the multipliers are zero and the flavour is \p true, \p false otherwise.
     *
     * @throws std::invalid_argument if \p this is not compatible with \p args.
     */
    bool is_unitary(const symbol_set &args) const
    {
        if (unlikely(!is_compatible(args))) {
            piranha_throw(std::invalid_argument, "invalid symbol set");
        }
        return (!m_value && m_flavour);
    }
    /// Trigonometric degree.
    /**
     * The type returned by this method is the type resulting from the addition of two instances
     * of \p T.
     *
     * @param[in] args reference set of piranha::symbol.
     *
     * @return trigonometric degree of the monomial.
     *
     * @throws std::overflow_error if the computation of the degree overflows.
     * @throws unspecified any exception thrown by unpack().
     */
    degree_type t_degree(const symbol_set &args) const
    {
        const auto tmp = unpack(args);
        // NOTE: this should be guaranteed by the unpack function.
        piranha_assert(tmp.size() == args.size());
        degree_type retval(0);
        for (const auto &x : tmp) {
            detail::safe_integral_adder(retval, static_cast<degree_type>(x));
        }
        return retval;
    }
    /// Low trigonometric degree (equivalent to the trigonometric degree).
    degree_type t_ldegree(const symbol_set &args) const
    {
        return t_degree(args);
    }
    /// Partial trigonometric degree.
    /**
     * Partial trigonometric degree of the monomial. The \p p argument is used to indicate which multipliers are to be
     * taken into account when
     * computing the partial degree. Multipliers not in \p p will be discarded during the computation of the partial
     * degree.
     *
     * The type returned by this method is the type resulting from the addition of two instances
     * of \p T.
     *
     * @param[in] p positions of the symbols to be considered.
     * @param[in] args reference set of piranha::symbol.
     *
     * @return the summation of all the multipliers of the monomial corresponding to the symbols at the positions in
     * \p p, or <tt>value_type(0)</tt> if no symbols in \p p appear in \p args.
     *
     * @throws std::invalid_argument if \p p is not compatible with \p args.
     * @throws std::overflow_error if the computation of the degree overflows.
     * @throws unspecified any exception thrown by unpack().
     */
    degree_type t_degree(const symbol_set::positions &p, const symbol_set &args) const
    {
        const auto tmp = unpack(args);
        piranha_assert(tmp.size() == args.size());
        if (unlikely(p.size() && p.back() >= tmp.size())) {
            piranha_throw(std::invalid_argument, "invalid positions");
        }
        auto cit = tmp.begin();
        degree_type retval(0);
        for (const auto &i : p) {
            detail::safe_integral_adder(retval, static_cast<degree_type>(cit[i]));
        }
        return retval;
    }
    /// Partial low trigonometric degree (equivalent to the partial trigonometric degree).
    degree_type t_ldegree(const symbol_set::positions &p, const symbol_set &args) const
    {
        return t_degree(p, args);
    }
    /// Trigonometric order.
    /**
     * The type returned by this method is the type resulting from the addition of the absolute
     * values of two instances of \p T.
     *
     * @param[in] args reference set of piranha::symbol.
     *
     * @return trigonometric order of the monomial.
     *
     * @throws std::overflow_error if the computation of the degree overflows.
     * @throws unspecified any exception thrown by unpack().
     */
    order_type t_order(const symbol_set &args) const
    {
        const auto tmp = unpack(args);
        piranha_assert(tmp.size() == args.size());
        order_type retval(0);
        for (const auto &x : tmp) {
            // NOTE: here the k codification is symmetric, we can always take the negative
            // safely.
            detail::safe_integral_adder(retval, static_cast<order_type>(math::abs(x)));
        }
        return retval;
    }
    /// Low trigonometric order (equivalent to the trigonometric order).
    order_type t_lorder(const symbol_set &args) const
    {
        return t_order(args);
    }
    /// Partial trigonometric order.
    /**
     * Partial trigonometric order of the monomial. The \p p argument is used to indicate which multipliers are to be
     * taken into account when
     * computing the partial order. Multipliers not in \p p will be discarded during the computation of the partial
     * order.
     *
     * The type returned by this method is the type resulting from the addition of the absolute
     * values of two instances of \p T.
     *
     * @param[in] p positions of the symbols to be considered.
     * @param[in] args reference set of piranha::symbol.
     *
     * @return the summation of the absolute values of all the multipliers of the monomial corresponding to the symbols
     * at the positions in
     * \p p, or <tt>value_type(0)</tt> if no symbols in \p p appear in \p args.
     *
     * @throws std::invalid_argument if \p p is not compatible with \p args.
     * @throws std::overflow_error if the computation of the degree overflows.
     * @throws unspecified any exception thrown by unpack().
     */
    order_type t_order(const symbol_set::positions &p, const symbol_set &args) const
    {
        const auto tmp = unpack(args);
        piranha_assert(tmp.size() == args.size());
        if (unlikely(p.size() && p.back() >= tmp.size())) {
            piranha_throw(std::invalid_argument, "invalid positions");
        }
        auto cit = tmp.begin();
        order_type retval(0);
        for (const auto &i : p) {
            detail::safe_integral_adder(retval, static_cast<order_type>(math::abs(cit[i])));
        }
        return retval;
    }
    /// Partial low trigonometric order (equivalent to the partial trigonometric order).
    order_type t_lorder(const symbol_set::positions &p, const symbol_set &args) const
    {
        return t_order(p, args);
    }
    /// Multiply terms with a trigonometric monomial.
    /**
     * \note
     * This method is enabled only if the following conditions hold:
     * - \p Cf satisfies piranha::is_cf,
     * - \p Cf satisfies piranha::has_mul3,
     * - \p Cf is divisible in-place by \p int,
     * - \p Cf is copy-assignable and it satisfies piranha::has_negate.
     *
     * This method will compute the result of the multiplication of the two terms \p t1 and \p t2 with trigonometric
     * key.
     * The result is stored in the two terms of \p res and it is computed using basic trigonometric formulae.
     * Note however that this method will **not** perform the division by two implied by Werner's formulae. Also, in
     * case
     * \p Cf is an instance of piranha::mp_rational, only the numerators of the coefficients will be multiplied.
     *
     * @param[out] res result of the multiplication.
     * @param[in] t1 first argument.
     * @param[in] t2 second argument.
     * @param[in] args reference set of piranha::symbol.
     *
     * @throws std::overflow_error if the computation of the result overflows type \p value_type.
     * @throws unspecified any exception thrown by:
     * - piranha::kronecker_array::encode(),
     * - unpack(),
     * - piranha::static_vector::push_back(),
     * - piranha::math::mul3(),
     * - copy-assignment on the coefficient type,
     * - piranha::math::negate().
     */
    template <typename Cf, multiply_enabler<Cf> = 0>
    static void multiply(std::array<term<Cf, real_trigonometric_kronecker_monomial>, multiply_arity> &res,
                         const term<Cf, real_trigonometric_kronecker_monomial> &t1,
                         const term<Cf, real_trigonometric_kronecker_monomial> &t2, const symbol_set &args)
    {
        // Coefficients first.
        detail::cf_mult_impl(res[0u].m_cf, t1.m_cf, t2.m_cf);
        res[1u].m_cf = res[0u].m_cf;
        const bool f1 = t1.m_key.get_flavour(), f2 = t2.m_key.get_flavour();
        if (f1 && f2) {
            // cos, cos: no change.
        } else if (!f1 && !f2) {
            // sin, sin: negate the plus.
            math::negate(res[0u].m_cf);
        } else if (!f1 && f2) {
            // sin, cos: no change.
        } else {
            // cos, sin: negate the minus.
            math::negate(res[1u].m_cf);
        }
        // Now the keys.
        auto &retval_plus = res[0u].m_key;
        auto &retval_minus = res[1u].m_key;
        // Flags to signal if a sign change in the multipliers was needed as part of the canonicalization.
        bool sign_plus = false, sign_minus = false;
        const auto size = args.size();
        const auto tmp1 = t1.m_key.unpack(args), tmp2 = t2.m_key.unpack(args);
        v_type result_plus, result_minus;
        for (typename v_type::size_type i = 0u; i < size; ++i) {
            result_plus.push_back(tmp1[i]);
            detail::safe_integral_adder(result_plus[i], tmp2[i]);
            // NOTE: it is safe here to take the negative because in kronecker_array we are guaranteed
            // that the range of each element is symmetric, so if tmp2[i] is representable also -tmp2[i] is.
            // NOTE: the static cast here is because if value_type is narrower than int, the unary minus will promote
            // to int and safe_adder won't work as it expects identical types.
            result_minus.push_back(tmp1[i]);
            detail::safe_integral_adder(result_minus[i], static_cast<value_type>(-tmp2[i]));
        }
        // Handle sign changes.
        sign_plus = canonicalise_impl(result_plus);
        sign_minus = canonicalise_impl(result_minus);
        // Compute them before assigning, so in case of exceptions we do not touch the return values.
        const auto re_plus = ka::encode(result_plus), re_minus = ka::encode(result_minus);
        retval_plus.m_value = re_plus;
        retval_minus.m_value = re_minus;
        const bool f = (t1.m_key.get_flavour() == t2.m_key.get_flavour());
        retval_plus.m_flavour = f;
        retval_minus.m_flavour = f;
        // If multiplier sign was changed and the result is a sine, negate the coefficient.
        if (sign_plus && !res[0u].m_key.get_flavour()) {
            math::negate(res[0u].m_cf);
        }
        if (sign_minus && !res[1u].m_key.get_flavour()) {
            math::negate(res[1u].m_cf);
        }
    }
    /// Hash value.
    /**
     * @return the internal integer instance, cast to \p std::size_t.
     */
    std::size_t hash() const
    {
        return static_cast<std::size_t>(m_value);
    }
    /// Equality operator.
    /**
     * @param[in] other comparison argument.
     *
     * @return \p true if the internal integral instance and the flavour of \p this are the same of \p other,
     * \p false otherwise.
     */
    bool operator==(const real_trigonometric_kronecker_monomial &other) const
    {
        return (m_value == other.m_value && m_flavour == other.m_flavour);
    }
    /// Inequality operator.
    /**
     * @param[in] other comparison argument.
     *
     * @return the opposite of operator==().
     */
    bool operator!=(const real_trigonometric_kronecker_monomial &other) const
    {
        return (m_value != other.m_value || m_flavour != other.m_flavour);
    }
    /// Unpack internal integer instance.
    /**
     * Will decode the internal integral instance into a piranha::static_vector of size equal to the size of \p args.
     *
     * @param[in] args reference set of piranha::symbol.
     *
     * @return piranha::static_vector containing the result of decoding the internal integral instance via
     * piranha::kronecker_array.
     *
     * @throws std::invalid_argument if the size of \p args is larger than the maximum size of piranha::static_vector.
     * @throws unspecified any exception thrown by piranha::kronecker_array::decode().
     */
    v_type unpack(const symbol_set &args) const
    {
        return detail::km_unpack<v_type, ka>(args, m_value);
    }
    /// Print.
    /**
     * Will print to stream a human-readable representation of the monomial.
     *
     * @param[in] os target stream.
     * @param[in] args reference set of piranha::symbol.
     *
     * @throws unspecified any exception thrown by unpack() or by streaming instances of \p value_type.
     */
    void print(std::ostream &os, const symbol_set &args) const
    {
        // Don't print anything in case all multipliers are zero.
        if (m_value == value_type(0)) {
            return;
        }
        if (m_flavour) {
            os << "cos(";
        } else {
            os << "sin(";
        }
        const auto tmp = unpack(args);
        piranha_assert(tmp.size() == args.size());
        const value_type zero(0), one(1), m_one(-1);
        bool empty_output = true;
        for (decltype(tmp.size()) i = 0u; i < tmp.size(); ++i) {
            if (tmp[i] != zero) {
                // A positive multiplier, in case previous output exists, must be preceded
                // by a "+" sign.
                if (tmp[i] > zero && !empty_output) {
                    os << '+';
                }
                // Print the multiplier, unless it's "-1": in that case, just print the minus sign.
                if (tmp[i] == m_one) {
                    os << '-';
                } else if (tmp[i] != one) {
                    os << detail::prepare_for_print(tmp[i]) << '*';
                }
                // Finally, print name of variable.
                os << args[i].get_name();
                empty_output = false;
            }
        }
        os << ")";
    }
    /// Print in TeX mode.
    /**
     * Will print to stream a TeX representation of the monomial.
     *
     * @param[in] os target stream.
     * @param[in] args reference set of piranha::symbol.
     *
     * @throws unspecified any exception thrown by unpack() or by streaming instances of \p value_type.
     */
    void print_tex(std::ostream &os, const symbol_set &args) const
    {
        // Don't print anything in case all multipliers are zero.
        if (m_value == value_type(0)) {
            return;
        }
        if (m_flavour) {
            os << "\\cos{\\left(";
        } else {
            os << "\\sin{\\left(";
        }
        const auto tmp = unpack(args);
        piranha_assert(tmp.size() == args.size());
        const value_type zero(0), one(1), m_one(-1);
        bool empty_output = true;
        for (decltype(tmp.size()) i = 0u; i < tmp.size(); ++i) {
            if (tmp[i] != zero) {
                // A positive multiplier, in case previous output exists, must be preceded
                // by a "+" sign.
                if (tmp[i] > zero && !empty_output) {
                    os << '+';
                }
                // Print the multiplier, unless it's "-1": in that case, just print the minus sign.
                if (tmp[i] == m_one) {
                    os << '-';
                } else if (tmp[i] != one) {
                    os << static_cast<long long>(tmp[i]);
                }
                // Finally, print name of variable.
                os << '{' << args[i].get_name() << '}';
                empty_output = false;
            }
        }
        os << "\\right)}";
    }
    /// Partial derivative.
    /**
     * This method will return the partial derivative of \p this with respect to the symbol at the position indicated by
     * \p p.
     * The result is a pair consisting of the multiplier associated to \p p and a copy of the monomial.
     * The sign of the multiplier and the flavour of the resulting monomial are set according to the standard
     * differentiation formulas for elementary trigonometric functions.
     * If \p p is empty or if the multiplier associated to it is zero,
     * the returned pair will be <tt>(0,real_trigonometric_kronecker_monomial{args})</tt>.
     *
     * @param[in] p position of the symbol with respect to which the differentiation will be calculated.
     * @param[in] args reference set of piranha::symbol.
     *
     * @return result of the differentiation.
     *
     * @throws std::invalid_argument if \p p is incompatible with \p args or it has a size greater than one.
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - piranha::math::is_zero().
     */
    std::pair<T, real_trigonometric_kronecker_monomial> partial(const symbol_set::positions &p,
                                                                const symbol_set &args) const
    {
        auto v = unpack(args);
        // Cannot take derivative wrt more than one variable, and the position of that variable
        // must be compatible with the monomial.
        if (p.size() > 1u || (p.size() == 1u && p.back() >= args.size())) {
            piranha_throw(std::invalid_argument, "invalid size of symbol_set::positions");
        }
        // Derivative wrt a variable not in the monomial: position is empty, or refers to a
        // variable with zero multiplier.
        // NOTE: safe to take v.begin() here, as the checks on the positions above ensure
        // there is a valid position and hence the size must be not zero.
        if (!p.size() || math::is_zero(v.begin()[*p.begin()])) {
            return std::make_pair(T(0), real_trigonometric_kronecker_monomial(args));
        }
        auto v_b = v.begin();
        // Original multiplier.
        T n(v_b[*p.begin()]);
        // Create a copy of this.
        real_trigonometric_kronecker_monomial tmp_m(*this);
        // Flip the flavour.
        tmp_m.set_flavour(!get_flavour());
        // Flip the sign of the multiplier as needed.
        if (get_flavour()) {
            // NOTE: this is safe, as it is coming out of a k codification which
            // is symmetric.
            math::negate(n);
        }
        return std::make_pair(std::move(n), std::move(tmp_m));
    }
    /// Integration.
    /**
     * Will return the antiderivative of \p this with respect to symbol \p s. The result is a pair
     * consisting of the multiplier associated to \p s and a copy of the monomial.
     * The sign of the multiplier and the flavour of the resulting monomial are set according to the standard
     * integration formulas for elementary trigonometric functions.
     * If \p s is not in \p args or if the multiplier associated to it is zero,
     * the returned pair will be <tt>(0,real_trigonometric_kronecker_monomial{})</tt>.
     *
     * @param[in] s symbol with respect to which the integration will be calculated.
     * @param[in] args reference set of piranha::symbol.
     *
     * @return result of the integration.
     *
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - piranha::math::is_zero(),
     * - monomial construction,
     * - the cast operator of piranha::integer.
     */
    std::pair<T, real_trigonometric_kronecker_monomial> integrate(const symbol &s, const symbol_set &args) const
    {
        auto v = unpack(args);
        for (min_int<decltype(args.size()), typename v_type::size_type> i = 0u; i < args.size(); ++i) {
            if (args[i] == s && !math::is_zero(v[i])) {
                integer tmp_n(v[i]);
                real_trigonometric_kronecker_monomial tmp_m(*this);
                // Flip the flavour.
                tmp_m.set_flavour(!get_flavour());
                // Flip the sign of the multiplier as needed.
                if (!get_flavour()) {
                    tmp_n.negate();
                }
                return std::make_pair(static_cast<value_type>(tmp_n), std::move(tmp_m));
            }
        }
        return std::make_pair(value_type(0), real_trigonometric_kronecker_monomial());
    }
    /// Evaluation.
    /**
     * \note
     * This template method is activated only if \p U supports the mathematical operations needed to compute
     * the return type, and if it is suitable for use in piranha::symbol_set::positions_map.
     *
     * The return value will be built by applying piranha::math::cos() or piranha::math:sin()
     * to the linear combination of the values in \p pmap with the multipliers.
     * If the size of the monomial is zero, 1 will be returned if the monomial is a cosine, 0 otherwise.
     * If the positions in \p pmap do not reference
     * only and all the multipliers in the monomial, an error will be thrown.
     *
     * @param[in] pmap piranha::symbol_set::positions_map that will be used for substitution.
     * @param[in] args reference set of piranha::symbol.
     *
     * @return the result of evaluating \p this with the values provided in \p pmap.
     *
     * @throws std::invalid_argument if \p pmap is not compatible with \p args.
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - construction of the return type,
     * - piranha::math::cos(), piranha::math::sin(), or the in-place addition and binary multiplication operators of the
     * types
     *   involved in the computation.
     */
    template <typename U>
    eval_type<U> evaluate(const symbol_set::positions_map<U> &pmap, const symbol_set &args) const
    {
        using return_type = eval_type<U>;
        if (unlikely(pmap.size() != args.size() || (pmap.size() && pmap.back().first != pmap.size() - 1u))) {
            piranha_throw(std::invalid_argument, "invalid positions map for evaluation");
        }
        // Run the unpack before the checks below in order to check the suitability of args.
        auto v = unpack(args);
        if (args.size() == 0u) {
            if (get_flavour()) {
                return return_type(1);
            } else {
                return return_type(0);
            }
        }
        using tmp_type = decltype(std::declval<U const &>() * std::declval<value_type const &>());
        tmp_type tmp(0);
        auto it = pmap.begin();
        for (min_int<decltype(args.size()), typename v_type::size_type> i = 0u; i < args.size(); ++i, ++it) {
            piranha_assert(it != pmap.end() && it->first == i);
            // NOTE: here it might make sense to use multiply_accumulate. There might be a perf gain
            // for things like real. Maybe fast FMA on Haswell too. Remember to adapt the type requirements
            // in case we implement this.
            tmp += it->second * v[i];
        }
        piranha_assert(it == pmap.end());
        if (get_flavour()) {
            return math::cos(tmp);
        }
        return math::sin(tmp);
    }
    /// Substitution.
    /**
     * \note
     * This method is enabled only if:
     * - the value type can be multiplied by \p U, and the result can be passed to piranha::math::cos()
     *   or piranha::math::sin(), yielding a type \p subs_type,
     * - \p subs_type is constructible from \p int, assignable and it supports piranha::math::negate().
     *
     * Substitute the symbol called \p s in the monomial with quantity \p x. The return value is a vector of two pairs
     * computed according to the standard angle sum identities. That is, given a monomial of the form
     * \f[
     * \begin{array}{c}
     * \sin \\
     * \cos
     * \end{array}
     * \left(na + b\right)
     * \f]
     * in which the symbol \f$ a \f$ is to be substituted with \f$ x \f$, the result of the substitution will be
     * one of
     * \f[
     * \begin{array}{c}
     * \left[\left(\sin nx,\cos b \right),\left(\cos nx,\sin b \right)\right], \\
     * \left[\left(\cos nx,\cos b \right),\left(-\sin nx,\sin b \right)\right],
     * \end{array}
     * \f]
     * where \f$ \cos b \f$ and \f$ \sin b \f$ are returned as monomials, and \f$ \cos nx \f$ and \f$ \sin nx \f$
     * as the return values of piranha::math::cos() and piranha::math::sin(). If \p s is not in \p args,
     * \f$ \cos nx \f$ will be initialised to 1 and \f$ \sin nx \f$ to 0. If, after the substitution, the first nonzero
     * multiplier
     * in \f$ b \f$ is negative, \f$ b \f$ will be negated and the other signs changed accordingly.
     *
     * @param[in] s name of the symbol that will be substituted.
     * @param[in] x quantity that will be substituted in place of \p s.
     * @param[in] args reference set of piranha::symbol.
     *
     * @return the result of substituting \p x for \p s.
     *
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - construction and assignment of the return value,
     * - piranha::math::cos(), piranha::math::sin() and piranha::math::negate(),
     * - piranha::static_vector::push_back(),
     * - piranha::kronecker_array::encode().
     */
    template <typename U>
    std::vector<std::pair<subs_type<U>, real_trigonometric_kronecker_monomial>> subs(const std::string &s, const U &x,
                                                                                     const symbol_set &args) const
    {
        using s_type = subs_type<U>;
        using ret_type = std::vector<std::pair<subs_type<U>, real_trigonometric_kronecker_monomial>>;
        ret_type retval;
        const auto v = unpack(args);
        v_type new_v;
        s_type retval_s_cos(1), retval_s_sin(0);
        for (min_int<decltype(args.size()), typename v_type::size_type> i = 0u; i < args.size(); ++i) {
            if (args[i].get_name() == s) {
                retval_s_cos = math::cos(v[i] * x);
                retval_s_sin = math::sin(v[i] * x);
                new_v.push_back(value_type(0));
            } else {
                new_v.push_back(v[i]);
            }
        }
        const bool sign_changed = canonicalise_impl(new_v);
        piranha_assert(new_v.size() == v.size());
        const auto new_int = ka::encode(new_v);
        real_trigonometric_kronecker_monomial cos_key(new_int, true), sin_key(new_int, false);
        if (get_flavour()) {
            retval.push_back(std::make_pair(std::move(retval_s_cos), std::move(cos_key)));
            retval.push_back(std::make_pair(std::move(retval_s_sin), std::move(sin_key)));
            // Need to flip the sign on the sin * sin product if sign was not changed.
            if (!sign_changed) {
                math::negate(retval[1u].first);
            }
        } else {
            retval.push_back(std::make_pair(std::move(retval_s_sin), std::move(cos_key)));
            retval.push_back(std::make_pair(std::move(retval_s_cos), std::move(sin_key)));
            // Need to flip the sign on the cos * sin product if sign was changed.
            if (sign_changed) {
                math::negate(retval[1u].first);
            }
        }
        return retval;
    }
    /// Trigonometric substitution.
    /**
     * \note
     * This method is enabled only if \p U supports the mathematical operations needed to compute the result. In
     * particular,
     * the implementation uses piranha::math::binomial() internally (thus, \p U must be interoperable with
     * piranha::integer).
     *
     * This method works in the same way as the subs() method, but the cosine \p c and sine \p s of \p name will be
     * substituted (instead of a direct
     * substitution of \p name).
     * The substitution is performed using standard trigonometric formulae, and it will result in a list of two
     * (substitution result,new monomial) pairs.
     *
     * @param[in] name symbol whose cosine and sine will be substituted.
     * @param[in] c cosine of \p name.
     * @param[in] s sine of \p name.
     * @param[in] args reference set of piranha::symbol.
     *
     * @return the result of substituting \p c and \p s for the cosine and sine of \p name.
     *
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - construction, assignment and arithmetics on the return value and on the intermediary values invovled in the
     * computation,
     * - piranha::math::negate() and piranha::math::binomial(),
     * - piranha::static_vector::push_back(),
     * - piranha::kronecker_array::encode().
     */
    template <typename U>
    std::vector<std::pair<t_subs_type<U>, real_trigonometric_kronecker_monomial>>
    t_subs(const std::string &name, const U &c, const U &s, const symbol_set &args) const
    {
        typedef decltype(this->t_subs(name, c, s, args)) ret_type;
        typedef typename ret_type::value_type::first_type res_type;
        const auto v = unpack(args);
        v_type new_v;
        value_type n(0);
        // Build the new vector key.
        for (min_int<decltype(args.size()), typename v_type::size_type> i = 0u; i < args.size(); ++i) {
            if (args[i].get_name() == name) {
                new_v.push_back(value_type(0));
                n = v[i];
            } else {
                new_v.push_back(v[i]);
            }
        }
        // Absolute value of the multiplier.
        const value_type abs_n = (n >= 0) ? n : static_cast<value_type>(-n);
        // Prepare the powers of c and s to be used in the multiple angles formulae.
        std::unordered_map<value_type, U> c_map, s_map;
        c_map[0] = U(1);
        s_map[0] = U(1);
        for (value_type k(0); k < abs_n; ++k) {
            c_map[k + value_type(1)] = c_map[k] * c;
            s_map[k + value_type(1)] = s_map[k] * s;
        }
        // Init with the first element in the summation.
        res_type cos_nx((cos_phase(abs_n) * math::binomial(abs_n, value_type(0)))
                        * (c_map[value_type(0)] * s_map[abs_n])),
            sin_nx((sin_phase(abs_n) * math::binomial(abs_n, value_type(0))) * (c_map[value_type(0)] * s_map[abs_n]));
        for (value_type k(0); k < abs_n; ++k) {
            const value_type p = abs_n - (k + value_type(1));
            piranha_assert(p >= value_type(0));
            // NOTE: here the type is slightly different from the decltype() that determines the return type, but as
            // long
            // as binomial(value_type,value_type) returns integer there will be no difference because of the
            // left-to-right associativity of multiplication.
            res_type tmp(math::binomial(abs_n, k + value_type(1)) * (c_map[k + value_type(1)] * s_map[p]));
            cos_nx += cos_phase(p) * tmp;
            sin_nx += sin_phase(p) * tmp;
        }
        // Change sign as necessary.
        if (abs_n != n) {
            math::negate(sin_nx);
        }
        // Buld the new keys and canonicalise as needed.
        const bool sign_changed = canonicalise_impl(new_v);
        piranha_assert(new_v.size() == v.size());
        const auto new_int = ka::encode(new_v);
        real_trigonometric_kronecker_monomial cos_key(new_int, true), sin_key(new_int, false);
        ret_type retval;
        if (get_flavour()) {
            retval.push_back(std::make_pair(std::move(cos_nx), std::move(cos_key)));
            retval.push_back(std::make_pair(std::move(sin_nx), std::move(sin_key)));
            // Need to flip the sign on the sin * sin product if sign was not changed.
            if (!sign_changed) {
                math::negate(retval[1u].first);
            }
        } else {
            retval.push_back(std::make_pair(std::move(sin_nx), std::move(cos_key)));
            retval.push_back(std::make_pair(std::move(cos_nx), std::move(sin_key)));
            // Need to flip the sign on the cos * sin product if sign was changed.
            if (sign_changed) {
                math::negate(retval[1u].first);
            }
        }
        return retval;
    }
    /// Identify symbols that can be trimmed.
    /**
     * This method is used in piranha::series::trim(). The input parameter \p candidates
     * contains a set of symbols that are candidates for elimination. The method will remove
     * from \p candidates those symbols whose multiplier in \p this is not zero.
     *
     * @param[in] candidates set of candidates for elimination.
     * @param[in] args reference arguments set.
     *
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - piranha::math::is_zero(),
     * - piranha::symbol_set::remove().
     */
    void trim_identify(symbol_set &candidates, const symbol_set &args) const
    {
        return detail::km_trim_identify<v_type, ka>(candidates, args, m_value);
    }
    /// Trim.
    /**
     * This method will return a copy of \p this with the multipliers associated to the symbols
     * in \p trim_args removed.
     *
     * @param[in] trim_args arguments whose multipliers will be removed.
     * @param[in] orig_args original arguments set.
     *
     * @return trimmed copy of \p this.
     *
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - piranha::static_vector::push_back().
     */
    real_trigonometric_kronecker_monomial trim(const symbol_set &trim_args, const symbol_set &orig_args) const
    {
        return real_trigonometric_kronecker_monomial(detail::km_trim<v_type, ka>(trim_args, orig_args, m_value),
                                                     m_flavour);
    }
    /// Comparison operator.
    /**
     * The values of the internal integral instances are used for comparison. If the values are the same,
     * the flavours of the monomials are compared to break the tie.
     *
     * @param[in] other comparison argument.
     *
     * @return \p true if \p this is less than \p other, \p false otherwise.
     */
    bool operator<(const real_trigonometric_kronecker_monomial &other) const
    {
        if (m_value < other.m_value) {
            return true;
        }
        if (other.m_value < m_value) {
            return false;
        }
        return m_flavour < other.m_flavour;
    }

private:
    template <typename U>
    using boost_save_binary_enabler
        = enable_if_t<conjunction<has_boost_save<boost::archive::binary_oarchive, U>,
                                  has_boost_save<boost::archive::binary_oarchive, bool>>::value,
                      int>;
    template <typename U>
    using boost_save_text_enabler
        = enable_if_t<conjunction<has_boost_save<boost::archive::text_oarchive, typename U::v_type>,
                                  has_boost_save<boost::archive::text_oarchive, bool>>::value,
                      int>;
    template <typename U>
    using boost_load_binary_enabler
        = enable_if_t<conjunction<has_boost_load<boost::archive::binary_iarchive, U>,
                                  has_boost_load<boost::archive::binary_iarchive, bool>>::value,
                      int>;
    template <typename U>
    using boost_load_text_enabler
        = enable_if_t<conjunction<has_boost_load<boost::archive::text_iarchive, typename U::v_type>,
                                  has_boost_load<boost::archive::text_iarchive, bool>>::value,
                      int>;

public:
    /// Save to Boost binary archive.
    /**
     * \note
     * This method is enabled only if \p T and \p bool support piranha::boost_save().
     *
     * This method will save to the archive \p oa the internal integral instance and the flavour.
     *
     * @param oa the target archive.
     *
     * @throws unspecified any exception thrown by piranha::boost_save().
     */
    template <typename U = T, boost_save_binary_enabler<U> = 0>
    void boost_save(boost::archive::binary_oarchive &oa, const symbol_set &) const
    {
        piranha::boost_save(oa, m_value);
        piranha::boost_save(oa, m_flavour);
    }
    /// Save to Boost text archive.
    /**
     * \note
     * This method is enabled only if piranha::real_trigonometric_kronecker_monomial::v_type
     * and \p bool support piranha::boost_save().
     *
     * This method will unpack \p this and save the vector of multipliers and the flavour to \p oa.
     *
     * @param oa the target archive.
     * @param args reference arguments set.
     *
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - piranha::boost_save().
     */
    template <typename U = real_trigonometric_kronecker_monomial, boost_save_text_enabler<U> = 0>
    void boost_save(boost::archive::text_oarchive &oa, const symbol_set &args) const
    {
        auto tmp = unpack(args);
        piranha::boost_save(oa, tmp);
        piranha::boost_save(oa, m_flavour);
    }
    /// Load from Boost binary archive.
    /**
     * \note
     * This method is enabled only if \p T and \p bool support piranha::boost_load().
     *
     * This method will load into \p this the content of the input archive \p ia. No checking is performed
     * on the content of \p ia. This method provides the basic exception safety guarantee.
     *
     * @param ia the source archive.
     *
     * @throws unspecified any exception thrown by piranha::boost_load().
     */
    template <typename U = T, boost_load_binary_enabler<U> = 0>
    void boost_load(boost::archive::binary_iarchive &ia, const symbol_set &)
    {
        piranha::boost_load(ia, m_value);
        piranha::boost_load(ia, m_flavour);
    }
    /// Load from Boost text archive.
    /**
     * \note
     * This method is enabled only if piranha::real_trigonometric_kronecker_monomial::v_type
     * and \p bool support piranha::boost_load().
     *
     * This method will load into \p this the content of the input archive \p ia. This method provides the basic
     * exception safety guarantee.
     *
     * @param ia the source archive.
     * @param args reference arguments set.
     *
     * @throws std::invalid_argument if the size of the serialized monomial is different from the size of \p args.
     * @throws unspecified any exception thrown by:
     * - piranha::boost_load(),
     * - the constructor of piranha::kronecker_monomial from a container.
     */
    template <typename U = real_trigonometric_kronecker_monomial, boost_load_text_enabler<U> = 0>
    void boost_load(boost::archive::text_iarchive &ia, const symbol_set &args)
    {
        v_type tmp;
        piranha::boost_load(ia, tmp);
        if (unlikely(tmp.size() != args.size())) {
            piranha_throw(std::invalid_argument, "invalid size detected in the deserialization of a real Kronercker "
                                                 "trigonometric monomial: the deserialized size is "
                                                     + std::to_string(tmp.size())
                                                     + " but the reference symbol set has a size of "
                                                     + std::to_string(args.size()));
        }
        // NOTE: here the exception safety is basic, as the last boost_load() could fail in principle.
        // It does not really matter much, as there's no real dependency between the multipliers and the flavour,
        // any combination is valid.
        *this = real_trigonometric_kronecker_monomial(tmp.begin(), tmp.end());
        piranha::boost_load(ia, m_flavour);
    }
#if defined(PIRANHA_WITH_MSGPACK)
private:
    template <typename Stream>
    using msgpack_pack_enabler =
        typename std::enable_if<conjunction<is_msgpack_stream<Stream>, has_msgpack_pack<Stream, T>,
                                            has_msgpack_pack<Stream, bool>, has_msgpack_pack<Stream,v_type>>::value,
                                int>::type;
    template <typename U>
    using msgpack_convert_enabler =
        typename std::enable_if<conjunction<has_msgpack_convert<typename U::value_type>, has_msgpack_convert<typename U::v_type>,
        has_msgpack_convert<bool>>::value, int>::type;

public:
    /// Serialize in msgpack format.
    /**
     * \note
     * This method is activated only if \p Stream satisfies piranha::is_msgpack_stream and \p T, \p bool
     * and piranha::real_trigonometric_kronecker_monomial::v_type satisfy piranha::has_msgpack_pack.
     *
     * This method will pack \p this into \p packer.
     *
     * @param[in] packer the target packer.
     * @param[in] f the serialization format.
     * @param[in] s reference arguments set.
     *
     * @throws unspecified any exception thrown by:
     * - the public interface of <tt>msgpack::packer</tt>,
     * - piranha::msgpack_pack().
     */
    template <typename Stream, msgpack_pack_enabler<Stream> = 0>
    void msgpack_pack(msgpack::packer<Stream> &packer, msgpack_format f, const symbol_set &s) const
    {
        packer.pack_array(2);
        if (f == msgpack_format::binary) {
            piranha::msgpack_pack(packer, m_value, f);
            piranha::msgpack_pack(packer, m_flavour, f);
        } else {
            auto tmp = unpack(s);
            piranha::msgpack_pack(packer, tmp, f);
            piranha::msgpack_pack(packer, m_flavour, f);
        }
    }
    /// Deserialize from msgpack object.
    /**
     * \note
     * This method is activated only if the \p T, \p bool
     * and piranha::real_trigonometric_kronecker_monomial::v_type satisfy piranha::has_msgpack_convert.
     *
     * This method will deserialize \p o into \p this. In binary mode, no check is performed on the content of \p o,
     * and calling this method will result in undefined behaviour if \p o does not contain a monomial serialized via
     * msgpack_pack(). This method provides the basic exception safety guarantee.
     *
     * @param[in] o msgpack object that will be deserialized.
     * @param[in] f serialization format.
     * @param[in] s reference arguments set.
     *
     * @throws std::invalid_argument if the size of the deserialized array differs from the size of \p s.
     * @throws unspecified any exception thrown by:
     * - the constructor of piranha::kronecker_monomial from a container,
     * - the public interface of <tt>msgpack::object</tt>,
     * - piranha::msgpack_convert().
     */
    template <typename U = real_trigonometric_kronecker_monomial, msgpack_convert_enabler<U> = 0>
    void msgpack_convert(const msgpack::object &o, msgpack_format f, const symbol_set &s)
    {
        std::array<msgpack::object, 2> tmp;
        o.convert(tmp);
        if (f == msgpack_format::binary) {
            piranha::msgpack_convert(m_value, tmp[0], f);
            piranha::msgpack_convert(m_flavour, tmp[1], f);
        } else {
            v_type tmp_v;
            piranha::msgpack_convert(tmp_v, tmp[0], f);
            if (unlikely(tmp_v.size() != s.size())) {
                piranha_throw(std::invalid_argument,
                              "incompatible symbol set in trigonometric monomial serialization: the reference "
                              "symbol set has a size of "
                                  + std::to_string(s.size())
                                  + ", while the trigonometric monomial being deserialized has a size of "
                                  + std::to_string(tmp_v.size()));
            }
            *this = real_trigonometric_kronecker_monomial(tmp_v.begin(), tmp_v.end());
            piranha::msgpack_convert(m_flavour, tmp[1], f);
        }
    }
#endif

private:
    value_type m_value;
    bool m_flavour;
};

template <typename T>
const std::size_t real_trigonometric_kronecker_monomial<T>::multiply_arity;

/// Alias for piranha::real_trigonometric_kronecker_monomial with default type.
using rtk_monomial = real_trigonometric_kronecker_monomial<>;
}

namespace std
{

/// Specialisation of \p std::hash for piranha::real_trigonometric_kronecker_monomial.
template <typename T>
struct hash<piranha::real_trigonometric_kronecker_monomial<T>> {
    /// Result type.
    typedef size_t result_type;
    /// Argument type.
    typedef piranha::real_trigonometric_kronecker_monomial<T> argument_type;
    /// Hash operator.
    /**
     * @param[in] a argument whose hash value will be computed.
     *
     * @return hash value of \p a computed via piranha::real_trigonometric_kronecker_monomial::hash().
     */
    result_type operator()(const argument_type &a) const
    {
        return a.hash();
    }
};
}

#endif
