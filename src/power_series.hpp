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

#ifndef PIRANHA_POWER_SERIES_HPP
#define PIRANHA_POWER_SERIES_HPP

#include <algorithm>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "detail/safe_integral_adder.hpp"
#include "forwarding.hpp"
#include "math.hpp"
#include "safe_cast.hpp"
#include "series.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"

namespace piranha
{

inline namespace impl
{

// Tag for power series instances.
struct power_series_tag {
};

// Detect power series terms.
template <typename T>
struct ps_term_score {
    typedef typename T::cf_type cf_type;
    typedef typename T::key_type key_type;
    static const unsigned value
        = static_cast<unsigned>(has_degree<cf_type>::value && has_ldegree<cf_type>::value)
          + (static_cast<unsigned>(key_has_degree<key_type>::value && key_has_ldegree<key_type>::value) << 1u);
};

// Common checks on degree/ldegree type for use in enabling conditions below.
template <typename T>
using common_degree_type_checks
    = std::integral_constant<bool, conjunction<std::is_constructible<T, int>, is_less_than_comparable<T>,
                                               is_container_element<T>>::value>;

// Total (low) degree computation.
#define PIRANHA_DEFINE_PS_PROPERTY_GETTER(property)                                                                    \
    template <typename Term, enable_if_t<ps_term_score<Term>::value == 1u, int> = 0>                                   \
    inline auto ps_get_##property(const Term &t, const symbol_set &)->decltype(math::property(t.m_cf))                 \
    {                                                                                                                  \
        return math::property(t.m_cf);                                                                                 \
    }                                                                                                                  \
    template <typename Term, enable_if_t<ps_term_score<Term>::value == 2u, int> = 0>                                   \
    inline auto ps_get_##property(const Term &t, const symbol_set &s)->decltype(t.m_key.property(s))                   \
    {                                                                                                                  \
        return t.m_key.property(s);                                                                                    \
    }                                                                                                                  \
    template <                                                                                                         \
        typename Term,                                                                                                 \
        enable_if_t<ps_term_score<Term>::value == 3u                                                                   \
                        && conjunction<std::is_integral<decltype(math::property(std::declval<const Term &>().m_cf))>,  \
                                       std::is_integral<decltype(std::declval<const Term &>().m_key.property(          \
                                           std::declval<const symbol_set &>()))>>::value,                              \
                    int> = 0>                                                                                          \
    inline auto ps_get_##property(const Term &t, const symbol_set &s)                                                  \
        ->decltype(math::property(t.m_cf) + t.m_key.property(s))                                                       \
    {                                                                                                                  \
        using ret_type = decltype(math::property(t.m_cf) + t.m_key.property(s));                                       \
        ret_type retval(math::property(t.m_cf));                                                                       \
        detail::safe_integral_adder(retval, static_cast<ret_type>(t.m_key.property(s)));                               \
        return retval;                                                                                                 \
    }                                                                                                                  \
    template <                                                                                                         \
        typename Term,                                                                                                 \
        enable_if_t<ps_term_score<Term>::value == 3u                                                                   \
                        && disjunction<negation<std::is_integral<decltype(                                             \
                                           math::property(std::declval<const Term &>().m_cf))>>,                       \
                                       negation<std::is_integral<decltype(std::declval<const Term &>().m_key.property( \
                                           std::declval<const symbol_set &>()))>>>::value,                             \
                    int> = 0>                                                                                          \
    inline auto ps_get_##property(const Term &t, const symbol_set &s)                                                  \
        ->decltype(math::property(t.m_cf) + t.m_key.property(s))                                                       \
    {                                                                                                                  \
        return math::property(t.m_cf) + t.m_key.property(s);                                                           \
    }                                                                                                                  \
    template <typename T>                                                                                              \
    using ps_##property##_type_ = decltype(                                                                            \
        ps_get_##property(std::declval<const typename T::term_type &>(), std::declval<const symbol_set &>()));         \
    template <typename T>                                                                                              \
    using ps_##property##_type                                                                                         \
        = enable_if_t<common_degree_type_checks<ps_##property##_type_<T>>::value, ps_##property##_type_<T>>;
PIRANHA_DEFINE_PS_PROPERTY_GETTER(degree)
PIRANHA_DEFINE_PS_PROPERTY_GETTER(ldegree)
#undef PIRANHA_DEFINE_PS_PROPERTY_GETTER

// Partial (low) degree computation.
#define PIRANHA_DEFINE_PARTIAL_PS_PROPERTY_GETTER(property)                                                            \
    template <typename Term, enable_if_t<ps_term_score<Term>::value == 1u, int> = 0>                                   \
    inline auto ps_get_##property(const Term &t, const std::vector<std::string> &names, const symbol_set::positions &, \
                                  const symbol_set &)                                                                  \
        ->decltype(math::property(t.m_cf, names))                                                                      \
    {                                                                                                                  \
        return math::property(t.m_cf, names);                                                                          \
    }                                                                                                                  \
    template <typename Term, enable_if_t<ps_term_score<Term>::value == 2u, int> = 0>                                   \
    inline auto ps_get_##property(const Term &t, const std::vector<std::string> &, const symbol_set::positions &p,     \
                                  const symbol_set &s)                                                                 \
        ->decltype(t.m_key.property(p, s))                                                                             \
    {                                                                                                                  \
        return t.m_key.property(p, s);                                                                                 \
    }                                                                                                                  \
    template <typename Term,                                                                                           \
              enable_if_t<ps_term_score<Term>::value == 3u                                                             \
                              && conjunction<std::is_integral<decltype(                                                \
                                                 math::property(std::declval<const Term &>().m_cf,                     \
                                                                std::declval<const std::vector<std::string> &>()))>,   \
                                             std::is_integral<decltype(std::declval<const Term &>().m_key.property(    \
                                                 std::declval<const symbol_set::positions &>(),                        \
                                                 std::declval<const symbol_set &>()))>>::value,                        \
                          int> = 0>                                                                                    \
    inline auto ps_get_##property(const Term &t, const std::vector<std::string> &names,                                \
                                  const symbol_set::positions &p, const symbol_set &s)                                 \
        ->decltype(math::property(t.m_cf, names) + t.m_key.property(p, s))                                             \
    {                                                                                                                  \
        using ret_type = decltype(math::property(t.m_cf, names) + t.m_key.property(p, s));                             \
        ret_type retval(math::property(t.m_cf, names));                                                                \
        detail::safe_integral_adder(retval, static_cast<ret_type>(t.m_key.property(p, s)));                            \
        return retval;                                                                                                 \
    }                                                                                                                  \
    template <                                                                                                         \
        typename Term,                                                                                                 \
        enable_if_t<ps_term_score<Term>::value == 3u                                                                   \
                        && disjunction<negation<std::is_integral<decltype(                                             \
                                           math::property(std::declval<const Term &>().m_cf,                           \
                                                          std::declval<const std::vector<std::string> &>()))>>,        \
                                       negation<std::is_integral<decltype(std::declval<const Term &>().m_key.property( \
                                           std::declval<const symbol_set::positions &>(),                              \
                                           std::declval<const symbol_set &>()))>>>::value,                             \
                    int> = 0>                                                                                          \
    inline auto ps_get_##property(const Term &t, const std::vector<std::string> &names,                                \
                                  const symbol_set::positions &p, const symbol_set &s)                                 \
        ->decltype(math::property(t.m_cf, names) + t.m_key.property(p, s))                                             \
    {                                                                                                                  \
        return math::property(t.m_cf, names) + t.m_key.property(p, s);                                                 \
    }                                                                                                                  \
    template <typename T>                                                                                              \
    using ps_p##property##_type_ = decltype(ps_get_##property(                                                         \
        std::declval<const typename T::term_type &>(), std::declval<const std::vector<std::string> &>(),               \
        std::declval<const symbol_set::positions &>(), std::declval<const symbol_set &>()));                           \
    template <typename T>                                                                                              \
    using ps_p##property##_type                                                                                        \
        = enable_if_t<common_degree_type_checks<ps_p##property##_type_<T>>::value, ps_p##property##_type_<T>>;
