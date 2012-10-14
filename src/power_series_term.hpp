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

#ifndef PIRANHA_POWER_SERIES_TERM_HPP
#define PIRANHA_POWER_SERIES_TERM_HPP

#include <boost/concept/assert.hpp>
#include <set>
#include <string>
#include <type_traits>

#include "concepts/degree_key.hpp"
#include "concepts/term.hpp"
#include "config.hpp"
#include "detail/base_term_fwd.hpp"
#include "detail/inherit.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

struct power_series_term_tag {};

}

/// Power series term toolbox.
/**
 * This toolbox is intended to extend the \p Term type with properties of formal power series terms.
 * 
 * Specifically, the toolbox will conditionally augment a \p Term type by adding methods to query the total and partial (low) degree
 * of a \p Term object. Such augmentation takes place if the coefficient and/or key type of \p Term satisfy the following
 * requirements:
 * 
 * - the piranha::has_degree type trait is specialised to be \p true for the coefficient type, as indicated in the type trait's documentation;
 * - the key type is a model of piranha::concept::DegreeKey.
 * 
 * As an additional requirement, the types returned when querying total and partial (low) degree must be addable and copy/move constructible.
 * If these additional requirements are not satisfied,
 * a compile-time error will be produced. The degree of the term will be the result of adding the degree of the coefficient to the degree of the key.
 * 
 * If neither the coefficient nor the key satisfy the above requirements, this class will not add any new functionality to the \p Term class and
 * will just provide generic constructors that will forward their arguments to the constructors of \p Term.
 * 
 * This class is a model of the piranha::concept::Term concept and, in case the above requirements are satisfied, of the piranha::concept::PowerSeriesTerm
 * concept.
 * 
 * \section type_requirements Type requirements
 * 
 * \p Term must be an instance of piranha::base_term.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the same guarantee as \p Term.
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics is equivalent to the move semantics of \p Term.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename Term, typename = void>
class power_series_term: public Term, detail::power_series_term_tag
{
		static_assert(std::is_base_of<detail::base_term_tag,Term>::value,"Term must be an instance of piranha::base_term.");
		typedef Term base;
		template <typename Term2, typename = void>
		struct degree_utils
		{
			static auto compute(const Term2 &t, const symbol_set &ss) -> decltype(has_degree<typename Term2::cf_type>::get(t.m_cf) + t.m_key.degree(ss))
			{
				return has_degree<typename Term2::cf_type>::get(t.m_cf) + t.m_key.degree(ss);
			}
			static auto compute(const Term2 &t, const symbol_set &ss, const std::set<std::string> &as) ->
				decltype(has_degree<typename Term2::cf_type>::get(t.m_cf,as) + t.m_key.degree(as,ss))
			{
				return has_degree<typename Term2::cf_type>::get(t.m_cf,as) + t.m_key.degree(as,ss);
			}
			static auto lcompute(const Term2 &t, const symbol_set &ss) -> decltype(has_degree<typename Term2::cf_type>::lget(t.m_cf) + t.m_key.ldegree(ss))
			{
				return has_degree<typename Term2::cf_type>::lget(t.m_cf) + t.m_key.ldegree(ss);
			}
			static auto lcompute(const Term2 &t, const symbol_set &ss, const std::set<std::string> &as) ->
				decltype(has_degree<typename Term2::cf_type>::lget(t.m_cf,as) + t.m_key.ldegree(as,ss))
			{
				return has_degree<typename Term2::cf_type>::lget(t.m_cf,as) + t.m_key.ldegree(as,ss);
			}
		};
		template <typename Term2>
		struct degree_utils<Term2,typename std::enable_if<!detail::key_has_degree<typename Term2::key_type>::value>::type>
		{
			static auto compute(const Term2 &t, const symbol_set &) -> decltype(has_degree<typename Term2::cf_type>::get(t.m_cf))
			{
				return has_degree<typename Term2::cf_type>::get(t.m_cf);
			}
			static auto compute(const Term2 &t, const symbol_set &, const std::set<std::string> &as) ->
				decltype(has_degree<typename Term2::cf_type>::get(t.m_cf,as))
			{
				return has_degree<typename Term2::cf_type>::get(t.m_cf,as);
			}
			static auto lcompute(const Term2 &t, const symbol_set &) -> decltype(has_degree<typename Term2::cf_type>::lget(t.m_cf))
			{
				return has_degree<typename Term2::cf_type>::lget(t.m_cf);
			}
			static auto lcompute(const Term2 &t, const symbol_set &, const std::set<std::string> &as) ->
				decltype(has_degree<typename Term2::cf_type>::lget(t.m_cf,as))
			{
				return has_degree<typename Term2::cf_type>::lget(t.m_cf,as);
			}
		};
		template <typename Term2>
		struct degree_utils<Term2,typename std::enable_if<!has_degree<typename Term2::cf_type>::value>::type>
		{
			static auto compute(const Term2 &t, const symbol_set &ss) -> decltype(t.m_key.degree(ss))
			{
				return t.m_key.degree(ss);
			}
			static auto compute(const Term2 &t, const symbol_set &ss, const std::set<std::string> &as) ->
				decltype(t.m_key.degree(as,ss))
			{
				return t.m_key.degree(as,ss);
			}
			static auto lcompute(const Term2 &t, const symbol_set &ss) -> decltype(t.m_key.ldegree(ss))
			{
				return t.m_key.ldegree(ss);
			}
			static auto lcompute(const Term2 &t, const symbol_set &ss, const std::set<std::string> &as) ->
				decltype(t.m_key.ldegree(as,ss))
			{
				return t.m_key.ldegree(as,ss);
			}
		};
		typedef decltype(degree_utils<power_series_term>::compute(std::declval<power_series_term>(),std::declval<symbol_set>())) d_type;
		typedef decltype(degree_utils<power_series_term>::compute(std::declval<power_series_term>(),std::declval<symbol_set>(),{})) pd_type;
		typedef decltype(degree_utils<power_series_term>::lcompute(std::declval<power_series_term>(),std::declval<symbol_set>())) ld_type;
		typedef decltype(degree_utils<power_series_term>::lcompute(std::declval<power_series_term>(),std::declval<symbol_set>(),{})) pld_type;
	public:
		/// Defaulted default constructor.
		power_series_term() = default;
		/// Defaulted copy constructor.
		power_series_term(const power_series_term &) = default;
		/// Defaulted move constructor.
		power_series_term(power_series_term &&) = default;
		PIRANHA_USING_CTOR(power_series_term,base)
		/// Trivial destructor.
		~power_series_term() piranha_noexcept_spec(true)
		{
			BOOST_CONCEPT_ASSERT((concept::Term<power_series_term>));
		}
		/// Defaulted copy assignment operator.
		power_series_term &operator=(const power_series_term &) = default;
		/// Trivial move assignment operator.
		power_series_term &operator=(power_series_term &&other) piranha_noexcept_spec(true)
		{
			base::operator=(std::move(other));
			return *this;
		}
		PIRANHA_USING_ASSIGNMENT(power_series_term,base)
		/// Total degree.
		/**
		 * @param[in] ss reference set of arguments.
		 * 
		 * @return the total degree of the term.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the calculation of the degree from the coefficient and the key,
		 * - the copy constructor of the return value.
		 */
		d_type degree(const symbol_set &ss) const
		{
			return degree_utils<power_series_term>::compute(*this,ss);
		}
		/// Partial degree.
		/**
		 * @param[in] as names of the arguments considered in the computation of the degree.
		 * @param[in] ss reference set of arguments.
		 * 
		 * @return the partial degree of the term.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the calculation of the degree from the coefficient and the key,
		 * - the copy constructor of the return value.
		 */
		pd_type degree(const std::set<std::string> &as, const symbol_set &ss) const
		{
			return degree_utils<power_series_term>::compute(*this,ss,as);
		}
		/// Total low degree.
		/**
		 * @param[in] ss reference set of arguments.
		 * 
		 * @return the total low degree of the term.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the calculation of the degree from the coefficient and the key,
		 * - the copy constructor of the return value.
		 */
		ld_type ldegree(const symbol_set &ss) const
		{
			return degree_utils<power_series_term>::lcompute(*this,ss);
		}
		/// Partial low degree.
		/**
		 * @param[in] as names of the arguments considered in the computation of the degree.
		 * @param[in] ss reference set of arguments.
		 * 
		 * @return the partial low degree of the term.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the calculation of the degree from the coefficient and the key,
		 * - the copy constructor of the return value.
		 */
		pld_type ldegree(const std::set<std::string> &as, const symbol_set &ss) const
		{
			return degree_utils<power_series_term>::lcompute(*this,ss,as);
		}
};

