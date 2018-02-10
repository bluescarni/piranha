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

#ifndef PIRANHA_REAL_TRIGONOMETRIC_KRONECKER_MONOMIAL_HPP
#define PIRANHA_REAL_TRIGONOMETRIC_KRONECKER_MONOMIAL_HPP

#include <algorithm>
#include <array>
#include <cinttypes>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
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

#include <piranha/config.hpp>
#include <piranha/detail/cf_mult_impl.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/detail/km_commons.hpp>
#include <piranha/detail/prepare_for_print.hpp>
#include <piranha/detail/safe_integral_adder.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/integer.hpp>
#include <piranha/is_cf.hpp>
#include <piranha/is_key.hpp>
#include <piranha/kronecker_array.hpp>
#include <piranha/math.hpp>
#include <piranha/math/binomial.hpp>
#include <piranha/math/cos.hpp>
#include <piranha/math/sin.hpp>
#include <piranha/s11n.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/static_vector.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/term.hpp>
#include <piranha/type_traits.hpp>

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
 * degree, defined as if the multipliers were exponents of a regular monomial (e.g., the total trigonometric degree is
 * the sum of the multipliers). Closely related is the concept of trigonometric order, calculated by adding the absolute
 * values of the multipliers.
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
 */
// NOTES:
// - it might make sense, for canonicalisation and is_compatible(), to provide a method in kronecker_array to get only
//   the first element of the array. This should be quite fast, and it will provide enough information for the
//   canon/compatibility.
// - related to the above: we can embed the flavour as the first element of the kronecker array - at that point checking
//   the flavour is just determining if the int value is even or odd.
// - need to do some reasoning on the impact of the codification in a specialised fast poisson series multiplier: how do
//   we deal with the canonical form without going through code/decode? if we require the last multiplier to be always
//   positive (instead of the first), can we just check/flip the sign of the coded value?
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
    static_assert(max_size <= std::numeric_limits<static_vector<int, 1u>::size_type>::max(), "Invalid max size.");

public:
    /// Default constructor.
    /**
     * After construction all multipliers in the monomial will be zero, and the flavour will be set to \p true.
     */
    real_trigonometric_kronecker_monomial() : m_value(0), m_flavour(true) {}
    /// Defaulted copy constructor.
    real_trigonometric_kronecker_monomial(const real_trigonometric_kronecker_monomial &) = default;
    /// Defaulted move constructor.
    real_trigonometric_kronecker_monomial(real_trigonometric_kronecker_monomial &&) = default;

private:
    // Enabler for ctor from init list.
    template <typename U>
    using init_list_enabler = enable_if_t<has_safe_cast<value_type, U>::value, int>;

public:
    /// Constructor from initalizer list.
    /**
     * \note
     * This constructor is enabled if \p U is safely convertible to \p T.
     *
     * The values in the initializer list are intended to represent the multipliers of the monomial:
     * they will be safely converted to type \p T (if \p T and \p U are not the same type),
     * encoded using piranha::kronecker_array::encode() and the result assigned to the internal integer instance.
     * The flavour will be set to ``flavour``.
     *
     * @param list an initializer list representing the multipliers.
     * @param flavour the flavour that will be assigned to the monomial.
     *
     * @throws unspecified any exception thrown by:
     * - piranha::kronecker_array::encode(),
     * - piranha::safe_cast(),
     * - piranha::static_vector::push_back().
     */
    template <typename U, init_list_enabler<U> = 0>
    explicit real_trigonometric_kronecker_monomial(std::initializer_list<U> list, bool flavour = true)
        : real_trigonometric_kronecker_monomial(std::begin(list), std::end(list), flavour)
    {
    }

private:
    // Enabler for ctor from iterator.
    template <typename Iterator>
    using it_ctor_enabler
        = enable_if_t<conjunction<is_input_iterator<Iterator>,
                                  has_safe_cast<value_type, decltype(*std::declval<const Iterator &>())>>::value,
                      int>;