PIRANHA_DEFINE_PARTIAL_PS_PROPERTY_GETTER(degree)
PIRANHA_DEFINE_PARTIAL_PS_PROPERTY_GETTER(ldegree)
#undef PIRANHA_DEFINE_PARTIAL_PS_PROPERTY_GETTER

// Total degree truncation.
// Case 1: coefficient can truncate, no degree or ldegree in key.
template <typename Term, typename T,
          enable_if_t<has_truncate_degree<typename Term::cf_type, T>::value && (ps_term_score<Term>::value >> 1u) == 0u,
                      int> = 0>
inline std::pair<bool, Term> ps_truncate_term(const Term &t, const T &max_degree, const symbol_set &)
{
    return std::make_pair(true, Term(math::truncate_degree(t.m_cf, max_degree), t.m_key));
}

// Case 2: coefficient cannot truncate, degree and ldegree in key, degrees are greater_than comparable.
// NOTE: here we do not have support for key truncation (yet), so we decide based on the low degree of the key:
// if it is larger than the max degree, remove the term, otherwise keep it - it is an all-or-nothing scenario.
template <
    typename Term, typename T,
    enable_if_t<(ps_term_score<Term>::value >> 1u) == 1u
                    && conjunction<negation<has_truncate_degree<typename Term::cf_type, T>>,
                                   is_greater_than_comparable<decltype(
                                                                  std::declval<const typename Term::key_type &>()
                                                                      .ldegree(std::declval<const symbol_set &>())),
                                                              T>>::value,
                int> = 0>
