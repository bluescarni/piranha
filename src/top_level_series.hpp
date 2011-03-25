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

#ifndef PIRANHA_TOP_LEVEL_SERIES_HPP
#define PIRANHA_TOP_LEVEL_SERIES_HPP

#include <iostream>
#include <type_traits>

#include "base_series.hpp"
#include "config.hpp"
#include "debug_access.hpp"
#include "detail/top_level_series_fwd.hpp"
#include "echelon_descriptor.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Top-level series class.
/**
 * This class is a model of the piranha::concept::Series concept.
 * 
 * \section type_requirements Type requirements
 * 
 * \p Term and \p Derived must be suitable for use in piranha::base_series.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the same exception safety guarantee as piranha::base_series.
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics is equivalent to piranha::base_series' move semantics.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename Term, typename Derived>
class top_level_series: public base_series<Term,Derived>
{
		template <typename T>
		friend class debug_access;
		static_assert(echelon_size<Term>::value > 0,"Error in the calculation of echelon size.");
		template <typename T>
		void dispatch_add(T &&x, typename std::enable_if<!std::is_base_of<detail::top_level_series_tag,typename strip_cv_ref<T>::type>::value>::type * = piranha_nullptr)
		{
			Term tmp(std::forward<T>(x),typename Term::key_type(m_ed.get_args<Term>()));
			this->insert(std::move(tmp),m_ed);
		}
// 		template <typename T>
// 		void dispatch_add(T &&other, typename std::enable_if<std::is_base_of<top_level_series_tag,typename strip_cv_ref<T>::type>::value>::type * = piranha_nullptr)
// 		{
// 			if (m_ed.get_args_tuple() == other.m_ed.get_args_tuple())
// 		}
		typedef base_series<Term,Derived> base;
	public:
		/// Defaulted default constructor.
		top_level_series() = default;
		/// Defaulted copy constructor.
		/**
		 * @throws unspecified any exception thrown by the copy constructor of piranha::base_series or
		 * piranha::echelon_descriptor.
		 */
		top_level_series(const top_level_series &) = default;
		/// Defaulted move constructor.
		top_level_series(top_level_series &&) = default;
		/// Trivial destructor.
		/**
		 * Equivalent to the defaulted destructor, apart from sanity checks being run
		 * in debug mode.
		 */
		~top_level_series() piranha_noexcept_spec(true)
		{
			piranha_assert(destruction_checks());
		}
		/// Move assignment operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 */
		top_level_series &operator=(top_level_series &&other) piranha_noexcept(true)
		{
			// Descriptor and base cannot throw on move.
			base::operator=(std::move(other));
			m_ed = std::move(other.m_ed);
			return *this;
		}
		/// Copy assignment operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the copy constructor of piranha::top_level_series.
		 */
		top_level_series &operator=(const top_level_series &other)
		{
			// Use copy+move idiom.
			if (this != &other) {
				top_level_series tmp(other);
				*this = std::move(tmp);
			}
			return *this;
		}
		/// In-place addition.
		/**
		 * The addition algorithm proceeds as follows:
		 * 
		 * - if \p other is an instance of piranha::top_level_series:
		 *   - if the echelon descriptors of \p this and \p other differ, they are merged, and \p this and \p other are
		 *     modified to be compatible with the merged descriptor (a copy of \p other might be created if necessary);
		 *   - all terms in \p other (or its copy) will be merged into \p this using piranha::base_series::merge_terms;
		 * - else:
		 *   - a piranha::base_series::term_type will be constructed, with \p other used to construct the coefficient
		 *     and with the arguments vector at echelon position 0 used to construct the key. This term will be inserted
		 *     into \p this.
		 * 
		 * @param[in] other object to be added to the series.
		 * 
		 * @return reference to \p this, cast to type \p Derived.
		 */
		template <typename T>
		Derived &operator+=(T &&other)
		{
			dispatch_add(std::forward<T>(other));
			return *static_cast<Derived *>(this);
		}
	private:
		bool destruction_checks() const
		{
			for (auto it = this->m_container.begin(); it != this->m_container.end(); ++it) {
				if (!it->is_compatible(m_ed)) {
					std::cout << "Term not compatible.\n";
					return false;
				}
				if (it->is_ignorable(m_ed)) {
					std::cout << "Term not ignorable.\n";
					return false;
				}
			}
			return true;
		}
	protected:
		echelon_descriptor<Term> m_ed;
};

}

#endif
