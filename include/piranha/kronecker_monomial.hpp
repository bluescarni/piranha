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

#ifndef PIRANHA_KRONECKER_MONOMIAL_HPP
#define PIRANHA_KRONECKER_MONOMIAL_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <piranha/config.hpp>
#include <piranha/detail/cf_mult_impl.hpp>
#include <piranha/detail/km_commons.hpp>
#include <piranha/detail/monomial_common.hpp>
#include <piranha/detail/prepare_for_print.hpp>
#include <piranha/detail/safe_integral_adder.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/is_cf.hpp>
#include <piranha/is_key.hpp>
#include <piranha/kronecker_array.hpp>
#include <piranha/math.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/mp_rational.hpp>
#include <piranha/pow.hpp>
#include <piranha/s11n.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/static_vector.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/term.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

inline namespace impl
{

// Check the size of a k_monomial after deserialization (s1) against the
// size of the reference symbol set (s2).
template <typename T, typename U>
inline void k_monomial_load_check_sizes(T s1, U s2)
{
    static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value && std::is_integral<U>::value
                      && std::is_unsigned<U>::value,
                  "Type error: this function requires unsigned integral types as input.");
    if (unlikely(s1 != s2)) {
        piranha_throw(std::invalid_argument, "invalid size detected in the deserialization of a Kronercker "
                                             "monomial: the deserialized size ("
                                                 + std::to_string(s1)
                                                 + ") differs from the size of the reference symbol set ("
                                                 + std::to_string(s2) + ")");
    }
}
}

/// Kronecker monomial class.
/**
 * This class represents a multivariate monomial with integral exponents. The values of the exponents are packed in a
 * signed integer using Kronecker substitution, using the facilities provided by piranha::kronecker_array.
 *
 * This class satisfies the piranha::is_key, piranha::key_has_degree, piranha::key_has_ldegree and
 * piranha::key_is_differentiable type traits.
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
// NOTE:
// - consider abstracting the km_commons in a class and use it both here and in rtkm.
// - it might be better to rework the machinery for the unpacking. An idea is to just use std::vectors
//   with TLS, and have the unpack function take retval as mutable ref rather than returning a vector.
template <typename T = std::make_signed<std::size_t>::type>
class kronecker_monomial
{
public:
    /// Alias for \p T.
    using value_type = T;

private:
    using ka = kronecker_array<T>;

public:
    /// Size type.
    /**
     * Used to represent the number of variables in the monomial. Equivalent to the size type of
     * piranha::kronecker_array.
     */
    using size_type = typename ka::size_type;
    /// Vector type used for temporary packing/unpacking.
    // NOTE: this essentially defines a maximum number of small ints that can be packed in m_value,
    // as we always need to pass through pack/unpack. In practice, it does not matter: in current
    // architectures the bit width limit will result in kronecker array's limits to be smaller than
    // 255 items.
    using v_type = static_vector<T, 255u>;
    /// Arity of the multiply() method.
    static const std::size_t multiply_arity = 1u;
    /// Default constructor.
    /**
     * After construction all exponents in the monomial will be zero.
     */
    kronecker_monomial() : m_value(0) {}
    /// Defaulted copy constructor.
    kronecker_monomial(const kronecker_monomial &) = default;
    /// Defaulted move constructor.
    kronecker_monomial(kronecker_monomial &&) = default;

private:
    // Enablers for the ctor from container.
    template <typename U>
    using container_ctor_enabler
        = enable_if_t<conjunction<has_input_begin_end<const U>,
                                  has_safe_cast<T, typename std::iterator_traits<decltype(
                                                       std::begin(std::declval<const U &>()))>::value_type>>::value,
                      int>;
    // Implementation of the ctor from range.
    template <typename Iterator>
    typename v_type::size_type construct_from_range(Iterator begin, Iterator end)
    {
        v_type tmp;
        std::transform(begin, end, std::back_inserter(tmp),
                       [](const uncvref_t<decltype(*begin)> &v) { return safe_cast<T>(v); });
        m_value = ka::encode(tmp);
        return tmp.size();
    }

public:
    /// Constructor from container.
    /**
     * \note
     * This constructor is enabled only if \p U satisfies piranha::has_input_begin_end, and the value type
     * of the iterator type of \p U can be safely cast to \p T.
     *
     * This constructor will build internally a vector of values from the input container \p c, encode it and assign the
     * result to the internal integer instance. The value type of the container is converted to \p T using
     * piranha::safe_cast().
     *
     * @param c the input container.
     *
     * @throws unspecified any exception thrown by kronecker_monomial::kronecker_monomial(Iterator, Iterator).
     */
    template <typename U, container_ctor_enabler<U> = 0>
    explicit kronecker_monomial(const U &c) : kronecker_monomial(std::begin(c), std::end(c))
    {
    }

private:
    template <typename U>
    using init_list_ctor_enabler = container_ctor_enabler<std::initializer_list<U>>;

public:
    /// Constructor from initializer list.
    /**
     * \note
     * This constructor is enabled only if \p U can be safely cast to \p T.
     *
     * @param list the input initializer list.
     *
     * @throws unspecified any exception thrown by kronecker_monomial::kronecker_monomial(Iterator, Iterator).
     */
    template <typename U, init_list_ctor_enabler<U> = 0>
    explicit kronecker_monomial(std::initializer_list<U> list) : kronecker_monomial(list.begin(), list.end())
    {
    }

private:
    template <typename Iterator>
    using it_ctor_enabler
        = enable_if_t<conjunction<is_input_iterator<Iterator>,
                                  has_safe_cast<T, typename std::iterator_traits<Iterator>::value_type>>::value,
                      int>;

