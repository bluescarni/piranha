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

#ifndef PIRANHA_SUBSTITUTABLE_SERIES_HPP
#define PIRANHA_SUBSTITUTABLE_SERIES_HPP

#include <string>
#include <type_traits>
#include <utility>

#include "forwarding.hpp"
#include "math.hpp"
#include "serialization.hpp"
#include "series.hpp"
#include "type_traits.hpp"

namespace piranha
{

template <typename Series, typename Derived>
class substitutable_series: public Series
{
		typedef Series base;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
		// Detect subs term.
		template <typename Term, typename T>
		struct subs_term_score
		{
			static const unsigned value = static_cast<unsigned>(has_subs<typename Term::cf_type,T>::value) +
				(static_cast<unsigned>(key_has_subs<typename Term::key_type,T>::value) << 1u);
		};
		// Case 1: subs only on cf.
		template <typename T, typename Term>
		using ret_type_1 = decltype(math::subs(std::declval<typename Term::cf_type const &>(),std::declval<std::string const &>(), \
				std::declval<T const &>()) * std::declval<Derived const &>());
		template <typename T, typename Term, typename std::enable_if<subs_term_score<Term,T>::value == 1u,int>::type = 0>
		static ret_type_1<T,Term> subs_impl(const Term &t, const std::string &name, const T &x, const symbol_set &s_set)
		{
			Derived tmp;
			tmp.m_symbol_set = s_set;
			tmp.insert(Term(typename Term::cf_type(1),t.m_key));
			return math::subs(t.m_cf,name,x) * tmp;
		}
		// Subs only on key.
//		template <typename T, typename Term>
//		using ret_type_2 = decltype(math::subs(std::declval<typename Term::cf_type const &>(),std::declval<std::string const &>(), \
//				std::declval<T const &>()) * std::declval<Derived const &>());
//		template <typename T, typename Term, typename std::enable_if<subs_term_score<Term,T>::value == 2u,int>::type = 0>
//		static ret_type_2<T,Term> subs_impl(const Term &t, const std::string &name, const T &x, const symbol_set &s_set)
//		{
//			auto ksubs = t.m_key.subs(x,name,s_set);
//
//
//			Derived tmp;
//			tmp.m_symbol_set = s_set;
//			tmp.insert(Term(typename Term::cf_type(1),t.m_key));
//			return math::subs(t.m_cf,name,x) * tmp;
//		}
	public:
		/// Defaulted default constructor.
		substitutable_series() = default;
		/// Defaulted copy constructor.
		substitutable_series(const substitutable_series &) = default;
		/// Defaulted move constructor.
		substitutable_series(substitutable_series &&) = default;
		PIRANHA_FORWARDING_CTOR(substitutable_series,base)
		/// Defaulted copy assignment operator.
		substitutable_series &operator=(const substitutable_series &) = default;
		/// Defaulted move assignment operator.
		substitutable_series &operator=(substitutable_series &&) = default;
		/// Trivial destructor.
		~substitutable_series()
		{
			PIRANHA_TT_CHECK(is_series,substitutable_series);
			PIRANHA_TT_CHECK(is_series,Derived);
			PIRANHA_TT_CHECK(std::is_base_of,substitutable_series,Derived);
		}
		PIRANHA_FORWARDING_ASSIGNMENT(substitutable_series,base)
		// Initial definition of the subs type.
		template <typename T>
		using subs_type_ = decltype(subs_impl(std::declval<typename Series::term_type const &>(),std::declval<std::string const &>(),
			std::declval<const T &>(),std::declval<symbol_set const &>()));
		// Enable conditionally based on the common requirements in the subs() method.
		template <typename T>
		using subs_type = typename std::enable_if<std::is_constructible<subs_type_<T>,int>::value && is_addable_in_place<subs_type_<T>>::value,
			subs_type_<T>>::type;
		template <typename T>
		subs_type<T> subs(const std::string &name, const T &x) const
		{
			subs_type<T> retval(0);
			for (const auto &t: this->m_container) {
				retval += subs_impl(t,name,x,this->m_symbol_set);
			}
			return retval;
		}
};

}

#endif