public:
    /// Constructor from range.
    /**
     * \note
     * This constructor is enabled if \p Iterator is an input iterator whose \p value_type is safely convertible
     * to \p T.
     *
     * Will build internally a vector of values from the input iterators, encode it and assign the result
     * to the internal integer instance. The value type of the iterator is converted to \p T using
     * piranha::safe_cast(). The flavour will be set to ``flavour``.
     *
     * @param start beginning of the range.
     * @param end end of the range.
     * @param flavour the flavour that will be assigned to the monomial.
     *
     * @throws unspecified any exception thrown by:
     * - piranha::kronecker_array::encode(),
     * - piranha::safe_cast(),
     * - piranha::static_vector::push_back().
     */
    template <typename Iterator, it_ctor_enabler<Iterator> = 0>
    explicit real_trigonometric_kronecker_monomial(const Iterator &start, const Iterator &end, bool flavour = true)
        : m_value(0), m_flavour(flavour)
    {
        v_type tmp;
        std::transform(
            start, end, std::back_inserter(tmp),
            [](const typename std::iterator_traits<Iterator>::value_type &v) { return safe_cast<value_type>(v); });
        m_value = ka::encode(tmp);
    }
    /// Constructor from set of symbols.
    /**
     * After construction all multipliers will be zero and the flavour will be set to \p true.
     */
    explicit real_trigonometric_kronecker_monomial(const symbol_fset &) : real_trigonometric_kronecker_monomial() {}
    /// Constructor from \p value_type and flavour.
    /**
     * This constructor will initialise the internal integer instance
     * to \p n and the flavour to \p f.
     *
     * @param n initializer for the internal integer instance.
     * @param f desired flavour.
     */
    explicit real_trigonometric_kronecker_monomial(const value_type &n, bool f) : m_value(n), m_flavour(f) {}
    /// Converting constructor.
    /**
     * This constructor is for use when converting from one term type to another in piranha::series. It will
     * set the internal integer instance and flavour to the same value of \p other.
     *
     * @param other the construction argument.
     */
    explicit real_trigonometric_kronecker_monomial(const real_trigonometric_kronecker_monomial &other,
                                                   const symbol_fset &)
        : real_trigonometric_kronecker_monomial(other)
    {
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
    /// Copy assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     */
    real_trigonometric_kronecker_monomial &operator=(const real_trigonometric_kronecker_monomial &other) = default;
    /// Move assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     */
    real_trigonometric_kronecker_monomial &operator=(real_trigonometric_kronecker_monomial &&other) = default;
    /// Set the internal integer instance.
    /**
     * @param n value to which the internal integer instance will be set.
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
     * @param f value to which the flavour will be set.
     */
    void set_flavour(bool f)
    {
        m_flavour = f;
    }

private:
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

public:
    /// Canonicalise.
    /**
     * A monomial is considered to be in canonical form when the first nonzero multiplier is positive.
     * If \p this is not in canonical form, the method will canonicalise \p this by switching
     * the signs of all multipliers and return \p true.
     * Otherwise, \p this will not be modified and \p false will be returned.
     *
     * @param args the reference piranha::symbol_fset.
     *
     * @return \p true if the monomial was canonicalised, \p false otherwise.
     *
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - piranha::kronecker_array::encode().
     */
    bool canonicalise(const symbol_fset &args)
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
     *   piranha::kronecker_array::get_limits(),
     * - the internal integer is not within the limits reported by piranha::kronecker_array::get_limits(),
     * - the first nonzero element of the vector of multipliers represented by the internal integer is negative.
     *
     * Otherwise, the monomial is considered to be compatible for insertion.
     *
     * @param args the reference piranha::symbol_fset.
     *
     * @return compatibility flag for the monomial.
     */
    bool is_compatible(const symbol_fset &args) const
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
    /// Zero check.
    /**
     * A trigonometric monomial is zero if all multipliers are zero and the flavour is \p false.
     *
     * @return ignorability flag.
     */
    bool is_zero(const symbol_fset &) const
    {
        return m_value == value_type(0) && !m_flavour;
    }
    /// Merge symbols.
    /**
     * This method will return a copy of \p this in which the value 0 has been inserted
     * at the positions specified by \p ins_map. Specifically, before each index appearing in \p ins_map
     * a number of zeroes equal to the size of the mapped piranha::symbol_fset will be inserted.
     *
     * For instance, given a piranha::real_trigonometric_kronecker_monomial containing the values <tt>[1,2,3,4]</tt>, a
     * symbol set \p args containing <tt>["c","e","g","h"]</tt> and an insertion map \p ins_map containing the pairs
     * <tt>[(0,["a","b"]),(1,["d"]),(2,["f"]),(4,["i"])]</tt>, the output of this method will be
     * <tt>[0,0,1,0,2,0,3,4,0]</tt>. That is, the symbols appearing in \p ins_map are merged into \p this
     * with a value of zero at the specified positions. The original flavour of ``this`` will be preserved
     * in the return value.
     *
     * @param ins_map the insertion map.
     * @param args the reference symbol set for \p this.
     *
     * @return a piranha::real_trigonometric_kronecker_monomial resulting from inserting into \p this zeroes at the
     * positions specified by \p ins_map.
     *
     * @throws std::invalid_argument in the following cases:
     * - the size of \p ins_map is zero,
     * - the last index in \p ins_map is greater than the size of \p args.
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - piranha::static_vector::push_back(),
     * - piranha::kronecker_array::encode().
     */
    real_trigonometric_kronecker_monomial merge_symbols(const symbol_idx_fmap<symbol_fset> &ins_map,
                                                        const symbol_fset &args) const
    {
        return real_trigonometric_kronecker_monomial(detail::km_merge_symbols<v_type, ka>(ins_map, args, m_value),
                                                     m_flavour);
    }
    /// Check if monomial is unitary.
    /**
     * @return \p true if all the multipliers are zero and the flavour is \p true, \p false otherwise.
     */
    bool is_unitary(const symbol_fset &) const
    {
        return (!m_value && m_flavour);
    }

private:
    // Degree utils.
    using degree_type = add_t<T, T>;

public:
    /// Trigonometric degree.
    /**
     * The type returned by this method is the type resulting from the addition of two instances
     * of \p T.
     *
     * @param args the reference symbol set for \p this.
     *
     * @return the trigonometric degree of the monomial.
     *
     * @throws std::overflow_error if the computation of the degree overflows.
     * @throws unspecified any exception thrown by unpack().
     */
    degree_type t_degree(const symbol_fset &args) const
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
    /**
     * @param args the reference symbol set for \p this.
     *
     * @return the output of t_degree(const symbol_fset &) const.
     *
     * @throws unspecified any exception thrown by t_degree(const symbol_fset &) const.
     */
    degree_type t_ldegree(const symbol_fset &args) const
    {
        return t_degree(args);
    }
    /// Partial trigonometric degree.
    /**
     * Partial trigonometric degree of the monomial: only the symbols at the positions specified by \p p are considered.
     * The type returned by this method is the type resulting from the addition of two instances
     * of \p T.
     *
     * @param p the positions of the symbols to be considered in the calculation of the degree.
     * @param args the reference piranha::symbol_fset.
     *
     * @return the summation of the multipliers of the monomial at the positions specified by \p p.
     *
     * @throws std::invalid_argument if the last element of \p p, if existing, is not less than the size
     * of \p args.
     * @throws std::overflow_error if the computation of the degree overflows.
     * @throws unspecified any exception thrown by unpack().
     */
    degree_type t_degree(const symbol_idx_fset &p, const symbol_fset &args) const
    {
        const auto tmp = unpack(args);
        piranha_assert(tmp.size() == args.size());
        if (unlikely(p.size() && *p.rbegin() >= tmp.size())) {
            piranha_throw(std::invalid_argument,
                          "the largest value in the positions set for the computation of the "
                          "partial trigonometric degree of a real trigonometric Kronecker monomial is "
                              + std::to_string(*p.rbegin()) + ", but the monomial has a size of only "
                              + std::to_string(tmp.size()));
        }
        degree_type retval(0);
        for (auto idx : p) {
            detail::safe_integral_adder(retval, static_cast<degree_type>(tmp[static_cast<decltype(tmp.size())>(idx)]));
        }
        return retval;
    }
    /// Partial low trigonometric degree (equivalent to the partial trigonometric degree).
    /**
     * @param p the positions of the symbols to be considered in the calculation of the degree.
     * @param args the reference piranha::symbol_fset.
     *
     * @return the output of t_degree(const symbol_idx_fset &, const symbol_fset &) const.
     *
     * @throws unspecified any exception thrown by t_degree(const symbol_idx_fset &, const symbol_fset &) const.
     */
    degree_type t_ldegree(const symbol_idx_fset &p, const symbol_fset &args) const
    {
        return t_degree(p, args);
    }

private:
    // Order utils.
    using order_type = decltype(math::abs(std::declval<const T &>()) + math::abs(std::declval<const T &>()));

public:
    /// Trigonometric order.
    /**
     * The type returned by this method is the type resulting from the addition of the absolute
     * values of two instances of \p T.
     *
     * @param args the reference piranha::symbol_fset.
     *
     * @return the trigonometric order of the monomial.
     *
     * @throws std::overflow_error if the computation of the order overflows.
     * @throws unspecified any exception thrown by unpack().
     */
    order_type t_order(const symbol_fset &args) const
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
    /**
     * @param args the reference piranha::symbol_fset.
     *
     * @return the output of t_order(const symbol_fset &) const.
     *
     * @throws unspecified any exception thrown by t_order(const symbol_fset &) const.
     */
    order_type t_lorder(const symbol_fset &args) const
    {
        return t_order(args);
    }
    /// Partial trigonometric order.
    /**
     * Partial trigonometric order of the monomial: only the symbols at the positions specified by \p p are considered.
     * The type returned by this method is the type resulting from the addition of the absolute values of two instances
     * of \p T.
     *
     * @param p the positions of the symbols to be considered in the calculation of the order.
     * @param args the reference piranha::symbol_fset.
     *
     * @return the partial trigonometric order of the monomial.
     *
     * @throws std::invalid_argument if the last element of \p p, if existing, is not less than the size
     * of \p args.
     * @throws std::overflow_error if the computation of the degree overflows.
     * @throws unspecified any exception thrown by unpack().
     */
    order_type t_order(const symbol_idx_fset &p, const symbol_fset &args) const
    {
        const auto tmp = unpack(args);
        piranha_assert(tmp.size() == args.size());
        if (unlikely(p.size() && *p.rbegin() >= tmp.size())) {
            piranha_throw(std::invalid_argument,
                          "the largest value in the positions set for the computation of the "
                          "partial trigonometric order of a real trigonometric Kronecker monomial is "
                              + std::to_string(*p.rbegin()) + ", but the monomial has a size of only "
                              + std::to_string(tmp.size()));
        }
        order_type retval(0);
        for (auto idx : p) {
            detail::safe_integral_adder(
                retval, static_cast<degree_type>(math::abs(tmp[static_cast<decltype(tmp.size())>(idx)])));
        }
        return retval;
    }
    /// Partial low trigonometric order (equivalent to the partial trigonometric order).
    /**
     * @param p positions of the symbols to be considered.
     * @param args the reference piranha::symbol_fset.
     *
     * @return the output of t_order(const symbol_idx_fset &, const symbol_fset &) const.
     *
     * @throws unspecified any exception thrown by t_order(const symbol_idx_fset &, const symbol_fset &) const.
     */
    order_type t_lorder(const symbol_idx_fset &p, const symbol_fset &args) const
    {
        return t_order(p, args);
    }

private:
    // Enabler for multiplication.
    template <typename Cf>
    using multiply_enabler = enable_if_t<
        conjunction<is_divisible_in_place<Cf, int>, has_negate<Cf>, std::is_copy_assignable<Cf>, has_mul3<Cf>>::value,
        int>;

public:
    /// Multiply terms with a trigonometric monomial.
    /**
     * \note
     * This method is enabled only if the following conditions hold:
     * - \p Cf satisfies piranha::has_mul3,
     * - \p Cf is divisible in-place by \p int,
     * - \p Cf is copy-assignable and it satisfies piranha::has_negate.
     *
     * This method will compute the result of the multiplication of the two terms \p t1 and \p t2 with trigonometric
     * key. The result is stored in the two terms of \p res and it is computed using basic trigonometric formulae.
     * Note however that this method will **not** perform the division by two implied by Werner's formulae. Also, in
     * case \p Cf is an mp++ rational, only the numerators of the coefficients will be multiplied.
     *
     * @param res result of the multiplication.
     * @param t1 first argument.
     * @param t2 second argument.
     * @param args the reference piranha::symbol_fset.
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
                         const term<Cf, real_trigonometric_kronecker_monomial> &t2, const symbol_fset &args)
    {
        // Coefficients first.
        cf_mult_impl(res[0u].m_cf, t1.m_cf, t2.m_cf);
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
     * @param other the comparison argument.
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
     * @param other the comparison argument.
     *
     * @return the opposite of operator==().
     */
    bool operator!=(const real_trigonometric_kronecker_monomial &other) const
    {
        return (m_value != other.m_value || m_flavour != other.m_flavour);
    }
    /// Unpack internal integer instance.
    /**
     * This method will decode the internal integral instance into a piranha::static_vector of size equal to the size of
     * \p args.
     *
     * @param args the reference piranha::symbol_fset.
     *
     * @return a piranha::static_vector containing the result of decoding the internal integral instance via
     * piranha::kronecker_array.
     *
     * @throws std::invalid_argument if the size of \p args is larger than the maximum size of piranha::static_vector.
     * @throws unspecified any exception thrown by piranha::kronecker_array::decode().
     */
    v_type unpack(const symbol_fset &args) const
    {
        return detail::km_unpack<v_type, ka>(args, m_value);
    }
    /// Print.
    /**
     * Thie method will print to stream a human-readable representation of the monomial.
     *
     * @param os target stream.
     * @param args the reference piranha::symbol_fset.
     *
     * @throws unspecified any exception thrown by unpack() or by streaming instances of \p value_type.
     */
    void print(std::ostream &os, const symbol_fset &args) const
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
        auto it_args = args.begin();
        for (decltype(tmp.size()) i = 0u; i < tmp.size(); ++i, ++it_args) {
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
                os << *it_args;
                empty_output = false;
            }
        }
        os << ")";
    }
    /// Print in TeX mode.
    /**
     * This method will print to stream a TeX representation of the monomial.
     *
     * @param os target stream.
     * @param args the reference piranha::symbol_fset.
     *
     * @throws unspecified any exception thrown by unpack() or by streaming instances of \p value_type.
     */
    void print_tex(std::ostream &os, const symbol_fset &args) const
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
        auto it_args = args.begin();
        for (decltype(tmp.size()) i = 0u; i < tmp.size(); ++i, ++it_args) {
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
                os << '{' << *it_args << '}';
                empty_output = false;
            }
        }
        os << "\\right)}";
    }
    /// Partial derivative.
    /**
     * This method will return the partial derivative of \p this with respect to the symbol at the position indicated by
     * \p p. The result is a pair consisting of the multiplier at the position \p p and a copy of the monomial.
     * The sign of the multiplier and the flavour of the resulting monomial are set according to the standard
     * differentiation formulas for elementary trigonometric functions.
     * If \p p is not smaller than the size of \p args or if its corresponding multiplier is
     * zero, the returned pair will be <tt>(0,real_trigonometric_kronecker_monomial{args})</tt>.
     *
     * @param p the position of the symbol with respect to which the differentiation will be calculated.
     * @param args the reference piranha::symbol_fset.
     *
     * @return the result of the differentiation.
     *
     * @throws unspecified any exception thrown by unpack().
     */
    std::pair<T, real_trigonometric_kronecker_monomial> partial(const symbol_idx &p, const symbol_fset &args) const
    {
        auto v = unpack(args);
        if (p >= args.size() || v[static_cast<decltype(v.size())>(p)] == T(0)) {
            // Derivative wrt a variable not in the monomial: the position is outside the bounds, or it refers to a
            // variable with zero multiplier.
            return std::make_pair(T(0), real_trigonometric_kronecker_monomial{args});
        }
        const auto v_b = v.begin();
        if (get_flavour()) {
            // cos(nx + b) -> -n*sin(nx + b)
            return std::make_pair(static_cast<T>(-v_b[p]), real_trigonometric_kronecker_monomial(m_value, false));
        }
        // sin(nx + b) -> n*cos(nx + b)
        return std::make_pair(v_b[p], real_trigonometric_kronecker_monomial(m_value, true));
    }
    /// Integration.
    /**
     * This method will return the antiderivative of \p this with respect to the symbol \p s. The result is a pair
     * consisting of the multiplier associated to \p s and a copy of the monomial.
     * The sign of the multiplier and the flavour of the resulting monomial are set according to the standard
     * integration formulas for elementary trigonometric functions.
     * If \p s is not in \p args or if the multiplier associated to it is zero,
     * the returned pair will be <tt>(0,real_trigonometric_kronecker_monomial{args})</tt>.
     *
     * @param s the symbol with respect to which the integration will be calculated.
     * @param args the reference piranha::symbol_fset.
     *
     * @return the result of the integration.
     *
     * @throws unspecified any exception thrown by unpack().
     */
    std::pair<T, real_trigonometric_kronecker_monomial> integrate(const std::string &s, const symbol_fset &args) const
    {
        auto v = unpack(args);
        auto it_args = args.begin();
        for (min_int<decltype(args.size()), typename v_type::size_type> i = 0u; i < args.size(); ++i, ++it_args) {
            if (*it_args == s && v[i] != value_type(0)) {
                if (get_flavour()) {
                    // cos(nx + b) -> sin(nx + b)
                    return std::make_pair(v[i], real_trigonometric_kronecker_monomial(m_value, false));
                }
                // sin(nx + b) -> -cos(nx + b)
                return std::make_pair(static_cast<value_type>(-v[i]),
                                      real_trigonometric_kronecker_monomial(m_value, true));
            }
            if (*it_args > s) {
                // The current symbol in args is lexicographically larger than s,
                // we won't find s any more in args.
                break;
            }
        }
        return std::make_pair(value_type(0), real_trigonometric_kronecker_monomial{args});
    }

