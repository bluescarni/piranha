/***************************************************************************
 *   Copyright (C) 2009-2011 by Francesco Biscani                          *
 *   bluescarni@gmail.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef PIRANHA_TRIGONOMETRIC_SERIES_HPP
#define PIRANHA_TRIGONOMETRIC_SERIES_HPP

#include <algorithm>
#include <type_traits>
#include <utility>

#include "forwarding.hpp"
#include "math.hpp"
#include "symbol_set.hpp"
#include "toolbox.hpp"

namespace piranha
{

namespace detail
{

// Tag for trigonometric series.
struct trigonometric_series_tag {};

// Utilities to detect trigonometric terms.
template <typename Cf>
struct cf_trig_score
{
	static const int value = has_t_degree<Cf>::value + has_t_ldegree<Cf>::value +
		has_t_order<Cf>::value + has_t_lorder<Cf>::value;
};

template <typename Key>
struct key_trig_score
{
	static const int value = key_has_t_degree<Key>::value + key_has_t_ldegree<Key>::value +
		key_has_t_order<Key>::value + key_has_t_lorder<Key>::value;
};

// Type trait for trigonometric terms.
template <typename Term>
struct is_trigonometric_term
{
	static const bool value = (cf_trig_score<typename Term::cf_type>::value == 4 &&
		key_trig_score<typename Term::key_type>::value == 0) ||
		(cf_trig_score<typename Term::cf_type>::value == 0 &&
		key_trig_score<typename Term::key_type>::value == 4);
};

}

/// Trigonometric series toolbox.
/**
 * This toolbox extends a series class with properties of trigonometric series. Specifically, this class will conditionally add methods
 * to query the trigonometric (partial,low) degree and order of a series. This augmentation takes places if either the coefficient or the
 * key satisfy the relevant type traits: piranha::has_t_degree and related for the coefficient type, piranha::key_has_t_degree and related
 * for the key type.
 * 
 * Note that in order for the trigonometric methods to be enabled, coefficient and key type cannot satisfy these type traits at the same time,
 * and all degree/order type traits need to be satisfied for the coefficient or key type.
 * 
 * If the above requirements are not satisfied, this class will not add any new functionality to the \p Series class and
 * will just provide generic constructors and assignment operators that will forward their arguments to \p Series.
 * 
 * This class is a model of the piranha::concept::Series concept.
 * 
 * \section type_requirements Type requirements
 * 
 * \p Series must be suitable for use in piranha::toolbox as first template parameter.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the same guarantee as \p Series.
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics is equivalent to the move semantics of \p Series.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename Series, typename Enable = void>
class trigonometric_series: public Series,detail::trigonometric_series_tag,toolbox<Series,trigonometric_series<Series,Enable>>
{
		typedef Series base;
		template <typename Term, typename = void>
		struct t_get
		{
			// Paranoia check.
			static_assert(detail::cf_trig_score<typename Term::cf_type>::value == 4,"Invalid trig score.");
			template <typename ... Args>
			static auto t_degree(const Term &t, const symbol_set &, const Args & ... args) -> decltype(math::t_degree(t.m_cf,args...))
			{
				return math::t_degree(t.m_cf,args...);
			}
			template <typename ... Args>
			static auto t_ldegree(const Term &t, const symbol_set &, const Args & ... args) -> decltype(math::t_ldegree(t.m_cf,args...))
			{
				return math::t_ldegree(t.m_cf,args...);
			}
			template <typename ... Args>
			static auto t_order(const Term &t, const symbol_set &, const Args & ... args) -> decltype(math::t_order(t.m_cf,args...))
			{
				return math::t_order(t.m_cf,args...);
			}
			template <typename ... Args>
			static auto t_lorder(const Term &t, const symbol_set &, const Args & ... args) -> decltype(math::t_lorder(t.m_cf,args...))
			{
				return math::t_lorder(t.m_cf,args...);
			}
		};
		template <typename Term>
		struct t_get<Term,typename std::enable_if<detail::key_trig_score<typename Term::key_type>::value == 4>::type>
		{
			template <typename ... Args>
			static auto t_degree(const Term &t, const symbol_set &s, const Args & ... args) -> decltype(t.m_key.t_degree(s,args...))
			{
				return t.m_key.t_degree(args...,s);
			}
			template <typename ... Args>
			static auto t_ldegree(const Term &t, const symbol_set &s, const Args & ... args) -> decltype(t.m_key.t_ldegree(s,args...))
			{
				return t.m_key.t_ldegree(args...,s);
			}
			template <typename ... Args>
			static auto t_order(const Term &t, const symbol_set &s, const Args & ... args) -> decltype(t.m_key.t_order(s,args...))
			{
				return t.m_key.t_order(args...,s);
			}
			template <typename ... Args>
			static auto t_lorder(const Term &t, const symbol_set &s, const Args & ... args) -> decltype(t.m_key.t_lorder(s,args...))
			{
				return t.m_key.t_lorder(args...,s);
			}
		};
		template <typename... Args>
		auto t_degree_impl(const Args & ... params) const
		{
			typedef typename Series::term_type term_type;
			typedef decltype(t_get<term_type>::t_degree(std::declval<term_type>(),this->m_symbol_set,params...)) return_type;
			const auto it = std::max_element(this->m_container.begin(),this->m_container.end(),
				[&](const term_type &t1, const term_type &t2) {
					return t_get<term_type>::t_degree(t1,this->m_symbol_set,params...) <
						t_get<term_type>::t_degree(t2,this->m_symbol_set,params...);
				});
			return (it == this->m_container.end()) ? return_type(0) :
				it->degree(std::forward<Args>(params)...,this->m_symbol_set);
		}
	public:
		/// Defaulted default constructor.
		trigonometric_series() = default;
		/// Defaulted copy constructor.
		trigonometric_series(const trigonometric_series &) = default;
		/// Defaulted move constructor.
		trigonometric_series(trigonometric_series &&) = default;
		PIRANHA_FORWARDING_CTOR(trigonometric_series,base)
		/// Defaulted copy assignment operator.
		trigonometric_series &operator=(const trigonometric_series &) = default;
		/// Defaulted move assignment operator.
		trigonometric_series &operator=(trigonometric_series &&) = default;
		PIRANHA_FORWARDING_ASSIGNMENT(trigonometric_series,base)
		auto t_degree() const -> decltype(std::declval<trigonometric_series>().degree_impl())
		{
			return degree_impl();
		}
};

template <typename Series>
class trigonometric_series<Series,typename std::enable_if<!detail::is_trigonometric_term<typename Series::term_type>::value>::type>
	: public Series,toolbox<Series,trigonometric_series<Series,Enable>>
{
		typedef Series base;
	public:
		trigonometric_series() = default;
		trigonometric_series(const trigonometric_series &) = default;
		trigonometric_series(trigonometric_series &&) = default;
		PIRANHA_FORWARDING_CTOR(trigonometric_series,base)
		trigonometric_series &operator=(const trigonometric_series &) = default;
		trigonometric_series &operator=(trigonometric_series &&) = default;
		PIRANHA_FORWARDING_ASSIGNMENT(trigonometric_series,base)
};

}

#endif

