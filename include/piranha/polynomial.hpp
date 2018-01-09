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

#ifndef PIRANHA_POLYNOMIAL_HPP
#define PIRANHA_POLYNOMIAL_HPP

#include <algorithm>
#include <boost/numeric/conversion/cast.hpp>
#include <cmath> // For std::ceil.
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <map>
#include <mutex>
#include <numeric>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <piranha/base_series_multiplier.hpp>
#include <piranha/config.hpp>
#include <piranha/debug_access.hpp>
#include <piranha/detail/atomic_flag_array.hpp>
#include <piranha/detail/cf_mult_impl.hpp>
#include <piranha/detail/divisor_series_fwd.hpp>
#include <piranha/detail/parallel_vector_transform.hpp>
#include <piranha/detail/poisson_series_fwd.hpp>
#include <piranha/detail/polynomial_fwd.hpp>
#include <piranha/detail/safe_integral_adder.hpp>
#include <piranha/detail/sfinae_types.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/forwarding.hpp>
#include <piranha/ipow_substitutable_series.hpp>
#include <piranha/is_cf.hpp>
#include <piranha/key_is_multipliable.hpp>
#include <piranha/kronecker_array.hpp>
#include <piranha/kronecker_monomial.hpp>
#include <piranha/math.hpp>
#include <piranha/monomial.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/pow.hpp>
#include <piranha/power_series.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/series.hpp>
#include <piranha/series_multiplier.hpp>
#include <piranha/settings.hpp>
#include <piranha/substitutable_series.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/t_substitutable_series.hpp>
#include <piranha/thread_pool.hpp>
#include <piranha/trigonometric_series.hpp>
#include <piranha/tuning.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace detail
{

// Tag for polynomial instances.
struct polynomial_tag {
};

// Type trait to check the key type in polynomial.
template <typename T>
struct is_polynomial_key {
    static const bool value = false;
};

template <typename T>
struct is_polynomial_key<kronecker_monomial<T>> {
    static const bool value = true;
};

template <typename T, typename U>
struct is_polynomial_key<monomial<T, U>> {
    static const bool value = true;
};

// Implementation detail to check if the monomial key supports the is_linear() method.
template <typename Key>
struct key_has_is_linear {
    template <typename U>
    using is_linear_t = decltype(std::declval<const U &>().is_linear(std::declval<const symbol_fset &>()));
    static const bool value = std::is_same<detected_t<is_linear_t, Key>, std::pair<bool, symbol_idx>>::value;
};
}