private:
    // The candidate type, resulting from math::cos()/math::sin() on T * U.
    template <typename U>
    using eval_t_cos = decltype(math::cos(std::declval<const mul_t<T, U> &>()));
    template <typename U>
    using eval_t_sin = decltype(math::sin(std::declval<const mul_t<T, U> &>()));
    // Definition of the evaluation type.
    template <typename U>
    using eval_type
        = enable_if_t<conjunction<
                          // sin/cos eval types must be the same.
                          std::is_same<eval_t_cos<U>, eval_t_sin<U>>,
                          // Eval type must be ctible form int.
                          std::is_constructible<eval_t_cos<U>, const int &>,
                          // Eval type must be returnable.
                          is_returnable<eval_t_cos<U>>,
                          // T * U must be addable in-place.
                          is_addable_in_place<mul_t<T, U>>,
                          // T * U must be ctible from int.
                          std::is_constructible<mul_t<T, U>, const int &>,
                          // T * U must be move ctible and dtible
                          std::is_move_constructible<mul_t<T, U>>, std::is_destructible<mul_t<T, U>>>::value,
                      eval_t_cos<U>>;

public:
    /// Evaluation.
    /**
     * \note
     * This template method is activated only if \p U supports the mathematical operations needed to compute
     * the return type.
     *
     * The return value will be built by applying piranha::math::cos() or piranha::math:sin()
     * to the linear combination of the values in ``values`` with the multipliers.
     *
     * @param values the values will be used for the evaluation.
     * @param args the reference piranha::symbol_fset.
     *
     * @return the result of evaluating \p this with the values provided in \p values.
     *
     * @throws std::invalid_argument if the sizes of \p values and \p args differ.
     * @throws unspecified any exception thrown by unpack() or by the computation of the return type.
     */
    template <typename U>
    eval_type<U> evaluate(const std::vector<U> &values, const symbol_fset &args) const
    {
        // NOTE: here we can check the values size only against args.
        if (unlikely(values.size() != args.size())) {
            piranha_throw(std::invalid_argument, "invalid vector of values for real trigonometric Kronecker monomial "
                                                 "evaluation: the size of the vector of values ("
                                                     + std::to_string(values.size())
                                                     + ") differs from the size of the reference set of symbols ("
                                                     + std::to_string(args.size()) + ")");
        }
        // Run the unpack before the checks below in order to check the suitability of args.
        const auto v = unpack(args);
        // Special casing if the monomial is empty, just return 0 or 1.
        if (!args.size()) {
            if (get_flavour()) {
                return eval_type<U>(1);
            }
            return eval_type<U>(0);
        }
#if defined(PIRANHA_COMPILER_IS_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
        // Init the accumulator with the first element of the linear
        // combination.
        auto tmp(v[0] * values[0]);
#if defined(PIRANHA_COMPILER_IS_GCC)
#pragma GCC diagnostic pop
#endif
        // Accumulate the sin/cos argument.
        for (decltype(values.size()) i = 1; i < values.size(); ++i) {
#if defined(PIRANHA_COMPILER_IS_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
            // NOTE: this could be optimised with an FMA eventually.
            tmp += v[static_cast<decltype(v.size())>(i)] * values[i];
#if defined(PIRANHA_COMPILER_IS_GCC)
#pragma GCC diagnostic pop
#endif
        }
        if (get_flavour()) {
            // NOTE: move it, in the future math::cos() may exploit this.
            return math::cos(std::move(tmp));
        }
        return math::sin(std::move(tmp));
    }

