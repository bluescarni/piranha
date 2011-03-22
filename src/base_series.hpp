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

#ifndef PIRANHA_BASE_SERIES_HPP
#define PIRANHA_BASE_SERIES_HPP

#include <boost/concept/assert.hpp>
#include <iostream>
#include <stdexcept>
#include <type_traits>

#include "base_term.hpp"
#include "concepts/crtp.hpp"
#include "concepts/term.hpp"
#include "config.hpp"
#include "debug_access.hpp"
#include "detail/base_series_fwd.hpp"
#include "echelon_descriptor.hpp"
#include "hop_table.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Base series class.
/**
 * \section type_requirements Type requirements
 * 
 * - \p Term must be a model of piranha::concept::Term.
 * - \p Derived must be a model of piranha::concept::CRTP.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename Term, typename Derived>
class base_series: detail::base_series_tag
{
		BOOST_CONCEPT_ASSERT((concept::Term<Term>));
		BOOST_CONCEPT_ASSERT((concept::CRTP<Derived>));
	public:
		/// Alias for term type.
		typedef Term term_type;
	private:
		// Make friend with debugging class.
		template <typename T>
		friend class debug_access;
		// Hash functor for term_type.
		struct term_hasher
		{
			std::size_t operator()(const term_type &term) const
			{
				return term.hash();
			}
		};
	protected:
		/// Container type for terms.
		typedef hop_table<term_type,term_hasher> container_type;
	private:
		// Overload for completely different term type: convert to term_type and proceed.
		template <bool Sign, typename T, typename EchelonDescriptor>
		void dispatch_insertion(T &&term, EchelonDescriptor &ed, typename std::enable_if<
			!std::is_same<typename strip_cv_ref<T>::type,term_type>::value
			>::type * = piranha_nullptr)
		{
std::cout << "converting to term type\n";
			dispatch_insertion<Sign>(term_type(std::forward<T>(term)),ed);
		}
		// Overload for term_type.
		template <bool Sign, typename T, typename EchelonDescriptor>
		void dispatch_insertion(T &&term, EchelonDescriptor &ed, typename std::enable_if<
			std::is_same<typename strip_cv_ref<T>::type,term_type>::value
			>::type * = piranha_nullptr)
		{
std::cout << "term type insert!\n";
			// Debug checks.
			piranha_assert(empty() || m_container.begin()->is_compatible(ed));
			// Generate error if term is not compatible.
			if (unlikely(!term.is_compatible(ed))) {
				piranha_throw(std::invalid_argument,"cannot insert incompatible term");
			}
			// Discard ignorable term.
			if (unlikely(term.is_ignorable(ed))) {
				return;
			}
			insertion_impl<Sign>(std::forward<T>(term),ed);
		}
		// Insert compatible, non-ignorable term.
		template <bool Sign, typename T, typename EchelonDescriptor>
		void insertion_impl(T &&term, EchelonDescriptor &ed, typename std::enable_if<
			std::is_same<typename strip_cv_ref<T>::type,term_type>::value
			>::type * = piranha_nullptr)
		{
			// Try to locate the term.
			const auto it = m_container.find(term);
			if (it == m_container.end()) {
std::cout << "new term!\n";
				// This is a new term, insert it.
				const auto result = m_container.insert(std::forward<T>(term));
				piranha_assert(result.second);
				// Change sign if requested.
				if (!Sign) {
std::cout << "negating!\n";
					result.first->m_cf.negate(ed);
				}
			} else {
std::cout << "updating existing term!\n";
				// Assert the existing term is not ignorable.
				piranha_assert(!it->is_ignorable(ed));
				// The term exists already, update it.
				if (Sign) {
					// TODO: move semantics here? Could be useful for coefficient series.
std::cout << "adding!\n";
					it->m_cf.add(term.m_cf,ed);
				} else {
std::cout << "subtracting!\n";
					it->m_cf.subtract(term.m_cf,ed);
				}
				// Check if the term has become ignorable after the modification.
				if (unlikely(it->is_ignorable(ed))) {
std::cout << "erasing term become ignorable!\n";
					m_container.erase(it);
				}
			}
		}
	public:
		/// Size type.
		/**
		 * Used to represent the number of terms in the series. Equivalent to piranha::hop_table::size_type.
		 */
		typedef typename container_type::size_type size_type;
		/// Defaulted default constructor.
		base_series() = default;
		/// Defaulted copy constructor.
		base_series(const base_series &) = default;
		/// Defaulted move constructor.
		base_series(base_series &&) = default;
		/// Defaulted destructor.
		~base_series() = default;
		/// Defaulted copy-assignment operator.
		base_series &operator=(const base_series &) = default;
		/// Move assignment operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 */
		base_series &operator=(base_series &&other) piranha_noexcept_spec(true)
		{
			m_container = std::move(other.m_container);
			return *this;
		}
		/// Series size.
		/**
		 * @return the number of terms in the series.
		 */
		size_type size() const
		{
			return m_container.size();
		}
		/// Empty test.
		/**
		 * @return \p true if size() is nonzero, \p false otherwise.
		 */
		bool empty() const
		{
			return !size();
		}
		/// Overload stream operator for piranha::base_series.
		/**
		 * Will direct to stream a human-readable representation of the series.
		 * 
		 * @param[in,out] os target stream.
		 * @param[in] bs piranha::base_series argument.
		 * 
		 * @return reference to \p os.
		 * 
		 * @throws unspecified any exception resulting from directing to stream instances of piranha::base_series::term_type.
		 */
		friend inline std::ostream &operator<<(std::ostream &os, const base_series &bs)
		{
			for (auto it = bs.m_container.begin(); it != bs.m_container.end(); ++it) {
				os << *it << '\n';
			}
			return os;
		}
	protected:
		/// Insert generic term.
		/**
		 * This template method is activated only if \p T derives from piranha::base_term, and will insert \p term into the series
		 * using \p ed as reference piranha::echelon_descriptor.
		 * 
		 * The insertions algorithm proceeds as follows:
		 * 
		 * - if \p term is not of type base_series::term_type, it is forwarded to construct an instance of
		 *   base_series::term_type, and the algorithm proceeds to insert that instead;
		 * - if the term is not compatible for insertion, a \p std::invalid_argument exception is thrown;
		 * - if the term is ignorable, the method will return;
		 * - if the term is already in the series, then:
		 *   - its coefficient is added (if \p Sign is \p true) or subtracted (if \p Sign is \p false)
		 *     to the existing term's coefficient;
		 *   - if, after the addition/subtraction the existing term is ignorable, it will be erased;
		 * - else:
		 *   - the term is inserted into the term container and, if \p Sign is \p false, its coefficient is negated.
		 * 
		 * @param[in] term term to be inserted.
		 * @param[in] ed reference piranha::echelon_descriptor.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the constructor of base_series::term_type from type \p T;
		 * - the \p is_compatible and \p is_ignorable methods of base_series::term_type;
		 * - piranha::hop_table::insert().
		 * @throws std::invalid_argument if \p term is ignorable.
		 */
		template <bool Sign, typename T, typename Term>
		void insert(T &&term, const echelon_descriptor<Term> &ed, typename std::enable_if<
			std::is_base_of<detail::base_term_tag,typename strip_cv_ref<T>::type>::value
			>::type * = piranha_nullptr)
		{
			dispatch_insertion<Sign>(std::forward<T>(term),ed);
		}
		/// Insert generic term with <tt>Sign = true</tt>.
		/**
		 * Convenience wrapper for the generic insert() method, with \p Sign set to \p true.
		 * 
		 * @param[in] term term to be inserted.
		 * @param[in] ed reference piranha::echelon_descriptor.
		 * 
		 * @throws unspecified any exception thrown by generic insert().
		 */
		template <typename T, typename EchelonDescriptor>
		void insert(T &&term, EchelonDescriptor &ed, typename std::enable_if<
			std::is_base_of<detail::base_term_tag,typename strip_cv_ref<T>::type>::value
			>::type * = piranha_nullptr)
		{
			insert<true>(std::forward<T>(term),ed);
		}
	protected:
		/// Terms container.
		container_type m_container;
};

}

#endif