inline std::pair<bool, Term> ps_truncate_term(const Term &t, const T &max_degree, const symbol_set &s)
{
    if (t.m_key.ldegree(s) > max_degree) {
        // Term must be discarded.
        return std::make_pair(false, Term());
    } else {
        // Keep the term as it is.
        return std::make_pair(true, Term(t.m_cf, t.m_key));
    }
}

// Case 3: coefficient can truncate, degree and ldegree in key, the new max degree type can be used in the
// coefficient truncation.
// NOTE: again, no key truncation, thus we decrement the real degree of the coefficient by the low degree of the
// key. This way we will keep all the important parts, plus some garbage.
template <typename Term, typename T,
          enable_if_t<has_truncate_degree<typename Term::cf_type,
                                          decltype(std::declval<const T &>()
                                                   - std::declval<const typename Term::key_type &>().ldegree(
                                                         std::declval<const symbol_set &>()))>::value
                          && (ps_term_score<Term>::value >> 1u) == 1u,
                      int> = 0>
inline std::pair<bool, Term> ps_truncate_term(const Term &t, const T &max_degree, const symbol_set &s)
{
    // The truncation level for the coefficient must be modified in order to take
    // into account the degree of the key.
    return std::make_pair(true, Term(math::truncate_degree(t.m_cf, max_degree - t.m_key.ldegree(s)), t.m_key));
}

// Partial degree truncation.
// Case 1: coefficient can truncate, no degree or ldegree in key.
template <typename Term, typename T,
          enable_if_t<has_truncate_degree<typename Term::cf_type, T>::value && (ps_term_score<Term>::value >> 1u) == 0u,
                      int> = 0>
inline std::pair<bool, Term> ps_truncate_term(const Term &t, const T &max_degree, const std::vector<std::string> &names,
                                              const symbol_set::positions &, const symbol_set &)
{
    return std::make_pair(true, Term(math::truncate_degree(t.m_cf, max_degree, names), t.m_key));
}