private:
    // Substitution utils.
    // NOTE: the only additional requirement here is to be able to negate.
    template <typename U>
    using subs_type = enable_if_t<has_negate<eval_type<U>>::value, eval_type<U>>;

public:
    /// Substitution.
    /**
     * \note
     * This template method is activated only if \p U supports the mathematical operations needed to compute
     * the return type.
     *
     * This method will substitute the symbols at the positions specified in the keys of ``smap`` with the mapped
     * values. The return value is a vector of two pairs computed according to the standard angle sum identities. That
     * is, given a monomial of the form
     * \f[
     * \begin{array}{c}
     * \sin \\
     * \cos
     * \end{array}
     * \left(na + mb + c\right)
     * \f]
     * in which the symbols \f$ a \f$ and \f$ b \f$ are to be substituted with \f$ x \f$ and \f$ y \f$, the result of
     * the substitution will be one of
     * \f[
     * \left\{\left[\sin \left(nx+my\right),\cos c \right],\left[\cos \left(nx+my\right),\sin c \right]\right\}
     * \f]
     * or
     * \f[
     * \left\{\left[\cos \left(nx+my\right),\cos c \right],\left[-\sin \left(nx+my\right),\sin c \right]\right\}
     * \f]
     * where \f$ \cos c \f$ and \f$ \sin c \f$ are returned as monomials, and \f$ \cos \left(nx+my\right) \f$ and \f$
     * \sin \left(nx+my\right) \f$ are computed via piranha::math::cos() and piranha::math::sin(). If \p s is not
     * in \p args, \f$ \cos \left(nx+my\right) \f$ will be initialised to 1 and \f$ \sin \left(nx+my\right) \f$ to 0.
     * If, after the substitution, the first nonzero multiplier in \f$ c \f$ is negative, \f$ c \f$ will be negated and
     * the other signs changed accordingly.
     *
     * @param smap the map relating the positions of the symbols to be substituted to the values
     * they will be substituted with.
     * @param args the reference piranha::symbol_fset.
     *
     * @return the result of the substitution.
     *
     * @throws std::invalid_argument if the last element of the substitution map is not smaller
     * than the size of ``args``.
     * @throws unspecified any exception thrown by unpack(), the computation of the return value or
     * memory errors in standard containers.
     */
    template <typename U>
    std::vector<std::pair<subs_type<U>, real_trigonometric_kronecker_monomial>> subs(const symbol_idx_fmap<U> &smap,
                                                                                     const symbol_fset &args) const
    {
        if (unlikely(smap.size() && smap.rbegin()->first >= args.size())) {
            // The last element of the substitution map must be a valid index into args.
            piranha_throw(std::invalid_argument, "invalid argument(s) for substitution in a real trigonometric "
                                                 "Kronecker monomial: the last index of the substitution map ("
                                                     + std::to_string(smap.rbegin()->first)
                                                     + ") must be smaller than the monomial's size ("
                                                     + std::to_string(args.size()) + ")");
        }
        std::vector<std::pair<subs_type<U>, real_trigonometric_kronecker_monomial>> retval;
        retval.reserve(2u);
        const auto f = get_flavour();
        if (smap.size()) {
            // The substitution map contains something, proceed to the substitution.
            auto v = unpack(args);
            // Init a tmp value from the linear combination of the first value
            // of the map with the first multiplier.
            auto it = smap.begin();
#if defined(PIRANHA_COMPILER_IS_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
            auto tmp(v[static_cast<decltype(v.size())>(it->first)] * it->second);
#if defined(PIRANHA_COMPILER_IS_GCC)
#pragma GCC diagnostic pop
#endif
            // Zero out the corresponding multiplier.
            v[static_cast<decltype(v.size())>(it->first)] = T(0);
            // Finish computing the linear combination of the values with
            // the corresponding multipliers.
            // NOTE: move to the next element in the init statement of the for loop.
            for (++it; it != smap.end(); ++it) {
#if defined(PIRANHA_COMPILER_IS_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
                // NOTE: FMA in the future, maybe.
                tmp += v[static_cast<decltype(v.size())>(it->first)] * it->second;
#if defined(PIRANHA_COMPILER_IS_GCC)
#pragma GCC diagnostic pop
#endif
                v[static_cast<decltype(v.size())>(it->first)] = T(0);
            }
            // Check if we need to canonicalise the vector, after zeroing out
            // one or more multipliers.
            const bool sign_changed = canonicalise_impl(v);
            // Encode the modified vector of multipliers.
            const auto new_value = ka::encode(v);
            // Pre-compute the sin/cos of tmp.
            auto s_tmp(math::sin(tmp)), c_tmp(math::cos(tmp));
            if (f) {
                // cos(tmp+x) -> cos(tmp)*cos(x) - sin(tmp)*sin(x)
                retval.emplace_back(std::move(c_tmp), real_trigonometric_kronecker_monomial(new_value, true));
                if (!sign_changed) {
                    // NOTE: we need to negate because of trig formulas, but if
                    // we switched the sign of x we have a double negation.
                    math::negate(s_tmp);
                }
                retval.emplace_back(std::move(s_tmp), real_trigonometric_kronecker_monomial(new_value, false));
            } else {
                // sin(tmp+x) -> sin(tmp)*cos(x) + cos(tmp)*sin(x)
                retval.emplace_back(std::move(s_tmp), real_trigonometric_kronecker_monomial(new_value, true));
                if (sign_changed) {
                    // NOTE: opposite of the above, if we switched the signs in x we need to negate
                    // cos(tmp) to restore the signs.
                    math::negate(c_tmp);
                }
                retval.emplace_back(std::move(c_tmp), real_trigonometric_kronecker_monomial(new_value, false));
            }
        } else {
            // The subs map is empty, return the original values.
            // NOTE: can we just return a 1-element vector here?
            if (f) {
                // cos(a) -> 1*cos(a) + 0*sin(a)
                retval.emplace_back(subs_type<U>(1), *this);
                retval.emplace_back(subs_type<U>(0), real_trigonometric_kronecker_monomial(m_value, false));
            } else {
                // sin(a) -> 0*cos(a) + 1*sin(a)
                retval.emplace_back(subs_type<U>(0), real_trigonometric_kronecker_monomial(m_value, true));
                retval.emplace_back(subs_type<U>(1), *this);
            }
        }
        return retval;
    }

