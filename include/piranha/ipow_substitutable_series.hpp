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

#ifndef PIRANHA_IPOW_SUBSTITUTABLE_SERIES_HPP
#define PIRANHA_IPOW_SUBSTITUTABLE_SERIES_HPP

#include <string>
#include <type_traits>
#include <utility>

#include <piranha/forwarding.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/series.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

inline namespace impl
{

struct ipow_substitutable_series_tag {
};
}

/// Toolbox for series suitable for integral power substitution.
/**
 * This toolbox will conditionally augment a \p Series type by adding methods to subtitute integral powers of symbols
 * with generic objects. Such augmentation takes place if the series' coefficient and/or key types expose substitution
 * methods (as established by the piranha::has_ipow_subs and piranha::key_has_ipow_subs type traits). If the
 * requirements outlined above are not satisfied, the substitution methods will be disabled.
 *
 * This class satisfies the piranha::is_series type trait.
 *
 * ## Type requirements ##
 *
 * - \p Series must satisfy the piranha::is_series type trait,
 * - \p Derived must derive from piranha::ipow_substitutable_series of \p Series and \p Derived.
 *
 * ## Exception safety guarantee ##
 *
 * This class provides the same guarantee as \p Series.
 *
 * ## Move semantics ##
 *
 * Move semantics is equivalent to the move semantics of \p Series.
 */
template <typename Series, typename Derived>
class ipow_substitutable_series : public Series, ipow_substitutable_series_tag
{
    typedef Series base;
    // Detect subs term.
    template <typename Term, typename T>
    struct subs_term_score {
        static const unsigned value
            = static_cast<unsigned>(has_ipow_subs<typename Term::cf_type, T>::value)
              + (static_cast<unsigned>(key_has_ipow_subs<typename Term::key_type, T>::value) << 1u);
    };
    // Case 1: subs only on cf.
    template <typename T, typename Term>
    using cf_subs_type
        = decltype(math::ipow_subs(std::declval<typename Term::cf_type const &>(), std::declval<std::string const &>(),
                                   std::declval<const integer &>(), std::declval<T const &>()));
    template <typename T, typename Term>
    using ret_type_1 = decltype(std::declval<const cf_subs_type<T, Term> &>() * std::declval<Derived const &>());
    template <typename T, typename Term, typename std::enable_if<subs_term_score<Term, T>::value == 1u, int>::type = 0>
    static ret_type_1<T, Term> subs_term_impl(const Term &t, const symbol_idx &idx, const std::string &name,
                                              const integer &n, const T &x, const symbol_fset &s_set)
    {
        piranha_assert(ss_index_of(s_set, name) == idx);
        (void)idx;
        Derived tmp;
        tmp.set_symbol_set(s_set);
        tmp.insert(Term(typename Term::cf_type(1), t.m_key));
        // NOTE: use moves here in case the multiplication can take advantage.
        return math::ipow_subs(t.m_cf, name, n, x) * std::move(tmp);
    }
    // Case 2: subs only on key.
    template <typename T, typename Term>
    using k_subs_type = typename decltype(std::declval<const typename Term::key_type &>().ipow_subs(
        std::declval<const symbol_idx &>(), std::declval<const integer &>(), std::declval<const T &>(),
        std::declval<const symbol_fset &>()))::value_type::first_type;
    template <typename T, typename Term>
    using ret_type_2_ = decltype(std::declval<Derived const &>() * std::declval<const k_subs_type<T, Term> &>());
    template <typename T, typename Term>
    using ret_type_2 = typename std::enable_if<is_addable_in_place<ret_type_2_<T, Term>>::value
                                                   && std::is_constructible<ret_type_2_<T, Term>, const int &>::value,
                                               ret_type_2_<T, Term>>::type;
    template <typename T, typename Term, typename std::enable_if<subs_term_score<Term, T>::value == 2u, int>::type = 0>
    static ret_type_2<T, Term> subs_term_impl(const Term &t, const symbol_idx &idx, const std::string &name,
                                              const integer &n, const T &x, const symbol_fset &s_set)
    {
        piranha_assert(ss_index_of(s_set, name) == idx);
        (void)name;
        ret_type_2<T, Term> retval(0);
        auto ksubs = t.m_key.ipow_subs(idx, n, x, s_set);
        for (auto &p : ksubs) {
            Derived tmp;
            tmp.set_symbol_set(s_set);
            tmp.insert(Term{t.m_cf, std::move(p.second)});
            // NOTE: possible use of multadd here in the future.
            retval += std::move(tmp) * std::move(p.first);
        }
        return retval;
    }
    // Case 3: subs on cf and key.
    // NOTE: the checks on type 2 are already present in the alias above.
    template <typename T, typename Term>
    using ret_type_3
        = decltype(std::declval<const cf_subs_type<T, Term> &>() * std::declval<const ret_type_2<T, Term> &>());
    template <typename T, typename Term, typename std::enable_if<subs_term_score<Term, T>::value == 3u, int>::type = 0>
    static ret_type_3<T, Term> subs_term_impl(const Term &t, const symbol_idx &idx, const std::string &name,
                                              const integer &n, const T &x, const symbol_fset &s_set)
    {
        piranha_assert(ss_index_of(s_set, name) == idx);
        // Accumulator for the sum below. This is the same type resulting from case 2.
        ret_type_2<T, Term> acc(0);
        auto ksubs = t.m_key.ipow_subs(idx, n, x, s_set);
        auto cf_subs = math::ipow_subs(t.m_cf, name, n, x);
        for (auto &p : ksubs) {
            Derived tmp;
            tmp.set_symbol_set(s_set);
            tmp.insert(Term(typename Term::cf_type(1), std::move(p.second)));
            // NOTE: multadd chance.
            acc += std::move(tmp) * std::move(p.first);
        }
        return std::move(cf_subs) * std::move(acc);
    }
    // Initial definition of the subs type.
    template <typename T>
    using subs_type_ = decltype(subs_term_impl(std::declval<typename Series::term_type const &>(),
                                               std::declval<const symbol_idx &>(), std::declval<const std::string &>(),
                                               std::declval<const integer &>(), std::declval<const T &>(),
                                               std::declval<symbol_fset const &>()));
    // Enable conditionally based on the common requirements in the ipow_subs() method.
    template <typename T>
    using ipow_subs_type = enable_if_t<
        conjunction<std::is_constructible<subs_type_<T>, const int &>, is_addable_in_place<subs_type_<T>>>::value,
        subs_type_<T>>;
    // Enabler for the alternate overload.
    template <typename Int>
    using ipow_subs_int_enabler = enable_if_t<std::is_integral<Int>::value, int>;

public:
    /// Defaulted default constructor.
    ipow_substitutable_series() = default;
    /// Defaulted copy constructor.
    ipow_substitutable_series(const ipow_substitutable_series &) = default;
    /// Defaulted move constructor.
    ipow_substitutable_series(ipow_substitutable_series &&) = default;
    PIRANHA_FORWARDING_CTOR(ipow_substitutable_series, base)
    /// Copy assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     *
     * @throws unspecified any exception thrown by the assignment operator of the base class.
     */
    ipow_substitutable_series &operator=(const ipow_substitutable_series &other) = default;
    /// Defaulted move assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     */
    ipow_substitutable_series &operator=(ipow_substitutable_series &&other) = default;
    /// Trivial destructor.
    ~ipow_substitutable_series()
    {
        PIRANHA_TT_CHECK(is_series, ipow_substitutable_series);
        PIRANHA_TT_CHECK(is_series, Derived);
        PIRANHA_TT_CHECK(std::is_base_of, ipow_substitutable_series, Derived);
    }
    PIRANHA_FORWARDING_ASSIGNMENT(ipow_substitutable_series, base)
    /// Substitution.
    /**
     * \note
     * This method is enabled only if the coefficient and/or key types support integral power substitution,
     * and if the types involved in the substitution support the necessary arithmetic operations
     * to compute the result.
     *
     * This method will return an object resulting from the substitution of the integral power of the symbol called \p
     * name in \p this with the generic object \p x.
     *
     * @param name name of the symbol to be substituted.
     * @param n integral power of the symbol to be substituted.
     * @param x object used for the substitution.
     *
     * @return the result of the substitution.
     *
     * @throws unspecified any exception resulting from:
     * - the substitution routines for the coefficients and/or keys,
     * - the computation of the return value,
     * - piranha::series::insert().
     */
    template <typename T>
    ipow_subs_type<T> ipow_subs(const std::string &name, const integer &n, const T &x) const
    {
        const auto idx = ss_index_of(this->m_symbol_set, name);
        ipow_subs_type<T> retval(0);
        for (const auto &t : this->m_container) {
            retval += subs_term_impl(t, idx, name, n, x, this->m_symbol_set);
        }
        return retval;
    }
    /// Substitution.
    /**
     * \note
     * This method is enabled only if the other ipow_subs() overload is enabled, and \p Int
     * is a C++ integral type.
     *
     * This is a convenience method that will call the other ipow_subs() overload after converting
     * \p n to piranha::integer.
     *
     * @param name name of the symbol to be substituted.
     * @param n integral power of the symbol to be substituted.
     * @param x object used for the substitution.
     *
     * @return the result of the substitution.
     *
     * @throws unspecified any exception resulting from calling the other ipow_subs() overload,
     * or by constructing a piranha::integer from a C++ integral type.
     */
    template <typename T, typename Int, ipow_subs_int_enabler<Int> = 0>
    ipow_subs_type<T> ipow_subs(const std::string &name, const Int &n, const T &x) const
    {
        return this->ipow_subs(name, integer(n), x);
    }
};

