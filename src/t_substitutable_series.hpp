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

#ifndef PIRANHA_T_SUBSTITUTABLE_SERIES_HPP
#define PIRANHA_T_SUBSTITUTABLE_SERIES_HPP

#include "detail/toolbox.hpp"
#include "forwarding.hpp"
#include "math.hpp"

namespace piranha
{

namespace detail
{

// Tag for trigonometric subs series.
struct t_substitutable_series_tag {};

// Detect t_subs term.
template <typename Term>
struct t_subs_term_score
{
	static const unsigned value = (unsigned)has_t_subs<typename Term::cf_type>::value |
		((unsigned)key_has_t_subs<typename Term::key_type>::value << 1u);
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
 * and all degree/order type traits need to be satisfied for the coefficient/key type. Note also that the types representing the degree/order
 * must be constructible from the integral constant zero and less-than and greater-than comparable.
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
template <typename Series, typename = void>
class t_substitutable_series: public Series,detail::t_substitutable_series_tag,detail::toolbox<Series,t_substitutable_series<Series,Enable>>
{
		typedef Series base;
		template <typename Term, typename = void>
		struct t_get
		{
			// Paranoia check.
			static_assert(detail::cf_trig_score<typename Term::cf_type>::value == 4,"Invalid trig score.");
			// NOTE: here and below we could do this with variadic templates, avoiding the dual overload for partial degree/order.
			// Unfortunately, as of GCC 4.6 decltype() with variadic unpacking is not implemented. Keep this in mind for future improvement:
			// template <typename ... Args>
			// static auto t_degree(const Term &t, const symbol_set &, const Args & ... args) -> decltype(math::t_degree(t.m_cf,args...))
			// {
			//	return math::t_degree(t.m_cf,args...);
			// }
			static auto t_degree(const Term &t, const symbol_set &) -> decltype(math::t_degree(t.m_cf))
			{
				return math::t_degree(t.m_cf);
			}
			static auto t_degree(const Term &t, const symbol_set &, const std::set<std::string> &names) -> decltype(math::t_degree(t.m_cf,names))
			{
				return math::t_degree(t.m_cf,names);
			}
			static auto t_ldegree(const Term &t, const symbol_set &) -> decltype(math::t_ldegree(t.m_cf))
			{
				return math::t_ldegree(t.m_cf);
			}
			static auto t_ldegree(const Term &t, const symbol_set &, const std::set<std::string> &names) -> decltype(math::t_ldegree(t.m_cf,names))
			{
				return math::t_ldegree(t.m_cf,names);
			}
			static auto t_order(const Term &t, const symbol_set &) -> decltype(math::t_order(t.m_cf))
			{
				return math::t_order(t.m_cf);
			}
			static auto t_order(const Term &t, const symbol_set &, const std::set<std::string> &names) -> decltype(math::t_order(t.m_cf,names))
			{
				return math::t_order(t.m_cf,names);
			}
			static auto t_lorder(const Term &t, const symbol_set &) -> decltype(math::t_lorder(t.m_cf))
			{
				return math::t_lorder(t.m_cf);
			}
			static auto t_lorder(const Term &t, const symbol_set &, const std::set<std::string> &names) -> decltype(math::t_lorder(t.m_cf,names))
			{
				return math::t_lorder(t.m_cf,names);
			}
		};
		template <typename Term>
		struct t_get<Term,typename std::enable_if<detail::key_trig_score<typename Term::key_type>::value == 4>::type>
		{
			static auto t_degree(const Term &t, const symbol_set &s) -> decltype(t.m_key.t_degree(s))
			{
				return t.m_key.t_degree(s);
			}
			static auto t_degree(const Term &t, const symbol_set &s, const std::set<std::string> &names) -> decltype(t.m_key.t_degree(names,s))
			{
				return t.m_key.t_degree(names,s);
			}
			static auto t_ldegree(const Term &t, const symbol_set &s) -> decltype(t.m_key.t_ldegree(s))
			{
				return t.m_key.t_ldegree(s);
			}
			static auto t_ldegree(const Term &t, const symbol_set &s, const std::set<std::string> &names) -> decltype(t.m_key.t_ldegree(names,s))
			{
				return t.m_key.t_ldegree(names,s);
			}
			static auto t_order(const Term &t, const symbol_set &s) -> decltype(t.m_key.t_order(s))
			{
				return t.m_key.t_order(s);
			}
			static auto t_order(const Term &t, const symbol_set &s, const std::set<std::string> &names) -> decltype(t.m_key.t_order(names,s))
			{
				return t.m_key.t_order(names,s);
			}
			static auto t_lorder(const Term &t, const symbol_set &s) -> decltype(t.m_key.t_lorder(s))
			{
				return t.m_key.t_lorder(s);
			}
			static auto t_lorder(const Term &t, const symbol_set &s, const std::set<std::string> &names) -> decltype(t.m_key.t_lorder(names,s))
			{
				return t.m_key.t_lorder(names,s);
			}
		};
	public:
		/// Defaulted default constructor.
		t_substitutable_series() = default;
		/// Defaulted copy constructor.
		t_substitutable_series(const t_substitutable_series &) = default;
		/// Defaulted move constructor.
		t_substitutable_series(t_substitutable_series &&) = default;
		PIRANHA_FORWARDING_CTOR(t_substitutable_series,base)
		/// Defaulted copy assignment operator.
		t_substitutable_series &operator=(const t_substitutable_series &) = default;
		/// Defaulted move assignment operator.
		t_substitutable_series &operator=(t_substitutable_series &&) = default;
		PIRANHA_FORWARDING_ASSIGNMENT(t_substitutable_series,base)
		// NOTE: this could be implemented generically with variadic template capture in the lambda, in order to avoid the partial overloads. Not implemented
		// yet in GCC though:
		// http://stackoverflow.com/questions/3377828/variadic-templates-for-lambda-expressions
		/// Total trigonometric degree.
		/**
		 * \note
		 * This method is available only if the series satisfies the requirements of piranha::trigonometric_series.
		 * 
		 * @return total trigonometric degree of the series.
		 * 
		 * @throws unspecified any exception resulting from the computation and comparison of the degree of the individual terms.
		 */
		auto t_degree() const -> decltype(t_get<typename Series::term_type>::t_degree(std::declval<typename Series::term_type>(),std::declval<symbol_set>()))
		{
			typedef typename Series::term_type term_type;
			typedef decltype(t_get<term_type>::t_degree(std::declval<term_type>(),std::declval<symbol_set>())) return_type;
			auto g = [this](const term_type &t) {
				return t_get<term_type>::t_degree(t,this->m_symbol_set);
			};
			return detail::generic_series_degree(this->m_container,g,std::less<return_type>());
		}
};

template <typename Series>
class t_substitutable_series<Series,typename std::enable_if<!detail::is_trigonometric_term<typename Series::term_type>::value>::type>
	: public Series,detail::toolbox<Series,t_substitutable_series<Series,typename std::enable_if<!detail::is_trigonometric_term<typename Series::term_type>::value>::type>>
{
		typedef Series base;
	public:
		t_substitutable_series() = default;
		t_substitutable_series(const t_substitutable_series &) = default;
		t_substitutable_series(t_substitutable_series &&) = default;
		PIRANHA_FORWARDING_CTOR(t_substitutable_series,base)
		t_substitutable_series &operator=(const t_substitutable_series &) = default;
		trigonometric_series &operator=(trigonometric_series &&) = default;
		PIRANHA_FORWARDING_ASSIGNMENT(trigonometric_series,base)
};

namespace math
{



}

}

#endif