// Case 2: coefficient cannot truncate, degree and ldegree in key, degrees are greater_than comparable.
template <
    typename Term, typename T,
    enable_if_t<(ps_term_score<Term>::value >> 1u) == 1u
                    && conjunction<negation<has_truncate_degree<typename Term::cf_type, T>>,
                                   is_greater_than_comparable<decltype(
                                                                  std::declval<const typename Term::key_type &>()
                                                                      .ldegree(
                                                                          std::declval<const symbol_set::positions &>(),
                                                                          std::declval<const symbol_set &>())),
                                                              T>>::value,
                int> = 0>
inline std::pair<bool, Term> ps_truncate_term(const Term &t, const T &max_degree, const std::vector<std::string> &,
                                              const symbol_set::positions &p, const symbol_set &s)
{
    if (t.m_key.ldegree(p, s) > max_degree) {
        return std::make_pair(false, Term());
    } else {
        return std::make_pair(true, Term(t.m_cf, t.m_key));
    }
}

// Case 3: coefficient can truncate, degree and ldegree in key, the new max degree type can be used in the
// coefficient truncation.
template <typename Term, typename T,
          enable_if_t<has_truncate_degree<typename Term::cf_type,
                                          decltype(std::declval<const T &>()
                                                   - std::declval<const typename Term::key_type &>().ldegree(
                                                         std::declval<const symbol_set::positions &>(),
                                                         std::declval<const symbol_set &>()))>::value
                          && (ps_term_score<Term>::value >> 1u) == 1u,
                      int> = 0>
inline std::pair<bool, Term> ps_truncate_term(const Term &t, const T &max_degree, const std::vector<std::string> &names,
                                              const symbol_set::positions &p, const symbol_set &s)
{
    return std::make_pair(true,
                          Term(math::truncate_degree(t.m_cf, max_degree - t.m_key.ldegree(p, s), names), t.m_key));
}
}

/// Power series toolbox.
/**
 * This toolbox is intended to extend the \p Series type with properties of formal power series.
 *
 * Specifically, the toolbox will conditionally augment a \p Series type by adding methods to query the total and
 * partial (low) degree  * of a \p Series object. Such augmentation takes place if the series' coefficient and/or key
 * types expose methods to query their degree properties (as established by the piranha::has_degree,
 * piranha::key_has_degree and similar type traits), and if the necessary arithmetic operations are supported by the
 * involved types. As an additional requirement, the types returned when querying the degree must be constructible from
 * \p int, less-than comparable and they must satisfy piranha::is_container_element. If the computation of the degree of
 * a single term involves only C++ integral types, then the computation will be checked for overflow.
 *
 * This toolbox provides also support for truncation based on the total or partial degree. In addition to the
 * requirements of the degree-querying methods, the truncation methods also require the supplied degree limit to be
 * comparable to the type returned by the degree-querying methods. The truncation methods will recursively truncate the
 * coefficients of the series via the piranha::math::truncate_degree() function.
 *
 * If the requirements outlined above are not satisfied, the degree-querying and the truncation methods will be
 * disabled.
 *
 * This class satisfies the piranha::is_series type trait.
 *
 * ## Type requirements ##
 *
 * - \p Series must satisfy the piranha::is_series type trait,
 * - \p Derived must derive from power_series of \p Series and \p Derived.
 *
 * ## Exception safety guarantee ##
 *
 * This class provides the same guarantee as \p Series. The auto-truncation methods offer the basic exception safety
 * guarantee.
 *
 * ## Move semantics ##
 *
 * Move semantics is equivalent to the move semantics of \p Series.
 */