inline namespace impl
{

// Enabler for the specialisation of ipow_subs functor for ipow_subs series.
template <typename Series, typename T>
using ipow_subs_mem_t = decltype(std::declval<const Series &>().ipow_subs(
    std::declval<const std::string &>(), std::declval<const integer &>(), std::declval<const T &>()));

template <typename Series, typename T>
using ipow_subs_impl_ipow_subs_series_enabler
    = enable_if_t<conjunction<std::is_base_of<ipow_substitutable_series_tag, Series>,
                              is_detected<ipow_subs_mem_t, Series, T>>::value>;
}

namespace math
{

/// Specialisation of the piranha::math::ipow_subs_impl functor for instances of piranha::ipow_substitutable_series.
/**
 * This specialisation is activated if \p Series is an instance of piranha::ipow_substitutable_series which supports
 * the substitution methods.
 */
template <typename Series, typename T>
struct ipow_subs_impl<Series, T, ipow_subs_impl_ipow_subs_series_enabler<Series, T>> {
    /// Call operator.
    /**
     * The call operator is equivalent to calling the substitution method on \p s.
     *
     * @param s target series.
     * @param name name of the symbol to be substituted.
     * @param n integral power of the symbol to be substituted.
     * @param x object used for substitution.
     *
     * @return the result of the substitution.
     *
     * @throws unspecified any exception thrown by the series' substitution method.
     */
    auto operator()(const Series &s, const std::string &name, const integer &n, const T &x) const
        -> decltype(s.ipow_subs(name, n, x))
    {
        return s.ipow_subs(name, n, x);
    }
};
}
}

#endif