private:
    // Trig subs bits.
    template <typename U>
    using t_subs_t = decltype(std::declval<const integer &>() * std::declval<const mul_t<U, U> &>());
    template <typename U>
    using t_subs_type = enable_if_t<
        conjunction<std::is_default_constructible<U>, std::is_constructible<U, const int &>, std::is_move_assignable<U>,
                    std::is_destructible<U>, std::is_assignable<U &, mul_t<U, U> &&>, std::is_destructible<mul_t<U, U>>,
                    std::is_move_constructible<t_subs_t<U>>, std::is_destructible<t_subs_t<U>>,
                    is_addable_in_place<t_subs_t<U>>, has_negate<t_subs_t<U>>, std::is_copy_constructible<t_subs_t<U>>,
                    std::is_move_constructible<mul_t<U, U>>>::value,
        t_subs_t<U>>;
    // Couple of helper functions for Vieta's formulae.
    static value_type cos_phase(const value_type &n)
    {
        piranha_assert(n >= value_type(0));
        constexpr value_type v[4] = {1, 0, -1, 0};
        return v[n % value_type(4)];
    }
    static value_type sin_phase(const value_type &n)
    {
        piranha_assert(n >= value_type(0));
        constexpr value_type v[4] = {0, 1, 0, -1};
        return v[n % value_type(4)];
    }