public:
    /// Constructor from range.
    /**
     * \note
     * This constructor is enabled only if \p Iterator is an input iterator whose value type
     * is safely convertible to \p T.
     *
     * This constructor will build internally a vector of values from the input iterators, encode it and assign the
     * result to the internal integer instance. The value type of the iterator is converted to \p T using
     * piranha::safe_cast().
     *
     * @param begin the beginning of the range.
     * @param end the end of the range.
     *
     * @throws unspecified any exception thrown by:
     * - piranha::kronecker_array::encode(),
     * - piranha::safe_cast(),
     * - piranha::static_vector::push_back(),
     * - increment and dereference of the input iterators.
     */
    template <typename Iterator, it_ctor_enabler<Iterator> = 0>
    explicit kronecker_monomial(Iterator begin, Iterator end)
    {
        construct_from_range(begin, end);
    }
    /// Constructor from range and symbol set.
    /**
     * \note
     * This constructor is enabled only if \p Iterator is an input iterator whose value type
     * is safely convertible to \p T.
     *
     * This constructor is identical to the constructor from range. In addition, after construction
     * it will also check that the distance between \p begin and \p end is equal to the size of \p s.
     *
     * @param begin the beginning of the range.
     * @param end the end of the range.
     * @param s the reference piranha::symbol_fset.
     *
     * @throws std::invalid_argument if the distance between \p begin and \p end is different from
     * the size of \p s.
     * @throws unspecified any exception thrown by kronecker_monomial::kronecker_monomial(Iterator, Iterator)
     */
    template <typename Iterator, it_ctor_enabler<Iterator> = 0>
    explicit kronecker_monomial(Iterator begin, Iterator end, const symbol_fset &s)
    {
        const auto c_size = construct_from_range(begin, end);
        if (unlikely(c_size != s.size())) {
            piranha_throw(std::invalid_argument, "the Kronecker monomial constructor from range and symbol set "
                                                 "yielded an invalid monomial: the range length ("
                                                     + std::to_string(c_size)
                                                     + ") differs from the size of the symbol set ("
                                                     + std::to_string(s.size()) + ")");
        }
    }
    /// Constructor from set of symbols.
    /**
     * After construction all exponents in the monomial will be zero.
     */
    explicit kronecker_monomial(const symbol_fset &) : kronecker_monomial() {}
    /// Converting constructor.
    /**
     * This constructor is for use when converting from one term type to another in piranha::series. It will
     * set the internal integer instance to the same value of \p other.
     *
     * @param other the construction argument.
     */
    explicit kronecker_monomial(const kronecker_monomial &other, const symbol_fset &) : m_value(other.m_value) {}
    /// Constructor from \p T.
    /**
     * This constructor will initialise the internal integer instance
     * to \p n.
     *
     * @param n the value that will be used to construct the internal integer instance.
     */
    explicit kronecker_monomial(const T &n) : m_value(n) {}
    /// Destructor.
    ~kronecker_monomial()
    {
        PIRANHA_TT_CHECK(is_key, kronecker_monomial);
        PIRANHA_TT_CHECK(key_has_degree, kronecker_monomial);
        PIRANHA_TT_CHECK(key_has_ldegree, kronecker_monomial);
        PIRANHA_TT_CHECK(key_is_differentiable, kronecker_monomial);
    }
    /// Copy assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     */
    kronecker_monomial &operator=(const kronecker_monomial &other) = default;
    /// Defaulted move assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     */
    kronecker_monomial &operator=(kronecker_monomial &&other) = default;
    /// Set the internal integer instance.
    /**
     * @param n the value to which the internal integer instance will be set.
     */
    void set_int(const T &n)
    {
        m_value = n;
    }
    /// Get internal instance.
    /**
     * @return value of the internal integer instance.
     */
    T get_int() const
    {
        return m_value;
    }
    /// Compatibility check.
    /**
     * A monomial is considered incompatible with a piranha::symbol_fset if any of these conditions holds:
     *
     * - the size of \p args is zero and the internal integer is not zero,
     * - the size of \p args is equal to or larger than the size of the output of
     *   piranha::kronecker_array::get_limits(),
     * - the internal integer is not within the limits reported by piranha::kronecker_array::get_limits().
     *
     * Otherwise, the monomial is considered to be incompatible.
     *
     * @param args the reference piranha::symbol_fset.
     *
     * @return compatibility flag for the monomial.
     */
    bool is_compatible(const symbol_fset &args) const noexcept
    {
        // NOTE: the idea here is to avoid unpack()ing for performance reasons: these checks
        // are already part of unpack(), and that's why unpack() is used instead of is_compatible()
        // in other methods.
        const auto s = args.size();
        // No args means the value must also be zero.
        if (!s) {
            return !m_value;
        }
        const auto &limits = ka::get_limits();
        // If we overflow the maximum size available, we cannot use this object as key in series.
        if (s >= limits.size()) {
            return false;
        }
        const auto &l = limits[static_cast<decltype(limits.size())>(s)];
        // Value is compatible if it is within the bounds for the given size.
        return (m_value >= std::get<1u>(l) && m_value <= std::get<2u>(l));
    }
    /// Zero check.
    /**
     * A monomial is never zero.
     *
     * @return \p false.
     */
    bool is_zero(const symbol_fset &) const noexcept
    {
        return false;
    }
    /// Merge symbols.
    /**
     * This method will return a copy of \p this in which the value 0 has been inserted
     * at the positions specified by \p ins_map. Specifically, before each index appearing in \p ins_map
     * a number of zeroes equal to the size of the mapped piranha::symbol_fset will be inserted.
     *
     * For instance, given a piranha::kronecker_monomial containing the values <tt>[1,2,3,4]</tt>, a symbol set
     * \p args containing <tt>["c","e","g","h"]</tt> and an insertion map \p ins_map containing the pairs
     * <tt>[(0,["a","b"]),(1,["d"]),(2,["f"]),(4,["i"])]</tt>, the output of this method will be
     * <tt>[0,0,1,0,2,0,3,4,0]</tt>. That is, the symbols appearing in \p ins_map are merged into \p this
     * with a value of zero at the specified positions.
     *
     * @param ins_map the insertion map.
     * @param args the reference symbol set for \p this.
     *
     * @return a piranha::kronecker_monomial resulting from inserting into \p this zeroes at the positions
     * specified by \p ins_map.
     *
     * @throws std::invalid_argument in the following cases:
     * - the size of \p ins_map is zero,
     * - the last index in \p ins_map is greater than the size of \p args.
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - piranha::static_vector::push_back(),
     * - piranha::kronecker_array::encode().
     */
    kronecker_monomial merge_symbols(const symbol_idx_fmap<symbol_fset> &ins_map, const symbol_fset &args) const
    {
        return kronecker_monomial(detail::km_merge_symbols<v_type, ka>(ins_map, args, m_value));
    }
    /// Check if monomial is unitary.
    /**
     * @return \p true if all the exponents are zero, \p false otherwise.
     */
    bool is_unitary(const symbol_fset &) const
    {
        // A kronecker code will be zero if all components are zero.
        return !m_value;
    }

