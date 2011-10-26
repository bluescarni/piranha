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

#ifndef PIRANHA_POWER_SERIES_HPP
#define PIRANHA_POWER_SERIES_HPP

#include <boost/concept/assert.hpp>
#include <type_traits>

#include "concepts/power_series_term.hpp"
#include "detail/series_fwd.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Power series.
template <typename Base>
class power_series: public Base
{
		BOOST_CONCEPT_ASSERT((concept::PowerSeriesTerm<typename Base::term_type>));
		static_assert(std::is_base_of<detail::series_tag,Base>::value,"Base class must derive from piranha::series.");
		typedef Base base;
		template <typename Term, typename Enable = void>
		struct degree_utils
		{
			typedef decltype(std::declval<typename Term::cf_type>().degree() +
				std::declval<typename Term::key_type>().degree(std::declval<symbol_set>())) type;
			static type compute(const Term &t, const symbol_set &ss)
			{
				return t.m_cf.degree() + t.m_key.degree(ss);
			}
		};
		template <typename Term>
		struct degree_utils<Term,typename std::enable_if<!detail::key_has_degree<typename Term::key_type>::value>::type>
		{
			typedef decltype(std::declval<typename Term::cf_type>().degree()) type;
			static type compute(const Term &t, const symbol_set &)
			{
				return t.m_cf.degree();
			}
		};
		template <typename Term>
		struct degree_utils<Term,typename std::enable_if<!detail::cf_has_degree<typename Term::key_type>::value>::type>
		{
			typedef decltype(std::declval<typename Term::key_type>().degree(std::declval<symbol_set>())) type;
			static type compute(const Term &t, const symbol_set &ss)
			{
				return t.m_key.degree(ss);
			}
		};
		typedef typename degree_utils<typename Base::term_type>::type degree_type;
	public:
		power_series() = default;
		power_series(const power_series &) = default;
		power_series(power_series &&) = default;
		template <typename T, typename... Args, typename std::enable_if<sizeof...(Args) || !std::is_base_of<power_series,typename std::decay<T>::type>::value>::type*& = enabler>
		explicit power_series(T &&arg1, Args && ... argn) : base(std::forward<T>(arg1),std::forward<Args>(argn)...) {}
		power_series &operator=(const power_series &) = default;
		power_series &operator=(power_series &&other)
		{
			base::operator=(std::move(other));
			return *this;
		}
		template <typename T>
		typename std::enable_if<!std::is_base_of<power_series,typename std::decay<T>::type>::value,power_series &>::type operator=(T &&x)
		{
			base::operator=(std::forward<T>(x));
			return *this;
		}
		degree_type degree() const
		{
			if (this->empty()) {
				return degree_type(0);
			}
			auto it = this->m_container.begin();
			const auto it_f = this->m_container.end();
			auto retval = degree_utils<typename Base::term_type>::compute(*it,this->m_symbol_set);
			++it;
			decltype(retval) tmp;
			for (; it != it_f; ++it) {
				tmp = degree_utils<typename Base::term_type>::compute(*it,this->m_symbol_set);
				if (tmp > retval) {
					retval = tmp;
				}
			}
			return retval;
		}
};

}

#endif