template <typename Term>
class power_series_term<Term,typename std::enable_if<!has_degree<typename Term::cf_type>::value &&
	!detail::key_has_degree<typename Term::key_type>::value>::type>: public Term
{
		static_assert(std::is_base_of<detail::base_term_tag,Term>::value,"Term must be an instance of piranha::base_term.");
		typedef Term base;
	public:
		power_series_term() = default;
		power_series_term(const power_series_term &) = default;
		power_series_term(power_series_term &&) = default;
		PIRANHA_USING_CTOR(power_series_term,base)
		~power_series_term() piranha_noexcept_spec(true)
		{
			BOOST_CONCEPT_ASSERT((concept::Term<power_series_term>));
		}
		power_series_term &operator=(const power_series_term &) = default;
		power_series_term &operator=(power_series_term &&other) piranha_noexcept_spec(true)
		{
			base::operator=(std::move(other));
			return *this;
		}
		PIRANHA_USING_ASSIGNMENT(power_series_term,base)
};

/// Type-trait for power series term.
/**
 * The value of the type-trait will be \p true if \p Term is an instance of piranha::power_series_term
 * that provides the methods to query the degree of the term, \p false otherwise.
 * 
 * \section type_requirements Type requirements
 * 
 * \p Term must be a model of the piranha::concept::Term concept.
 */
template <typename Term>
class is_power_series_term
{
		BOOST_CONCEPT_ASSERT((concept::Term<Term>));
	public:
		/// Type-trait value.
		static const bool value = std::is_base_of<detail::power_series_term_tag,Term>::value;
};

template <typename Term>
const bool is_power_series_term<Term>::value;

}

#endif