public:
    /// Trigonometric substitution.
    /**
     * \note
     * This method is enabled only if \p U supports the operations needed to compute the result.
     *
     * This method will substitute the cosine and sine of the symbol at the position ``idx`` with,
     * respectively, ``c`` and ``s``. The substitution is performed using standard trigonometric formulae, and it will
     * result in a list of two ``(substitution result, new monomial)`` pairs. If ``idx`` is not smaller
     * than the size of ``this``, the result of the substitution will be the original monomial unchanged.
     *
     * @param idx the index of the symbol whose cosine and sine will be substituted.
     * @param c the quantity that will replace the cosine of the symbol at the position ``idx``.
     * @param s the quantity that will replace the sine of the symbol at the position ``idx``.
     * @param args the reference piranha::symbol_fset.
     *
     * @return the result of the substitution.
     *
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - construction, assignment and arithmetics on the return value and on the intermediary
     *   values invovled in the computation,
     * - piranha::kronecker_array::encode().
     */
    template <typename U>
    std::vector<std::pair<t_subs_type<U>, real_trigonometric_kronecker_monomial>>
    t_subs(const symbol_idx &idx, const U &c, const U &s, const symbol_fset &args) const
    {
        auto v = unpack(args);
        T n(0);
        if (idx < args.size()) {
            // NOTE: this will extract the affected multiplier
            // into n and zero out the multiplier in v.
            std::swap(n, v[static_cast<decltype(v.size())>(idx)]);
        }
        const auto abs_n = static_cast<T>(std::abs(n));
        // Prepare the powers of c and s to be used in the multiple angles formulae.
        // NOTE: probably a vector would be better here.
        std::unordered_map<T, U> c_map, s_map;
        // TREQ: U def ctible, ctible from int, move-assignable and dtible.
        c_map[0] = U(1);
        s_map[0] = U(1);
        for (T k = 0; k < abs_n; ++k) {
            // TREQ: U assignable from mult_t<U,U> &&, mult_t<U,U> dtible.
            c_map[static_cast<T>(k + 1)] = c_map[k] * c;
            s_map[static_cast<T>(k + 1)] = s_map[k] * s;
        }
        // Init with the first element in the summation.
        // NOTE: we promote abs_n to integer in these formulae in order to force
        // the use of the binomial() overload for integer. In the future we might want
        // to disable the binomial() overload for integral C++ types.
        // TREQ: t_subs_type<U> move-ctible, dtible.
        t_subs_type<U> cos_nx(cos_phase(abs_n) * piranha::binomial(integer(abs_n), T(0))
                              * (c_map[T(0)] * s_map[abs_n])),
            sin_nx(sin_phase(abs_n) * piranha::binomial(integer(abs_n), T(0)) * (c_map[T(0)] * s_map[abs_n]));
        // Run the main iteration.
        for (T k = 0; k < abs_n; ++k) {
            const auto p = static_cast<T>(abs_n - (k + 1));
            piranha_assert(p >= T(0));
            // TREQ: mult_t<U,U> move ctible.
            const auto tmp = c_map[static_cast<T>(k + 1)] * s_map[p];
            const auto tmp_bin = piranha::binomial(integer(abs_n), k + T(1));
            // TREQ: t_subs_type<U> addable in-place.
            cos_nx += cos_phase(p) * tmp_bin * tmp;
            sin_nx += sin_phase(p) * tmp_bin * tmp;
        }
        // Change sign as necessary.
        if (abs_n != n) {
            // TREQ: t_subs_type<U> has_negate.
            math::negate(sin_nx);
        }
        // Buld the new keys and canonicalise as needed.
        const bool sign_changed = canonicalise_impl(v);
        // Encode the modified vector.
        const auto new_value = ka::encode(v);
        // Init the retval.
        std::vector<std::pair<t_subs_type<U>, real_trigonometric_kronecker_monomial>> retval;
        retval.reserve(2);
        if (get_flavour()) {
            retval.emplace_back(std::move(cos_nx), real_trigonometric_kronecker_monomial(new_value, true));
            retval.emplace_back(std::move(sin_nx), real_trigonometric_kronecker_monomial(new_value, false));
            // Need to flip the sign on the sin * sin product if sign was not changed.
            if (!sign_changed) {
                math::negate(retval[1u].first);
            }
        } else {
            retval.emplace_back(std::move(sin_nx), real_trigonometric_kronecker_monomial(new_value, true));
            retval.emplace_back(std::move(cos_nx), real_trigonometric_kronecker_monomial(new_value, false));
            // Need to flip the sign on the cos * sin product if sign was changed.
            if (sign_changed) {
                math::negate(retval[1u].first);
            }
        }
        return retval;
    }
    /// Identify symbols that can be trimmed.
    /**
     * This method is used in piranha::series::trim(). The input parameter \p trim_mask
     * is a vector of boolean flags (i.e., a mask) which signals which elements in \p args are candidates
     * for trimming (i.e., a zero value means that the symbol at the corresponding position
     * in \p args is *not* a candidate for trimming, while a nonzero value means that the symbol is
     * a candidate for trimming). This method will set to zero those values in \p trim_mask
     * for which the corresponding element in \p this is nonzero.
     *
     * For instance, if \p this contains the values <tt>[0,5,3,0,4]</tt> and \p trim_mask originally contains
     * the values <tt>[1,1,0,1,0]</tt>, after a call to this method \p trim_mask will contain
     * <tt>[1,0,0,1,0]</tt> (that is, the second element was set from 1 to 0 as the corresponding element
     * in \p this has a value of 5 and thus must not be trimmed).
     *
     * @param trim_mask a mask signalling candidate elements for trimming.
     * @param args the reference piranha::symbol_fset.
     *
     * @throws std::invalid_argument if the size of \p trim_mask differs from the size of \p args.
     * @throws unspecified any exception thrown by unpack().
     */
    void trim_identify(std::vector<char> &trim_mask, const symbol_fset &args) const
    {
        detail::km_trim_identify<v_type, ka>(trim_mask, args, m_value);
    }
    /// Trim.
    /**
     * This method is used in piranha::series::trim(). The input mask \p trim_mask
     * is a vector of boolean flags signalling (with nonzero values) elements
     * of \p this to be removed. The method will return a copy of \p this in which
     * the specified elements have been removed.
     *
     * For instance, if \p this contains the values <tt>[0,5,3,0,4]</tt> and \p trim_mask contains
     * the values <tt>[false,false,false,true,false]</tt>, then the output of this method will be
     * the array of values <tt>[0,5,3,4]</tt> (that is, the fourth element has been removed as indicated
     * by a \p true value in <tt>trim_mask</tt>'s fourth element).
     *
     * @param trim_mask a mask indicating which element will be removed.
     * @param args the reference piranha::symbol_fset.
     *
     * @return a trimmed copy of \p this.
     *
     * @throws std::invalid_argument if the size of \p trim_mask differs from the size of \p args.
     * @throws unspecified any exception thrown by unpack() or piranha::static_vector::push_back().
     */
    real_trigonometric_kronecker_monomial trim(const std::vector<char> &trim_mask, const symbol_fset &args) const
    {
        return real_trigonometric_kronecker_monomial(detail::km_trim<v_type, ka>(trim_mask, args, m_value), m_flavour);
    }
    /// Comparison operator.
    /**
     * The values of the internal integral instances are used for comparison. If the values are the same,
     * the flavours of the monomials are compared to break the tie.
     *
     * @param other comparison argument.
     *
     * @return \p true if \p this is less than \p other, \p false otherwise.
     */
    bool operator<(const real_trigonometric_kronecker_monomial &other) const
    {
        if (m_value == other.m_value) {
            return m_flavour < other.m_flavour;
        }
        return m_value < other.m_value;
    }

