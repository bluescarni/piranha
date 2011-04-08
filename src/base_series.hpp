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

#include <algorithm>
#include <boost/concept/assert.hpp>
#include <iostream>
#include <stdexcept>
#include <type_traits>

#include "base_term.hpp"
#include "concepts/container_element.hpp"
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
 * - \p Derived must be a model of piranha::concept::CRTP, with piranha::base_series
 *   of \p Term and \p Derived as base class.
 * - \p Derived must be a model of piranha::concept::ContainerElement.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the same exception safety guarantee as piranha::hop_table. In particular, exceptions thrown during
 * any operation involving term insertion will leave the object in an undefined but valid state.
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics is equivalent to piranha::hop_table's move semantics.
 * 
 * \todo improve performance in insertion: avoid calculating hash value multiple times, use low-level methods og hop_table
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename Term, typename Derived>
class base_series: detail::base_series_tag
{
		BOOST_CONCEPT_ASSERT((concept::Term<Term>));
		BOOST_CONCEPT_ASSERT((concept::CRTP<base_series<Term,Derived>,Derived>));
	public:
		/// Alias for term type.
		typedef Term term_type;
	private:
		// Make friend with all base series.
		template <typename Term2, typename Derived2>
		friend class base_series;
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
std::cout << "term type incompatible LOL!\n";
				piranha_throw(std::invalid_argument,"cannot insert incompatible term");
			}
			// Discard ignorable term.
			if (unlikely(term.is_ignorable(ed))) {
std::cout << "term type ignorable!\n";
				return;
			}
			insertion_impl<Sign>(std::forward<T>(term),ed);
		}
		template <bool Sign, typename Iterator, typename EchelonDescriptor>
		static void insertion_cf_arithmetics(Iterator &it, const term_type &term, const EchelonDescriptor &ed)
		{
			if (Sign) {
std::cout << "adding!\n";
				it->m_cf.add(term.m_cf,ed);
			} else {
std::cout << "subtracting!\n";
				it->m_cf.subtract(term.m_cf,ed);
			}
		}
		template <bool Sign, typename Iterator, typename EchelonDescriptor>
		static void insertion_cf_arithmetics(Iterator &it, term_type &&term, const EchelonDescriptor &ed)
		{
			if (Sign) {
std::cout << "move adding!\n";
				it->m_cf.add(std::move(term.m_cf),ed);
			} else {
std::cout << "move subtracting!\n";
				it->m_cf.subtract(std::move(term.m_cf),ed);
			}
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
				insertion_cf_arithmetics<Sign>(it,std::forward<T>(term),ed);
				// Check if the term has become ignorable after the modification.
				if (unlikely(it->is_ignorable(ed))) {
std::cout << "erasing term become ignorable!\n";
					m_container.erase(it);
				}
			}
		}
		// Overload in case that T derives from the same type as this (base_series<Term,Derived>).
		template <bool Sign, typename T, typename Term2>
		void merge_terms_impl0(T &&series, const echelon_descriptor<Term2> &ed,
			typename std::enable_if<std::is_base_of<base_series<Term,Derived>,typename strip_cv_ref<T>::type>::value>::type * = piranha_nullptr)
		{
			// NOTE: here we can take the pointer to series and compare it to this because we know from enable_if that
			// series is an instance of the type of this.
			if (unlikely(&series == this)) {
std::cout << "ZOMG merging with self\n";
				// If the two series are the same object, we need to make a copy.
				// NOTE: we do not forward here, when making the copy, because if T is a non-const
				// rvalue reference we might actually erase this: with Derived(series), a move
				// constructor might end up being called.
				merge_terms_impl1<Sign>(base_series<Term,Derived>(series),ed);
			} else {
std::cout << "all normal, merging with other\n";
				merge_terms_impl1<Sign>(std::forward<T>(series),ed);
			}
		}
		// Overload in case that T is not an instance of the type of this.
		template <bool Sign, typename T, typename Term2>
		void merge_terms_impl0(T &&series, const echelon_descriptor<Term2> &ed,
			typename std::enable_if<!std::is_base_of<base_series<Term,Derived>,typename strip_cv_ref<T>::type>::value>::type * = piranha_nullptr)
		{
			// No worries about same object, just forward.
			merge_terms_impl1<Sign>(std::forward<T>(series),ed);
		}
		// Overload if we cannot move objects from series.
		template <bool Sign, typename T, typename Term2>
		void merge_terms_impl1(T &&series, const echelon_descriptor<Term2> &ed,
			typename std::enable_if<!is_nonconst_rvalue_ref<T &&>::value>::type * = piranha_nullptr)
		{
std::cout << "LOL copy!\n";
			const auto it_f = series.m_container.end();
			try {
				for (auto it = series.m_container.begin(); it != it_f; ++it) {
					insert<Sign>(*it,ed);
				}
			} catch (...) {
				// In case of any insertion error, zero out this series.
				m_container.clear();
				throw;
			}
		}
		static void swap_for_merge(container_type &&c1, container_type &&c2, bool &swap)
		{
			// Swap only if the buckets count (i.e., the memory allocated) is greater.
			if (c1.n_buckets() > c2.n_buckets()) {
				container_type tmp(std::move(c1));
				c1 = std::move(c2);
				c2 = std::move(tmp);
				swap = true;
			}
std::cout << "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
		}
		// This overloads the above in the case we are dealing with two different container types:
		// in such a condition, we won't do any swapping.
		template <typename OtherContainerType>
		static void swap_for_merge(container_type &&, OtherContainerType &&, bool &)
		{
static_assert(!std::is_same<typename strip_cv_ref<OtherContainerType>::type,container_type>::value,"ahi ahi");
std::cout << "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBb\n";
		}
		// Overload if we can move objects from series.
		template <bool Sign, typename T, typename Term2>
		void merge_terms_impl1(T &&series, const echelon_descriptor<Term2> &ed,
			typename std::enable_if<is_nonconst_rvalue_ref<T &&>::value>::type * = piranha_nullptr)
		{
std::cout << "LOL move!\n";
			bool swap = false;
			// Try to steal memory from other.
			swap_for_merge(std::move(m_container),std::move(series.m_container),swap);
			try {
				const auto it_f = series.m_container._m_end();
				for (auto it = series.m_container._m_begin(); it != it_f; ++it) {
					insert<Sign>(std::move(*it),ed);
				}
				// If we swapped the operands and a negative merge was performed, we need to change
				// the signs of all coefficients.
				if (swap && !Sign) {
std::cout << "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC\n";
					const auto it_f2 = m_container.end();
					for (auto it = m_container.begin(); it != it_f2; ++it) {
						it->m_cf.negate(ed);
					}
				}
			} catch (...) {
				// In case of any insertion error, zero out both series.
				m_container.clear();
				series.m_container.clear();
				throw;
			}
			// The other series must alway be cleared, since we moved out the terms.
			series.m_container.clear();
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
		/**
		 * @throw unspecified any exception thrown by the copy constructor of piranha::hop_table.
		 */
		base_series(const base_series &) = default;
		/// Defaulted move constructor.
		base_series(base_series &&) = default;
		/// Trivial destructor.
		~base_series() piranha_noexcept_spec(true)
		{
			BOOST_CONCEPT_ASSERT((concept::ContainerElement<Derived>));
		}
		/// Defaulted copy-assignment operator.
		/**
		 * @throw unspecified any exception thrown by the copy assignment operator of piranha::hop_table.
		 * 
		 * @return reference to \p this.
		 */
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
		 * This method will insert \p term into the series using internally piranha::hop_table::insert and
		 * with \p ed as reference piranha::echelon_descriptor.
		 * 
		 * The insertion algorithm proceeds as follows:
		 * 
		 * - if \p term is not of type base_series::term_type, it is forwarded to construct an instance of
		 *   base_series::term_type, and the algorithm proceeds to insert that instead;
		 * - if the term is not compatible for insertion, an \p std::invalid_argument exception is thrown;
		 * - if the term is ignorable, the method will return;
		 * - if the term is already in the series, then:
		 *   - its coefficient is added (i.e., \p Sign is \p true) or subtracted (i.e., \p Sign is \p false)
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
		template <bool Sign, typename T, typename Term2>
		void insert(T &&term, const echelon_descriptor<Term2> &ed)
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
		template <typename T, typename Term2>
		void insert(T &&term, const echelon_descriptor<Term2> &ed)
		{
			insert<true>(std::forward<T>(term),ed);
		}
		/// Merge terms from another series.
		/**
		 * This template method is activated only if \p T derives from piranha::base_series.
		 * 
		 * All terms in \p series will be inserted into \p this using piranha::base_series::insert. If any exception occurs during the insertion process,
		 * \p this will be left in an undefined (but valid) state. \p series is allowed to be \p this (in such a case, a copy of \p series will be created
		 * before proceeding with the merge).
		 * 
		 * @param[in] series piranha::base_series whose terms will be merged into \p this.
		 * @param[in] ed reference piranha::echelon_descriptor.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::base_series::insert,
		 * - the copy constructor of piranha::base_series,
		 * - the coefficient type's <tt>negate()</tt> method.
		 */
		template <bool Sign, typename T, typename Term2>
		void merge_terms(T &&series, const echelon_descriptor<Term2> &ed,
			typename std::enable_if<std::is_base_of<base_series_tag,typename strip_cv_ref<T>::type>::value>::type * = piranha_nullptr)
		{
			merge_terms_impl0<Sign>(std::forward<T>(series),ed);
		}
		/// Merge arguments.
		/**
		 * This method will return a piranha::base_series resulting from merging a new set of arguments into the current series. \p orig_ed is the
		 * piranha::echelon_descriptor which the current terms refer to, while \p new_ed is the piranha::echelon_descriptor containing the new set of arguments.
		 * 
		 * The algorithm for merging iterates over the terms of the current series, performing at each iteration the following operations:
		 * 
		 * - create a new coefficient \p c from the output of the current coefficient's <tt>merge_args()</tt> method,
		 * - create a new key \p k from the output of the current key's <tt>merge_args()</tt> method,
		 * - create a new piranha::base_series::term_type \p t from \p c and \p k,
		 * - insert \p t into the series to be returned.
		 * 
		 * @param[in] orig_ed original piranha::echelon_descriptor.
		 * @param[in] new_ed new piranha::echelon_descriptor.
		 * 
		 * @return a piranha::base_series whose terms result from merging the new echelon descriptor into the terms of \p this.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::base_series::insert(),
		 * - the <tt>merge_args()</tt> method of coefficient and/or key types,
		 * - the constructor of piranha::base_series::term_type from a coefficient - key pair.
		 */
		template <typename Term2>
		base_series merge_args(const echelon_descriptor<Term2> &orig_ed, const echelon_descriptor<Term2> &new_ed) const
		{
			piranha_assert(std::is_sorted(orig_ed.template get_args<term_type>().begin(),orig_ed.template get_args<term_type>().end()));
			piranha_assert(std::is_sorted(new_ed.template get_args<term_type>().begin(),new_ed.template get_args<term_type>().end()));
			piranha_assert(new_ed.template get_args<term_type>().size() > orig_ed.template get_args<term_type>().size());
			typedef typename term_type::cf_type cf_type;
			typedef typename term_type::key_type key_type;
			base_series retval;
			for (auto it = m_container.begin(); it != m_container.end(); ++it) {
				cf_type new_cf(it->m_cf.merge_args(orig_ed,new_ed));
				key_type new_key(it->m_key.merge_args(orig_ed.template get_args<term_type>(),new_ed.template get_args<term_type>()));
				retval.insert(term_type(std::move(new_cf),std::move(new_key)),new_ed);
			}
			return retval;
		}
	protected:
		/// Terms container.
		container_type m_container;
};

}

#endif