/// Polynomial class.
/**
 * This class represents multivariate polynomials as collections of multivariate polynomial terms.
 * \p Cf represents the ring over which the polynomial is defined, while \p Key represents the monomial type.
 *
 * Polynomials support an automatic degree-based truncation mechanism, disabled by default, which comes into play during
 * polynomial multiplication. It allows to discard automatically all those terms, generated during series
 * multiplication, whose total or partial degree is greater than a specified limit. This mechanism can be configured via
 * a set of thread-safe static methods, and it is enabled if:
 * - the total and partial degree of the series are represented by the same type \p D,
 * - all the truncation-related requirements in piranha::power_series are satsified,
 * - the type \p D is equality-comparable, subtractable and the type resulting from the subtraction is still \p D.
 *
 * This class satisfies the piranha::is_series and piranha::is_cf type traits.
 *
 * ## Type requirements ##
 *
 * \p Cf must be suitable for use in piranha::series as first template argument,
 * \p Key must be an instance of either piranha::monomial or piranha::kronecker_monomial.
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
class polynomial
    : public power_series<
          trigonometric_series<ipow_substitutable_series<
              substitutable_series<t_substitutable_series<series<Cf, Key, polynomial<Cf, Key>>, polynomial<Cf, Key>>,
                                   polynomial<Cf, Key>>,
              polynomial<Cf, Key>>>,
          polynomial<Cf, Key>>,
      detail::polynomial_tag
{
    // Check the key.
    PIRANHA_TT_CHECK(detail::is_polynomial_key, Key);
    // Make friend with debug class.
    template <typename>
    friend class debug_access;
    // Make friend with all poly types.
    template <typename, typename>
    friend class polynomial;
    // Make friend with Poisson series.
    template <typename>
    friend class poisson_series;
    // Make friend with divisor series.
    template <typename, typename>
    friend class divisor_series;
    // The base class.
    using base = power_series<
        trigonometric_series<ipow_substitutable_series<
            substitutable_series<t_substitutable_series<series<Cf, Key, polynomial<Cf, Key>>, polynomial<Cf, Key>>,
                                 polynomial<Cf, Key>>,
            polynomial<Cf, Key>>>,
        polynomial<Cf, Key>>;
    // String constructor.
    template <typename Str>
    void construct_from_string(Str &&str)
    {
        using term_type = typename base::term_type;
        // Insert the symbol.
        this->m_symbol_set.emplace_hint(this->m_symbol_set.end(), std::forward<Str>(str));
        // Construct and insert the term.
        this->insert(term_type(Cf(1), typename term_type::key_type{1}));
    }
    template <typename T = Key,
              enable_if_t<conjunction<detail::key_has_is_linear<T>, has_safe_cast<integer, Cf>>::value, int> = 0>
    std::map<std::string, integer> integral_combination() const
    {
        std::map<std::string, integer> retval;
        for (auto it = this->m_container.begin(); it != this->m_container.end(); ++it) {
            const auto p = it->m_key.is_linear(this->m_symbol_set);
            if (unlikely(!p.first)) {
                piranha_throw(std::invalid_argument, "polynomial is not an integral linear combination");
            }
            retval[*(this->m_symbol_set.nth(p.second))] = safe_cast<integer>(it->m_cf);
        }
        return retval;
    }
    template <
        typename T = Key,
        enable_if_t<disjunction<negation<detail::key_has_is_linear<T>>, negation<has_safe_cast<integer, Cf>>>::value,
                    int> = 0>
    std::map<std::string, integer> integral_combination() const
    {
        piranha_throw(std::invalid_argument,
                      "the polynomial type does not support the extraction of a linear combination");
    }
    // Integration utils.
    // Empty for SFINAE.
    template <typename T, typename = void>
    struct integrate_type_ {
    };
    // The type resulting from the integration of the key of series T.
    template <typename T>
    using key_integrate_type
        = decltype(std::declval<const typename T::term_type::key_type &>()
                       .integrate(std::declval<const std::string &>(), std::declval<const symbol_fset &>())
                       .first);
    // Basic integration requirements for series T, to be satisfied both when the coefficient is integrable
    // and when it is not. ResT is the type of the result of the integration.
    template <typename T, typename ResT>
    using basic_integrate_requirements = typename std::enable_if<
        // Coefficient differentiable, and can call is_zero on the result.
        has_is_zero<decltype(math::partial(std::declval<const typename T::term_type::cf_type &>(),
                                           std::declval<const std::string &>()))>::value
        &&
        // The key is integrable.
        detail::true_tt<key_integrate_type<T>>::value &&
        // The result needs to be addable in-place.
        is_addable_in_place<ResT>::value &&
        // It also needs to be ctible from zero.
        std::is_constructible<ResT, int>::value>::type;
    // Non-integrable coefficient.
    template <typename T>
    using nic_res_type = decltype((std::declval<const T &>() * std::declval<const typename T::term_type::cf_type &>())
                                  / std::declval<const key_integrate_type<T> &>());
    template <typename T>
    struct integrate_type_<
        T, typename std::enable_if<!is_integrable<typename T::term_type::cf_type>::value
                                   && detail::true_tt<basic_integrate_requirements<T, nic_res_type<T>>>::value>::type> {
        using type = nic_res_type<T>;
    };
    // Integrable coefficient.
    // The type resulting from the differentiation of the key of series T.
    template <typename T>
    using key_partial_type
        = decltype(std::declval<const typename T::term_type::key_type &>()
                       .partial(std::declval<const symbol_idx &>(), std::declval<const symbol_fset &>())
                       .first);
    // Type resulting from the integration of the coefficient.
    template <typename T>
    using i_cf_type = decltype(
        math::integrate(std::declval<const typename T::term_type::cf_type &>(), std::declval<const std::string &>()));
    // Type above, multiplied by the type coming out of the derivative of the key.
    template <typename T>
    using i_cf_type_p = decltype(std::declval<const i_cf_type<T> &>() * std::declval<const key_partial_type<T> &>());
    // Final series type.
    template <typename T>
    using ic_res_type = decltype(std::declval<const i_cf_type_p<T> &>() * std::declval<const T &>());
    template <typename T>
    struct integrate_type_<
        T, typename std::enable_if<
               is_integrable<typename T::term_type::cf_type>::value
               && detail::true_tt<basic_integrate_requirements<T, ic_res_type<T>>>::value &&
               // We need to be able to add the non-integrable type.
               is_addable_in_place<ic_res_type<T>, nic_res_type<T>>::value &&
               // We need to be able to compute the partial degree and cast it to integer.
               has_safe_cast<integer,
                             decltype(std::declval<const typename T::term_type::key_type &>().degree(
                                 std::declval<const symbol_idx_fset &>(), std::declval<const symbol_fset &>()))>::value
               &&
               // This is required in the initialisation of the return value.
               std::is_constructible<i_cf_type_p<T>, i_cf_type<T>>::value &&
               // We need to be able to assign the integrated coefficient times key partial.
               std::is_assignable<i_cf_type_p<T> &, i_cf_type_p<T>>::value &&
               // Needs math::negate().
               has_negate<i_cf_type_p<T>>::value>::type> {
        using type = ic_res_type<T>;
    };
    // Final typedef.
    template <typename T>
    using integrate_type = typename std::enable_if<is_returnable<typename integrate_type_<T>::type>::value,
                                                   typename integrate_type_<T>::type>::type;
    // Integration with integrable coefficient.
    template <typename T = polynomial>
    integrate_type<T> integrate_impl(const std::string &s, const typename base::term_type &term,
                                     const std::true_type &) const
    {
        typedef typename base::term_type term_type;
        typedef typename term_type::cf_type cf_type;
        typedef typename term_type::key_type key_type;
        // Get the partial degree of the monomial in integral form.
        integer degree;
        const auto idx = symbol_idx_fset{ss_index_of(this->m_symbol_set, s)};
        try {
            // Check if s is actually in the symbol set or not.
            if (*idx.begin() < this->m_symbol_set.size()) {
                degree = safe_cast<integer>(term.m_key.degree(idx, this->m_symbol_set));
            } else {
                degree = 0;
            }
        } catch (const safe_cast_failure &) {
            piranha_throw(std::invalid_argument,
                          "unable to perform polynomial integration: cannot extract the integral form of an exponent");
        }
        // If the degree is negative, integration by parts won't terminate.
        if (degree.sgn() < 0) {
            piranha_throw(std::invalid_argument,
                          "unable to perform polynomial integration: negative integral exponent");
        }
        polynomial tmp;
        tmp.set_symbol_set(this->m_symbol_set);
        key_type tmp_key = term.m_key;
        tmp.insert(term_type(cf_type(1), tmp_key));
        i_cf_type_p<T> i_cf(math::integrate(term.m_cf, s));
        integrate_type<T> retval(i_cf * tmp);
        for (integer i(1); i <= degree; ++i) {
            // Update coefficient and key. These variables are persistent across loop iterations.
            auto partial_key = tmp_key.partial(*idx.begin(), this->m_symbol_set);
            i_cf = math::integrate(i_cf, s) * std::move(partial_key.first);
            // Account for (-1)**i.
            math::negate(i_cf);
            // Build the other factor from the derivative of the monomial.
            tmp = polynomial{};
            tmp.set_symbol_set(this->m_symbol_set);
            tmp_key = std::move(partial_key.second);
            // NOTE: don't move tmp_key, as it needs to hold a valid value
            // for the next loop iteration.
            tmp.insert(term_type(cf_type(1), tmp_key));
            retval += i_cf * tmp;
        }
        return retval;
    }
    // Integration with non-integrable coefficient.
    template <typename T = polynomial>
    integrate_type<T> integrate_impl(const std::string &, const typename base::term_type &,
                                     const std::false_type &) const
    {
        piranha_throw(std::invalid_argument,
                      "unable to perform polynomial integration: coefficient type is not integrable");
    }
    // Template alias for use in pow() overload. Will check via SFINAE that the base pow() method can be called with
    // argument T and that exponentiation of key type is legal.
    template <typename T>
    using key_pow_t
        = decltype(std::declval<Key const &>().pow(std::declval<const T &>(), std::declval<const symbol_fset &>()));
    template <typename T>
    using pow_ret_type = enable_if_t<is_detected<key_pow_t, T>::value,
                                     decltype(std::declval<series<Cf, Key, polynomial<Cf, Key>> const &>().pow(
                                         std::declval<const T &>()))>;
    // Invert utils.
    template <typename Series>
    using inverse_type = decltype(std::declval<const Series &>().pow(-1));
    // Auto-truncation machinery.
    // The degree and partial degree types, detected via math::degree().
    template <typename T>
    using degree_type = decltype(math::degree(std::declval<const T &>()));
    template <typename T>
    using pdegree_type = decltype(math::degree(std::declval<const T &>(), std::declval<const symbol_fset &>()));
    // Enablers for auto-truncation: degree and partial degree must be the same, series must support
    // math::truncate_degree(), degree type must be subtractable and yield the same type.
    template <typename T>
    using at_degree_enabler = typename std::enable_if<
        std::is_same<degree_type<T>, pdegree_type<T>>::value && has_truncate_degree<T, degree_type<T>>::value
            && std::is_same<decltype(std::declval<const degree_type<T> &>() - std::declval<const degree_type<T> &>()),
                            degree_type<T>>::value
            && is_equality_comparable<degree_type<T>>::value,
        int>::type;
    // For the setter, we need the above plus we need to be able to convert safely U to the degree type.
    template <typename T, typename U>
    using at_degree_set_enabler =
        typename std::enable_if<detail::true_tt<at_degree_enabler<T>>::value && has_safe_cast<degree_type<T>, U>::value,
                                int>::type;
    // This needs to be separate from the other static inits because we don't have anything to init
    // if the series does not support degree computation.
    // NOTE: here the important thing is that this method does not return the same object for different series types,
    // as the intent of the truncation mechanism is that each polynomial type has its own settings.
    // We need to keep this in mind if we need static resources that must be unique for the series type, sometimes
    // adding the Derived series as template argument in a toolbox might actually be necessary because of this. Note
    // that, contrary to the, e.g., custom derivatives map in series.hpp here we don't care about the type of T - we
    // just need to be able to extract the term type from it.
    template <typename T = polynomial>
    static degree_type<T> &get_at_degree_max()
    {
        // Init to zero for peace of mind - though this should never be accessed
        // if the auto-truncation is not used.
        static degree_type<T> at_degree_max(0);
        return at_degree_max;
    }
    // Enabler for string construction.
    template <typename Str>
    using str_enabler =
        typename std::enable_if<std::is_same<typename std::decay<Str>::type, std::string>::value
                                    || std::is_same<typename std::decay<Str>::type, char *>::value
                                    || std::is_same<typename std::decay<Str>::type, const char *>::value,
                                int>::type;
    // Implementation of find_cf().
    template <typename T>
    using find_cf_enabler = typename std::enable_if<
        std::is_constructible<typename base::term_type::key_type, decltype(std::begin(std::declval<const T &>())),
                              decltype(std::end(std::declval<const T &>())), const symbol_fset &>::value
            && has_input_begin_end<const T>::value,
        int>::type;
    template <typename T>
    using find_cf_init_list_enabler = find_cf_enabler<std::initializer_list<T>>;
    template <typename Iterator>
    Cf find_cf_impl(Iterator begin, Iterator end) const
    {
        typename base::term_type tmp_term{Cf(0), Key(begin, end, this->m_symbol_set)};
        auto it = this->m_container.find(tmp_term);
        if (it == this->m_container.end()) {
            return Cf(0);
        }
        return it->m_cf;
    }
    // Enabler for untruncated multiplication.
    template <typename T>
    using um_enabler =
        typename std::enable_if<std::is_same<T, decltype(std::declval<const T &>() * std::declval<const T &>())>::value,
                                int>::type;
    // Enabler for truncated multiplication.
    template <typename T, typename U>
    using tm_enabler =
        typename std::enable_if<std::is_same<T, decltype(std::declval<const T &>() * std::declval<const T &>())>::value
                                    && has_safe_cast<degree_type<T>, U>::value
                                    && detail::true_tt<at_degree_enabler<T>>::value,
                                int>::type;
    // Common bits for truncated/untruncated multiplication. Will do the usual merging of the symbol sets
    // before calling the runner functor, which performs the actual multiplication.
    template <typename Functor>
    static polynomial um_tm_implementation(const polynomial &p1, const polynomial &p2, const Functor &runner)
    {
        series_merge_f(p1, p2, runner);
    }
    // Helper function to clear the pow cache when a new auto truncation limit is set.
    template <typename T>
    static void truncation_clear_pow_cache(int mode, const T &max_degree, const symbol_fset &names)
    {
        // The pow cache is cleared only if we are actually changing the truncation settings.
        if (s_at_degree_mode != mode || get_at_degree_max() != max_degree || names != s_at_degree_names) {
            polynomial::clear_pow_cache();
        }
    }

public:
    /// Series rebind alias.
    template <typename Cf2>
    using rebind = polynomial<Cf2, Key>;
    /// Defaulted default constructor.
    /**
     * Will construct a polynomial with zero terms.
     */
    polynomial() = default;
    /// Defaulted copy constructor.
    polynomial(const polynomial &) = default;
    /// Defaulted move constructor.
    polynomial(polynomial &&) = default;
    /// Constructor from symbol name.
    /**
     * \note
     * This template constructor is enabled only if the decay type of \p Str is a C or C++ string.
     *
     * Will construct a univariate polynomial made of a single term with unitary coefficient and exponent, representing
     * the symbolic variable \p name. The type of \p name must be a string type (either C or C++).
     *
     * @param name name of the symbolic variable that the polynomial will represent.
     *
     * @throws unspecified any exception thrown by:
     * - the public interface of piranha::symbol_fset,
     * - the invoked constructor of the coefficient type,
     * - the invoked constructor of the key type,
     * - the constructor of the term type from coefficient and key,
     * - piranha::series::insert().
     */
    template <typename Str, str_enabler<Str> = 0>
    explicit polynomial(Str &&name) : base()
    {
        construct_from_string(std::forward<Str>(name));
    }
    PIRANHA_FORWARDING_CTOR(polynomial, base)
    /// Trivial destructor.
    ~polynomial()
    {
        PIRANHA_TT_CHECK(is_cf, polynomial);
        PIRANHA_TT_CHECK(is_series, polynomial);
    }
    /// Copy assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     *
     * @throws unspecified any exception thrown by the assignment operator of the base class.
     */
    polynomial &operator=(const polynomial &other) = default;
    /// Move assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     */
    polynomial &operator=(polynomial &&other) = default;
    PIRANHA_FORWARDING_ASSIGNMENT(polynomial, base)
    /// Override default exponentiation method.
    /**
     * \note
     * This method is enabled only if piranha::series::pow() can be called with exponent \p x
     * and the key type can be raised to the power of \p x via its exponentiation method.
     *
     * This exponentiation override will check if the polynomial consists of a single-term with non-unitary
     * key. In that case, the return polynomial will consist of a single term with coefficient computed via
     * piranha::math::pow() and key computed via the monomial exponentiation method. Otherwise, the base
     * (i.e., default) exponentiation method will be used.
     *
     * @param x exponent.
     *
     * @return \p this to the power of \p x.
     *
     * @throws unspecified any exception thrown by:
     * - the <tt>is_unitary()</tt> and exponentiation methods of the key type,
     * - piranha::math::pow(),
     * - construction of coefficient, key and term,
     * - piranha::series::insert() , piranha::series::set_symbol_set() and piranha::series::pow().
     */
    template <typename T>
    pow_ret_type<T> pow(const T &x) const
    {
        using ret_type = pow_ret_type<T>;
        typedef typename ret_type::term_type term_type;
        typedef typename term_type::cf_type cf_type;
        typedef typename term_type::key_type key_type;
        if (this->size() == 1u && !this->m_container.begin()->m_key.is_unitary(this->m_symbol_set)) {
            cf_type cf(math::pow(this->m_container.begin()->m_cf, x));
            key_type key(this->m_container.begin()->m_key.pow(x, this->m_symbol_set));
            ret_type retval;
            retval.set_symbol_set(this->m_symbol_set);
            retval.insert(term_type(std::move(cf), std::move(key)));
            return retval;
        }
        return static_cast<series<Cf, Key, polynomial<Cf, Key>> const *>(this)->pow(x);
    }
    /// Inversion.
    /**
     * \note
     * This method is enabled only if <tt>piranha::polynomial::pow(-1)</tt> is a well-formed
     * expression.
     *
     * @return the calling polynomial raised to -1 using piranha::polynomial::pow().
     *
     * @throws unspecified any exception thrown by piranha::polynomial::pow().
     */
    template <typename Series = polynomial>
    inverse_type<Series> invert() const
    {
        return this->pow(-1);
    }
    /// Integration.
    /**
     * \note
     * This method is enabled only if the algorithm described below is supported by all the involved types.
     *
     * This method will attempt to compute the antiderivative of the polynomial term by term. If the term's coefficient
     * does not depend on
     * the integration variable, the result will be calculated via the integration of the corresponding monomial.
     * Integration with respect to a variable appearing to the power of -1 will fail.
     *
     * Otherwise, a strategy of integration by parts is attempted, its success depending on the integrability
     * of the coefficient and on the value of the exponent of the integration variable. The integration will
     * fail if the exponent is negative or non-integral.
     *
     * @param name integration variable.
     *
     * @return the antiderivative of \p this with respect to \p name.
     *
     * @throws std::invalid_argument if the integration procedure fails.
     * @throws unspecified any exception thrown by:
     * - the public interface of piranha::symbol_fset,
     * - piranha::math::partial(), piranha::math::is_zero(), piranha::math::integrate(), piranha::safe_cast()
     *   and piranha::math::negate(),
     * - term construction,
     * - coefficient construction, assignment and arithmetics,
     * - integration, construction, assignment, differentiation and degree querying methods of the key type,
     * - insert(),
     * - series arithmetics.
     */
    template <typename T = polynomial>
    integrate_type<T> integrate(const std::string &name) const
    {
        typedef typename base::term_type term_type;
        typedef typename term_type::cf_type cf_type;
        integrate_type<T> retval(0);
        // A copy of the current symbol set plus name. If name is
        // in the set already, it will be just a copy.
        const auto aug_ss = [this, &name]() -> symbol_fset {
            symbol_fset retval(this->m_symbol_set);
            retval.insert(name);
            return retval;
        }();
        const auto it_f = this->m_container.end();
        for (auto it = this->m_container.begin(); it != it_f; ++it) {
            // If the derivative of the coefficient is null, we just need to deal with
            // the integration of the key.
            if (math::is_zero(math::partial(it->m_cf, name))) {
                polynomial tmp;
                tmp.set_symbol_set(aug_ss);
                auto key_int = it->m_key.integrate(name, this->m_symbol_set);
                tmp.insert(term_type(cf_type(1), std::move(key_int.second)));
                retval += (tmp * it->m_cf) / key_int.first;
            } else {
                retval += integrate_impl(name, *it, std::integral_constant<bool, is_integrable<cf_type>::value>{});
            }
        }
        return retval;
    }
    /// Set total-degree-based auto-truncation.
    /**
     * \note
     * This method is available only if the requisites outlined in piranha::polynomial are satisfied
     * and if \p U can be safely cast to the degree type.
     *
     * Setup the degree-based auto-truncation mechanism to truncate according to the total maximum degree.
     * If the new auto truncation settings are different from the currently active ones, the natural power cache
     * defined in piranha::series will be cleared.
     *
     * @param max_degree maximum total degree that will be retained during automatic truncation.
     *
     * @throws unspecified any exception thrown by:
     * - threading primitives,
     * - piranha::safe_cast(),
     * - the constructor of the degree type.
     */
    template <typename U, typename T = polynomial, at_degree_set_enabler<T, U> = 0>
    static void set_auto_truncate_degree(const U &max_degree)
    {
        // Init out for exception safety.
        auto new_degree = safe_cast<degree_type<T>>(max_degree);
        // Initialisation of function-level statics is thread-safe, no need to lock. We get
        // a ref before the lock because the initialisation of the static could throw in principle,
        // and we want the section after the lock to be exception-free.
        auto &at_dm = get_at_degree_max();
        std::lock_guard<std::mutex> lock(s_at_degree_mutex);
        // NOTE: here in principle there could be an exception thrown as a consequence of the degree comparison.
        // This is not a problem as at this stage no setting has been modified.
        truncation_clear_pow_cache(1, new_degree, symbol_fset{});
        s_at_degree_mode = 1;
        // NOTE: the degree type of polys satisfies is_container_element, so move assignment is noexcept.
        at_dm = std::move(new_degree);
        // This should not throw (a vector of strings, destructors and deallocation should be noexcept).
        s_at_degree_names.clear();
    }
    /// Set partial-degree-based auto-truncation.
    /**
     * \note
     * This method is available only if the requisites outlined in piranha::polynomial are satisfied
     * and if \p U can be safely cast to the degree type.
     *
     * Setup the degree-based auto-truncation mechanism to truncate according to the partial degree.
     * If the new auto truncation settings are different from the currently active ones, the natural power cache
     * defined in piranha::series will be cleared.
     *
     * @param max_degree maximum partial degree that will be retained during automatic truncation.
     * @param names names of the variables that will be considered during the computation of the
     * partial degree.
     *
     * @throws unspecified any exception thrown by:
     * - threading primitives,
     * - piranha::safe_cast(),
     * - the constructor of the degree type,
     * - memory allocation errors in standard containers.
     */
    template <typename U, typename T = polynomial, at_degree_set_enabler<T, U> = 0>
    static void set_auto_truncate_degree(const U &max_degree, const symbol_fset &names)
    {
        // Copy+move for exception safety.
        auto new_degree = safe_cast<degree_type<T>>(max_degree);
        auto new_names = names;
        auto &at_dm = get_at_degree_max();
        std::lock_guard<std::mutex> lock(s_at_degree_mutex);
        truncation_clear_pow_cache(2, new_degree, new_names);
        s_at_degree_mode = 2;
        at_dm = std::move(new_degree);
        s_at_degree_names = std::move(new_names);
    }
    /// Disable degree-based auto-truncation.
    /**
     * \note
     * This method is available only if the requisites outlined in piranha::polynomial are satisfied.
     *
     * Disable the degree-based auto-truncation mechanism.
     *
     * @throws unspecified any exception thrown by:
     * - threading primitives,
     * - the constructor of the degree type,
     * - memory allocation errors in standard containers.
     */
    template <typename T = polynomial, at_degree_enabler<T> = 0>
    static void unset_auto_truncate_degree()
    {
        degree_type<T> new_degree(0);
        auto &at_dm = get_at_degree_max();
        std::lock_guard<std::mutex> lock(s_at_degree_mutex);
        s_at_degree_mode = 0;
        at_dm = std::move(new_degree);
        s_at_degree_names.clear();
    }
    /// Query the status of the degree-based auto-truncation mechanism.
    /**
     * \note
     * This method is available only if the requisites outlined in piranha::polynomial are satisfied.
     *
     * This method will return a tuple of three elements describing the status of the degree-based auto-truncation
     * mechanism.
     * The elements of the tuple have the following meaning:
     * - truncation mode (0 if disabled, 1 for total-degree truncation and 2 for partial-degree truncation),
     * - the maximum degree allowed,
     * - the list of names to be considered for partial truncation.
     *
     * @return a tuple representing the status of the degree-based auto-truncation mechanism.
     *
     * @throws unspecified any exception thrown by threading primitives or by the involved constructors.
     */
    template <typename T = polynomial, at_degree_enabler<T> = 0>
    static std::tuple<int, degree_type<T>, symbol_fset> get_auto_truncate_degree()
    {
        std::lock_guard<std::mutex> lock(s_at_degree_mutex);
        return std::make_tuple(s_at_degree_mode, get_at_degree_max(), s_at_degree_names);
    }
    /// Find coefficient.
    /**
     * \note
     * This method is enabled only if:
     * - \p T satisfies piranha::has_input_begin_end,
     * - \p Key can be constructed from the begin/end iterators of \p c and a piranha::symbol_fset.
     *
     * This method will first construct a term with zero coefficient and key initialised from the begin/end iterators
     * of \p c and the symbol set of \p this, and it will then try to locate the term inside \p this.
     * If the term is found, its coefficient will be returned. Otherwise, a coefficient initialised
     * from 0 will be returned.
     *
     * @param c the container that will be used to construct the \p Key to be located.
     *
     * @returns the coefficient of the term whose \p Key corresponds to \p c if such term exists,
     * zero otherwise.
     *
     * @throws unspecified any exception thrown by:
     * - term, coefficient and key construction,
     * - piranha::hash_set::find().
     */
    template <typename T, find_cf_enabler<T> = 0>
    Cf find_cf(const T &c) const
    {
        return find_cf_impl(std::begin(c), std::end(c));
    }
    /// Find coefficient.
    /**
     * \note
     * This method is enabled only if \p Key can be constructed from the begin/end iterators of \p l and a
     * piranha::symbol_fset.
     *
     * This method is identical to the other overload with the same name, and it is provided for convenience.
     *
     * @param l the list that will be used to construct the \p Key to be located.
     *
     * @returns the coefficient of the term whose \p Key corresponds to \p l if such term exists,
     * zero otherwise.
     *
     * @throws unspecified any exception thrown by the other overload.
     */
    template <typename T, find_cf_init_list_enabler<T> = 0>
    Cf find_cf(std::initializer_list<T> l) const
    {
        return find_cf_impl(std::begin(l), std::end(l));
    }
    /// Untruncated multiplication.
    /**
     * \note
     * This function template is enabled only if the calling piranha::polynomial satisfies piranha::is_multipliable,
     * returning the calling piranha::polynomial as return type.
     *
     * This function will return the product of \p p1 and \p p2, computed without truncation (regardless
     * of the current automatic truncation settings). Note that this function is
     * available only if the operands are of the same type and no type promotions affect the coefficient types
     * during multiplication.
     *
     * @param p1 the first operand.
     * @param p2 the second operand.
     *
     * @return the product of \p p1 and \p p2.
     *
     * @throws unspecified any exception thrown by:
     * - the public interface of the specialisation of piranha::series_multiplier for piranha::polynomial,
     * - the public interface of piranha::symbol_fset,
     * - the public interface of piranha::series.
     */
    template <typename T = polynomial, um_enabler<T> = 0>
    static polynomial untruncated_multiplication(const polynomial &p1, const polynomial &p2)
    {
        auto runner = [](const polynomial &a, const polynomial &b) {
            return series_multiplier<polynomial>(a, b)._untruncated_multiplication();
        };
        return um_tm_implementation(p1, p2, runner);
    }
    /// Truncated multiplication (total degree).
    /**
     * \note
     * This function template is enabled only if the following conditions hold:
     * - the calling piranha::polynomial satisfies piranha::is_multipliable, returning the calling piranha::polynomial
     *   as return type,
     * - the requirements for truncated multiplication outlined in piranha::polynomial are satisfied,
     * - \p U can be safely cast to the degree type of the calling piranha::polynomial.
     *
     * This function will return the product of \p p1 and \p p2, truncated to the maximum total degree
     * of \p max_degree (regardless of the current automatic truncation settings).
     * Note that this function is
     * available only if the operands are of the same type and no type promotions affect the coefficient types
     * during multiplication.
     *
     * @param p1 the first operand.
     * @param p2 the second operand.
     * @param max_degree the maximum total degree in the result.
     *
     * @return the truncated product of \p p1 and \p p2.
     *
     * @throws unspecified any exception thrown by:
     * - the public interface of the specialisation of piranha::series_multiplier for piranha::polynomial,
     * - the public interface of piranha::symbol_fset,
     * - the public interface of piranha::series,
     * - piranha::safe_cast().
     */
    template <typename U, typename T = polynomial, tm_enabler<T, U> = 0>
    static polynomial truncated_multiplication(const polynomial &p1, const polynomial &p2, const U &max_degree)
    {
        // NOTE: these 2 implementations may be rolled into one once we can safely capture variadic arguments
        // in lambdas.
        auto runner = [&max_degree](const polynomial &a, const polynomial &b) {
            return series_multiplier<polynomial>(a, b)._truncated_multiplication(safe_cast<degree_type<T>>(max_degree));
        };
        return um_tm_implementation(p1, p2, runner);
    }
    /// Truncated multiplication (partial degree).
    /**
     * \note
     * This function template is enabled only if the following conditions hold:
     * - the calling piranha::polynomial satisfies piranha::is_multipliable, returning the calling piranha::polynomial
     *   as return type,
     * - the requirements for truncated multiplication outlined in piranha::polynomial are satisfied,
     * - \p U can be safely cast to the degree type of the calling piranha::polynomial.
     *
     * This function will return the product of \p p1 and \p p2, truncated to the maximum partial degree
     * of \p max_degree (regardless of the current automatic truncation settings). Note that this function is
     * available only if the operands are of the same type and no type promotions affect the coefficient types
     * during multiplication.
     *
     * @param p1 the first operand.
     * @param p2 the second operand.
     * @param max_degree the maximum degree in the result.
     * @param names the set of the symbols that will be considered in the computation of the degree.
     *
     * @return the truncated product of \p p1 and \p p2.
     *
     * @throws unspecified any exception thrown by:
     * - the public interface of the specialisation of piranha::series_multiplier for piranha::polynomial,
     * - the public interface of piranha::symbol_fset,
     * - the public interface of piranha::series,
     * - piranha::safe_cast().
     */
    template <typename U, typename T = polynomial, tm_enabler<T, U> = 0>
    static polynomial truncated_multiplication(const polynomial &p1, const polynomial &p2, const U &max_degree,
                                               const symbol_fset &names)
    {
        // NOTE: total and partial degree must be the same.
        auto runner = [&max_degree, &names](const polynomial &a, const polynomial &b) -> polynomial {
            const auto idx = ss_intersect_idx(a.get_symbol_set(), names);
            return series_multiplier<polynomial>(a, b)._truncated_multiplication(safe_cast<degree_type<T>>(max_degree),
                                                                                 names, idx);
        };
        return um_tm_implementation(p1, p2, runner);
    }