#if defined(PIRANHA_WITH_MSGPACK)
private:
    template <typename Stream>
    using msgpack_pack_enabler
        = enable_if_t<conjunction<is_msgpack_stream<Stream>, has_msgpack_pack<Stream, T>,
                                  has_msgpack_pack<Stream, bool>, has_msgpack_pack<Stream, v_type>>::value,
                      int>;
    template <typename U>
    using msgpack_convert_enabler
        = enable_if_t<conjunction<has_msgpack_convert<typename U::value_type>, has_msgpack_convert<typename U::v_type>,
                                  has_msgpack_convert<bool>>::value,
                      int>;

public:
    /// Serialize in msgpack format.
    /**
     * \note
     * This method is activated only if \p Stream satisfies piranha::is_msgpack_stream and \p T, \p bool
     * and piranha::real_trigonometric_kronecker_monomial::v_type satisfy piranha::has_msgpack_pack.
     *
     * This method will pack \p this into \p packer.
     *
     * @param packer the target packer.
     * @param f the serialization format.
     * @param s the reference piranha::symbol_fset.
     *
     * @throws unspecified any exception thrown by:
     * - the public interface of <tt>msgpack::packer</tt>,
     * - piranha::msgpack_pack().
     */
    template <typename Stream, msgpack_pack_enabler<Stream> = 0>
    void msgpack_pack(msgpack::packer<Stream> &packer, msgpack_format f, const symbol_fset &s) const
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
     * @param o msgpack object that will be deserialized.
     * @param f serialization format.
     * @param s the reference piranha::symbol_fset.
     *
     * @throws std::invalid_argument if the size of the deserialized array differs from the size of \p s.
     * @throws unspecified any exception thrown by:
     * - the constructor of piranha::kronecker_monomial from a container,
     * - the public interface of <tt>msgpack::object</tt>,
     * - piranha::msgpack_convert().
     */
    template <typename U = real_trigonometric_kronecker_monomial, msgpack_convert_enabler<U> = 0>
    void msgpack_convert(const msgpack::object &o, msgpack_format f, const symbol_fset &s)
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