private:
    // Degree utils.
    using degree_type = decltype(std::declval<const T &>() + std::declval<const T &>());

public:
    /// Degree.
    /**
     * The type returned by this method is the type resulting from the addition of two instances
     * of \p T.
     *
     * @param args the reference piranha::symbol_fset.
     *
     * @return the degree of the monomial.
     *
     * @throws std::overflow_error if the computation of the degree overflows.
     * @throws unspecified any exception thrown by unpack().
     */
    degree_type degree(const symbol_fset &args) const
    {
        const auto tmp = unpack(args);
        // NOTE: this should be guaranteed by the unpack function.
        piranha_assert(tmp.size() == args.size());
        degree_type retval(0);
        for (const auto &x : tmp) {
            // NOTE: here it might be possible to demonstrate that overflow can
            // never occur, and that we can use a normal integral addition.
            detail::safe_integral_adder(retval, static_cast<degree_type>(x));
        }
        return retval;
    }
    /// Low degree (equivalent to the degree).
    /**
     * @param args the reference piranha::symbol_fset.
     *
     * @return the output of degree(const symbol_fset &) const.
     *
     * @throws unspecified any exception thrown by degree(const symbol_fset &) const.
     */
    degree_type ldegree(const symbol_fset &args) const
    {
        return degree(args);
    }
    /// Partial degree.
    /**
     * Partial degree of the monomial: only the symbols at the positions specified by \p p are considered.
     * The type returned by this method is the type resulting from the addition of two instances
     * of \p T.
     *
     * @param p the positions of the symbols to be considered in the calculation of the degree.
     * @param args the reference piranha::symbol_fset.
     *
     * @return the summation of the exponents of the monomial at the positions specified by \p p.
     *
     * @throws std::invalid_argument if the last element of \p p, if existing, is not less than the size
     * of \p args.
     * @throws std::overflow_error if the computation of the degree overflows.
     * @throws unspecified any exception thrown by unpack().
     */
    degree_type degree(const symbol_idx_fset &p, const symbol_fset &args) const
    {
        const auto tmp = unpack(args);
        piranha_assert(tmp.size() == args.size());
        if (unlikely(p.size() && *p.rbegin() >= tmp.size())) {
            piranha_throw(std::invalid_argument, "the largest value in the positions set for the computation of the "
                                                 "partial degree of a Kronecker monomial is "
                                                     + std::to_string(*p.rbegin())
                                                     + ", but the monomial has a size of only "
                                                     + std::to_string(tmp.size()));
        }
        degree_type retval(0);
        for (auto idx : p) {
            detail::safe_integral_adder(retval, static_cast<degree_type>(tmp[static_cast<decltype(tmp.size())>(idx)]));
        }
        return retval;
    }
    /// Partial low degree (equivalent to the partial degree).
    /**
     * @param p the positions of the symbols to be considered in the calculation of the degree.
     * @param args the reference piranha::symbol_fset.
     *
     * @return the output of degree(const symbol_idx_fset &, const symbol_fset &) const.
     *
     * @throws unspecified any exception thrown by degree(const symbol_idx_fset &, const symbol_fset &) const.
     */
    degree_type ldegree(const symbol_idx_fset &p, const symbol_fset &args) const
    {
        return degree(p, args);
    }

private:
    // Enabler for multiply().
    template <typename Cf>
    using multiply_enabler = enable_if_t<std::is_same<detail::cf_mult_enabler<Cf>, void>::value, int>;

