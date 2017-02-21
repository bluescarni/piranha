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

#ifndef PIRANHA_DIVISOR_SERIES_HPP
#define PIRANHA_DIVISOR_SERIES_HPP

#include <algorithm>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <piranha/base_series_multiplier.hpp>
#include <piranha/config.hpp>
#include <piranha/detail/divisor_series_fwd.hpp>
#include <piranha/detail/polynomial_fwd.hpp>
#include <piranha/divisor.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/forwarding.hpp>
#include <piranha/invert.hpp>
#include <piranha/ipow_substitutable_series.hpp>
#include <piranha/is_cf.hpp>
#include <piranha/key_is_multipliable.hpp>
#include <piranha/math.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/power_series.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/series.hpp>
#include <piranha/series_multiplier.hpp>
#include <piranha/substitutable_series.hpp>
#include <piranha/symbol_set.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace detail
{

struct divisor_series_tag {
};

// Type trait to check the key type in divisor_series.
template <typename T>
struct is_divisor_series_key {
    static const bool value = false;
};

template <typename T>
struct is_divisor_series_key<divisor<T>> {
    static const bool value = true;
};

// See the workaround description below.
template <typename T>
struct base_getter {
    using type = typename T::base;
};
}

/// Divisor series.
/**
 * This class represents series in which the keys are divisor (see piranha::divisor) and the coefficient type
 * is generic. This class satisfies the piranha::is_series and piranha::is_cf type traits.
 *
 * ## Type requirements ##
 *
 * \p Cf must be suitable for use in piranha::series as first template argument, \p Key must be an instance
 * of piranha::divisor.
 *
 * ## Exception safety guarantee ##
 *
 * This class provides the same guarantee as the base series type it derives from.
 *
 * ## Move semantics ##
 *
 * Move semantics is equivalent to the move semantics of the base series type it derives from.
 */