template <typename Series, typename Derived>
class power_series : public Series, power_series_tag
{
    PIRANHA_TT_CHECK(is_series, Series);
    using base = Series;
    // Enabler for total degree truncation.
    template <typename T>
    using t_truncate_term_t = decltype(ps_truncate_term(std::declval<const typename base::term_type &>(),
                                                        std::declval<const T &>(), std::declval<const symbol_set &>()));
    template <typename T>
    using truncate_degree_enabler = enable_if_t<is_detected<t_truncate_term_t, T>::value, int>;
    // Enabler for partial degree truncation.
    template <typename T>
    using p_truncate_term_t
        = decltype(ps_truncate_term(std::declval<const typename base::term_type &>(), std::declval<const T &>(),
                                    std::declval<const std::vector<std::string> &>(),
                                    std::declval<const symbol_set::positions &>(), std::declval<const symbol_set &>()));
    template <typename T>
    using truncate_pdegree_enabler = enable_if_t<is_detected<p_truncate_term_t, T>::value, int>;
    // Lift definitions from the impl namespace.
    template <typename T>
    using degree_type = ps_degree_type<T>;
    template <typename T>
    using ldegree_type = ps_ldegree_type<T>;
    template <typename T>
    using pdegree_type = ps_pdegree_type<T>;
    template <typename T>
    using pldegree_type = ps_pldegree_type<T>;

public:
    /// Defaulted default constructor.
    power_series() = default;
    /// Defaulted copy constructor.
    power_series(const power_series &) = default;
    /// Defaulted move constructor.
    power_series(power_series &&) = default;
    PIRANHA_FORWARDING_CTOR(power_series, base)
    /// Copy assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     *
     * @throws unspecified any exception thrown by the assignment operator of the base class.
     */
    power_series &operator=(const power_series &other) = default;
    /// Move assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     */
    power_series &operator=(power_series &&other) = default;
    PIRANHA_FORWARDING_ASSIGNMENT(power_series, base)
    /// Trivial destructor.
    ~power_series()
    {
        PIRANHA_TT_CHECK(is_series, power_series);
        PIRANHA_TT_CHECK(std::is_base_of, power_series, Derived);
    }
    /// Total degree.
    /**
     * \note
     * This method is available only if the requisites outlined in piranha::power_series are satisfied.
     *
     * The degree of the series is the maximum degree of its terms. If the series is empty, zero will be returned.
     *
     * @return the total degree of the series.
     *
     * @throws std::overflow_error if the computation results in an overflow.
     * @throws unspecified any exception thrown by:
     * - the construction of return type,
     * - the calculation of the degree of each term,
     * - the assignment and less-than operators for the return type.
     */
    template <typename T = power_series>
    degree_type<T> degree() const
    {
        using term_type = typename T::term_type;
        auto it = std::max_element(
            this->m_container.begin(), this->m_container.end(), [this](const term_type &t1, const term_type &t2) {
                return ps_get_degree(t1, this->m_symbol_set) < ps_get_degree(t2, this->m_symbol_set);
            });
        return (it == this->m_container.end()) ? degree_type<T>(0) : ps_get_degree(*it, this->m_symbol_set);
    }
    /// Total low degree.
    /**
     * \note
     * This method is available only if the requisites outlined in piranha::power_series are satisfied.
     *
     * The low degree of the series is the minimum low degree of its terms. If the series is empty, zero will be
     * returned.
     *
     * @return the total low degree of the series.
     *
     * @throws std::overflow_error if the computation results in an overflow.
     * @throws unspecified any exception thrown by:
     * - the construction of return type,
     * - the calculation of the low degree of each term,
     * - the assignment and less-than operators for the return type.
     */
    template <typename T = power_series>
    ldegree_type<T> ldegree() const
    {
        using term_type = typename T::term_type;
        auto it = std::min_element(
            this->m_container.begin(), this->m_container.end(), [this](const term_type &t1, const term_type &t2) {
                return ps_get_ldegree(t1, this->m_symbol_set) < ps_get_ldegree(t2, this->m_symbol_set);
            });
        return (it == this->m_container.end()) ? ldegree_type<T>(0) : ps_get_ldegree(*it, this->m_symbol_set);
    }
    /// Partial degree.
    /**
     * \note
     * This method is available only if the requisites outlined in piranha::power_series are satisfied.
     *
     * The partial degree of the series is the maximum partial degree of its terms. If the series is empty, zero will be
     * returned.
     *
     * @param names names of the variables to be considered in the computation of the degree.
     *
     * @return the partial degree of the series.
     *
     * @throws std::overflow_error if the computation results in an overflow.
     * @throws unspecified any exception thrown by:
     * - the construction of return type,
     * - the calculation of the degree of each term,
     * - the assignment and less-than operators for the return type.
     */
    template <typename T = power_series>
    pdegree_type<T> degree(const std::vector<std::string> &names) const
    {
        using term_type = typename T::term_type;
        const symbol_set::positions p(this->m_symbol_set, symbol_set(names.begin(), names.end()));
        auto it = std::max_element(this->m_container.begin(), this->m_container.end(),
                                   [this, &p, &names](const term_type &t1, const term_type &t2) {
                                       return ps_get_degree(t1, names, p, this->m_symbol_set)
                                              < ps_get_degree(t2, names, p, this->m_symbol_set);
                                   });
        return (it == this->m_container.end()) ? pdegree_type<T>(0) : ps_get_degree(*it, names, p, this->m_symbol_set);
    }
    /// Partial low degree.
    /**
     * \note
     * This method is available only if the requisites outlined in piranha::power_series are satisfied.
     *
     * The partial low degree of the series is the minimum partial low degree of its terms. If the series is empty, zero
     * will be returned.
     *
     * @param names names of the variables to be considered in the computation of the low degree.
     *
     * @return the partial low degree of the series.
     *
     * @throws std::overflow_error if the computation results in an overflow.
     * @throws unspecified any exception thrown by:
     * - the construction of return type,
     * - the calculation of the low degree of each term,
     * - the assignment and less-than operators for the return type.
     */
    template <typename T = power_series>
    pldegree_type<T> ldegree(const std::vector<std::string> &names) const
    {
        using term_type = typename T::term_type;
        const symbol_set::positions p(this->m_symbol_set, symbol_set(names.begin(), names.end()));
        auto it = std::min_element(this->m_container.begin(), this->m_container.end(),
                                   [this, &p, &names](const term_type &t1, const term_type &t2) {
                                       return ps_get_ldegree(t1, names, p, this->m_symbol_set)
                                              < ps_get_ldegree(t2, names, p, this->m_symbol_set);
                                   });
        return (it == this->m_container.end()) ? pldegree_type<T>(0)
                                               : ps_get_ldegree(*it, names, p, this->m_symbol_set);
    }
    /// Total degree truncation.
    /**
     * \note
     * This method is available only if the requisites outlined in piranha::power_series are satisfied.
     *
     * This method can be used to eliminate the parts of a series whose degree is greater than \p max_degree.
     * This includes the elimination of whole terms, but also the recursive truncation of coefficients
     * via the piranha::math::truncate_degree() function, if supported by the coefficient. It must be noted
     * that, in general, this method is not guaranteed to eliminate all the parts whose degree is greater than \p
     * max_degree (in particular, in the current implementation there is no truncation implemented for keys -
     * a key is kept as-is or completely eliminated).
     *
     * @param max_degree maximum allowed total degree.
     *
     * @return the truncated counterpart of \p this.
     *
     * @throws unspecified any exception thrown by:
     * - piranha::math::truncate_degree(), if used,
     * - the constructor of the term type,
     * - the computation and comparison of degree types,
     * - piranha::series::insert().
     */
    template <typename T, truncate_degree_enabler<T> = 0>
    Derived truncate_degree(const T &max_degree) const
    {
        Derived retval;
        retval.m_symbol_set = this->m_symbol_set;
        const auto it_f = this->m_container.end();
        for (auto it = this->m_container.begin(); it != it_f; ++it) {
            auto tmp = ps_truncate_term(*it, max_degree, retval.m_symbol_set);
            if (tmp.first) {
                retval.insert(std::move(tmp.second));
            }
        }
        return retval;
    }
    /// Partial degree truncation.
    /**
     * \note
     * This method is available only if the requisites outlined in piranha::power_series are satisfied.
     *
     * This method is equivalent to the other overload, the only difference being that the partial degree is considered
     * in the computation.
     *
     * @param max_degree maximum allowed partial degree.
     * @param names names of the variables to be considered in the computation of the partial degree.
     *
     * @return the truncated counterpart of \p this.
     *
     * @throws unspecified any exception thrown by:
     * - piranha::math::truncate_degree(), if used,
     * - the constructor of the term type,
     * - the computation and comparison of degree types,
     * - piranha::series::insert().
     */
    template <typename T, truncate_pdegree_enabler<T> = 0>
    Derived truncate_degree(const T &max_degree, const std::vector<std::string> &names) const
    {
        Derived retval;
        retval.m_symbol_set = this->m_symbol_set;
        const symbol_set::positions p(this->m_symbol_set, symbol_set(names.begin(), names.end()));
        const auto it_f = this->m_container.end();
        for (auto it = this->m_container.begin(); it != it_f; ++it) {
            auto tmp = ps_truncate_term(*it, max_degree, names, p, retval.m_symbol_set);
            if (tmp.first) {
                retval.insert(std::move(tmp.second));
            }
        }
        return retval;
    }
};