private:
    // Static data for auto_truncate_degree.
    static std::mutex s_at_degree_mutex;
    static int s_at_degree_mode;
    static symbol_fset s_at_degree_names;
};

// Static inits.
template <typename Cf, typename Key>
std::mutex polynomial<Cf, Key>::s_at_degree_mutex;

template <typename Cf, typename Key>
int polynomial<Cf, Key>::s_at_degree_mode = 0;

template <typename Cf, typename Key>
symbol_fset polynomial<Cf, Key>::s_at_degree_names;

namespace detail
{

// Identification of key types for dispatching in the multiplier.
template <typename T>
struct is_kronecker_monomial {
    static const bool value = false;
};

template <typename T>
struct is_kronecker_monomial<kronecker_monomial<T>> {
    static const bool value = true;
};

template <typename T>
struct is_monomial {
    static const bool value = false;
};

template <typename T, typename S>
struct is_monomial<monomial<T, S>> {
    static const bool value = true;
};

// Identify the presence of auto-truncation methods in the poly multiplier.
template <typename S, typename T>
class has_set_auto_truncate_degree : sfinae_types
{
    // NOTE: if we have total degree auto truncation, we also have partial degree truncation.
    template <typename S1, typename T1>
    static auto test(const S1 &, const T1 &t) -> decltype(S1::set_auto_truncate_degree(t), void(), yes());
    static no test(...);

public:
    static const bool value = std::is_same<yes, decltype(test(std::declval<S>(), std::declval<T>()))>::value;
};

template <typename S, typename T>
const bool has_set_auto_truncate_degree<S, T>::value;

template <typename S>
class has_get_auto_truncate_degree : sfinae_types
{
    template <typename S1>
    static auto test(const S1 &) -> decltype(S1::get_auto_truncate_degree(), void(), yes());
    static no test(...);

public:
    static const bool value = std::is_same<yes, decltype(test(std::declval<S>()))>::value;
};

template <typename S>
const bool has_get_auto_truncate_degree<S>::value;

// Global enabler for the polynomial multiplier.
template <typename Series>
using poly_multiplier_enabler = typename std::enable_if<std::is_base_of<detail::polynomial_tag, Series>::value>::type;
}

