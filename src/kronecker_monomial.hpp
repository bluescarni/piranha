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

#ifndef PIRANHA_KRONECKER_MONOMIAL_HPP
#define PIRANHA_KRONECKER_MONOMIAL_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

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
#include "mp_rational.hpp"
#include "pow.hpp"
#include "safe_cast.hpp"
#include "serialization.hpp"
#include "static_vector.hpp"
#include "symbol.hpp"
#include "symbol_set.hpp"
#include "term.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Kronecker monomial class.
/**
 * This class represents a multivariate monomial with integral exponents.
 * The values of the exponents are packed in a signed integer using Kronecker substitution, using the facilities
 * provided
 * by piranha::kronecker_array.
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
 *
 * ## Serialization ##
 *
 * This class supports serialization.
 */
// TODO:
// - consider abstracting the km_commons in a class and use it both here and in rtkm.
template <typename T = std::make_signed<std::size_t>::type>
class kronecker_monomial
{
public:
    /// Alias for \p T.
    typedef T value_type;

private:
    typedef kronecker_array<value_type> ka;

public:
    /// Size type.
    /**
     * Used to represent the number of variables in the monomial. Equivalent to the size type of
     * piranha::kronecker_array.
     */
    typedef typename ka::size_type size_type;
    /// Vector type used for temporary packing/unpacking.
    // NOTE: this essentially defines a maximum number of small ints that can be packed in m_value,
    // as we always need to pass through pack/unpack. In practice, it does not matter: in current
    // architectures the bit width limit will result in kronecker array's limits to be smaller than
    // 255 items.
    using v_type = static_vector<value_type, 255u>;

private:
#if !defined(PIRANHA_DOXYGEN_INVOKED)
    // Eval and sub typedef.
    template <typename U, typename = void>
    struct eval_type_ {
    };
    template <typename U>
    using e_type = decltype(math::pow(std::declval<U const &>(), std::declval<value_type const &>()));
    template <typename U>
    struct eval_type_<U, typename std::enable_if<is_multipliable_in_place<e_type<U>>::value
                                                 && std::is_constructible<e_type<U>, int>::value
                                                 && detail::is_pmappable<U>::value>::type> {
        using type = e_type<U>;
    };
    // The final typedef.
    template <typename U>
    using eval_type = typename eval_type_<U>::type;
    // Enabler for pow.
    template <typename U>
    using pow_enabler = typename std::
        enable_if<has_safe_cast<T, decltype(std::declval<integer &&>() * std::declval<const U &>())>::value, int>::type;
    // Serialization support.
    friend class boost::serialization::access;
    template <typename Archive>
    void serialize(Archive &ar, unsigned int)
    {
        ar &m_value;
    }
    // Enabler for multiply().
    template <typename Cf>
    using multiply_enabler = typename std::enable_if<detail::true_tt<detail::cf_mult_enabler<Cf>>::value, int>::type;
    // Subs utilities.
    template <typename U>
    using subs_type__ = decltype(math::pow(std::declval<const U &>(), std::declval<const value_type &>()));
    template <typename U, typename = void>
    struct subs_type_ {
    };
    template <typename U>
    struct subs_type_<U,
                      typename std::enable_if<std::is_constructible<subs_type__<U>, int>::value
                                              && std::is_assignable<subs_type__<U> &, subs_type__<U>>::value>::type> {
        using type = subs_type__<U>;
    };
    template <typename U>
    using subs_type = typename subs_type_<U>::type;
    // ipow subs utilities.
    template <typename U>
    using ipow_subs_type__ = decltype(math::pow(std::declval<const U &>(), std::declval<const integer &>()));
    template <typename U, typename = void>
    struct ipow_subs_type_ {
    };
    template <typename U>
    struct ipow_subs_type_<U, typename std::enable_if<std::is_constructible<ipow_subs_type__<U>, int>::value
                                                      && std::is_assignable<ipow_subs_type__<U> &,
                                                                            ipow_subs_type__<U>>::value>::type> {
        using type = ipow_subs_type__<U>;
    };
    template <typename U>
    using ipow_subs_type = typename ipow_subs_type_<U>::type;
    // Enablers for the ctors from container, init list and iterator.
    template <typename U>
    using container_ctor_enabler =
        typename std::enable_if<has_begin_end<const U>::value
                                    && has_safe_cast<T, typename std::iterator_traits<decltype(
                                                            std::begin(std::declval<const U &>()))>::value_type>::value,
                                int>::type;
    template <typename U>
    using init_list_ctor_enabler = container_ctor_enabler<std::initializer_list<U>>;
    template <typename Iterator>
    using it_ctor_enabler =
        typename std::enable_if<is_input_iterator<Iterator>::value
                                    && has_safe_cast<value_type,
                                                     typename std::iterator_traits<Iterator>::value_type>::value,
                                int>::type;
    // Implementation of the ctor from range.
    template <typename Iterator>
    typename v_type::size_type construct_from_range(Iterator begin, Iterator end)
    {
        v_type tmp;
        for (; begin != end; ++begin) {
            tmp.push_back(safe_cast<value_type>(*begin));
        }
        m_value = ka::encode(tmp);
        return tmp.size();
    }
    // Degree utils.
    using degree_type = decltype(std::declval<const T &>() + std::declval<const T &>());
#endif
public:
    /// Arity of the multiply() method.
    static const std::size_t multiply_arity = 1u;
    /// Default constructor.
    /**
     * After construction all exponents in the monomial will be zero.
     */
    kronecker_monomial() : m_value(0)
    {
    }
    /// Defaulted copy constructor.
    kronecker_monomial(const kronecker_monomial &) = default;
    /// Defaulted move constructor.
    kronecker_monomial(kronecker_monomial &&) = default;
    /// Constructor from container.
    /**
     * \note
     * This constructor is enabled only if \p U satisfies piranha::has_begin_end, and the value type
     * of the iterator type of \p U can be safely cast to \p T.
     *
     * This constructor will build internally a vector of values from the input container \p c, encode it and assign the
     * result
     * to the internal integer instance. The value type of the container is converted to \p T using
     * piranha::safe_cast().
     *
     * @param[in] c the input container.
     *
     * @throws std::overflow_error if the container has a size greater than an implementation-defined value.
     * @throws unspecified any exception thrown by:
     * - piranha::kronecker_array::encode(),
     * - piranha::safe_cast(),
     * - piranha::static_vector::push_back().
     */
    template <typename U, container_ctor_enabler<U> = 0>
    explicit kronecker_monomial(const U &c) : m_value(0)
    {
        construct_from_range(std::begin(c), std::end(c));
    }
    /// Constructor from initializer list.
    /**
     * \note
     * This constructor is enabled only if the corresponding constructor from container is enabled.
     *
     * This constructor is identical to the constructor from container. It is provided for convenience.
     *
     * @param[in] list the input initializer list.
     *
     * @throws unspecified any exception thrown by the constructor from container.
     */
    template <typename U, init_list_ctor_enabler<U> = 0>
    explicit kronecker_monomial(std::initializer_list<U> list) : m_value(0)
    {
        construct_from_range(list.begin(), list.end());
    }
    /// Constructor from range.
    /**
     * \note
     * This constructor is enabled only if \p Iterator is an input iterator whose value type
     * is safely convertible to \p T.
     *
     * This constructor will build internally a vector of values from the input iterators, encode it and assign the
     * result
     * to the internal integer instance. The value type of the iterator is converted to \p T using
     * piranha::safe_cast().
     *
     * @param[in] begin beginning of the range.
     * @param[in] end end of the range.
     *
     * @throws std::overflow_error if the distance between \p begin and \p end is greater than an implementation-defined
     * value.
     * @throws unspecified any exception thrown by:
     * - piranha::kronecker_array::encode(),
     * - piranha::safe_cast(),
     * - piranha::static_vector::push_back(),
     * - increment and dereference of the input iterators.
     */
    template <typename Iterator, it_ctor_enabler<Iterator> = 0>
    explicit kronecker_monomial(Iterator begin, Iterator end) : m_value(0)
    {
        construct_from_range(begin, end);
    }
    /// Constructor from range and symbol set.
    /**
     * \note
     * This constructor is enabled only if the corresponding range constructor is enabled.
     *
     * This constructor is identical to the constructor from range. In addition, after construction
     * it will also check that the distance between \p begin and \p end is equal to the size of \p s.
     * This constructor is used by piranha::polynomial::find_cf().
     *
     * @param[in] begin beginning of the range.
     * @param[in] end end of the range.
     * @param[in] s reference symbol set.
     *
     * @throws std::invalid_argument if the distance between \p begin and \p end is different from
     * the size of \p s.
     * @throws unspecified any exception thrown by the constructor from range.
     */
    template <typename Iterator, it_ctor_enabler<Iterator> = 0>
    explicit kronecker_monomial(Iterator begin, Iterator end, const symbol_set &s) : m_value(0)
    {
        if (unlikely(construct_from_range(begin, end) != s.size())) {
            piranha_throw(std::invalid_argument, "invalid Kronecker monomial");
        }
    }
    /// Constructor from set of symbols.
    /**
     * After construction all exponents in the monomial will be zero.
     *
     * @param[in] args reference set of piranha::symbol.
     *
     * @throws unspecified any exception thrown by:
     * - piranha::kronecker_array::encode(),
     * - piranha::static_vector::push_back().
     */
    explicit kronecker_monomial(const symbol_set &args)
    {
        // NOTE: this does incur in some overhead, but on the other hand it runs all sorts of
        // checks on the size of args, the size of tmp, the encoding limits, etc. Probably it is
        // better to leave it like this at the moment, unless it becomes a serious bottleneck.
        v_type tmp;
        for (auto it = args.begin(); it != args.end(); ++it) {
            tmp.push_back(value_type(0));
        }
        m_value = ka::encode(tmp);
    }
    /// Converting constructor.
    /**
     * This constructor is for use when converting from one term type to another in piranha::series. It will
     * set the internal integer instance to the same value of \p other, after having checked that
     * \p other is compatible with \p args.
     *
     * @param[in] other construction argument.
     * @param[in] args reference set of piranha::symbol.
     *
     * @throws std::invalid_argument if \p other is not compatible with \p args.
     */
    explicit kronecker_monomial(const kronecker_monomial &other, const symbol_set &args) : m_value(other.m_value)
    {
        if (unlikely(!other.is_compatible(args))) {
            piranha_throw(std::invalid_argument, "incompatible arguments");
        }
    }
    /// Constructor from \p value_type.
    /**
     * This constructor will initialise the internal integer instance
     * to \p n.
     *
     * @param[in] n initializer for the internal integer instance.
     */
    explicit kronecker_monomial(const value_type &n) : m_value(n)
    {
    }
    /// Trivial destructor.
    ~kronecker_monomial()
    {
        PIRANHA_TT_CHECK(is_key, kronecker_monomial);
        PIRANHA_TT_CHECK(key_has_degree, kronecker_monomial);
        PIRANHA_TT_CHECK(key_has_ldegree, kronecker_monomial);
        PIRANHA_TT_CHECK(key_is_differentiable, kronecker_monomial);
    }
    /// Defaulted copy assignment operator.
    kronecker_monomial &operator=(const kronecker_monomial &) = default;
    /// Defaulted move assignment operator.
    kronecker_monomial &operator=(kronecker_monomial &&) = default;
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
    /// Compatibility check.
    /**
     * Monomial is considered incompatible if any of these conditions holds:
     *
     * - the size of \p args is zero and the internal integer is not zero,
     * - the size of \p args is equal to or larger than the size of the output of
     * piranha::kronecker_array::get_limits(),
     * - the internal integer is not within the limits reported by piranha::kronecker_array::get_limits().
     *
     * Otherwise, the monomial is considered to be compatible for insertion.
     *
     * @param[in] args reference set of piranha::symbol.
     *
     * @return compatibility flag for the monomial.
     */
    bool is_compatible(const symbol_set &args) const noexcept
    {
        // NOTE: the idea here is to avoid unpack()ing for performance reasons: these checks
        // are already part of unpack(), and that's why unpack() is used instead of is_compatible()
        // in other methods.
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
        const auto &l = limits[static_cast<decltype(limits.size())>(s)];
        // Value is compatible if it is within the bounds for the given size.
        return (m_value >= std::get<1u>(l) && m_value <= std::get<2u>(l));
    }
    /// Ignorability check.
    /**
     * A monomial is never considered ignorable.
     *
     * @return \p false.
     */
    bool is_ignorable(const symbol_set &) const noexcept
    {
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
    kronecker_monomial merge_args(const symbol_set &orig_args, const symbol_set &new_args) const
    {
        return kronecker_monomial(detail::km_merge_args<v_type, ka>(orig_args, new_args, m_value));
    }
    /// Check if monomial is unitary.
    /**
     * @param[in] args reference set of piranha::symbol.
     *
     * @return \p true if all the exponents are zero, \p false otherwise.
     *
     * @throws std::invalid_argument if \p this is not compatible with \p args.
     */
    bool is_unitary(const symbol_set &args) const
    {
        if (unlikely(!is_compatible(args))) {
            piranha_throw(std::invalid_argument, "invalid symbol set");
        }
        // A kronecker code will be zero if all components are zero.
        return !m_value;
    }
    /// Degree.
    /**
     * The type returned by this method is the type resulting from the addition of two instances
     * of \p T.
     *
     * @param[in] args reference set of symbols.
     *
     * @return degree of the monomial.
     *
     * @throws std::overflow_error if the computation of the degree overflows.
     * @throws unspecified any exception thrown by unpack().
     */
    degree_type degree(const symbol_set &args) const
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
    degree_type ldegree(const symbol_set &args) const
    {
        return degree(args);
    }
    /// Partial degree.
    /**
     * Partial degree of the monomial: only the symbols at the positions specified by \p p are considered.
     * The type returned by this method is the type resulting from the addition of two instances
     * of \p T.
     *
     * @param[in] p positions of the symbols to be considered in the calculation of the degree.
     * @param[in] args reference set of piranha::symbol.
     *
     * @return the summation of the exponents of the monomial at the positions specified by \p p.
     *
     * @throws std::invalid_argument if \p p is not compatible with \p args.
     * @throws std::overflow_error if the computation of the degree overflows.
     * @throws unspecified any exception thrown by unpack().
     */
    degree_type degree(const symbol_set::positions &p, const symbol_set &args) const
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
    /// Partial low degree (equivalent to the partial degree).
    degree_type ldegree(const symbol_set::positions &p, const symbol_set &args) const
    {
        return degree(p, args);
    }
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
     * @param[out] res return value.
     * @param[in] t1 first argument.
     * @param[in] t2 second argument.
     *
     * @throws unspecified any exception thrown by piranha::math::mul3().
     */
    template <typename Cf, multiply_enabler<Cf> = 0>
    static void multiply(std::array<term<Cf, kronecker_monomial>, multiply_arity> &res,
                         const term<Cf, kronecker_monomial> &t1, const term<Cf, kronecker_monomial> &t2,
                         const symbol_set &)
    {
        auto &t = res[0u];
        // Coefficient first.
        detail::cf_mult_impl(t.m_cf, t1.m_cf, t2.m_cf);
        // Now the key.
        math::add3(t.m_key.m_value, t1.m_key.get_int(), t2.m_key.get_int());
    }
    /// Multiply Kronecker monomials.
    /**
     * Multiply \p a by \p b, storing the result in \p res.
     * No check is performed for overflow of either the limits of the integral type or the limits of the Kronecker
     * codification.
     *
     * @param[out] res return value.
     * @param[in] a first argument.
     * @param[in] b second argument.
     */
    static void multiply(kronecker_monomial &res, const kronecker_monomial &a, const kronecker_monomial &b,
                         const symbol_set &)
    {
        math::add3(res.m_value, a.m_value, b.m_value);
    }
    /// Divide Kronecker monomials.
    /**
     * Divide \p a by \p b, storing the result in \p res.
     * No check is performed for overflow of either the limits of the integral type or the limits of the Kronecker
     * codification.
     *
     * @param[out] res return value.
     * @param[in] a first argument.
     * @param[in] b second argument.
     */
    static void divide(kronecker_monomial &res, const kronecker_monomial &a, const kronecker_monomial &b,
                       const symbol_set &)
    {
        math::sub3(res.m_value, a.m_value, b.m_value);
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
     * @return \p true if the internal integral instance of \p this is equal to the integral instance of \p other,
     * \p false otherwise.
     */
    bool operator==(const kronecker_monomial &other) const
    {
        return m_value == other.m_value;
    }
    /// Inequality operator.
    /**
     * @param[in] other comparison argument.
     *
     * @return the opposite of operator==().
     */
    bool operator!=(const kronecker_monomial &other) const
    {
        return m_value != other.m_value;
    }
    /// Name of the linear argument.
    /**
     * If the monomial is linear in a variable (i.e., all exponents are zero apart from a single unitary
     * exponent), the name of the variable will be returned. Otherwise, an error will be raised.
     *
     * @param[in] args reference set of piranha::symbol.
     *
     * @return name of the linear variable.
     *
     * @throws std::invalid_argument if the monomial is not linear.
     * @throws unspecified any exception thrown by unpack().
     */
    std::string linear_argument(const symbol_set &args) const
    {
        const auto v = unpack(args);
        const auto size = args.size();
        decltype(args.size()) n_linear = 0u, candidate = 0u;
        for (typename v_type::size_type i = 0u; i < size; ++i) {
            integer tmp = safe_cast<integer>(v[i]);
            if (tmp.sign() == 0) {
                continue;
            }
            if (tmp != 1) {
                piranha_throw(std::invalid_argument, "exponent is not unitary");
            }
            candidate = i;
            ++n_linear;
        }
        if (n_linear != 1u) {
            piranha_throw(std::invalid_argument, "monomial is not linear");
        }
        return args[static_cast<decltype(args.size())>(candidate)].get_name();
    }
    /// Exponentiation.
    /**
     * \note
     * This method is enabled only if \p U is multipliable by piranha::integer and the result type can be
     * safely cast back to \p T.
     *
     * Will return a monomial corresponding to \p this raised to the <tt>x</tt>-th power. The exponentiation
     * is computed via the multiplication of the exponents promoted to piranha::integer by \p x. The result will
     * be cast back to \p T via piranha::safe_cast().
     *
     * @param[in] x exponent.
     * @param[in] args reference set of piranha::symbol.
     *
     * @return \p this to the power of \p x.
     *
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - piranha::safe_cast(),
     * - the constructor and multiplication operator of piranha::integer,
     * - piranha::kronecker_array::encode().
     */
    template <typename U, pow_enabler<U> = 0>
    kronecker_monomial pow(const U &x, const symbol_set &args) const
    {
        auto v = unpack(args);
        for (auto &n : v) {
            n = safe_cast<value_type>(integer(n) * x);
        }
        kronecker_monomial retval;
        retval.m_value = ka::encode(v);
        return retval;
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
        const auto tmp = unpack(args);
        piranha_assert(tmp.size() == args.size());
        const value_type zero(0), one(1);
        bool empty_output = true;
        for (decltype(tmp.size()) i = 0u; i < tmp.size(); ++i) {
            if (tmp[i] != zero) {
                if (!empty_output) {
                    os << '*';
                }
                os << args[i].get_name();
                empty_output = false;
                if (tmp[i] != one) {
                    os << "**" << detail::prepare_for_print(tmp[i]);
                }
            }
        }
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
        const auto tmp = unpack(args);
        std::ostringstream oss_num, oss_den, *cur_oss;
        const value_type zero(0), one(1);
        value_type cur_value;
        for (decltype(tmp.size()) i = 0u; i < tmp.size(); ++i) {
            cur_value = tmp[i];
            if (cur_value != zero) {
                // NOTE: here negate() is safe because of the symmetry in kronecker_array.
                cur_oss
                    = (cur_value > zero) ? std::addressof(oss_num) : (math::negate(cur_value), std::addressof(oss_den));
                (*cur_oss) << "{" << args[i].get_name() << "}";
                if (cur_value != one) {
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
     * \p p.
     * The result is a pair consisting of the exponent associated to \p p before differentiation and the monomial itself
     * after differentiation. If \p p is empty or if the exponent associated to it is zero,
     * the returned pair will be <tt>(0,kronecker_monomial{args})</tt>.
     *
     * @param[in] p position of the symbol with respect to which the differentiation will be calculated.
     * @param[in] args reference set of piranha::symbol.
     *
     * @return result of the differentiation.
     *
     * @throws std::invalid_argument if the computation of the derivative causes a negative overflow,
     * or if \p p is incompatible with \p args or it has a size greater than one.
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - piranha::math::is_zero(),
     * - piranha::kronecker_array::encode().
     */
    std::pair<T, kronecker_monomial> partial(const symbol_set::positions &p, const symbol_set &args) const
    {
        auto v = unpack(args);
        // Cannot take derivative wrt more than one variable, and the position of that variable
        // must be compatible with the monomial.
        if (p.size() > 1u || (p.size() == 1u && p.back() >= args.size())) {
            piranha_throw(std::invalid_argument, "invalid size of symbol_set::positions");
        }
        // Derivative wrt a variable not in the monomial: position is empty, or refers to a
        // variable with zero exponent.
        // NOTE: safe to take v.begin() here, as the checks on the positions above ensure
        // there is a valid position and hence the size must be not zero.
        if (!p.size() || math::is_zero(v.begin()[*p.begin()])) {
            return std::make_pair(T(0), kronecker_monomial(args));
        }
        auto v_b = v.begin();
        // Original exponent.
        T n(v_b[*p.begin()]);
        // Decrement the exponent in the monomial.
        if (unlikely(n == std::numeric_limits<T>::min())) {
            piranha_throw(std::invalid_argument, "negative overflow error in the calculation of the "
                                                 "partial derivative of a monomial");
        }
        v_b[*p.begin()] = static_cast<T>(n - T(1));
        kronecker_monomial tmp_km;
        tmp_km.m_value = ka::encode(v);
        return std::make_pair(n, std::move(tmp_km));
    }
    /// Integration.
    /**
     * Will return the antiderivative of \p this with respect to symbol \p s. The result is a pair
     * consisting of the exponent associated to \p s increased by one and the monomial itself
     * after integration. If \p s is not in \p args, the returned monomial will have an extra exponent
     * set to 1 in the same position \p s would have if it were added to \p args.
     *
     * If the exponent corresponding to \p s is -1, an error will be produced.
     *
     * @param[in] s symbol with respect to which the integration will be calculated.
     * @param[in] args reference set of piranha::symbol.
     *
     * @return result of the integration.
     *
     * @throws std::invalid_argument if the exponent associated to \p s is -1 or if the value of an exponent overflows.
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - piranha::math::is_zero(),
     * - piranha::static_vector::push_back(),
     * - piranha::kronecker_array::encode().
     */
    std::pair<T, kronecker_monomial> integrate(const symbol &s, const symbol_set &args) const
    {
        v_type v = unpack(args), retval;
        value_type expo(0), one(1);
        for (min_int<typename v_type::size_type, decltype(args.size())> i = 0u; i < args.size(); ++i) {
            if (math::is_zero(expo) && s < args[i]) {
                // If we went past the position of s in args and still we
                // have not performed the integration, it means that we need to add
                // a new exponent.
                retval.push_back(one);
                expo = one;
            }
            retval.push_back(v[i]);
            if (args[i] == s) {
                // NOTE: here using i is safe: if retval gained an extra exponent in the condition above,
                // we are never going to land here as args[i] is at this point never going to be s.
                if (unlikely(retval[i] == std::numeric_limits<value_type>::max())) {
                    piranha_throw(std::invalid_argument,
                                  "positive overflow error in the calculation of the integral of a monomial");
                }
                retval[i] = static_cast<value_type>(retval[i] + value_type(1));
                if (math::is_zero(retval[i])) {
                    piranha_throw(std::invalid_argument,
                                  "unable to perform monomial integration: negative unitary exponent");
                }
                expo = retval[i];
            }
        }
        // If expo is still zero, it means we need to add a new exponent at the end.
        if (math::is_zero(expo)) {
            retval.push_back(one);
            expo = one;
        }
        return std::make_pair(expo, kronecker_monomial(ka::encode(retval)));
    }
    /// Evaluation.
    /**
     * \note
     * This method is available only if \p U satisfies the following requirements:
     * - it can be used in piranha::symbol_set::positions_map,
     * - it can be used in piranha::math::pow() with the monomial exponents as powers, yielding a type \p eval_type,
     * - \p eval_type is constructible from \p int,
     * - \p eval_type is multipliable in place.
     *
     * The return value will be built by iteratively applying piranha::math::pow() using the values provided
     * by \p pmap as bases and the values in the monomial as exponents. If the size of the monomial is zero, 1 will be
     * returned. If the positions in \p pmap do not reference
     * only and all the exponents in the monomial, an error will be thrown.
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
     * - piranha::math::pow() or the in-place multiplication operator of the return type.
     */
    template <typename U>
    eval_type<U> evaluate(const symbol_set::positions_map<U> &pmap, const symbol_set &args) const
    {
        using return_type = eval_type<U>;
        using size_type = typename v_type::size_type;
        // NOTE: here we can check the pmap size only against args.
        if (unlikely(pmap.size() != args.size() || (pmap.size() && pmap.back().first != pmap.size() - 1u))) {
            piranha_throw(std::invalid_argument, "invalid positions map for evaluation");
        }
        auto v = unpack(args);
        return_type retval(1);
        auto it = pmap.begin();
        for (min_int<size_type, decltype(args.size())> i = 0u; i < args.size(); ++i, ++it) {
            piranha_assert(it != pmap.end() && it->first == i);
            retval *= math::pow(it->second, v[i]);
        }
        piranha_assert(it == pmap.end());
        return retval;
    }
    /// Substitution.
    /**
     * \note
     * This method is enabled only if:
     * - \p U can be raised to the value type, yielding a type \p subs_type,
     * - \p subs_type can be constructed from \p int and it is assignable.
     *
     * The algorithm is equivalent to the one implemented in piranha::monomial::subs().
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
     * - piranha::math::pow(),
     * - piranha::static_vector::push_back(),
     * - piranha::kronecker_array::encode().
     */
    template <typename U>
    std::vector<std::pair<subs_type<U>, kronecker_monomial>> subs(const std::string &s, const U &x,
                                                                  const symbol_set &args) const
    {
        using s_type = subs_type<U>;
        std::vector<std::pair<s_type, kronecker_monomial>> retval;
        const auto v = unpack(args);
        v_type new_v;
        s_type retval_s(1);
        for (min_int<typename v_type::size_type, decltype(args.size())> i = 0u; i < args.size(); ++i) {
            if (args[i].get_name() == s) {
                retval_s = math::pow(x, v[i]);
                new_v.push_back(value_type(0));
            } else {
                new_v.push_back(v[i]);
            }
        }
        piranha_assert(new_v.size() == v.size());
        retval.push_back(std::make_pair(std::move(retval_s), kronecker_monomial(ka::encode(new_v))));
        return retval;
    }
    /// Substitution of integral power.
    /**
     * \note
     * This method is enabled only if:
     * - \p U can be raised to a piranha::integer power, yielding a type \p subs_type,
     * - \p subs_type is constructible from \p int and assignable.
     *
     * This method works in the same way as piranha::monomial::ipow_subs().
     *
     * @param[in] s name of the symbol that will be substituted.
     * @param[in] n power of \p s that will be substituted.
     * @param[in] x quantity that will be substituted in place of \p s to the power of \p n.
     * @param[in] args reference set of piranha::symbol.
     *
     * @return the result of substituting \p x for \p s to the power of \p n.
     *
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - construction and assignment of the return value,
     * - construction of piranha::rational,
     * - piranha::safe_cast(),
     * - piranha::math::pow(),
     * - piranha::static_vector::push_back(),
     * - the in-place subtraction operator of the exponent type,
     * - piranha::kronecker_array::encode().
     */
    template <typename U>
    std::vector<std::pair<ipow_subs_type<U>, kronecker_monomial>> ipow_subs(const std::string &s, const integer &n,
                                                                            const U &x, const symbol_set &args) const
    {
        using s_type = ipow_subs_type<U>;
        const auto v = unpack(args);
        v_type new_v;
        s_type retval_s(1);
        for (min_int<typename v_type::size_type, decltype(args.size())> i = 0u; i < args.size(); ++i) {
            new_v.push_back(v[i]);
            if (args[i].get_name() == s) {
                const rational tmp(safe_cast<integer>(v[i]), n);
                if (tmp >= 1) {
                    const auto tmp_t = static_cast<integer>(tmp);
                    retval_s = math::pow(x, tmp_t);
                    new_v[i] -= tmp_t * n;
                }
            }
        }
        std::vector<std::pair<s_type, kronecker_monomial>> retval;
        retval.push_back(std::make_pair(std::move(retval_s), kronecker_monomial(ka::encode(new_v))));
        return retval;
    }
    /// Identify symbols that can be trimmed.
    /**
     * This method is used in piranha::series::trim(). The input parameter \p candidates
     * contains a set of symbols that are candidates for elimination. The method will remove
     * from \p candidates those symbols whose exponent in \p this is not zero.
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
     * This method will return a copy of \p this with the exponents associated to the symbols
     * in \p trim_args removed.
     *
     * @param[in] trim_args arguments whose exponents will be removed.
     * @param[in] orig_args original arguments set.
     *
     * @return trimmed copy of \p this.
     *
     * @throws unspecified any exception thrown by:
     * - unpack(),
     * - piranha::static_vector::push_back().
     */
    kronecker_monomial trim(const symbol_set &trim_args, const symbol_set &orig_args) const
    {
        return kronecker_monomial(detail::km_trim<v_type, ka>(trim_args, orig_args, m_value));
    }
    /// Comparison operator.
    /**
     * @param[in] other comparison argument.
     *
     * @return \p true if the internal integral value of \p this is less than the internal
     * integral value of \p other, \p false otherwise.
     */
    bool operator<(const kronecker_monomial &other) const
    {
        return m_value < other.m_value;
    }
    /// Extract the vector of exponents.
    /**
     * This method will write into \p out the content of \p this. If necessary, \p out will
     * be resized to match the size of \p args.
     *
     * @param[out] out vector into which the exponents will be copied.
     * @param[in] args reference set of arguments.
     *
     * @throws unspecified any exception thrown by:
     * - piranha::safe_cast(),
     * - the resizing of \p out,
     * - unpack().
     */
    void extract_exponents(std::vector<value_type> &out, const symbol_set &args) const
    {
        using v_size_type = decltype(out.size());
        auto tmp = unpack(args);
        if (unlikely(out.size() != args.size())) {
            out.resize(safe_cast<v_size_type>(args.size()));
        }
        std::copy(tmp.begin(), tmp.end(), out.begin());
    }
    /// Split.
    /**
     * This method will split \p this into two monomials: the second monomial will contain the exponent
     * of the first variable in \p args, the first monomial will contain all the other exponents.
     *
     * @param[in] args reference arguments set.
     *
     * @return a pair of monomials, the second one containing the first exponent, the first one containing all the
     * other exponents.
     *
     * @throws std::invalid_argument if the size of \p args is less than 2.
     * @throws unspecified any exception thrown by unpack() or by the constructor from a range.
     */
    std::pair<kronecker_monomial, kronecker_monomial> split(const symbol_set &args) const
    {
        if (unlikely(args.size() < 2u)) {
            piranha_throw(std::invalid_argument, "only monomials with 2 or more variables can be split");
        }
        auto tmp = unpack(args);
        return std::make_pair(kronecker_monomial(tmp.begin() + 1, tmp.end()), kronecker_monomial(tmp[0u]));
    }
    /// Detect negative exponents.
    /**
     * This method will return \p true if at least one exponent is less than zero, \p false otherwise.
     *
     * @param[in] args reference arguments set.
     *
     * @return \p true if at least one exponent is less than zero, \p false otherwise.
     *
     * @throws unspecified any exception thrown by unpack().
     */
    bool has_negative_exponent(const symbol_set &args) const
    {
        auto tmp = unpack(args);
        return std::any_of(tmp.begin(), tmp.end(), [](const value_type &e) { return e < value_type(0); });
    }

private:
    value_type m_value;
};

/// Alias for piranha::kronecker_monomial with default type.
using k_monomial = kronecker_monomial<>;
}

namespace std
{

/// Specialisation of \p std::hash for piranha::kronecker_monomial.
template <typename T>
struct hash<piranha::kronecker_monomial<T>> {
    /// Result type.
    typedef size_t result_type;
    /// Argument type.
    typedef piranha::kronecker_monomial<T> argument_type;
    /// Hash operator.
    /**
     * @param[in] a argument whose hash value will be computed.
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