public:
    /// Multiply terms with a Kronecker monomial key.
    /**
     * \note
     * This method is enabled only if \p Cf satisfies piranha::is_cf and piranha::has_mul3.
     *
     * Multiply \p t1 by \p t2, storing the result in the only element of \p res. This method
     * offers the basic exception safety guarantee. If \p Cf is an instance of piranha::mp_rational, then
     * only the numerators of the coefficients will be multiplied.
     *
     * Note that the key of the return value is generated directly from the addition of the values of the input keys.
     * No check is performed for overflow of either the limits of the integral type or the limits of the Kronecker
     * codification.
     *
     * @param res the return value.
     * @param t1 the first argument.
     * @param t2 the second argument.
     *
     * @throws unspecified any exception thrown by piranha::math::mul3().
     */
    template <typename Cf, multiply_enabler<Cf> = 0>
    static void multiply(std::array<term<Cf, kronecker_monomial>, multiply_arity> &res,
                         const term<Cf, kronecker_monomial> &t1, const term<Cf, kronecker_monomial> &t2,
                         const symbol_fset &)
    {
        // Coefficient first.
        detail::cf_mult_impl(res[0u].m_cf, t1.m_cf, t2.m_cf);
        // Now the key.
        math::add3(res[0u].m_key.m_value, t1.m_key.get_int(), t2.m_key.get_int());
    }
    /// Multiply Kronecker monomials.
    /**
     * Multiply \p a by \p b, storing the result in \p res.
     * No check is performed for overflow of either the limits of the integral type or the limits of the Kronecker
     * codification.
     *
     * @param res the return value.
     * @param a the first argument.
     * @param b the second argument.
     */
    static void multiply(kronecker_monomial &res, const kronecker_monomial &a, const kronecker_monomial &b,
                         const symbol_fset &)
    {
        math::add3(res.m_value, a.m_value, b.m_value);
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
     * @return \p true if the internal integral instance of \p this is equal to the integral instance of \p other,
     * \p false otherwise.
     */
    bool operator==(const kronecker_monomial &other) const
    {
        return m_value == other.m_value;
    }
    /// Inequality operator.
    /**
     * @param other the comparison argument.
     *
     * @return the opposite of operator==().
     */
    bool operator!=(const kronecker_monomial &other) const
    {
        return !operator==(other);
    }
    /// Detect linear monomial.
    /**
     * If the monomial is linear in a variable (i.e., all exponents are zero apart from a single unitary
     * exponent), then this method will return a pair formed by the ``true`` value and the position,
     * in ``args``, of the linear variable. Otherwise, the returned value will be a pair formed by the
     * ``false`` value and an unspecified position value.
     *
     * @param args the reference piranha::symbol_fset.
     *
     * @return a pair indicating if the monomial is linear.
     *
     * @throws unspecified any exception thrown by unpack().
     */
    std::pair<bool, symbol_idx> is_linear(const symbol_fset &args) const
    {
        const auto v = unpack(args);
        const auto size = v.size();
        decltype(v.size()) n_linear = 0, candidate = 0;
        for (decltype(v.size()) i = 0; i < size; ++i) {
            if (!v[i]) {
                continue;
            }
            if (v[i] != T(1)) {
                return std::make_pair(false, symbol_idx{0});
            }
            candidate = i;
            ++n_linear;
        }
        if (n_linear != 1u) {
            return std::make_pair(false, symbol_idx{0});
        }
        return std::make_pair(true, symbol_idx{candidate});
    }

private:
    // Enabler for pow.
    template <typename U>
    using pow_enabler = monomial_pow_enabler<T, U>;

public:
    /// Exponentiation.
    /**
     * This method will return a monomial corresponding to \p this raised to the ``x``-th power. The exponentiation
     * is computed via the multiplication of the exponents by \p x. The multiplication is performed in different
     * ways, depending on the type ``U``:
     * - if ``U`` is a C++ integral type, the multiplication is checked for overflow; otherwise,
     * - if the multiplication of ``T`` by ``U`` results in another type ``T2``, then the multiplication
     *   is performed via the binary multiplication operator and the result is cast back to ``T`` via
     *   piranha::safe_cast().
     *
     * If the type ``U`` cannot be used as indicated above (e.g., because the multiplication of ``T`` by ``U`` is
     * not available, or ``T2`` does not support piranha::safe_cast()), then the method will be disabled.
     *
     * @param x the exponent.
     * @param args the reference piranha::symbol_fset.
     *
     * @return \p this to the power of \p x.
     *
     * @throws std::overflow_error if ``U`` is an integral type and the exponentiation
     * causes overflow.
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - the multiplication of the monomial's exponents by ``x``,
     * - piranha::safe_cast(),
     * - piranha::kronecker_array::encode().
     */
    template <typename U, pow_enabler<U> = 0>
    kronecker_monomial pow(const U &x, const symbol_fset &args) const
    {
        auto v = unpack(args);
        for (auto &n : v) {
            monomial_pow_mult_exp(n, n, x, monomial_pow_dispatcher<T, U>{});
        }
        return kronecker_monomial(ka::encode(v));
    }
    /// Unpack internal integer instance.
    /**
     * This method will decode the internal integral instance into a piranha::static_vector of size equal to the size of
     * \p args.
     *
     * @param args the reference piranha::symbol_fset.
     *
     * @return piranha::static_vector containing the result of decoding the internal integral instance via
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
     * This method will print to stream a human-readable representation of the monomial.
     *
     * @param os the target stream.
     * @param args the reference piranha::symbol_fset.
     *
     * @throws unspecified any exception thrown by unpack() or by streaming instances of \p T.
     */
    void print(std::ostream &os, const symbol_fset &args) const
    {
        const auto tmp = unpack(args);
        piranha_assert(tmp.size() == args.size());
        bool empty_output = true;
        for (decltype(tmp.size()) i = 0u; i < tmp.size(); ++i) {
            if (tmp[i] != T(0)) {
                if (!empty_output) {
                    os << '*';
                }
                os << *args.nth(static_cast<decltype(args.size())>(i));
                empty_output = false;
                if (tmp[i] != T(1)) {
                    os << "**" << detail::prepare_for_print(tmp[i]);
                }
            }
        }
    }
    /// Print in TeX mode.
    /**
     * This method will print to stream a TeX representation of the monomial.
     *
     * @param os the target stream.
     * @param args the reference piranha::symbol_fset.
     *
     * @throws unspecified any exception thrown by unpack() or by streaming instances of \p T.
     */
    void print_tex(std::ostream &os, const symbol_fset &args) const
    {
        const auto tmp = unpack(args);
        std::ostringstream oss_num, oss_den, *cur_oss;
        T cur_value;
        for (decltype(tmp.size()) i = 0u; i < tmp.size(); ++i) {
            cur_value = tmp[i];
            if (cur_value != T(0)) {
                // NOTE: here negate() is safe because of the symmetry in kronecker_array.
                cur_oss = (cur_value > T(0)) ? &oss_num : (math::negate(cur_value), &oss_den);
                (*cur_oss) << "{" << *args.nth(static_cast<decltype(args.size())>(i)) << "}";
                if (cur_value != T(1)) {
                    (*cur_oss) << "^{" << static_cast<long long>(cur_value) << "}";
                }
            }
        }
        const std::string num_str = oss_num.str(), den_str = oss_den.str();
        if (!num_str.empty() && !den_str.empty()) {
            os << "\\frac{" << num_str << "}{" << den_str << "}";
        } else if (!num_str.empty() && den_str.empty()) {
            os << num_str;
        } else if (num_str.empty() && !den_str.empty()) {
            os << "\\frac{1}{" << den_str << "}";
        }
    }
    /// Partial derivative.
    /**
     * This method will return the partial derivative of \p this with respect to the symbol at the position indicated by
     * \p p. The result is a pair consisting of the exponent associated to \p p before differentiation and the monomial
     * itself after differentiation. If \p p is not smaller than the size of \p args or if its corresponding exponent is
     * zero, the returned pair will be <tt>(0,kronecker_monomial{})</tt>.
     *
     * @param p the position of the symbol with respect to which the differentiation will be calculated.
     * @param args the reference piranha::symbol_fset.
     *
     * @return the result of the differentiation.
     *
     * @throws std::overflow_error if the computation of the derivative causes a negative overflow.
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - piranha::kronecker_array::encode().
     */
    std::pair<T, kronecker_monomial> partial(const symbol_idx &p, const symbol_fset &args) const
    {
        auto v = unpack(args);
        if (p >= args.size() || v[static_cast<decltype(v.size())>(p)] == T(0)) {
            // Derivative wrt a variable not in the monomial: position is outside the bounds, or it refers to a
            // variable with zero exponent.
            return std::make_pair(T(0), kronecker_monomial{});
        }
        auto v_b = v.begin();
        // Original exponent.
        const T n(v_b[p]);
        // Decrement the exponent in the monomial.
        // NOTE: maybe replace with the safe integral subber, eventually.
        if (unlikely(n == std::numeric_limits<T>::min())) {
            piranha_throw(std::overflow_error, "negative overflow error in the calculation of the "
                                               "partial derivative of a Kronecker monomial");
        }
        v_b[p] = static_cast<T>(n - T(1));
        return std::make_pair(n, kronecker_monomial(ka::encode(v)));
    }
    /// Integration.
    /**
     * This method will return the antiderivative of \p this with respect to the symbol \p s. The result is a pair
     * consisting of the exponent associated to \p s increased by one and the monomial itself
     * after integration. If \p s is not in \p args, the returned monomial will have an extra exponent
     * set to 1 in the same position \p s would have if it were added to \p args.
     * If the exponent corresponding to \p s is -1, an error will be produced.
     *
     * @param s the symbol with respect to which the integration will be calculated.
     * @param args the reference piranha::symbol_fset.
     *
     * @return result of the integration.
     *
     * @throws std::invalid_argument if the exponent associated to \p s is -1 or if the value of an exponent overflows.
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - piranha::static_vector::push_back(),
     * - piranha::kronecker_array::encode().
     */
    std::pair<T, kronecker_monomial> integrate(const std::string &s, const symbol_fset &args) const
    {
        v_type v = unpack(args), retval;
        T expo(0);
        for (decltype(v.size()) i = 0; i < v.size(); ++i) {
            const auto &cur_sym = *args.nth(static_cast<decltype(args.size())>(i));
            if (expo == T(0) && s < cur_sym) {
                // If we went past the position of s in args and still we
                // have not performed the integration, it means that we need to add
                // a new exponent.
                retval.push_back(T(1));
                expo = T(1);
            }
            retval.push_back(v[i]);
            if (cur_sym == s) {
                // NOTE: here using i is safe: if retval gained an extra exponent in the condition above,
                // we are never going to land here as cur_sym is at this point never going to be s.
                if (unlikely(retval[i] == std::numeric_limits<T>::max())) {
                    piranha_throw(std::overflow_error,
                                  "positive overflow error in the calculation of the integral of a Kronecker monomial");
                }
                retval[i] = static_cast<T>(retval[i] + T(1));
                if (math::is_zero(retval[i])) {
                    piranha_throw(std::invalid_argument,
                                  "unable to perform Kronecker monomial integration: a negative "
                                  "unitary exponent was encountered in correspondence of the variable '"
                                      + cur_sym + "'");
                }
                expo = retval[i];
            }
        }
        // If expo is still zero, it means we need to add a new exponent at the end.
        if (expo == T(0)) {
            retval.push_back(T(1));
            expo = T(1);
        }
        return std::make_pair(expo, kronecker_monomial(ka::encode(retval)));
    }

private:
    // Determination of the eval type.
    template <typename U>
    using e_type = decltype(math::pow(std::declval<const U &>(), std::declval<const T &>()));
    template <typename U>
    using eval_type = enable_if_t<conjunction<is_multipliable_in_place<e_type<U>>,
                                              std::is_constructible<e_type<U>, int>, is_returnable<e_type<U>>>::value,
                                  e_type<U>>;

public:
    /// Evaluation.
    /**
     * \note
     * This method is available only if \p U satisfies the following requirements:
     * - it can be used in piranha::math::pow() with the monomial exponents as powers, yielding a type \p eval_type,
     * - \p eval_type is constructible from \p int,
     * - \p eval_type is multipliable in place,
     * - \p eval_type satisfies piranha::is_returnable.
     *
     * The return value will be built by iteratively applying piranha::math::pow() using the values provided
     * by \p values as bases and the values in the monomial as exponents. If the size of the monomial is zero, 1 will be
     * returned.
     *
     * @param values the values will be used for substitution.
     * @param args the reference piranha::symbol_fset.
     *
     * @return the result of evaluating \p this with the values provided in \p values.
     *
     * @throws std::invalid_argument if the sizes of \p values and \p args differ.
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - construction of the return type,
     * - piranha::math::pow() or the in-place multiplication operator of the return type.
     */
    template <typename U>
    eval_type<U> evaluate(const std::vector<U> &values, const symbol_fset &args) const
    {
        // NOTE: here we can check the values size only against args.
        if (unlikely(values.size() != args.size())) {
            piranha_throw(
                std::invalid_argument,
                "invalid vector of values for Kronecker monomial evaluation: the size of the vector of values ("
                    + std::to_string(values.size()) + ") differs from the size of the reference set of symbols ("
                    + std::to_string(args.size()) + ")");
        }
        if (args.size()) {
            const auto v = unpack(args);
            eval_type<U> retval(math::pow(values[0], v[0]));
            for (decltype(v.size()) i = 1; i < v.size(); ++i) {
                // NOTE: here maybe we could use mul3() and pow3() (to be implemented?).
                // NOTE: math::pow() for C++ integrals produces an integer result, no need
                // to worry about overflows.
                retval *= math::pow(values[static_cast<decltype(values.size())>(i)], v[i]);
            }
            return retval;
        }
        return eval_type<U>(1);
    }

private:
    // Subs type is same as eval_type.
    template <typename U>
    using subs_type = eval_type<U>;

public:
    /// Substitution.
    /**
     * \note
     * This method is available only if \p U satisfies the following requirements:
     * - it can be used in piranha::math::pow() with the monomial exponents as powers, yielding a type \p subs_type,
     * - \p subs_type is constructible from \p int,
     * - \p subs_type is multipliable in place,
     * - \p subs_type satisfies piranha::is_returnable.
     *
     * This method will substitute the symbols at the positions specified in the keys of ``smap`` with the mapped
     * values. The return value is a vector containing one pair in which the first element is the result of the
     * substitution (i.e., the product of the values of ``smap`` raised to the corresponding exponents in the monomial),
     * and the second element is the monomial after the substitution has been performed (i.e., with the exponents
     * at the positions specified by the keys of ``smap`` set to zero). If ``smap`` is empty,
     * the return value will be <tt>(1,this)</tt> (i.e., the monomial is unchanged and the substitution yields 1).
     *
     * For instance, given the monomial ``[2,3,4]``, the reference piranha::symbol_fset ``["x","y","z"]``
     * and the substitution map ``[(0,1),(2,-3)]``, then the return value will be a vector containing
     * the single pair ``(81,[0,3,0])``.
     *
     * @param smap the map relating the positions of the symbols to be substituted to the values
     * they will be substituted with.
     * @param args the reference piranha::symbol_fset.
     *
     * @return the result of the substitution.
     *
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - the construction of the return value,
     * - piranha::math::pow() or the in-place multiplication operator of the return type,
     * - piranha::kronecker_array::encode().
     */
    template <typename U>
    std::vector<std::pair<subs_type<U>, kronecker_monomial>> subs(const symbol_idx_fmap<U> &smap,
                                                                  const symbol_fset &args) const
    {
        if (unlikely(smap.size() && smap.rbegin()->first >= args.size())) {
            // The last element of the insertion map must be a valid index into args.
            piranha_throw(
                std::invalid_argument,
                "invalid argument(s) for substitution in a Kronecker monomial: the last index of the substitution map ("
                    + std::to_string(smap.rbegin()->first) + ") must be smaller than the monomial's size ("
                    + std::to_string(args.size()) + ")");
        }
        std::vector<std::pair<subs_type<U>, kronecker_monomial>> retval;
        if (smap.size()) {
            // The substitution map contains something, proceed to the substitution.
            auto v = unpack(args);
            // Init the return value from the exponentiation of the first value in the map.
            auto it = smap.begin();
            auto ret(math::pow(it->second, v[static_cast<decltype(v.size())>(it->first)]));
            // Zero out the corresponding exponent.
            v[static_cast<decltype(v.size())>(it->first)] = T(0);
            //  NOTE: move to the next element in the init statement of the for loop.
            for (++it; it != smap.end(); ++it) {
                ret *= math::pow(it->second, v[static_cast<decltype(v.size())>(it->first)]);
                v[static_cast<decltype(v.size())>(it->first)] = T(0);
            }
            retval.emplace_back(std::move(ret), kronecker_monomial(ka::encode(v)));
        } else {
            // Otherwise, the substitution yields 1 and the monomial is the original one.
            retval.emplace_back(subs_type<U>(1), *this);
        }
        return retval;
    }

private:
    // ipow subs utilities.
    template <typename U>
    using ips_type = decltype(math::pow(std::declval<const U &>(), std::declval<const integer &>()));
    template <typename U>
    using ipow_subs_type
        = enable_if_t<conjunction<std::is_constructible<ips_type<U>, int>, is_returnable<ips_type<U>>>::value,
                      ips_type<U>>;

public:
    /// Substitution of integral power.
    /**
     * \note
     * This method is enabled only if:
     * - \p U can be raised to a piranha::integer power, yielding a type \p subs_type,
     * - \p subs_type is constructible from \p int,
     * - \p subs_type satisfies piranha::is_returnable.
     *
     * This method will substitute the <tt>n</tt>-th power of the symbol at the position \p p with the quantity \p x.
     * The return value is a vector containing a single pair in which the first element is the result of the
     * substitution, and the second element the monomial after the substitution has been performed.
     * If \p p is not less than the size of \p args, the return value will be <tt>(1,this)</tt> (i.e., the monomial is
     * unchanged and the substitution yields 1).
     *
     * The method will substitute also powers higher than \p n in absolute value.
     * For instance, the substitution of <tt>y**2</tt> with \p a in the monomial <tt>y**7</tt> will produce
     * <tt>a**3 * y</tt>, and the substitution of <tt>y**-2</tt> with \p a in the monomial <tt>y**-7</tt> will produce
     * <tt>a**3 * y**-1</tt>.
     *
     * @param p the position of the symbol that will be substituted.
     * @param n the integral power that will be substituted.
     * @param x the quantity that will be substituted.
     * @param args the reference piranha::symbol_fset.
     *
     * @return the result of substituting \p x for the <tt>n</tt>-th power of the symbol at the position \p p.
     *
     * @throws std::invalid_argument is \p n is zero.
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - construction of the return value,
     * - piranha::math::pow(),
     * - arithmetics on piranha::integer,
     * - piranha::kronecker_array::encode().
     */
    template <typename U>
    std::vector<std::pair<ipow_subs_type<U>, kronecker_monomial>> ipow_subs(const symbol_idx &p, const integer &n,
                                                                            const U &x, const symbol_fset &args) const
    {
        if (unlikely(!n.sgn())) {
            piranha_throw(
                std::invalid_argument,
                "invalid integral power in for ipow_subs() in a Kronecker monomial: the power must be nonzero");
        }
        std::vector<std::pair<ipow_subs_type<U>, kronecker_monomial>> retval;
        if (p < args.size()) {
            PIRANHA_MAYBE_TLS integer q, r, d;
            auto v = unpack(args);
            d = v[static_cast<decltype(v.size())>(p)];
            tdiv_qr(q, r, d, n);
            if (q.sgn() > 0) {
                v[static_cast<decltype(v.size())>(p)] = static_cast<T>(r);
                retval.emplace_back(math::pow(x, q), kronecker_monomial(ka::encode(v)));
                return retval;
            }
        }
        // Otherwise, the substitution yields 1 and the monomial is the original one.
        retval.emplace_back(ipow_subs_type<U>(1), *this);
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
    kronecker_monomial trim(const std::vector<char> &trim_mask, const symbol_fset &args) const
    {
        return kronecker_monomial(detail::km_trim<v_type, ka>(trim_mask, args, m_value));
    }
    /// Comparison operator.
    /**
     * @param other comparison argument.
     *
     * @return \p true if the internal integral value of \p this is less than the internal
     * integral value of \p other, \p false otherwise.
     */
    bool operator<(const kronecker_monomial &other) const
    {
        return m_value < other.m_value;
    }

#if defined(PIRANHA_WITH_MSGPACK)
private:
    // Enablers for msgpack serialization.
    template <typename Stream>
    using msgpack_pack_enabler = enable_if_t<
        conjunction<is_msgpack_stream<Stream>, has_msgpack_pack<Stream, T>, has_msgpack_pack<Stream, v_type>>::value,
        int>;
    template <typename U>
    using msgpack_convert_enabler = enable_if_t<
        conjunction<has_msgpack_convert<typename U::value_type>, has_msgpack_convert<typename U::v_type>>::value, int>;

public:
    /// Serialize in msgpack format.
    /**
     * \note
     * This method is activated only if \p Stream satisfies piranha::is_msgpack_stream and both \p T and
     * piranha::kronecker_monomial::v_type satisfy piranha::has_msgpack_pack.
     *
     * This method will pack \p this into \p packer. The packed object is the internal integral instance in binary
     * format, an array of exponents in portable format.
     *
     * @param packer the target packer.
     * @param f the serialization format.
     * @param s reference piranha::symbol_fset.
     *
     * @throws unspecified any exception thrown by unpack() or piranha::msgpack_pack().
     */
    template <typename Stream, msgpack_pack_enabler<Stream> = 0>
    void msgpack_pack(msgpack::packer<Stream> &packer, msgpack_format f, const symbol_fset &s) const
    {
        if (f == msgpack_format::binary) {
            piranha::msgpack_pack(packer, m_value, f);
        } else {
            auto tmp = unpack(s);
            piranha::msgpack_pack(packer, tmp, f);
        }
    }
    /// Deserialize from msgpack object.
    /**
     * \note
     * This method is activated only if both \p T and piranha::kronecker_monomial::v_type satisfy
     * piranha::has_msgpack_convert.
     *
     * This method will deserialize \p o into \p this. In binary mode, no check is performed on the content of \p o,
     * and calling this method will result in undefined behaviour if \p o does not contain a monomial serialized via
     * msgpack_pack().
     *
     * @param o msgpack object that will be deserialized.
     * @param f serialization format.
     * @param s reference piranha::symbol_fset.
     *
     * @throws std::invalid_argument if the size of the deserialized array differs from the size of \p s.
     * @throws unspecified any exception thrown by:
     * - the constructor of piranha::kronecker_monomial from a container,
     * - piranha::msgpack_convert().
     */
    template <typename U = kronecker_monomial, msgpack_convert_enabler<U> = 0>
    void msgpack_convert(const msgpack::object &o, msgpack_format f, const symbol_fset &s)
    {
        if (f == msgpack_format::binary) {
            piranha::msgpack_convert(m_value, o, f);
        } else {
            v_type tmp;
            piranha::msgpack_convert(tmp, o, f);
            k_monomial_load_check_sizes(tmp.size(), s.size());
            *this = kronecker_monomial(tmp);
        }
    }
#endif

private:
    T m_value;
};

/// Alias for piranha::kronecker_monomial with default type.
using k_monomial = kronecker_monomial<>;
}