#if defined(PIRANHA_WITH_BOOST_S11N)

// Implementation of the Boost s11n api.
namespace boost
{
namespace serialization
{

template <typename Archive, typename T>
inline void save(Archive &ar,
                 const piranha::boost_s11n_key_wrapper<piranha::real_trigonometric_kronecker_monomial<T>> &k, unsigned)
{
    if (std::is_same<Archive, boost::archive::binary_oarchive>::value) {
        piranha::boost_save(ar, k.key().get_int());
    } else {
        auto tmp = k.key().unpack(k.ss());
        piranha::boost_save(ar, tmp);
    }
    piranha::boost_save(ar, k.key().get_flavour());
}

template <typename Archive, typename T>
inline void load(Archive &ar, piranha::boost_s11n_key_wrapper<piranha::real_trigonometric_kronecker_monomial<T>> &k,
                 unsigned)
{
    if (std::is_same<Archive, boost::archive::binary_iarchive>::value) {
        T value;
        piranha::boost_load(ar, value);
        k.key().set_int(value);
    } else {
        typename piranha::real_trigonometric_kronecker_monomial<T>::v_type tmp;
        piranha::boost_load(ar, tmp);
        if (unlikely(tmp.size() != k.ss().size())) {
            piranha_throw(std::invalid_argument, "invalid size detected in the deserialization of a real Kronercker "
                                                 "trigonometric monomial: the deserialized size is "
                                                     + std::to_string(tmp.size())
                                                     + " but the reference symbol set has a size of "
                                                     + std::to_string(k.ss().size()));
        }
        // NOTE: here the exception safety is basic, as the last boost_load() could fail in principle.
        // It does not really matter much, as there's no real dependency between the multipliers and the flavour,
        // any combination is valid.
        k.key() = piranha::real_trigonometric_kronecker_monomial<T>(tmp.begin(), tmp.end());
    }
    // The flavour loading is common.
    bool f;
    piranha::boost_load(ar, f);
    k.key().set_flavour(f);
}

template <typename Archive, typename T>
inline void serialize(Archive &ar,
                      piranha::boost_s11n_key_wrapper<piranha::real_trigonometric_kronecker_monomial<T>> &k,
                      unsigned version)
{
    split_free(ar, k, version);
}
}
}

namespace piranha
{

inline namespace impl
{

template <typename Archive, typename T>
using rtk_monomial_boost_save_enabler = enable_if_t<
    conjunction<has_boost_save<Archive, T>, has_boost_save<Archive, bool>,
                has_boost_save<Archive, typename real_trigonometric_kronecker_monomial<T>::v_type>>::value>;

template <typename Archive, typename T>
using rtk_monomial_boost_load_enabler = enable_if_t<
    conjunction<has_boost_load<Archive, T>, has_boost_load<Archive, bool>,
                has_boost_load<Archive, typename real_trigonometric_kronecker_monomial<T>::v_type>>::value>;
}

/// Specialisation of piranha::boost_save() for piranha::real_trigonometric_kronecker_monomial.
/**
 * \note
 * This specialisation is enabled only if \p T, \p bool and piranha::real_trigonometric_kronecker_monomial::v_type
 * satisfy piranha::has_boost_save.
 *
 * If \p Archive is \p boost::archive::binary_oarchive, the internal integral instance is saved.
 * Otherwise, the monomial is unpacked and the vector of multipliers is saved.
 *
 * @throws unspecified any exception thrown by piranha::boost_save() or
 * piranha::real_trigonometric_kronecker_monomial::unpack().
 */
template <typename Archive, typename T>
struct boost_save_impl<Archive, boost_s11n_key_wrapper<real_trigonometric_kronecker_monomial<T>>,
                       rtk_monomial_boost_save_enabler<Archive, T>>
    : boost_save_via_boost_api<Archive, boost_s11n_key_wrapper<real_trigonometric_kronecker_monomial<T>>> {
};

/// Specialisation of piranha::boost_load() for piranha::real_trigonometric_kronecker_monomial.
/**
 * \note
 * This specialisation is enabled only if \p T, \p bool and piranha::real_trigonometric_kronecker_monomial::v_type
 * satisfy piranha::has_boost_load.
 *
 * The basic exception safety guarantee is provided.
 *
 * @throws std::invalid_argument if the size of the serialized monomial is different from the size of the symbol set.
 * @throws unspecified any exception thrown by:
 * - piranha::boost_load(),
 * - the constructor of piranha::real_trigonometric_kronecker_monomial from a container.
 */
template <typename Archive, typename T>
struct boost_load_impl<Archive, boost_s11n_key_wrapper<real_trigonometric_kronecker_monomial<T>>,
                       rtk_monomial_boost_load_enabler<Archive, T>>
    : boost_load_via_boost_api<Archive, boost_s11n_key_wrapper<real_trigonometric_kronecker_monomial<T>>> {
};
}

#endif

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
     * @param a argument whose hash value will be computed.
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
