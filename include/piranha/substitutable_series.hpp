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

#ifndef PIRANHA_SUBSTITUTABLE_SERIES_HPP
#define PIRANHA_SUBSTITUTABLE_SERIES_HPP

#include <string>
#include <type_traits>
#include <utility>

#include <piranha/detail/init.hpp>
#include <piranha/forwarding.hpp>
#include <piranha/math.hpp>
#include <piranha/series.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace detail
{

// Tag for the subs toolbox.
struct substitutable_series_tag {
};
}

/// Toolbox for substitutable series.
/**
 * This toolbox will conditionally augment a \p Series type by adding a method to subtitute symbols with generic
 * objects. Such augmentation takes place if the series' coefficient and/or key types expose substitution methods (as
 * established by the piranha::has_subs and piranha::key_has_subs type traits). If the requirements outlined above are
 * not satisfied, the substitution method will be disabled.
 *
 * This class satisfies the piranha::is_series type trait.
 *
 * ## Type requirements ##
 *
 * - \p Series must satisfy the piranha::is_series type trait,
 * - \p Derived must derive from piranha::substitutable_series of \p Series and \p Derived.
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
class substitutable_series : public Series, detail::substitutable_series_tag
{
    typedef Series base;
    // Detect subs term.
    template <typename Term, typename T>
    using subs_term_score
        = std::integral_constant<unsigned,
                                 static_cast<unsigned>(has_subs<typename Term::cf_type, T>::value)
                                     + (static_cast<unsigned>(key_has_subs<typename Term::key_type, T>::value) << 1u)>;
    // Case 1: subs only on cf.
    template <typename T, typename Term>
    using cf_subs_type
        = decltype(math::subs(std::declval<typename Term::cf_type const &>(), std::declval<const symbol_fmap<T> &>()));
    template <typename T, typename Term>
    using ret_type_1 = decltype(std::declval<const cf_subs_type<T, Term> &>() * std::declval<Derived const &>());
    template <typename T, typename Term, enable_if_t<subs_term_score<Term, T>::value == 1u, int> = 0>
    static ret_type_1<T, Term> subs_term_impl(const Term &t, const symbol_fmap<T> &dict, const symbol_idx_fmap<T> &,
                                              const symbol_fset &s_set)
    {
        // NOTE: in these functions we could probably have optimised implementations, like in series'
        // partial, that use term insertion instead of arithmetics if the types allow it.
        Derived tmp;
        tmp.set_symbol_set(s_set);
        tmp.insert(Term(typename Term::cf_type(1), t.m_key));
        // NOTE: use moves here in case the multiplication can take advantage.
        return math::subs(t.m_cf, dict) * std::move(tmp);
    }
    // Case 2: subs only on key.
    template <typename T, typename Term>
    using k_subs_type = typename decltype(std::declval<const typename Term::key_type &>().subs(
        std::declval<const symbol_idx_fmap<T> &>(), std::declval<const symbol_fset &>()))::value_type::first_type;
    template <typename T, typename Term>
    using ret_type_2_ = decltype(std::declval<Derived const &>() * std::declval<const k_subs_type<T, Term> &>());
    template <typename T, typename Term>
    using ret_type_2 = enable_if_t<conjunction<is_addable_in_place<ret_type_2_<T, Term>>,
                                               std::is_constructible<ret_type_2_<T, Term>, const int &>>::value,
                                   ret_type_2_<T, Term>>;
    template <typename T, typename Term, enable_if_t<subs_term_score<Term, T>::value == 2u, int> = 0>
    static ret_type_2<T, Term> subs_term_impl(const Term &t, const symbol_fmap<T> &, const symbol_idx_fmap<T> &idx,
                                              const symbol_fset &s_set)
    {
        ret_type_2<T, Term> retval(0);
        auto ksubs = t.m_key.subs(idx, s_set);
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
    template <typename T, typename Term, enable_if_t<subs_term_score<Term, T>::value == 3u, int> = 0>
    static ret_type_3<T, Term> subs_term_impl(const Term &t, const symbol_fmap<T> &dict, const symbol_idx_fmap<T> &idx,
                                              const symbol_fset &s_set)
    {
        // Accumulator for the sum below. This is the same type resulting from case 2.
        ret_type_2<T, Term> acc(0);
        auto ksubs = t.m_key.subs(idx, s_set);
        auto cf_subs = math::subs(t.m_cf, dict);
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
    using subs_type_ = decltype(
        subs_term_impl(std::declval<typename Series::term_type const &>(), std::declval<const symbol_fmap<T> &>(),
                       std::declval<const symbol_idx_fmap<T> &>(), std::declval<symbol_fset const &>()));
    // Enable conditionally based on the common requirements in the subs() method, plus on the fact that
    // the subs type must be returnable.
    // NOTE: the returnable check here should be enough, as the functions above will not be called if the check
    // here is not passed.
    template <typename T>
    using subs_type
        = enable_if_t<conjunction<std::is_constructible<subs_type_<T>, const int &>, is_addable_in_place<subs_type_<T>>,
                                  is_returnable<subs_type_<T>>, has_sm_intersect_idx<T>>::value,
                      subs_type_<T>>;

public:
    /// Defaulted default constructor.
    substitutable_series() = default;
    /// Defaulted copy constructor.
    substitutable_series(const substitutable_series &) = default;
    /// Defaulted move constructor.
    substitutable_series(substitutable_series &&) = default;
    PIRANHA_FORWARDING_CTOR(substitutable_series, base)
    /// Trivial destructor.
    ~substitutable_series()
    {
        PIRANHA_TT_CHECK(is_series, substitutable_series);
        PIRANHA_TT_CHECK(is_series, Derived);
        PIRANHA_TT_CHECK(std::is_base_of, substitutable_series, Derived);
    }
    /// Copy assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     *
     * @throws unspecified any exception thrown by the assignment operator of the base class.
     */
    substitutable_series &operator=(const substitutable_series &other) = default;
    /// Move assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     */
    substitutable_series &operator=(substitutable_series &&other) = default;
    PIRANHA_FORWARDING_ASSIGNMENT(substitutable_series, base)
    /// Substitution.
    /**
     * \note
     * This method is enabled only if the coefficient and/or key types support substitution,
     * and if the types involved in the substitution support the necessary arithmetic operations
     * to compute the result. Also, the return type must satisfy piranha::is_returnable, and
     * ``T`` must be suitable for use in sm_intersect_idx().
     *
     * This method will return an object resulting from the substitution in \p this of the symbols in \p dict
     * with the mapped values.
     *
     * @param dict a dictionary mapping a set of symbols to the values that will be substituted for them.
     *
     * @return the result of the substitution.
     *
     * @throws unspecified any exception resulting from:
     * - the substitution routines for the coefficients and/or keys,
     * - the computation of the return value,
     * - piranha::series::insert().
     */
    template <typename T>
    subs_type<T> subs(const symbol_fmap<T> &dict) const
    {
        const auto idx = sm_intersect_idx(this->m_symbol_set, dict);
        subs_type<T> retval(0);
        for (const auto &t : this->m_container) {
            retval += subs_term_impl(t, dict, idx, this->m_symbol_set);
        }
        return retval;
    }
};

namespace detail
{

// Enabler for the specialisation of subs functor for subs series.
template <typename Series, typename T>
using subs_impl_subs_series_enabler = enable_if_t<conjunction<
    std::is_base_of<substitutable_series_tag, Series>,
    is_returnable<decltype(std::declval<const Series &>().subs(std::declval<const symbol_fmap<T> &>()))>>::value>;
}

namespace math
{

/// Specialisation of the piranha::math::subs_impl functor for instances of piranha::substitutable_series.
/**
 * This specialisation is activated if \p Series is an instance of piranha::substitutable_series which supports
 * the substitution method, returning a type which satisfies piranha::is_returnable.
 */
template <typename Series, typename T>
struct subs_impl<Series, T, detail::subs_impl_subs_series_enabler<Series, T>> {
    /// Call operator.
    /**
     * The call operator is equivalent to calling the substitution method on \p s.
     *
     * @param s the target series.
     * @param dict the substitution dictionary.
     *
     * @return the result of the substitution.
     *
     * @throws unspecified any exception thrown by the series' substitution method.
     */
    auto operator()(const Series &s, const symbol_fmap<T> &dict) const -> decltype(s.subs(dict))
    {
        return s.subs(dict);
    }
};
}
}

#endif