// Implementation of the Boost s11n api.
namespace boost
{
namespace serialization
{

template <typename Archive, typename T>
inline void save(Archive &ar, const piranha::boost_s11n_key_wrapper<piranha::kronecker_monomial<T>> &k, unsigned)
{
    if (std::is_same<Archive, boost::archive::binary_oarchive>::value) {
        piranha::boost_save(ar, k.key().get_int());
    } else {
        auto tmp = k.key().unpack(k.ss());
        piranha::boost_save(ar, tmp);
    }
}

template <typename Archive, typename T>
inline void load(Archive &ar, piranha::boost_s11n_key_wrapper<piranha::kronecker_monomial<T>> &k, unsigned)
{
    if (std::is_same<Archive, boost::archive::binary_iarchive>::value) {
        T value;
        piranha::boost_load(ar, value);
        k.key().set_int(value);
    } else {
        typename piranha::kronecker_monomial<T>::v_type tmp;
        piranha::boost_load(ar, tmp);
        piranha::k_monomial_load_check_sizes(tmp.size(), k.ss().size());
        k.key() = piranha::kronecker_monomial<T>(tmp);
    }
}

template <typename Archive, typename T>
inline void serialize(Archive &ar, piranha::boost_s11n_key_wrapper<piranha::kronecker_monomial<T>> &k, unsigned version)
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
using k_monomial_boost_save_enabler = enable_if_t<
    conjunction<has_boost_save<Archive, T>, has_boost_save<Archive, typename kronecker_monomial<T>::v_type>>::value>;

template <typename Archive, typename T>
using k_monomial_boost_load_enabler = enable_if_t<
    conjunction<has_boost_load<Archive, T>, has_boost_load<Archive, typename kronecker_monomial<T>::v_type>>::value>;
}

/// Specialisation of piranha::boost_save() for piranha::kronecker_monomial.
/**
 * \note
 * This specialisation is enabled only if \p T and piranha::kronecker_monomial::v_type satisfy
 * piranha::has_boost_save.
 *
 * If \p Archive is \p boost::archive::binary_oarchive, the internal integral instance is saved.
 * Otherwise, the monomial is unpacked and the vector of exponents is saved.
 *
 * @throws unspecified any exception thrown by piranha::boost_save() or piranha::kronecker_monomial::unpack().
 */
template <typename Archive, typename T>
struct boost_save_impl<Archive, boost_s11n_key_wrapper<kronecker_monomial<T>>,
                       k_monomial_boost_save_enabler<Archive, T>>
    : boost_save_via_boost_api<Archive, boost_s11n_key_wrapper<kronecker_monomial<T>>> {
};

/// Specialisation of piranha::boost_load() for piranha::kronecker_monomial.
/**
 * \note
 * This specialisation is enabled only if \p T and piranha::kronecker_monomial::v_type satisfy
 * piranha::has_boost_load.
 *
 * @throws std::invalid_argument if the size of the serialized monomial is different from the size of the symbol set.
 * @throws unspecified any exception thrown by:
 * - piranha::boost_load(),
 * - the constructor of piranha::kronecker_monomial from a container.
 */
template <typename Archive, typename T>
struct boost_load_impl<Archive, boost_s11n_key_wrapper<kronecker_monomial<T>>,
                       k_monomial_boost_load_enabler<Archive, T>>
    : boost_load_via_boost_api<Archive, boost_s11n_key_wrapper<kronecker_monomial<T>>> {
};
}

namespace std
{

/// Specialisation of \p std::hash for piranha::kronecker_monomial.
template <typename T>
struct hash<piranha::kronecker_monomial<T>> {
    /// Result type.
    using result_type = size_t;
    /// Argument type.
    using argument_type = piranha::kronecker_monomial<T>;
    /// Hash operator.
    /**
     * @param a argument whose hash value will be computed.
     *
     * @return hash value of \p a computed via piranha::kronecker_monomial::hash().
     */
    result_type operator()(const argument_type &a) const
    {
        return a.hash();
    }
};
}

#endif