/// Specialisation of piranha::series_multiplier for piranha::polynomial.
/**
 * This specialisation of piranha::series_multiplier is enabled when \p Series is an instance of
 * piranha::polynomial.
 *
 * ## Type requirements ##
 *
 * \p Series must be suitable for use in piranha::base_series_multiplier.
 *
 * ## Exception safety guarantee ##
 *
 * This class provides the same guarantee as piranha::base_series_multiplier.
 *
 * ## Move semantics ##
 *
 * Move semantics is equivalent to piranha::base_series_multiplier's move semantics.
 */
template <typename Series>
class series_multiplier<Series, detail::poly_multiplier_enabler<Series>> : public base_series_multiplier<Series>
{
    // Base multiplier type.
    using base = base_series_multiplier<Series>;
    // Cf type getter shortcut.
    template <typename T>
    using cf_t = typename T::term_type::cf_type;
    // Key type getter shortcut.
    template <typename T>
    using key_t = typename T::term_type::key_type;
    // Bounds checking.
    // Functor to return un updated copy of p if v is less than p.first or greater than p.second.
    struct update_minmax {
        template <typename T>
        std::pair<T, T> operator()(const std::pair<T, T> &p, const T &v) const
        {
            return std::make_pair(v < p.first ? v : p.first, v > p.second ? v : p.second);
        }
    };
    // No bounds checking if key is a monomial with non-integral exponents.
    template <
        typename T = Series,
        typename std::enable_if<
            detail::is_monomial<key_t<T>>::value && !std::is_integral<typename key_t<T>::value_type>::value, int>::type
        = 0>
    void check_bounds() const
    {
    }
    // Monomial with integral exponents.
    template <
        typename T = Series,
        typename std::enable_if<
            detail::is_monomial<key_t<T>>::value && std::is_integral<typename key_t<T>::value_type>::value, int>::type
        = 0>
    void check_bounds() const
    {
        using expo_type = typename key_t<T>::value_type;
        using term_type = typename Series::term_type;
        using mm_vec = std::vector<std::pair<expo_type, expo_type>>;
        using v_ptr = typename base::v_ptr;
        // NOTE: we know that the input series are not null.
        piranha_assert(this->m_v1.size() != 0u && this->m_v2.size() != 0u);
        // Sync mutex, actually used only in mt mode.
        std::mutex mut;
        // Checker for monomial sizes in debug mode.
        auto monomial_checker = [this](const term_type &t) { return t.m_key.size() == this->m_ss.size(); };
        (void)monomial_checker;
        // The function used to determine minmaxs for the two series. This is used both in
        // single-thread and multi-thread mode.
        auto thread_func = [&mut, this, &monomial_checker](unsigned t_idx, const v_ptr *vp, mm_vec *mmv) {
            piranha_assert(t_idx < this->m_n_threads);
            // Establish the block size.
            const auto block_size = vp->size() / this->m_n_threads;
            auto start = vp->data() + t_idx * block_size;
            const auto end
                = vp->data() + ((t_idx == this->m_n_threads - 1u) ? vp->size() : ((t_idx + 1u) * block_size));
            // We need to make sure we have at least 1 element to process. This is guaranteed
            // in the single-threaded implementation but not in multithreading.
            if (start == end) {
                piranha_assert(this->m_n_threads > 1u);
                return;
            }
            piranha_assert(monomial_checker(**start));
            // Local vector that will hold the minmax values for this thread.
            mm_vec minmax_values;
            // Init the mimnax.
            // NOTE: we can use this as we are sure the series has at least one element (start != end).
            std::transform((*start)->m_key.begin(), (*start)->m_key.end(), std::back_inserter(minmax_values),
                           [](const expo_type &v) { return std::make_pair(v, v); });
            // Move to the next element and go with the loop.
            ++start;
            for (; start != end; ++start) {
                piranha_assert(monomial_checker(**start));
                // NOTE: std::transform is allowed to do transformations in-place - i.e., here the output range is
                // the same as the first or second input range:
                // http://stackoverflow.com/questions/19200528/is-it-safe-for-the-input-iterator-and-output-iterator-in-stdtransform-to-be-fr
                // The important part is that the functor *itself* must not mutate the elements.
                std::transform(minmax_values.begin(), minmax_values.end(), (*start)->m_key.begin(),
                               minmax_values.begin(), update_minmax{});
            }
            if (this->m_n_threads == 1u) {
                // In single thread the output mmv should be written only once, after being def-inited.
                piranha_assert(mmv->empty());
                // Just move in the local minmax values in single-threaded mode.
                *mmv = std::move(minmax_values);
            } else {
                std::lock_guard<std::mutex> lock(mut);
                if (mmv->empty()) {
                    // If mmv has not been inited yet, just move in the current values.
                    *mmv = std::move(minmax_values);
                } else {
                    piranha_assert(minmax_values.size() == mmv->size());
                    // Otherwise, update the minmaxs.
                    auto updater
                        = [](const std::pair<expo_type, expo_type> &p1, const std::pair<expo_type, expo_type> &p2) {
                              return std::make_pair(p1.first < p2.first ? p1.first : p2.first,
                                                    p1.second > p2.second ? p1.second : p2.second);
                          };
                    std::transform(minmax_values.begin(), minmax_values.end(), mmv->begin(), mmv->begin(), updater);
                }
            }
        };
        // minmax vectors for the two series.
        mm_vec minmax_values1, minmax_values2;
        check_bounds_impl(minmax_values1, minmax_values2, thread_func);
        // Compute the sum of the two minmaxs, using multiprecision to avoid overflow (this is a simple interval
        // addition).
        std::vector<std::pair<integer, integer>> minmax_values;
        std::transform(
            minmax_values1.begin(), minmax_values1.end(), minmax_values2.begin(), std::back_inserter(minmax_values),
            [](const std::pair<expo_type, expo_type> &p1, const std::pair<expo_type, expo_type> &p2) {
                return std::make_pair(integer(p1.first) + integer(p2.first), integer(p1.second) + integer(p2.second));
            });
        piranha_assert(minmax_values.size() == minmax_values1.size());
        piranha_assert(minmax_values.size() == minmax_values2.size());
        // Now do the checking.
        for (decltype(minmax_values.size()) i = 0u; i < minmax_values.size(); ++i) {
            try {
                (void)static_cast<expo_type>(minmax_values[i].first);
                (void)static_cast<expo_type>(minmax_values[i].second);
            } catch (...) {
                piranha_throw(std::overflow_error, "monomial components are out of bounds");
            }
        }
    }
    template <typename T = Series,
              typename std::enable_if<detail::is_kronecker_monomial<key_t<T>>::value, int>::type = 0>
    void check_bounds() const
    {
        using value_type = typename key_t<Series>::value_type;
        using ka = kronecker_array<value_type>;
        using v_ptr = typename base::v_ptr;
        using mm_vec = std::vector<std::pair<value_type, value_type>>;
        piranha_assert(this->m_v1.size() != 0u && this->m_v2.size() != 0u);
        // NOTE: here we are sure about this since the symbol set in a series should never
        // overflow the size of the limits, as the check for compatibility in Kronecker monomial
        // would kick in.
        piranha_assert(this->m_ss.size() < ka::get_limits().size());
        std::mutex mut;
        auto thread_func = [&mut, this](unsigned t_idx, const v_ptr *vp, mm_vec *mmv) {
            piranha_assert(t_idx < this->m_n_threads);
            const auto block_size = vp->size() / this->m_n_threads;
            auto start = vp->data() + t_idx * block_size;
            const auto end
                = vp->data() + ((t_idx == this->m_n_threads - 1u) ? vp->size() : ((t_idx + 1u) * block_size));
            if (start == end) {
                piranha_assert(this->m_n_threads > 1u);
                return;
            }
            mm_vec minmax_values;
            // Tmp vector for unpacking, inited with the first element in the range.
            // NOTE: we need to check that the exponents of the monomials in the result do not
            // go outside the bounds of the Kronecker codification. We need to unpack all monomials
            // in the operands and examine them, we cannot operate on the codes for this.
            auto tmp_vec = (*start)->m_key.unpack(this->m_ss);
            // Init the mimnax.
            std::transform(tmp_vec.begin(), tmp_vec.end(), std::back_inserter(minmax_values),
                           [](const value_type &v) { return std::make_pair(v, v); });
            ++start;
            for (; start != end; ++start) {
                tmp_vec = (*start)->m_key.unpack(this->m_ss);
                std::transform(minmax_values.begin(), minmax_values.end(), tmp_vec.begin(), minmax_values.begin(),
                               update_minmax{});
            }
            if (this->m_n_threads == 1u) {
                piranha_assert(mmv->empty());
                *mmv = std::move(minmax_values);
            } else {
                std::lock_guard<std::mutex> lock(mut);
                if (mmv->empty()) {
                    *mmv = std::move(minmax_values);
                } else {
                    piranha_assert(minmax_values.size() == mmv->size());
                    auto updater
                        = [](const std::pair<value_type, value_type> &p1, const std::pair<value_type, value_type> &p2) {
                              return std::make_pair(p1.first < p2.first ? p1.first : p2.first,
                                                    p1.second > p2.second ? p1.second : p2.second);
                          };
                    std::transform(minmax_values.begin(), minmax_values.end(), mmv->begin(), mmv->begin(), updater);
                }
            }
        };
        mm_vec minmax_values1, minmax_values2;
        check_bounds_impl(minmax_values1, minmax_values2, thread_func);
        std::vector<std::pair<integer, integer>> minmax_values;
        std::transform(
            minmax_values1.begin(), minmax_values1.end(), minmax_values2.begin(), std::back_inserter(minmax_values),
            [](const std::pair<value_type, value_type> &p1, const std::pair<value_type, value_type> &p2) {
                return std::make_pair(integer(p1.first) + integer(p2.first), integer(p1.second) + integer(p2.second));
            });
        // Bounds of the Kronecker representation for each component.
        const auto &minmax_vec
            = std::get<0u>(ka::get_limits()[static_cast<decltype(ka::get_limits().size())>(this->m_ss.size())]);
        piranha_assert(minmax_values.size() == minmax_vec.size());
        piranha_assert(minmax_values.size() == minmax_values1.size());
        piranha_assert(minmax_values.size() == minmax_values2.size());
        for (decltype(minmax_values.size()) i = 0u; i < minmax_values.size(); ++i) {
            if (unlikely(minmax_values[i].first < -minmax_vec[i] || minmax_values[i].second > minmax_vec[i])) {
                piranha_throw(std::overflow_error, "Kronecker monomial components are out of bounds");
            }
        }
    }
    // Implementation detail of the bound checking logic. This is common enough to be shared.
    template <typename MmVec, typename Func>
    void check_bounds_impl(MmVec &minmax_values1, MmVec &minmax_values2, Func &thread_func) const
    {
        if (this->m_n_threads == 1u) {
            thread_func(0u, &(this->m_v1), &minmax_values1);
            thread_func(0u, &(this->m_v2), &minmax_values2);
        } else {
            // Series 1.
            {
                future_list<void> ff_list;
                try {
                    for (unsigned i = 0u; i < this->m_n_threads; ++i) {
                        ff_list.push_back(thread_pool::enqueue(i, thread_func, i, &(this->m_v1), &minmax_values1));
                    }
                    // First let's wait for everything to finish.
                    ff_list.wait_all();
                    // Then, let's handle the exceptions.
                    ff_list.get_all();
                } catch (...) {
                    ff_list.wait_all();
                    throw;
                }
            }
            // Series 2.
            {
                future_list<void> ff_list;
                try {
                    for (unsigned i = 0u; i < this->m_n_threads; ++i) {
                        ff_list.push_back(thread_pool::enqueue(i, thread_func, i, &(this->m_v2), &minmax_values2));
                    }
                    // First let's wait for everything to finish.
                    ff_list.wait_all();
                    // Then, let's handle the exceptions.
                    ff_list.get_all();
                } catch (...) {
                    ff_list.wait_all();
                    throw;
                }
            }
        }
    }
    // Enabler for the call operator.
    template <typename T>
    using call_enabler = typename std::enable_if<
        key_is_multipliable<cf_t<T>, key_t<T>>::value && has_multiply_accumulate<cf_t<T>>::value, int>::type;
    // Utility helpers for the subtraction of degree types in the truncation routines. The specialisation
    // for integral types will check the operation for overflow.
    template <typename T, typename std::enable_if<!std::is_integral<T>::value, int>::type = 0>
    static T degree_sub(const T &a, const T &b)
    {
        return a - b;
    }
    template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    static T degree_sub(const T &a, const T &b)
    {
        T retval(a);
        detail::safe_integral_subber(retval, b);
        return retval;
    }
    // Dispatch of untruncated multiplication.
    template <typename T = Series,
              typename std::enable_if<detail::is_kronecker_monomial<typename T::term_type::key_type>::value, int>::type
              = 0>
    Series um_impl() const
    {
        return untruncated_kronecker_mult();
    }
    template <typename T = Series,
              typename std::enable_if<!detail::is_kronecker_monomial<typename T::term_type::key_type>::value, int>::type
              = 0>
    Series um_impl() const
    {
        return this->plain_multiplication();
    }

public:
    /// Constructor.
    /**
     * The constructor will call the base constructor and run these additional checks:
     * - if the key is a piranha::kronecker_monomial, it will be checked that the result of the multiplication does
     *   not overflow the representation limits of piranha::kronecker_monomial;
     * - if the key is a piranha::monomial of a C++ integral type, it will be checked that the result of the
     *   multiplication does not overflow the limits of the integral type.
     *
     * If any check fails, a runtime error will be produced.
     *
     * @param s1 first series operand.
     * @param s2 second series operand.
     *
     * @throws std::overflow_error if a bounds check, as described above, fails.
     * @throws unspecified any exception thrown by:
     * - the base constructor,
     * - standard threading primitives,
     * - memory errors in standard containers,
     * - thread_pool::enqueue(),
     * - future_list::push_back().
     */
    explicit series_multiplier(const Series &s1, const Series &s2) : base(s1, s2)
    {
        // Nothing to do if the series are null or the merged symbol set is empty.
        if (unlikely(this->m_v1.empty() || this->m_v2.empty() || this->m_ss.size() == 0u)) {
            return;
        }
        check_bounds();
    }
    /// Perform multiplication.
    /**
     * \note
     * This template operator is enabled only if:
     * - the coefficient and key type of \p Series satisfy piranha::key_is_multipliable,
     * - the coefficient type of \p Series supports piranha::math::mul3() and piranha::math::multiply_accumulate().
     *
     * This method will perform the multiplication of the series operands passed to the constructor. Depending on
     * the key type of \p Series, the implementation will use either base_series_multiplier::plain_multiplication()
     * with base_series_multiplier::plain_multiplier or a different algorithm.
     *
     * If a polynomial truncation threshold is defined and the degree type of the polynomial is a C++ integral type,
     * the integral arithmetic operations involved in the truncation logic will be checked for overflow.
     *
     * @return the result of the multiplication of the input series operands.
     *
     * @throws std::overflow_error in case of overflow errors.
     * @throws unspecified any exception thrown by:
     * - piranha::base_series_multiplier::plain_multiplication(),
     * - piranha::base_series_multiplier::estimate_final_series_size(),
     * - piranha::base_series_multiplier::sanitise_series(),
     * - piranha::base_series_multiplier::finalise_series(),
     * - <tt>boost::numeric_cast()</tt>,
     * - the public interface of piranha::hash_set,
     * - piranha::safe_cast(),
     * - memory errors in standard containers,
     * - piranha::math::mul3(),
     * - piranha::math::multiply_accumulate(),
     * - thread_pool::enqueue(),
     * - future_list::push_back(),
     * - _truncated_multiplication(),
     * - polynomial::get_auto_truncate_degree().
     */
    template <typename T = Series, call_enabler<T> = 0>
    Series operator()() const
    {
        return execute();
    }
    /** @name Low-level interface
     * Low-level methods, on top of which the call operator is implemented.
     */
    //@{
    /// Untruncated multiplication.
    /**
     * \note
     * This method can be used only if operator()() can be called.
     *
     * This method will return the result of multiplying the two polynomials used as input arguments
     * in the class' constructor. The multiplication will be untruncated, regardless of the current global
     * truncation settings.
     *
     * @return the result of the untruncated multiplication of the two operands used in the construction of \p this.
     *
     * @throws unspecified any exception thrown by:
     * - piranha::base_series_multiplier::plain_multiplication(),
     * - piranha::base_series_multiplier::estimate_final_series_size(),
     * - piranha::base_series_multiplier::sanitise_series(),
     * - piranha::base_series_multiplier::finalise_series(),
     * - <tt>boost::numeric_cast()</tt>,
     * - the public interface of piranha::hash_set,
     * - piranha::safe_cast(),
     * - memory errors in standard containers,
     * - piranha::math::mul3(),
     * - piranha::math::multiply_accumulate(),
     * - thread_pool::enqueue(),
     * - future_list::push_back().
     */
    Series _untruncated_multiplication() const
    {
        return um_impl();
    }
    /// Truncated multiplication.
    /**
     * \note
     * This method can be used only if the following conditions apply:
     * - the conditions for truncated multiplication outlined in piranha::polynomial are satisfied,
     * - the type \p T is the same as the degree type of the polynomial,
     * - the number and types of \p Args is as specified below,
     * - piranha::base_series_multiplier::plain_multiplication() and _get_skip_limits() can be called.
     *
     * This method will perform the truncated multiplication of the series operands passed to the constructor.
     * The truncation degree is set to \p max_degree, and it is either:
     * - the total maximum degree, if the number of \p Args is zero, or
     * - the partial degree, if the number of \p Args is two.
     *
     * In the latter case, the two arguments must be:
     * - an \p std::vector of \p std::string representing the names of the variables which will be taken
     *   into account when computing the partial degree,
     * - a piranha::symbol_idx_fset referring to the positions of the variables of the first argument
     *   in the merged symbol set of the two operands.
     *
     * @param max_degree the maximum degree of the result of the multiplication.
     * @param args either an empty argument, or a pair of arguments as described above.
     *
     * @return the result of the truncated multiplication of the operands used for construction.
     *
     * @throws unspecified any exception thrown by:
     * - memory errors in standard containers,
     * - piranha::safe_cast(),
     * - arithmetic and other operations on the degree type,
     * - base_series_multiplier::plain_multiplication(),
     * - _get_skip_limits().
     */
    template <typename T, typename... Args>
    Series _truncated_multiplication(const T &max_degree, const Args &... args) const
    {
        // NOTE: a possible optimisation here is the following: if the sum degrees of the arguments is less than
        // or equal to the max truncation degree, just do the normal multiplication - which can also then take
        // advantage of faster Kronecker multiplication, if the series are suitable.
        using term_type = typename Series::term_type;
        // NOTE: degree type is the same in total and partial.
        using degree_type = decltype(ps_get_degree(term_type{}, this->m_ss));
        using size_type = typename base::size_type;
        namespace sph = std::placeholders;
        static_assert(std::is_same<T, degree_type>::value, "Invalid degree type");
        static_assert(detail::has_get_auto_truncate_degree<Series>::value, "Invalid series type");
        // First let's create two vectors with the degrees of the terms in the two series.
        using d_size_type = typename std::vector<degree_type>::size_type;
        std::vector<degree_type> v_d1(safe_cast<d_size_type>(this->m_v1.size())),
            v_d2(safe_cast<d_size_type>(this->m_v2.size()));
        detail::parallel_vector_transform(
            this->m_n_threads, this->m_v1, v_d1,
            std::bind(term_degree_getter{}, sph::_1, std::cref(this->m_ss), std::cref(args)...));
        detail::parallel_vector_transform(
            this->m_n_threads, this->m_v2, v_d2,
            std::bind(term_degree_getter{}, sph::_1, std::cref(this->m_ss), std::cref(args)...));
        // Next we need to order the terms in the second series, and also the corresponding degree vector.
        // First we create a vector of indices and we fill it.
        std::vector<size_type> idx_vector(safe_cast<typename std::vector<size_type>::size_type>(this->m_v2.size()));
        std::iota(idx_vector.begin(), idx_vector.end(), size_type(0u));
        // Second, we sort the vector of indices according to the degrees in the second series.
        std::stable_sort(idx_vector.begin(), idx_vector.end(), [&v_d2](const size_type &i1, const size_type &i2) {
            return v_d2[static_cast<d_size_type>(i1)] < v_d2[static_cast<d_size_type>(i2)];
        });
        // Finally, we apply the permutation to v_d2 and m_v2.
        decltype(this->m_v2) v2_copy(this->m_v2.size());
        decltype(v_d2) v_d2_copy(v_d2.size());
        std::transform(idx_vector.begin(), idx_vector.end(), v2_copy.begin(),
                       [this](const size_type &i) { return this->m_v2[i]; });
        std::transform(idx_vector.begin(), idx_vector.end(), v_d2_copy.begin(),
                       [&v_d2](const size_type &i) { return v_d2[static_cast<d_size_type>(i)]; });
        this->m_v2 = std::move(v2_copy);
        v_d2 = std::move(v_d2_copy);
        // Now get the skip limits and we build the limits functor.
        const auto sl = _get_skip_limits(v_d1, v_d2, max_degree);
        auto lf = [&sl](const size_type &idx1) {
            return sl[static_cast<typename std::vector<size_type>::size_type>(idx1)];
        };
        return this->plain_multiplication(lf);
    }
    /// Establish skip limits for truncated multiplication.
    /**
     * \note
     * This method can be called only if \p Series supports truncated multiplication (as explained in
     * piranha::polynomial) and
     * \p T is the same type as the degree type.
     *
     * This method assumes that \p v_d1 and \p v_d2 are vectors containing the degrees of each term in the first and
     * second series respectively, and that \p v_d2 is sorted in ascending order.
     * It will return a vector \p v of indices in the second series such that, given an index \p i in the first
     * series, the term of index <tt>v[i]</tt> in the second series is the first term such that the term-by-term
     * multiplication with the <tt>i</tt>-th term in the first series produces a term of degree greater than
     * \p max_degree. That is, terms of index equal to or greater than <tt>v[i]</tt> in the second series
     * will produce terms with degree greater than \p max_degree when multiplied by the <tt>i</tt>-th term in the first
     * series.
     *
     * @param v_d1 a vector containing the degrees of the terms in the first series.
     * @param v_d2 a sorted vector containing the degrees of the terms in the second series.
     * @param max_degree the truncation degree.
     *
     * @return the vector of skip limits, as explained above.
     *
     * @throws unspecified any exception thrown by:
     * - memory errors in standard containers,
     * - piranha::safe_cast(),
     * - operations on the degree type.
     */
    template <typename T>
    std::vector<typename base::size_type> _get_skip_limits(const std::vector<T> &v_d1, const std::vector<T> &v_d2,
                                                           const T &max_degree) const
    {
        // NOTE: this can be parallelised, but we need to check the heuristic
        // for selecting the number of threads as it is pretty fast wrt the multiplication.
        // Check that we are allowed to call this method.
        PIRANHA_TT_CHECK(detail::has_get_auto_truncate_degree, Series);
        PIRANHA_TT_CHECK(std::is_same, T, decltype(math::degree(Series{})));
        using size_type = typename base::size_type;
        using d_size_type = typename std::vector<T>::size_type;
        piranha_assert(std::is_sorted(v_d2.begin(), v_d2.end()));
        // A vector of indices into the second series.
        std::vector<size_type> idx_vector(safe_cast<typename std::vector<size_type>::size_type>(this->m_v2.size()));
        std::iota(idx_vector.begin(), idx_vector.end(), size_type(0u));
        // The return value.
        std::vector<size_type> retval;
        for (const auto &d1 : v_d1) {
            // Here we will find the index of the first term t2 in the second series such that
            // the degree d2 of t2 is > max_degree - d1, that is, d1 + d2 > max_degree.
            // NOTE: we need to use upper_bound, instead of lower_bound, because we need to find the first
            // element which is *strictly* greater than the max degree, as upper bound of a half closed
            // interval.
            // NOTE: the functor of lower_bound works inversely wrt upper_bound. See the notes on the type
            // requirements for the functor here:
            // http://en.cppreference.com/w/cpp/algorithm/upper_bound
            const T comp = degree_sub(max_degree, d1);
            const auto it = std::upper_bound(
                idx_vector.begin(), idx_vector.end(), comp,
                [&v_d2](const T &c, const size_type &idx) { return c < v_d2[static_cast<d_size_type>(idx)]; });
            retval.push_back(it == idx_vector.end() ? static_cast<size_type>(idx_vector.size()) : *it);
        }
        // Check the consistency of the result in debug mode.
        auto retval_checker = [&retval, &v_d1, &v_d2, &max_degree, this]() -> bool {
            for (decltype(retval.size()) i = 0u; i < retval.size(); ++i) {
                // NOTE: this just means that all terms in s2 are within the limit.
                if (retval[i] == v_d2.size()) {
                    continue;
                }
                if (retval[i] > v_d2.size()) {
                    return false;
                }
                if (!(v_d2[static_cast<d_size_type>(retval[i])]
                      > this->degree_sub(max_degree, v_d1[static_cast<d_size_type>(i)]))) {
                    return false;
                }
            }
            return true;
        };
        (void)retval_checker;
        piranha_assert(retval.size() == this->m_v1.size());
        piranha_assert(retval_checker());
        return retval;
    }
    //@}
private:
    // NOTE: wrapper to multadd that treats specially rational coefficients. We need to decide in the future
    // if this stays here or if it is better to generalise it.
    template <typename T, typename std::enable_if<!is_mp_rational<T>::value, int>::type = 0>
    static void fma_wrap(T &a, const T &b, const T &c)
    {
        math::multiply_accumulate(a, b, c);
    }
    template <typename T, typename std::enable_if<is_mp_rational<T>::value, int>::type = 0>
    static void fma_wrap(T &a, const T &b, const T &c)
    {
        math::multiply_accumulate(a._num(), b.num(), c.num());
    }
    // Wrapper for the plain multiplication routine.
    // Case 1: no auto truncation available, just run the plain multiplication.
    template <typename T = Series,
              typename std::enable_if<!detail::has_get_auto_truncate_degree<T>::value, int>::type = 0>
    Series plain_multiplication_wrapper() const
    {
        return this->plain_multiplication();
    }
    // Case 2: auto-truncation available. Check if auto truncation is active.
    template <typename T = Series,
              typename std::enable_if<detail::has_get_auto_truncate_degree<T>::value, int>::type = 0>
    Series plain_multiplication_wrapper() const
    {
        const auto t = T::get_auto_truncate_degree();
        if (std::get<0u>(t) == 0) {
            // No truncation active.
            return this->plain_multiplication();
        }
        // Truncation is active.
        if (std::get<0u>(t) == 1) {
            // Total degree truncation.
            return _truncated_multiplication(std::get<1u>(t));
        }
        piranha_assert(std::get<0u>(t) == 2);
        // Partial degree truncation.
        const auto idx = ss_intersect_idx(this->m_ss, std::get<2u>(t));
        return _truncated_multiplication(std::get<1u>(t), std::get<2u>(t), idx);
    }
    // NOTE: the existence of these functors is because GCC 4.8 has troubles capturing variadic arguments in lambdas
    // in _truncated_multiplication, and we need to use std::bind instead. Once we switch to 4.9, we can revert
    // to lambdas and drop the <functional> header.
    struct term_degree_sorter {
        using term_type = typename Series::term_type;
        template <typename... Args>
        bool operator()(term_type const *p1, term_type const *p2, const symbol_fset &ss, const Args &... args) const
        {
            return ps_get_degree(*p1, args..., ss) < ps_get_degree(*p2, args..., ss);
        }
    };
    struct term_degree_getter {
        using term_type = typename Series::term_type;
        template <typename... Args>
        auto operator()(term_type const *p, const symbol_fset &ss, const Args &... args) const
            -> decltype(ps_get_degree(*p, args..., ss))
        {
            return ps_get_degree(*p, args..., ss);
        }
    };
    // execute() is the top level dispatch for the actual multiplication.
    // Case 1: not a Kronecker monomial, do the plain mult.
    template <typename T = Series,
              typename std::enable_if<!detail::is_kronecker_monomial<typename T::term_type::key_type>::value, int>::type
              = 0>
    Series execute() const
    {
        return plain_multiplication_wrapper();
    }
    // Checking for active truncation.
    template <typename T = Series,
              typename std::enable_if<detail::has_get_auto_truncate_degree<T>::value, int>::type = 0>
    bool check_truncation() const
    {
        const auto t = T::get_auto_truncate_degree();
        return std::get<0u>(t) != 0;
    }
    template <typename T = Series,
              typename std::enable_if<!detail::has_get_auto_truncate_degree<T>::value, int>::type = 0>
    bool check_truncation() const
    {
        return false;
    }
    // Case 2: Kronecker mult, do the special multiplication unless a truncation is active. In that case, run the
    // plain mult.
    template <typename T = Series,
              typename std::enable_if<detail::is_kronecker_monomial<typename T::term_type::key_type>::value, int>::type
              = 0>
    Series execute() const
    {
        if (check_truncation()) {
            return plain_multiplication_wrapper();
        }
        return untruncated_kronecker_mult();
    }
    template <typename T = Series,
              typename std::enable_if<detail::is_kronecker_monomial<typename T::term_type::key_type>::value, int>::type
              = 0>
    Series untruncated_kronecker_mult() const
    {
        // Cache the sizes.
        const auto size1 = this->m_v1.size(), size2 = this->m_v2.size();
        // Determine whether we want to estimate or not. We check the threshold, and
        // we force the estimation in multithreaded mode.
        bool estimate = true;
        const auto e_thr = tuning::get_estimate_threshold();
        if (integer(size1) * size2 < integer(e_thr) * e_thr && this->m_n_threads == 1u) {
            estimate = false;
        }
        // If estimation is not worth it, we go with the plain multiplication.
        // NOTE: this is probably not optimal, but we have to do like this as the sparse
        // Kronecker multiplication below requires estimation. Maybe in the future we can
        // have a version without estimation.
        if (!estimate) {
            return this->plain_multiplication();
        }
        // Setup the return value.
        Series retval;
        retval.set_symbol_set(this->m_ss);
        // Do not do anything if one of the two series is empty, just return an empty series.
        if (unlikely(!size1 || !size2)) {
            return retval;
        }
        // Rehash the retun value's container accordingly. Check the tuning flag to see if we want to use
        // multiple threads for initing the return value.
        // NOTE: it is important here that we use the same n_threads for multiplication and memset as
        // we tie together pinned threads with potentially different NUMA regions.
        const unsigned n_threads_rehash = tuning::get_parallel_memory_set() ? this->m_n_threads : 1u;
        // Use the plain functor in normal mode for the estimation.
        const auto est
            = this->template estimate_final_series_size<1u, typename base::template plain_multiplier<false>>();
        // NOTE: if something goes wrong here, no big deal as retval is still empty.
        retval._container().rehash(boost::numeric_cast<typename Series::size_type>(
                                       std::ceil(static_cast<double>(est) / retval._container().max_load_factor())),
                                   n_threads_rehash);
        piranha_assert(retval._container().bucket_count());
        sparse_kronecker_multiplication(retval);
        return retval;
    }
    void sparse_kronecker_multiplication(Series &retval) const
    {
        using bucket_size_type = typename base::bucket_size_type;
        using size_type = typename base::size_type;
        using term_type = typename Series::term_type;
        // Type representing multiplication tasks:
        // - the current term index from s1,
        // - the first term index in s2,
        // - the last term index in s2.
        using task_type = std::tuple<size_type, size_type, size_type>;
        // Cache a few quantities.
        auto &v1 = this->m_v1;
        auto &v2 = this->m_v2;
        const auto size1 = v1.size();
        const auto size2 = v2.size();
        auto &container = retval._container();
        // A convenience functor to compute the destination bucket
        // of a term into retval.
        auto r_bucket = [&container](term_type const *p) { return container._bucket_from_hash(p->hash()); };
        // Sort input terms according to bucket positions in retval.
        auto term_cmp = [&r_bucket](term_type const *p1, term_type const *p2) { return r_bucket(p1) < r_bucket(p2); };
        std::stable_sort(v1.begin(), v1.end(), term_cmp);
        std::stable_sort(v2.begin(), v2.end(), term_cmp);
        // Task comparator. It will compare the bucket index of the terms resulting from
        // the multiplication of the term in the first series by the first term in the block
        // of the second series. This is essentially the first bucket index of retval in which the task
        // will write.
        // NOTE: this is guaranteed not to overflow as the max bucket size in the hash set is 2**(nbits-1),
        // and the max value of bucket_size_type is 2**nbits - 1.
        auto task_cmp = [&r_bucket, &v1, &v2](const task_type &t1, const task_type &t2) {
            return r_bucket(v1[std::get<0u>(t1)]) + r_bucket(v2[std::get<1u>(t1)])
                   < r_bucket(v1[std::get<0u>(t2)]) + r_bucket(v2[std::get<1u>(t2)]);
        };
        // Task block size.
        const size_type block_size = safe_cast<size_type>(tuning::get_multiplication_block_size());
        // Task splitter: split a task in block_size sized tasks and append them to out.
        auto task_split = [block_size](const task_type &t, std::vector<task_type> &out) {
            size_type start = std::get<1u>(t), end = std::get<2u>(t);
            while (static_cast<size_type>(end - start) > block_size) {
                out.emplace_back(std::get<0u>(t), start, static_cast<size_type>(start + block_size));
                start = static_cast<size_type>(start + block_size);
            }
            if (end != start) {
                out.emplace_back(std::get<0u>(t), start, end);
            }
        };
        // End of the container, always the same value.
        const auto it_end = container.end();
        // Function to perform all the term-by-term multiplications in a task, using tmp_term
        // as a temporary value for the computation of the result.
        auto task_consume = [&v1, &v2, &container, it_end, this](const task_type &task, term_type &tmp_term) {
            // Get the term in the first series.
            term_type const *t1 = v1[std::get<0u>(task)];
            // Get pointers to the second series.
            term_type const **start2 = &(v2[std::get<1u>(task)]), **end2 = &(v2[std::get<2u>(task)]);
            // NOTE: these will have to be adapted for kd_monomial.
            using int_type = decltype(t1->m_key.get_int());
            // Get shortcuts to cf and key in t1.
            const auto &cf1 = t1->m_cf;
            const int_type key1 = t1->m_key.get_int();
            // Iterate over the task.
            for (; start2 != end2; ++start2) {
                // Const ref to the current term in the second series.
                const auto &cur = **start2;
                // Add the keys.
                // NOTE: this will have to be adapted for kd_monomial.
                tmp_term.m_key.set_int(static_cast<int_type>(key1 + cur.m_key.get_int()));
                // Try to locate the term into retval.
                auto bucket_idx = container._bucket(tmp_term);
                const auto it = container._find(tmp_term, bucket_idx);
                if (it == it_end) {
                    // NOTE: for coefficient series, we might want to insert with move() below,
                    // as we are not going to re-use the allocated resources in tmp.m_cf.
                    // Take care of multiplying the coefficient.
                    cf_mult_impl(tmp_term.m_cf, cf1, cur.m_cf);
                    container._unique_insert(tmp_term, bucket_idx);
                } else {
                    // NOTE: here we need to decide if we want to give the same treatment to fmp as we did with
                    // cf_mult_impl.
                    // For the moment it is an implementation detail of this class.
                    this->fma_wrap(it->m_cf, cf1, cur.m_cf);
                }
            }
        };
        if (this->m_n_threads == 1u) {
            try {
                // Single threaded case.
                // Create the vector of tasks.
                std::vector<task_type> tasks;
                for (decltype(v1.size()) i = 0u; i < size1; ++i) {
                    task_split(std::make_tuple(i, size_type(0u), size2), tasks);
                }
                // Sort the tasks.
                std::stable_sort(tasks.begin(), tasks.end(), task_cmp);
                // Iterate over the tasks and run the multiplication.
                term_type tmp_term;
                for (const auto &t : tasks) {
                    task_consume(t, tmp_term);
                }
                this->sanitise_series(retval, this->m_n_threads);
                this->finalise_series(retval);
            } catch (...) {
                retval._container().clear();
                throw;
            }
            return;
        }
        // Number of buckets in retval.
        const bucket_size_type bucket_count = container.bucket_count();
        // Compute the number of zones in which the output container will be subdivided,
        // a multiple of the number of threads.
        // NOTE: zm is a tuning parameter.
        const unsigned zm = 10u;
        const bucket_size_type n_zones = static_cast<bucket_size_type>(integer(this->m_n_threads) * zm);
        // Number of buckets per zone (can be zero).
        const bucket_size_type bpz = static_cast<bucket_size_type>(bucket_count / n_zones);
        // For each zone, we need to define a vector of tasks that will write only into that zone.
        std::vector<std::vector<task_type>> task_table;
        task_table.resize(safe_cast<decltype(task_table.size())>(n_zones));
        // Lower bound implementation. Adapted from:
        // http://en.cppreference.com/w/cpp/algorithm/lower_bound
        // Given the [first,last[ index range in v2, find the first index idx in the v2 range such that the i-th
        // term in v1 multiplied by the idx-th term in v2 will be written into retval at a bucket index not less than
        // zb.
        auto l_bound
            = [&v1, &v2, &r_bucket](size_type first, size_type last, bucket_size_type zb, size_type i) -> size_type {
            piranha_assert(first <= last);
            bucket_size_type ib = r_bucket(v1[i]);
            // Avoid zb - ib below wrapping around.
            if (zb < ib) {
                return 0u;
            }
            const auto cmp = static_cast<bucket_size_type>(zb - ib);
            size_type idx, step, count = static_cast<size_type>(last - first);
            while (count > 0u) {
                idx = first;
                step = static_cast<size_type>(count / 2u);
                idx = static_cast<size_type>(idx + step);
                if (r_bucket(v2[idx]) < cmp) {
                    idx = static_cast<size_type>(idx + 1u);
                    first = idx;
                    if (count <= step + 1u) {
                        break;
                    }
                    count = static_cast<size_type>(count - (step + 1u));
                } else {
                    count = step;
                }
            }
            return first;
        };
        // Fill the task table.
        auto table_filler = [&task_table, bpz, this, bucket_count, size1, size2, &l_bound, &task_split,
                             &task_cmp](const unsigned &thread_idx) {
            for (unsigned n = 0u; n < zm; ++n) {
                std::vector<task_type> cur_tasks;
                // [a,b[ is the container zone.
                bucket_size_type a = static_cast<bucket_size_type>(thread_idx * bpz * zm + n * bpz);
                bucket_size_type b;
                if (n == zm - 1u && thread_idx == this->m_n_threads - 1u) {
                    // Special casing if this is the last zone in the container.
                    b = bucket_count;
                } else {
                    b = static_cast<bucket_size_type>(a + bpz);
                }
                // First batch of tasks.
                for (size_type i = 0u; i < size1; ++i) {
                    auto t = std::make_tuple(i, l_bound(0u, size2, a, i), l_bound(0u, size2, b, i));
                    if (std::get<1u>(t) == 0u && std::get<2u>(t) == 0u) {
                        // This means that all the next tasks we will compute will be empty,
                        // no sense in calculating them.
                        break;
                    }
                    task_split(t, cur_tasks);
                }
                // Second batch of tasks.
                // Note: we can always compute a,b + bucket_count because of the limits on the maximum value of
                // bucket_count.
                for (size_type i = 0u; i < size1; ++i) {
                    auto t = std::make_tuple(i, l_bound(0u, size2, static_cast<bucket_size_type>(a + bucket_count), i),
                                             l_bound(0u, size2, static_cast<bucket_size_type>(b + bucket_count), i));
                    if (std::get<1u>(t) == 0u && std::get<2u>(t) == 0u) {
                        break;
                    }
                    task_split(t, cur_tasks);
                }
                // Sort the task vector.
                std::stable_sort(cur_tasks.begin(), cur_tasks.end(), task_cmp);
                // Move the vector of tasks in the table.
                task_table[static_cast<decltype(task_table.size())>(thread_idx * zm + n)] = std::move(cur_tasks);
            }
        };
        // Go with the threads to fill the task table.
        future_list<decltype(table_filler(0u))> ff_list;
        try {
            for (unsigned i = 0u; i < this->m_n_threads; ++i) {
                ff_list.push_back(thread_pool::enqueue(i, table_filler, i));
            }
            // First let's wait for everything to finish.
            ff_list.wait_all();
            // Then, let's handle the exceptions.
            ff_list.get_all();
        } catch (...) {
            ff_list.wait_all();
            throw;
        }
        // Check the consistency of the table for debug purposes.
        auto table_checker = [&task_table, size1, size2, &r_bucket, bpz, bucket_count, &v1, &v2]() -> bool {
            // Total number of term-by-term multiplications. Needs to be equal
            // to size1 * size2 at the end.
            integer tot_n(0);
            // Tmp term for multiplications.
            term_type tmp_term;
            for (decltype(task_table.size()) i = 0u; i < task_table.size(); ++i) {
                const auto &v = task_table[i];
                // Bucket limits of each zone.
                bucket_size_type a = static_cast<bucket_size_type>(bpz * i), b;
                // Special casing for the last zone in the table.
                if (i == task_table.size() - 1u) {
                    b = bucket_count;
                } else {
                    b = static_cast<bucket_size_type>(a + bpz);
                }
                for (const auto &t : v) {
                    auto idx1 = std::get<0u>(t), start2 = std::get<1u>(t), end2 = std::get<2u>(t);
                    using int_type = decltype(v1[idx1]->m_key.get_int());
                    piranha_assert(start2 <= end2);
                    tot_n += end2 - start2;
                    for (; start2 != end2; ++start2) {
                        tmp_term.m_key.set_int(
                            static_cast<int_type>(v1[idx1]->m_key.get_int() + v2[start2]->m_key.get_int()));
                        auto b_idx = r_bucket(&tmp_term);
                        if (b_idx < a || b_idx >= b) {
                            return false;
                        }
                    }
                }
            }
            return tot_n == integer(size1) * size2;
        };
        (void)table_checker;
        piranha_assert(table_checker());
        // Init the vector of atomic flags.
        detail::atomic_flag_array af(safe_cast<std::size_t>(task_table.size()));
        // Thread functor.
        auto thread_functor = [&task_table, &af, &task_consume](const unsigned &thread_idx) {
            using t_size_type = decltype(task_table.size());
            // Temporary term_type for caching.
            term_type tmp_term;
            // The starting index in the task table.
            auto t_idx = static_cast<t_size_type>(t_size_type(thread_idx) * zm);
            const auto start_t_idx = t_idx;
            while (true) {
                // If this returns false, it means that the tasks still need to be consumed;
                if (!af[static_cast<std::size_t>(t_idx)].test_and_set()) {
                    // Current vector of tasks.
                    const auto &cur_tasks = task_table[t_idx];
                    for (const auto &t : cur_tasks) {
                        task_consume(t, tmp_term);
                    }
                }
                // Update the index, wrapping around if necessary.
                t_idx = static_cast<t_size_type>(t_idx + 1u);
                if (t_idx == task_table.size()) {
                    t_idx = 0u;
                }
                // If we got back to the original index, get out.
                if (t_idx == start_t_idx) {
                    break;
                }
            }
        };
        // Go with the multiplication threads.
        future_list<decltype(thread_functor(0u))> ft_list;
        try {
            for (unsigned i = 0u; i < this->m_n_threads; ++i) {
                ft_list.push_back(thread_pool::enqueue(i, thread_functor, i));
            }
            // First let's wait for everything to finish.
            ft_list.wait_all();
            // Then, let's handle the exceptions.
            ft_list.get_all();
            // Finally, fix and finalise the series.
            this->sanitise_series(retval, this->m_n_threads);
            this->finalise_series(retval);
        } catch (...) {
            ft_list.wait_all();
            // Clean up and re-throw.
            retval._container().clear();
            throw;
        }
    }
};
}

#endif