template <typename Cf, typename Key>
class divisor_series
    : public power_series<ipow_substitutable_series<substitutable_series<series<Cf, Key, divisor_series<Cf, Key>>,
                                                                         divisor_series<Cf, Key>>,
                                                    divisor_series<Cf, Key>>,
                          divisor_series<Cf, Key>>,
      detail::divisor_series_tag
{
    // NOTE: this is a workaround for GCC < 5. The enabler condition for the special invert() method is a struct defined
    // within this class, and it needs to access the "base" private typedef via the inverse_type<> alias. The enabler
    // condition
    // is not granted access to the private base typedef (erroneously, since it is defined in the scope of this class),
    // so we use this friend helper in order to reach the base typedef.
    template <typename>
    friend struct detail::base_getter;
    PIRANHA_TT_CHECK(detail::is_divisor_series_key, Key);
    using base = power_series<ipow_substitutable_series<substitutable_series<series<Cf, Key, divisor_series<Cf, Key>>,
                                                                             divisor_series<Cf, Key>>,
                                                        divisor_series<Cf, Key>>,
                              divisor_series<Cf, Key>>;
    // Value type of the divisor.
    using dv_type = typename Key::value_type;
    // Partial utils.
    // Handle exponent increase in a safe way.
    template <typename T, enable_if_t<std::is_integral<T>::value, int> = 0>
    static void expo_increase(T &e)
    {
        if (unlikely(e == std::numeric_limits<T>::max())) {
            piranha_throw(std::overflow_error, "overflow in the computation of the partial derivative "
                                               "of a divisor series");
        }
        e = static_cast<T>(e + T(1));
    }
    template <typename T, enable_if_t<!std::is_integral<T>::value, int> = 0>
    static void expo_increase(T &e)
    {
        ++e;
    }
    // Safe computation of the integral multiplier.
    template <typename T, enable_if_t<std::is_integral<T>::value, int> = 0>
    static auto safe_mult(const T &n, const T &m) -> decltype(n * m)
    {
        auto ret(integer(n) * m);
        ret.neg();
        return static_cast<decltype(n * m)>(ret);
    }
    template <typename T, enable_if_t<!std::is_integral<T>::value, int> = 0>
    static auto safe_mult(const T &n, const T &m) -> decltype(-(n * m))
    {
        return -(n * m);
    }
    // This is the first stage - the second part of the chain rule for each term.
    template <typename T>
    using d_partial_type_0
        = decltype(std::declval<const dv_type &>() * std::declval<const dv_type &>() * std::declval<const T &>());
    template <typename T>
    using d_partial_type_1 = decltype(std::declval<const T &>() * std::declval<const d_partial_type_0<T> &>());
    // Requirements on the final type for the first stage.
    template <typename T>
    using d_partial_type = enable_if_t<conjunction<std::is_constructible<d_partial_type_1<T>, d_partial_type_0<T>>,
                                                   std::is_constructible<d_partial_type_1<T>, int>,
                                                   is_addable_in_place<d_partial_type_1<T>>>::value,
                                       d_partial_type_1<T>>;
    template <typename T = divisor_series>
    d_partial_type<T> d_partial_impl(typename T::term_type::key_type &key, const symbol_set::positions &pos) const
    {
        using term_type = typename base::term_type;
        using cf_type = typename term_type::cf_type;
        using key_type = typename term_type::key_type;
        piranha_assert(key.size() != 0u);
        // Construct the first part of the derivative.
        key_type tmp_div;
        // Insert all the terms that depend on the variable, apart from the
        // first one.
        const auto it_f = key.m_container.end();
        auto it = key.m_container.begin(), it_b(it);
        auto first(*it), first_copy(*it);
        // Size type of the multipliers' vector.
        using vs_type = decltype(first.v.size());
        ++it;
        for (; it != it_f; ++it) {
            tmp_div.m_container.insert(*it);
        }
        // Remove the first term from the original key.
        key.m_container.erase(it_b);
        // Extract from the first dependent term the aij and the exponent, and multiply+negate them.
        const auto mult = safe_mult(first.e, first.v[static_cast<vs_type>(pos.back())]);
        // Increase by one the exponent of the first dep. term.
        expo_increase(first.e);
        // Insert the modified first term. Don't move, as we need first below.
        tmp_div.m_container.insert(first);
        // Now build the first part of the derivative.
        divisor_series tmp_ds;
        tmp_ds.set_symbol_set(this->m_symbol_set);
        tmp_ds.insert(term_type(cf_type(1), std::move(tmp_div)));
        // Init the retval.
        d_partial_type<T> retval(mult * tmp_ds);
        // Now the second part of the derivative, if appropriate.
        if (!key.m_container.empty()) {
            // Build a series with only the first dependent term and unitary coefficient.
            key_type tmp_div_01;
            tmp_div_01.m_container.insert(std::move(first_copy));
            divisor_series tmp_ds_01;
            tmp_ds_01.set_symbol_set(this->m_symbol_set);
            tmp_ds_01.insert(term_type(cf_type(1), std::move(tmp_div_01)));
            // Recurse.
            retval += tmp_ds_01 * d_partial_impl(key, pos);
        }
        return retval;
    }
    template <typename T = divisor_series>
    d_partial_type<T> divisor_partial(const typename T::term_type &term, const symbol_set::positions &pos) const
    {
        using term_type = typename base::term_type;
        // Return zero if the variable is not in the series.
        if (pos.size() == 0u) {
            return d_partial_type<T>(0);
        }
        // Initial split of the divisor.
        auto sd = term.m_key.split(pos, this->m_symbol_set);
        // If the variable is not in the divisor, just return zero.
        if (sd.first.size() == 0u) {
            return d_partial_type<T>(0);
        }
        // Init the constant part of the derivative: the coefficient and the part of the divisor
        // which does not depend on the variable.
        divisor_series tmp_ds;
        tmp_ds.set_symbol_set(this->m_symbol_set);
        tmp_ds.insert(term_type(term.m_cf, std::move(sd.second)));
        // Construct and return the result.
        return tmp_ds * d_partial_impl(sd.first, pos);
    }
    // The final type.
    template <typename T>
    using partial_type_ = decltype(
        math::partial(std::declval<const typename T::term_type::cf_type &>(), std::declval<const std::string &>())
            * std::declval<const T &>()
        + std::declval<const d_partial_type<T> &>());
    template <typename T>
    using partial_type = enable_if_t<conjunction<std::is_constructible<partial_type_<T>, int>,
                                                 is_addable_in_place<partial_type_<T>>>::value,
                                     partial_type_<T>>;
    // Integrate utils.
    template <typename T>
    using integrate_type_ = decltype(
        math::integrate(std::declval<const typename T::term_type::cf_type &>(), std::declval<const std::string &>())
        * std::declval<const T &>());
    template <typename T>
    using integrate_type = enable_if_t<conjunction<std::is_constructible<integrate_type_<T>, int>,
                                                   is_addable_in_place<integrate_type_<T>>>::value,
                                       integrate_type_<T>>;
    // Invert utils.
    // Type coming out of invert() for the base type. This will also be the final type.
    template <typename T>
    using inverse_type = decltype(math::invert(std::declval<const typename detail::base_getter<T>::type &>()));
    // Enabler for the special inversion algorithm. We need a polynomial in the coefficient hierarchy
    // and the inversion type must support the operations needed in the algorithm.
    template <typename T, typename = void>
    struct has_special_invert {
        static constexpr bool value = false;
    };
    template <typename T>
    struct has_special_invert<T,
                              enable_if_t<conjunction<detail::poly_in_cf<T>,
                                                      std::is_constructible<inverse_type<T>,
                                                                            decltype(
                                                                                std::declval<const inverse_type<T> &>()
                                                                                / std::declval<const integer &>())>>::
                                              value>> {
        static constexpr bool value = true;
    };
    // Case 0: Series is not suitable for special invert() implementation. Just forward to the base one, via casting.
    template <typename T = divisor_series, enable_if_t<!has_special_invert<T>::value, int> = 0>
    inverse_type<T> invert_impl() const
    {
        return math::invert(*static_cast<const base *>(this));
    }
    // Case 1: Series is suitable for special invert() implementation. This can fail at runtime depending on what is
    // contained in the coefficients. The return type is the same as the base one.
    template <typename T = divisor_series, enable_if_t<has_special_invert<T>::value, int> = 0>
    inverse_type<T> invert_impl() const
    {
        return special_invert<inverse_type<T>>(*this);
    }
    // Special invert() implementation when we have reached the first polynomial coefficient in the hierarchy.
    template <typename RetT, typename T,
              enable_if_t<std::is_base_of<detail::polynomial_tag, typename T::term_type::cf_type>::value, int> = 0>
    RetT special_invert(const T &s) const
    {
        if (s.is_single_coefficient() && !s.empty()) {
            try {
                using term_type = typename RetT::term_type;
                using cf_type = typename term_type::cf_type;
                using key_type = typename term_type::key_type;
                // Extract the linear combination from the poly coefficient.
                auto lc = s._container().begin()->m_cf.integral_combination();
                // NOTE: lc cannot be empty as we are coming in with a non-zero polynomial.
                piranha_assert(!lc.empty());
                symbol_set ss(lc.begin(), lc.end(),
                              [](const typename decltype(lc)::value_type &p) { return symbol(p.first); });
                // NOTE: lc contains unique strings, so the symbol_set should not contain any duplicate.
                piranha_assert(ss.size() == lc.size());
                std::vector<integer> v_int;
                std::transform(lc.begin(), lc.end(), std::back_inserter(v_int),
                               [](const typename decltype(lc)::value_type &p) { return p.second; });
                // We need to canonicalise the term: switch the sign if the first
                // nonzero element is negative, and divide by the common denom.
                bool first_nonzero_found = false, need_negate = false;
                integer cd;
                for (auto &n : v_int) {
                    if (!first_nonzero_found && !math::is_zero(n)) {
                        if (n < 0) {
                            need_negate = true;
                        }
                        first_nonzero_found = true;
                    }
                    if (need_negate) {
                        math::negate(n);
                    }
                    // NOTE: gcd(0,n) == n for all n, zero included.
                    // NOTE: the gcd computation here is safe as we are operating on integers.
                    math::gcd3(cd, cd, n);
                }
                // GCD on integers should always return non-negative numbers, and cd should never be zero: if all
                // elements in v_int are zero, we would not have been able to extract the linear combination.
                piranha_assert(cd.sgn() > 0);
                // Divide by the cd.
                for (auto &n : v_int) {
                    n /= cd;
                }
                // Now build the key.
                key_type tmp_key;
                tmp_key.insert(v_int.begin(), v_int.end(), integer{1});
                // The return value.
                RetT retval;
                retval.set_symbol_set(ss);
                retval.insert(term_type(cf_type(1), std::move(tmp_key)));
                // If we negated in the canonicalisation, we need to re-negate
                // the common divisor before the final division.
                if (need_negate) {
                    math::negate(cd);
                }
                return RetT(retval / cd);
            } catch (const std::invalid_argument &) {
                // Interpret invalid_argument as a failure in extracting integral combination,
                // and move on.
            }
        }
        return math::invert(*static_cast<const base *>(this));
    }
    // The coefficient is not a polynomial: recurse to the inner coefficient type, if the current coefficient type
    // is suitable.
    template <typename RetT, typename T,
              enable_if_t<!std::is_base_of<detail::polynomial_tag, typename T::term_type::cf_type>::value, int> = 0>
    RetT special_invert(const T &s) const
    {
        if (s.is_single_coefficient() && !s.empty()) {
            return special_invert<RetT>(s._container().begin()->m_cf);
        }
        return math::invert(*static_cast<const base *>(this));
    }

public:
    /// Series rebind alias.
    template <typename Cf2>
    using rebind = divisor_series<Cf2, Key>;
    /// Defaulted default constructor.
    divisor_series() = default;
    /// Defaulted copy constructor.
    divisor_series(const divisor_series &) = default;
    /// Defaulted move constructor.
    divisor_series(divisor_series &&) = default;
    PIRANHA_FORWARDING_CTOR(divisor_series, base)
    /// Trivial destructor.
    ~divisor_series()
    {
        PIRANHA_TT_CHECK(is_series, divisor_series);
        PIRANHA_TT_CHECK(is_cf, divisor_series);
    }
    /// Copy assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     *
     * @throws unspecified any exception thrown by the assignment operator of the base class.
     */
    divisor_series &operator=(const divisor_series &other) = default;
    /// Move assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     */
    divisor_series &operator=(divisor_series &&other) = default;
    PIRANHA_FORWARDING_ASSIGNMENT(divisor_series, base)
    /// Inversion.
    /**
     * \note
     * This template method is enabled only if math::invert() can be called on the class
     * from which piranha::divisor_series derives (i.e., only if the default math::invert()
     * implementation for series is appropriate).
     *
     * This method works exactly like the default implementation of piranha::math::invert() for
     * series types, unless:
     *
     * - a piranha::polynomial appears in the coefficient hierarchy and
     * - the calling series is not empty and
     * - the calling series satisfies piranha::series::is_single_coefficient() and
     * - the return type is divisible by piranha::integer, yielding a type which can be used
     *   to construct the return type.
     *
     * Under these circumstances, this method will attempt to construct a divisor from the first polynomial
     * coefficient encountered in the hierarchy, and, if successful, it will return a piranha::divisor_series
     * consisting of a single term with unitary
     * coefficient and with the key containing the divisor built from the polynomial.
     *
     * For example, inverting the following divisor series with polynomial coefficient
     * \f[
     * 2x+4y
     * \f]
     * will result in the construction of the divisor series
     * \f[
     * \frac{1}{2}\frac{1}{\left(x+2y\right)}.
     * \f]
     *
     * In order for the procedure described above to be successful, the polynomial coefficient must be equivalent
     * to an integral linear combination of symbols with no constant term. If this special inversion fails,
     * a call to the default implementation of piranha::math::invert() will be attempted.
     *
     * @return the inverse of \p this.
     *
     * @throws unspecified any exception thrown by:
     * - the default implementation of piranha::math::invert(),
     * - piranha::series::is_single_coefficient(),
     * - the extraction of an integral linear combination of symbols from \p p,
     * - memory errors in standard containers,
     * - the public interface of piranha::symbol_set,
     * - piranha::math::is_zero(), piranha::math::negate(),
     * - the construction of terms, coefficients and keys,
     * - piranha::divisor::insert(),
     * - piranha::series::insert(), piranha::series::set_symbol_set(),
     * - arithmetics on piranha::divisor_series.
     */
    template <typename T = divisor_series>
    inverse_type<T> invert() const
    {
        return invert_impl();
    }
    /// Partial derivative.
    /**
     * \note
     * This method is enabled only if the algorithm described below is supported by all the
     * involved types.
     *
     * This method will compute the partial derivative of \p this with respect to the symbol called \p name.
     * The derivative is computed via differentiation of the coefficients and the application of the product
     * rule.
     *
     * @param name name of the variable with respect to which the differentiation will be computed.
     *
     * @return the partial derivative of \p this with respect to \p name.
     *
     * @throws std::overflow_error if the manipulation of integral exponents and multipliers in the divisors
     * results in an overflow.
     * @throws unspecified any exception thrown by:
     * - the construction of the return type,
     * - the arithmetics operations needed to compute the result,
     * - the public interface of piranha::symbol_set,
     * - piranha::math::partial(), piranha::math::negate(),
     * - construction and insertion of series terms,
     * - piranha::series::set_symbol_set(),
     * - piranha::divisor::split(),
     * - piranha::hash_set::insert().
     */
    template <typename T = divisor_series>
    partial_type<T> partial(const std::string &name) const
    {
        using term_type = typename base::term_type;
        using cf_type = typename term_type::cf_type;
        partial_type<T> retval(0);
        const auto it_f = this->m_container.end();
        const symbol_set::positions pos(this->m_symbol_set, symbol_set{symbol(name)});
        for (auto it = this->m_container.begin(); it != it_f; ++it) {
            divisor_series tmp;
            tmp.set_symbol_set(this->m_symbol_set);
            tmp.insert(term_type(cf_type(1), it->m_key));
            retval += math::partial(it->m_cf, name) * tmp + divisor_partial(*it, pos);
        }
        return retval;
    }
    /// Integration.
    /**
     * \note
     * This template method is enabled only if the coefficient type is integrable, and the result of the multiplication
     * of an integrated coefficient by a piranha::divisor_series is addable in-place and constructible from \p int.
     *
     * Integration for divisor series is supported only if the coefficient is integrable and no divisor in the calling
     * series depends on the integration variable.
     *
     * @param name name of the variable with respect to which the integration will be performed.
     *
     * @return the antiderivative of \p this with respect to \p name.
     *
     * @throws std::invalid_argument if at least one divisor depends on the integration variable.
     * @throws unspecified any exception thrown by:
     * - construction of and arithmetics on the return type,
     * - the public interface of piranha::symbol and piranha::symbol_set,
     * - piranha::series::set_symbol_set(), piranha::series::insert(),
     * - construction of the coefficients, keys and terms.
     */
    template <typename T = divisor_series>
    integrate_type<T> integrate(const std::string &name) const
    {
        using term_type = typename base::term_type;
        using cf_type = typename term_type::cf_type;
        integrate_type<T> retval(0);
        const auto it_f = this->m_container.end();
        // Turn name into symbol position.
        const symbol_set::positions pos(this->m_symbol_set, symbol_set{symbol(name)});
        for (auto it = this->m_container.begin(); it != it_f; ++it) {
            if (pos.size() == 1u) {
                // If the variable is in the symbol set, then we need to make sure
                // that each multiplier associated to it is zero. Otherwise, the divisor
                // depends on the variable and we cannot perform the integration.
                const auto it2_f = it->m_key.m_container.end();
                for (auto it2 = it->m_key.m_container.begin(); it2 != it2_f; ++it2) {
                    using size_type = decltype(it2->v.size());
                    piranha_assert(pos.back() < it2->v.size());
                    if (unlikely(it2->v[static_cast<size_type>(pos.back())] != 0)) {
                        piranha_throw(std::invalid_argument, "unable to integrate with respect to divisor variables");
                    }
                }
            }
            divisor_series tmp;
            tmp.set_symbol_set(this->m_symbol_set);
            tmp.insert(term_type(cf_type(1), it->m_key));
            retval += math::integrate(it->m_cf, name) * tmp;
        }
        return retval;
    }
};

namespace detail
{

template <typename Series>
using divisor_series_multiplier_enabler = enable_if_t<std::is_base_of<divisor_series_tag, Series>::value>;
}

/// Specialisation of piranha::series_multiplier for piranha::divisor_series.
template <typename Series>
class series_multiplier<Series, detail::divisor_series_multiplier_enabler<Series>>
    : public base_series_multiplier<Series>
{
    using base = base_series_multiplier<Series>;
    template <typename T>
    using call_enabler
        = enable_if_t<key_is_multipliable<typename T::term_type::cf_type, typename T::term_type::key_type>::value, int>;

public:
    /// Inherit base constructors.
    using base::base;
    /// Call operator.
    /**
     * \note
     * This operator is enabled only if the coefficient and key types of \p Series satisfy
     * piranha::key_is_multipliable.
     *
     * The call operator will use base_series_multiplier::plain_multiplication().
     *
     * @return the result of the multiplication.
     *
     * @throws unspecified any exception thrown by base_series_multiplier::plain_multiplication().
     */
    template <typename T = Series, call_enabler<T> = 0>
    Series operator()() const
    {
        return this->plain_multiplication();
    }
};
}

#endif