inline namespace impl
{

// Enabler for the implementation of degree-related math functions for power_series.
template <typename Series>
using ps_degree_enabler = enable_if_t<std::is_base_of<power_series_tag, Series>::value>;
}

namespace math
{

/// Specialisation of the piranha::math::degree() functor for instances of piranha::power_series.
/**
 * This specialisation is activated if \p Series is an instance of piranha::power_series. If \p Series
 * does not fulfill the requirements outlined in piranha::power_series, the call operator will be disabled.
 */
template <typename Series>
struct degree_impl<Series, ps_degree_enabler<Series>> {
    /// Call operator.
    /**
     * If available, it will call piranha::power_series::degree().
     *
     * @param s input power series.
     * @param args additional arguments that will be passed to the series' method.
     *
     * @return the degree of input series \p s.
     *
     * @throws unspecified any exception thrown by the invoked method of the series.
     */
    template <typename... Args>
    auto operator()(const Series &s, const Args &... args) const -> decltype(s.degree(args...))
    {
        return s.degree(args...);
    }
};

/// Specialisation of the piranha::math::ldegree() functor for instances of piranha::power_series.
/**
 * This specialisation is activated if \p Series is an instance of piranha::power_series. If \p Series
 * does not fulfill the requirements outlined in piranha::power_series, the call operator will be disabled.
 */
template <typename Series>
struct ldegree_impl<Series, ps_degree_enabler<Series>> {
    /// Call operator.
    /**
     * If available, it will call piranha::power_series::ldegree().
     *
     * @param s input power series.
     * @param args additional arguments that will be passed to the series' method.
     *
     * @return the low degree of input series \p s.
     *
     * @throws unspecified any exception thrown by the invoked method of the series.
     */
    template <typename... Args>
    auto operator()(const Series &s, const Args &... args) const -> decltype(s.ldegree(args...))
    {
        return s.ldegree(args...);
    }
};

/// Specialisation of the piranha::math::truncate_degree() functor for instances of piranha::power_series.
/**
 * This specialisation is activated if \p Series is an instance of piranha::power_series. If \p Series
 * does not fulfill the requirements outlined in piranha::power_series, the call operator will be disabled.
 */
template <typename Series, typename T>
struct truncate_degree_impl<Series, T, ps_degree_enabler<Series>> {
    /// Call operator.
    /**
     * If available, it will call piranha::power_series::truncate_degree().
     *
     * @param s input power series.
     * @param max_degree maximum degree.
     * @param args additional arguments that will be passed to the series' method.
     *
     * @return the truncated version of input series \p s.
     *
     * @throws unspecified any exception thrown by the invoked method of the series.
     */
    template <typename... Args>
    auto operator()(const Series &s, const T &max_degree, const Args &... args) const
        -> decltype(s.truncate_degree(max_degree, args...))
    {
        return s.truncate_degree(max_degree, args...);
    }
};
}
}

#endif
