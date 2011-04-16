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

#include <boost/concept/assert.hpp>
#include <iostream>
#include <type_traits>

#include "base_series.hpp"
#include "concepts/container_element.hpp"
#include "concepts/crtp.hpp"
#include "config.hpp"
#include "debug_access.hpp"
#include "detail/base_series_fwd.hpp"
#include "detail/top_level_series_fwd.hpp"
#include "echelon_descriptor.hpp"
#include "math.hpp"
#include "series_binary_operators.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Top-level series class.
/**
 * This class is a model of the piranha::concept::Series concept.
 * 
 * \section type_requirements Type requirements
 * 
 * - \p Term and \p Derived must be suitable for use in piranha::base_series.
 * - \p Derived must be a model of piranha::concept::CRTP, with piranha::top_level_series
 *   of \p Term and \p Derived as base class.
 * - \p Derived must be a model of piranha::concept::ContainerElement.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the same exception safety guarantee as piranha::base_series. Additional specific exception safety
 * guarantees are described for some methods (e.g., negate()).
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics is equivalent to piranha::base_series' move semantics.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename Term, typename Derived>
class top_level_series: public base_series<Term,Derived>, public series_binary_operators, detail::top_level_series_tag
{
		BOOST_CONCEPT_ASSERT((concept::CRTP<top_level_series<Term,Derived>,Derived>));
		// Shortcut for base_series base.
		typedef base_series<Term,Derived> base;
		// Make friends for debug.
		template <typename T>
		friend class debug_access;
		// Make friend with all top_level_series.
		template <typename Term2, typename Derived2>
		friend class top_level_series;
		static_assert(echelon_size<Term>::value > 0,"Error in the calculation of echelon size.");
		template <bool Sign, typename T>
		void dispatch_add(T &&x, typename std::enable_if<!std::is_base_of<detail::top_level_series_tag,typename strip_cv_ref<T>::type>::value>::type * = piranha_nullptr)
		{
			static_assert(!std::is_base_of<detail::base_series_tag,typename strip_cv_ref<T>::type>::value,
				"Cannot add non top level series.");
			Term tmp(typename Term::cf_type(std::forward<T>(x),m_ed),typename Term::key_type(m_ed.template get_args<Term>()));
			this->template insert<Sign>(std::move(tmp),m_ed);
		}
		template <bool Sign, typename T>
		void dispatch_add(T &&other, typename std::enable_if<std::is_base_of<detail::top_level_series_tag,typename strip_cv_ref<T>::type>::value>::type * = piranha_nullptr)
		{
			static_assert(echelon_size<Term>::value == echelon_size<typename strip_cv_ref<T>::type::term_type>::value,
				"Cannot add/subtract series with different echelon sizes.");
			// NOTE: if they are not the same, we are going to do heavy calculations anyway: mark it "likely".
			if (likely(m_ed.get_args_tuple() == other.m_ed.get_args_tuple())) {
std::cout << "LOL same args tuple!\n";
				this->template merge_terms<Sign>(std::forward<T>(other),m_ed);
			} else {
std::cout << "oh NOES different!\n";
				// Let's deal with the first series.
				auto merge1 = m_ed.merge(other.m_ed);
				if (merge1.first.get_args_tuple() != m_ed.get_args_tuple()) {
					base::operator=(this->merge_args(m_ed,merge1.first));
					m_ed = std::move(merge1.first);
				}
				// Second series.
				auto merge2 = other.m_ed.merge(m_ed);
				piranha_assert(merge2.first.get_args_tuple() == m_ed.get_args_tuple());
				if (merge2.first.get_args_tuple() != other.m_ed.get_args_tuple()) {
					auto other_base_copy = other.merge_args(other.m_ed,merge2.first);
					this->template merge_terms<Sign>(std::move(other_base_copy),m_ed);
				} else {
					this->template merge_terms<Sign>(std::forward<T>(other),m_ed);
				}
			}
		}
		// Overload for assignment from non-series type.
		template <typename T>
		void dispatch_generic_assignment(T &&x, typename std::enable_if<
			!std::is_base_of<detail::top_level_series_tag,typename strip_cv_ref<T>::type>::value>::type * = piranha_nullptr)
		{
std::cout << 0 << '\n';
			static_assert(!std::is_base_of<detail::base_series_tag,typename strip_cv_ref<T>::type>::value,
				"Cannot assign non top level series.");
			echelon_descriptor<Term> empty_ed;
			Term tmp(typename Term::cf_type(std::forward<T>(x),empty_ed),typename Term::key_type(empty_ed.template get_args<Term>()));
			this->m_container.clear();
			m_ed = std::move(empty_ed);
			this->template insert<true>(std::move(tmp),m_ed);
		}
		// Assign from series with same term type, move semantics.
		template <typename Series>
		void dispatch_series_assignment(Series &&s,
			typename std::enable_if<
			std::is_same<Term,typename strip_cv_ref<Series>::type::term_type>::value &&
			is_nonconst_rvalue_ref<Series &&>::value
			>::type * = piranha_nullptr)
		{
std::cout << 1 << '\n';
			// This is to check for self-assignment, which could happen in the case Series derives from same type as this
			// and the canonical move-assignment operator does not kick in.
			if (likely(&m_ed != &s.m_ed)) {
				m_ed = std::move(s.m_ed);
				this->m_container = std::move(s.m_container);
			}
		}
		// Assign from series with same term type, copy semantics.
		template <typename Series>
		void dispatch_series_assignment(Series &&s,
			typename std::enable_if<
			std::is_same<Term,typename strip_cv_ref<Series>::type::term_type>::value &&
			!is_nonconst_rvalue_ref<Series &&>::value
			>::type * = piranha_nullptr)
		{
std::cout << 2 << '\n';
			if (likely(&m_ed != &s.m_ed)) {
				auto new_ed = s.m_ed;
				auto new_container = s.m_container;
				m_ed = std::move(new_ed);
				this->m_container = std::move(new_container);
			}
		}
		// Assign from series with different term type.
		template <typename Series>
		void dispatch_series_assignment(Series &&s,
			typename std::enable_if<
			!std::is_same<Term,typename strip_cv_ref<Series>::type::term_type>::value
			>::type * = piranha_nullptr)
		{
std::cout << 3 << '\n';
			// NOTE: we do not move away the other echelon descriptor, since if something goes wrong
			// in the term merging below, series s could be left in an inconsistent state.
			echelon_descriptor<Term> new_ed(s.m_ed);
			// The next two lines are exception-safe.
			m_ed = std::move(new_ed);
			this->m_container.clear();
			this->template merge_terms<true>(std::forward<Series>(s),m_ed);
		}
		// Overload for assignment from series type.
		template <typename Series>
		void dispatch_generic_assignment(Series &&s, typename std::enable_if<
			std::is_base_of<detail::top_level_series_tag,typename strip_cv_ref<Series>::type>::value>::type * = piranha_nullptr)
		{
			static_assert(echelon_size<Term>::value == echelon_size<typename strip_cv_ref<Series>::type::term_type>::value,
				"Cannot add/subtract series with different echelon sizes.");
			dispatch_series_assignment(std::forward<Series>(s));
		}
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
			BOOST_CONCEPT_ASSERT((concept::ContainerElement<Derived>));
			piranha_assert(destruction_checks());
		}
		/// Move assignment operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 */
		top_level_series &operator=(top_level_series &&other) piranha_noexcept_spec(true)
		{
			if (likely(this != &other)) {
				// Descriptor and base cannot throw on move.
				base::operator=(std::move(other));
				m_ed = std::move(other.m_ed);
			}
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
		/// Generic assignment operator.
		/**
		 * The generic assignment algorithm works as follow:
		 * 
		 * - if \p T derives from piranha::top_level_series,
		 *   - if the term type of \p T is the same as that of \p this:
		 *     - terms container and echelon descriptor are assigned to \p this;
		 *   - else:
		 *     - \p this is emptied and all terms from \p x are inserted using piranha::base_series::merge_terms()
		 *       (the echelon descriptor of \p x is also assigned to \p this);
		 * - else:
		 *   - \p x is used to construct a new term as follows:
		 *     - \p x is forwarded, together with an empty echelon descriptor, to construct a coefficient;
		 *     - the arguments vector at echelon position 0 of the echelon descriptor of \p this will be used to construct a key;
		 *     - coefficient and key are used to construct the new term instance;
		 *   - an empty echelon descriptor is assigned to \p this;
		 *   - all terms of this are discarded;
		 *   - the new term is inserted into \p this using piranha::base_series::insert().
		 * 
		 * If \p other is a piranha::top_level_series and its term type's echelon size is different from the echelon size of
		 * the term type of this series, a compile-time error will be produced. A compile-time error will also be produced if \p other
		 * derives from piranha::base_series without being a piranha::top_level_series.
		 * 
		 * @param[in] x object to assign from.
		 * 
		 * @return reference to \p this.
		 */
		template <typename T>
		top_level_series &operator=(T &&x)
		{
			dispatch_generic_assignment(std::forward<T>(x));
			return *this;
		}
		/// Negate series in-place.
		/**
		 * This method will call the <tt>negate()</tt> method on the coefficients of all terms. In case of exceptions,
		 * the basic exception safety guarantee is provided. Specifically, only a susbet of the series' coefficients will
		 * have been negated in face of an exception.
		 * 
		 * @throws unspecified any exception thrown by the <tt>negate()</tt> method of the coefficient type.
		 */
		void negate()
		{
			const auto it_f = this->m_container.end();
			for (auto it = this->m_container.begin(); it != it_f; ++it) {
				it->m_cf.negate(m_ed);
				piranha_assert(it->is_compatible(m_ed));
				piranha_assert(!it->is_ignorable(m_ed));
			}
		}
		/// In-place addition.
		/**
		 * The addition algorithm proceeds as follows:
		 * 
		 * - if \p other is an instance of piranha::top_level_series:
		 *   - if the echelon descriptors of \p this and \p other differ, they are merged using piranha::echelon_descriptor::merge(),
		 *     and \p this and \p other are
		 *     modified as necessary to be compatible with the merged descriptor using piranha::base_series::merge_args()
		 *     (a copy of \p other might be created if it requires modifications);
		 *   - all terms in \p other (or its copy) will be merged into \p this using piranha::base_series::merge_terms() and the merged descriptor;
		 * - else:
		 *   - a \p Term instance will be constructed as follows:
		 *     - \p other will be forwarded, together with the echelon descriptor of \p this, to construct the coefficient;
		 *     - the arguments vector at echelon position 0 of the echelon descriptor of \p this will be used to construct the key;
		 *   - the term will be inserted into \p this using piranha::base_series::insert().
		 * 
		 * If \p other is a piranha::top_level_series and its term type's echelon size is different from the echelon size of
		 * the term type of this series, a compile-time error will be produced. A compile-time error will also be produced if \p other
		 * derives from piranha::base_series without being a piranha::top_level_series.
		 * 
		 * @param[in] other object to be added to the series.
		 * 
		 * @return reference to \p this, cast to type \p Derived.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::base_series::merge_args(),
		 * - piranha::echelon_descriptor::merge(),
		 * - piranha::base_series::merge_terms(),
		 * - piranha::base_series::insert(),
		 * - one of the following constructors:
		 *   - \p Term from its coefficient and key,
		 *   - coefficient from \p T,
		 *   - key from arguments vector.
		 */
		template <typename T>
		Derived &operator+=(T &&other)
		{
			dispatch_add<true>(std::forward<T>(other));
			return *static_cast<Derived *>(this);
		}
		/// In-place subtraction.
		/**
		 * The subtraction algorithm is equivalent, apart to a change in sign, to the addition algorithm described in operator+=().
		 * 
		 * @param[in] other object to be subtracted from the series.
		 * 
		 * @return reference to \p this, cast to type \p Derived.
		 * 
		 * @throws unspecified the same exceptions type possibly thrown by operator+=().
		 */
		template <typename T>
		Derived &operator-=(T &&other)
		{
			dispatch_add<false>(std::forward<T>(other));
			return *static_cast<Derived *>(this);
		}
// 		/// Overload stream operator for piranha::top_level_series.
// 		/**
// 		 * Will direct to stream a human-readable representation of the series.
// 		 * 
// 		 * @param[in,out] os target stream.
// 		 * @param[in] tls piranha::base_series argument.
// 		 * 
// 		 * @return reference to \p os.
// 		 * 
// 		 * @throws unspecified any exception resulting from directing to stream instances of piranha::base_series::term_type.
// 		 */
// 		friend inline std::ostream &operator<<(std::ostream &os, const top_level_series &tls)
// 		{
// std::cout << "LOL stream!!!!\n";
// 			return os;
// 		}
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
		/// Echelon descriptor.
		echelon_descriptor<Term> m_ed;
};


namespace detail
{

// Overload negation for top_level_series.
template <typename TopLevelSeries>
struct math_negate_impl<TopLevelSeries,typename std::enable_if<std::is_base_of<top_level_series_tag,TopLevelSeries>::value>::type>
{
	static void run(TopLevelSeries &s)
	{
		s.negate();
	}
};

}

}

#endif
