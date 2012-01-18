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

#ifndef PIRANHA_SERIES_HPP
#define PIRANHA_SERIES_HPP

#include <algorithm>
#include <boost/any.hpp>
#include <boost/concept/assert.hpp>
#include <boost/integer_traits.hpp>
#include <boost/iterator/indirect_iterator.hpp>
#include <boost/math/special_functions/trunc.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "base_term.hpp"
#include "concepts/container_element.hpp"
#include "concepts/crtp.hpp"
#include "concepts/term.hpp"
#include "concepts/truncator.hpp"
#include "config.hpp"
#include "debug_access.hpp"
#include "detail/series_fwd.hpp"
#include "echelon_size.hpp"
#include "hash_set.hpp"
#include "integer.hpp"
#include "math.hpp" // For negate() and math specialisations.
#include "print_coefficient.hpp"
#include "series_multiplier.hpp"
#include "series_binary_operators.hpp"
#include "settings.hpp"
#include "symbol_set.hpp"
#include "tracing.hpp"
#include "truncator.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

// Hash functor for term type in series.
template <typename Term>
struct term_hasher
{
	std::size_t operator()(const Term &term) const
	{
		return term.hash();
	}
};

}

/// Series class.
/**
 * This class provides arithmetic and relational operators overloads for interaction with other series and non-series (scalar) types.
 * Addition and subtraction are implemented directly within this class, both for series and scalar operands. Multiplication of series by
 * scalar types is also implemented in this class, whereas series-by-series multiplication is provided via the external helper class
 * piranha::series_multiplier (whose behaviour can be specialised to provide fast multiplication capabilities).
 * Division by scalar types is also supported.
 * 
 * Support for comparison with series and scalar type is also provided.
 * 
 * \section type_requirements Type requirements
 * 
 * - \p Term must be a model of piranha::concept::Term.
 * - \p Derived must be a model of piranha::concept::CRTP, with piranha::series
 *   of \p Term and \p Derived as base class.
 * - \p Derived must be a model of piranha::concept::ContainerElement.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * Unless otherwise specified, this class provides the strong exception safety guarantee for all operations.
 * 
 * \section move_semantics Move semantics
 * 
 * Moved-from series are left in a state equivalent to an empty series.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo cast operator, to series and non-series types.
 * \todo cast operator would allow to define in-place operators with fundamental types as first operand.
 */
template <typename Term, typename Derived>
class series: series_binary_operators, detail::series_tag
{
		BOOST_CONCEPT_ASSERT((concept::Term<Term>));
		BOOST_CONCEPT_ASSERT((concept::CRTP<series<Term,Derived>,Derived>));
	public:
		/// Alias for term type.
		typedef Term term_type;
	private:
		// Make friend with all series.
		template <typename Term2, typename Derived2>
		friend class series;
		// Make friend with debugging class.
		template <typename T>
		friend class debug_access;
		// Make friend with series multiplier class.
		template <typename Series1, typename Series2, typename Enable>
		friend class series_multiplier;
		// Make friend with all truncator classes.
		template <typename... Series>
		friend class truncator;
		// Make friend with series binary operators class.
		friend class series_binary_operators;
	protected:
		/// Container type for terms.
		typedef hash_set<term_type,detail::term_hasher<Term>> container_type;
	private:
		// Avoid confusing doxygen.
		typedef decltype(std::declval<container_type>().evaluate_sparsity()) sparsity_info_type;
		// Overload for completely different term type: copy-convert to term_type and proceed.
		template <bool Sign, typename T>
		void dispatch_insertion(T &&term, typename std::enable_if<
			!std::is_same<typename std::decay<T>::type,term_type>::value &&
			!is_nonconst_rvalue_ref<T &&>::value
			>::type * = piranha_nullptr)
		{
			dispatch_insertion<Sign>(term_type(typename term_type::cf_type(term.m_cf),
				typename term_type::key_type(term.m_key,m_symbol_set)));
		}
		// Overload for completely different term type: move-convert to term_type and proceed.
		template <bool Sign, typename T>
		void dispatch_insertion(T &&term, typename std::enable_if<
			!std::is_same<typename std::decay<T>::type,term_type>::value &&
			is_nonconst_rvalue_ref<T &&>::value
			>::type * = piranha_nullptr)
		{
			dispatch_insertion<Sign>(term_type(typename term_type::cf_type(std::move(term.m_cf)),
				typename term_type::key_type(std::move(term.m_key),m_symbol_set)));
		}
		// Overload for term_type.
		template <bool Sign, typename T>
		void dispatch_insertion(T &&term, typename std::enable_if<
			std::is_same<typename std::decay<T>::type,term_type>::value
			>::type * = piranha_nullptr)
		{
			// Debug checks.
			piranha_assert(empty() || m_container.begin()->is_compatible(m_symbol_set));
			// Generate error if term is not compatible.
			if (unlikely(!term.is_compatible(m_symbol_set))) {
				piranha_throw(std::invalid_argument,"cannot insert incompatible term");
			}
			// Discard ignorable term.
			if (unlikely(term.is_ignorable(m_symbol_set))) {
				return;
			}
			insertion_impl<Sign>(std::forward<T>(term));
		}
		template <bool Sign, typename Iterator>
		static void insertion_cf_arithmetics(Iterator &it, const term_type &term)
		{
			if (Sign) {
				it->m_cf += term.m_cf;
			} else {
				it->m_cf -= term.m_cf;
			}
		}
		template <bool Sign, typename Iterator>
		static void insertion_cf_arithmetics(Iterator &it, term_type &&term)
		{
			if (Sign) {
				it->m_cf += std::move(term.m_cf);
			} else {
				it->m_cf -= std::move(term.m_cf);
			}
		}
		// Insert compatible, non-ignorable term.
		template <bool Sign, typename T>
		void insertion_impl(T &&term, typename std::enable_if<
			std::is_same<typename std::decay<T>::type,term_type>::value
			>::type * = piranha_nullptr)
		{
			// NOTE: here we are basically going to reconstruct hash_set::insert() with the goal
			// of optimising things by avoiding one branch.
			// Handle the case of a table with no buckets.
			if (unlikely(!m_container.bucket_count())) {
				m_container._increase_size();
			}
			// Try to locate the element.
			auto bucket_idx = m_container._bucket(term);
			const auto it = m_container._find(term,bucket_idx);
			// Cleanup function that checks ignorability and compatibility of an element in the hash set,
			// and removes it if necessary.
			auto cleanup = [this](const typename container_type::const_iterator &it) -> void {
				if (unlikely(!it->is_compatible(this->m_symbol_set) || it->is_ignorable(this->m_symbol_set))) {
					this->m_container.erase(it);
				}
			};
			if (it == m_container.end()) {
				if (unlikely(m_container.size() == boost::integer_traits<size_type>::const_max)) {
					piranha_throw(std::overflow_error,"maximum number of elements reached");
				}
				// Term is new. Handle the case in which we need to rehash because of load factor.
				if (unlikely(static_cast<double>(m_container.size() + size_type(1u)) / m_container.bucket_count() >
					m_container.max_load_factor()))
				{
					m_container._increase_size();
					// We need a new bucket index in case of a rehash.
					bucket_idx = m_container._bucket(term);
				}
				const auto new_it = m_container._unique_insert(std::forward<T>(term),bucket_idx);
				m_container._update_size(m_container.size() + size_type(1u));
				// Insertion was successful, change sign if requested.
				if (!Sign) {
					try {
						math::negate(new_it->m_cf);
						cleanup(new_it);
					} catch (...) {
						// Run the cleanup function also in case of exceptions, as we do not know
						// in which state the modified term is.
						cleanup(new_it);
						throw;
					}
				}
			} else {
				// Assert the existing term is not ignorable and it is compatible.
				piranha_assert(!it->is_ignorable(m_symbol_set) && it->is_compatible(m_symbol_set));
				try {
					// The term exists already, update it.
					insertion_cf_arithmetics<Sign>(it,std::forward<T>(term));
					// Check if the term has become ignorable or incompatible after the modification.
					cleanup(it);
				} catch (...) {
					cleanup(it);
					throw;
				}
			}
		}
		// Terms merging
		// =============
		// NOTE: ideas to improve the algorithm:
		// - optimization when merging with self: add each coefficient to itself, instead of copying and merging.
		// - optimization when merging with series with same bucket size: avoid computing the destination bucket,
		//   as it will be the same as the original.
		// Overload in case that T derives from the same type as this (series<Term,Derived>).
		template <bool Sign, typename T>
		void merge_terms_impl0(T &&s,
			typename std::enable_if<std::is_base_of<series<Term,Derived>,typename std::decay<T>::type>::value>::type * = piranha_nullptr)
		{
			// NOTE: here we can take the pointer to series and compare it to this because we know from enable_if that
			// series is an instance of the type of this.
			if (unlikely(&s == this)) {
				// If the two series are the same object, we need to make a copy.
				// NOTE: we do not forward here, when making the copy, because if T is a non-const
				// rvalue reference we might actually erase this: with Derived(series), a move
				// constructor might end up being called.
				merge_terms_impl1<Sign>(series<Term,Derived>(s));
			} else {
				merge_terms_impl1<Sign>(std::forward<T>(s));
			}
		}
		// Overload in case that T is not an instance of the type of this.
		template <bool Sign, typename T>
		void merge_terms_impl0(T &&s,
			typename std::enable_if<!std::is_base_of<series<Term,Derived>,typename std::decay<T>::type>::value>::type * = piranha_nullptr)
		{
			// No worries about same object, just forward.
			merge_terms_impl1<Sign>(std::forward<T>(s));
		}
		// Overload if we cannot move objects from series.
		template <bool Sign, typename T>
		void merge_terms_impl1(T &&s,
			typename std::enable_if<!is_nonconst_rvalue_ref<T &&>::value>::type * = piranha_nullptr)
		{
			const auto it_f = s.m_container.end();
			try {
				for (auto it = s.m_container.begin(); it != it_f; ++it) {
					insert<Sign>(*it);
				}
			} catch (...) {
				// In case of any insertion error, zero out this series.
				m_container.clear();
				throw;
			}
		}
		static void swap_for_merge(container_type &&c1, container_type &&c2, bool &swap)
		{
			piranha_assert(!swap);
			// Do not do anything in case of overflows.
			if (unlikely(c1.size() > boost::integer_traits<size_type>::const_max - c2.size())) {
				return;
			}
			// This is the maximum number of terms in the return value.
			const auto max_size = c1.size() + c2.size();
			// This will be the maximum required number of buckets.
			size_type max_n_buckets;
			try {
				piranha_assert(c1.max_load_factor() > 0);
				max_n_buckets = boost::numeric_cast<size_type>(boost::math::trunc(max_size / c1.max_load_factor()));
			} catch (...) {
				// Ignore any error on conversions.
				return;
			}
			// Now we swap the containers, if the first one is not enough to contain the expected number of terms
			// and if the second one is larger.
			if (c1.bucket_count() < max_n_buckets && c2.bucket_count() > c1.bucket_count()) {
				container_type tmp(std::move(c1));
				c1 = std::move(c2);
				c2 = std::move(tmp);
				swap = true;
			}
		}
		// This overloads the above in the case we are dealing with two different container types:
		// in such a condition, we won't do any swapping.
		template <typename OtherContainerType>
		static void swap_for_merge(container_type &&, OtherContainerType &&, bool &)
		{}
		// Overload if we can move objects from series.
		template <bool Sign, typename T>
		void merge_terms_impl1(T &&s,
			typename std::enable_if<is_nonconst_rvalue_ref<T &&>::value>::type * = piranha_nullptr)
		{
			bool swap = false;
			// Try to steal memory from other.
			swap_for_merge(std::move(m_container),std::move(s.m_container),swap);
			try {
				const auto it_f = s.m_container._m_end();
				for (auto it = s.m_container._m_begin(); it != it_f; ++it) {
					insert<Sign>(std::move(*it));
				}
				// If we swapped the operands and a negative merge was performed, we need to change
				// the signs of all coefficients.
				if (swap && !Sign) {
					const auto it_f2 = m_container.end();
					for (auto it = m_container.begin(); it != it_f2;) {
						math::negate(it->m_cf);
						if (unlikely(!it->is_compatible(m_symbol_set) || it->is_ignorable(m_symbol_set))) {
							// Erase the invalid term.
							it = m_container.erase(it);
						} else {
							++it;
						}
					}
				}
			} catch (...) {
				// In case of any insertion error, zero out both series.
				m_container.clear();
				s.m_container.clear();
				throw;
			}
			// The other series must alway be cleared, since we moved out the terms.
			s.m_container.clear();
		}
		// Generic construction
		// ====================
		// Series with same echelon size, same term type, move.
		template <typename Series>
		void dispatch_generic_construction(Series &&s,
			typename std::enable_if<std::is_base_of<detail::series_tag,typename std::decay<Series>::type>::value &&
			echelon_size<term_type>::value == echelon_size<typename std::decay<Series>::type::term_type>::value &&
			std::is_same<term_type,typename std::decay<Series>::type::term_type>::value &&
			is_nonconst_rvalue_ref<Series &&>::value
			>::type * = piranha_nullptr)
		{
			static_assert(!std::is_base_of<series,typename std::decay<Series>::type>::value,"Invalid series type for generic construction.");
			m_symbol_set = std::move(s.m_symbol_set);
			m_container = std::move(s.m_container);
		}
		// Series with same echelon size, same term type, copy.
		template <typename Series>
		void dispatch_generic_construction(Series &&s,
			typename std::enable_if<std::is_base_of<detail::series_tag,typename std::decay<Series>::type>::value &&
			echelon_size<term_type>::value == echelon_size<typename std::decay<Series>::type::term_type>::value &&
			std::is_same<term_type,typename std::decay<Series>::type::term_type>::value &&
			!is_nonconst_rvalue_ref<Series &&>::value
			>::type * = piranha_nullptr)
		{
			static_assert(!std::is_base_of<series,typename std::decay<Series>::type>::value,"Invalid series type for generic construction.");
			m_symbol_set = s.m_symbol_set;
			m_container = s.m_container;
		}
		// Series with same echelon size and different term type, move.
		template <typename Series>
		void dispatch_generic_construction(Series &&s,
			typename std::enable_if<std::is_base_of<detail::series_tag,typename std::decay<Series>::type>::value &&
			echelon_size<term_type>::value == echelon_size<typename std::decay<Series>::type::term_type>::value &&
			!std::is_same<term_type,typename std::decay<Series>::type::term_type>::value &&
			is_nonconst_rvalue_ref<Series &&>::value
			>::type * = piranha_nullptr)
		{
			m_symbol_set = std::move(s.m_symbol_set);
			merge_terms<true>(std::forward<Series>(s));
		}
		// Series with same echelon size and different term type, copy.
		template <typename Series>
		void dispatch_generic_construction(Series &&s,
			typename std::enable_if<std::is_base_of<detail::series_tag,typename std::decay<Series>::type>::value &&
			echelon_size<term_type>::value == echelon_size<typename std::decay<Series>::type::term_type>::value &&
			!std::is_same<term_type,typename std::decay<Series>::type::term_type>::value &&
			!is_nonconst_rvalue_ref<Series &&>::value
			>::type * = piranha_nullptr)
		{
			m_symbol_set = s.m_symbol_set;
			merge_terms<true>(s);
		}
		// Series with smaller echelon size.
		template <typename Series>
		void dispatch_generic_construction(Series &&s,
			typename std::enable_if<std::is_base_of<detail::series_tag,typename std::decay<Series>::type>::value &&
			echelon_size<typename std::decay<Series>::type::term_type>::value < echelon_size<term_type>::value
			>::type * = piranha_nullptr)
		{
			dispatch_generic_construction_from_cf(std::forward<Series>(s));
		}
		// Non-series.
		template <typename T>
		void dispatch_generic_construction(T &&x,
			typename std::enable_if<!std::is_base_of<detail::series_tag,typename std::decay<T>::type>::value>::type * = piranha_nullptr)
		{
			dispatch_generic_construction_from_cf(std::forward<T>(x));
		}
		// Construct as coefficient helper.
		template <typename T>
		void dispatch_generic_construction_from_cf(T &&x)
		{
			typedef typename term_type::cf_type cf_type;
			typedef typename term_type::key_type key_type;
			cf_type cf(std::forward<T>(x));
			key_type key(m_symbol_set);
			insert(term_type(std::move(cf),std::move(key)));
		}
		// In-place add/subtract
		// =====================
		// Overload for non-series type.
		template <bool Sign, typename T>
		void dispatch_in_place_add(T &&x, typename std::enable_if<!std::is_base_of<detail::series_tag,typename std::decay<T>::type>::value>::type * = piranha_nullptr)
		{
			term_type tmp(typename term_type::cf_type(std::forward<T>(x)),typename term_type::key_type(m_symbol_set));
			insert<Sign>(std::move(tmp));
		}
		// Overload for series type with smaller echelon size.
		template <bool Sign, typename T>
		void dispatch_in_place_add(T &&series, typename std::enable_if<std::is_base_of<detail::series_tag,typename std::decay<T>::type>::value &&
			echelon_size<typename std::decay<T>::type::term_type>::value < echelon_size<term_type>::value>::type * = piranha_nullptr)
		{
			term_type tmp(typename term_type::cf_type(std::forward<T>(series)),typename term_type::key_type(m_symbol_set));
			insert<Sign>(std::move(tmp));
		}
		// Overload for series with same echelon size.
		template <bool Sign, typename T>
		void dispatch_in_place_add(T &&other, typename std::enable_if<std::is_base_of<detail::series_tag,typename std::decay<T>::type>::value &&
			echelon_size<typename std::decay<T>::type::term_type>::value == echelon_size<term_type>::value>::type * = piranha_nullptr)
		{
			// NOTE: if they are not the same, we are going to do heavy calculations anyway: mark it "likely".
			if (likely(m_symbol_set == other.m_symbol_set)) {
				merge_terms<Sign>(std::forward<T>(other));
			} else {
				// Let's deal with the first series.
				auto merge = m_symbol_set.merge(other.m_symbol_set);
				if (merge != m_symbol_set) {
					operator=(merge_args(merge));
				}
				// Second series.
				if (merge != other.m_symbol_set) {
					auto other_copy = other.merge_args(merge);
					merge_terms<Sign>(std::move(other_copy));
				} else {
					merge_terms<Sign>(std::forward<T>(other));
				}
			}
		}
		// In-place multiply
		// =================
		// Overloads for non-series and series with smaller echelon size.
		template <typename T>
		void mixed_multiply(T &&x)
		{
			const auto it_f = m_container.end();
			try {
				for (auto it = m_container.begin(); it != it_f;) {
					// NOTE: no forwarding here, as x is needed in multiple places.
					// Maybe it could be forwarded just for the last term?
					it->m_cf *= x;
					if (unlikely(!it->is_compatible(m_symbol_set) || it->is_ignorable(m_symbol_set))) {
						// Erase will return the next iterator.
						it = m_container.erase(it);
					} else {
						++it;
					}
				}
			} catch (...) {
				// In case of any error, just clear the series out.
				m_container.clear();
				throw;
			}
		}
		template <typename T>
		void dispatch_multiply(T &&x, typename std::enable_if<!std::is_base_of<detail::series_tag,typename std::decay<T>::type>::value>::type * = piranha_nullptr)
		{
			mixed_multiply(std::forward<T>(x));
		}
		template <typename T>
		void dispatch_multiply(T &&x, typename std::enable_if<
			std::is_base_of<detail::series_tag,typename std::decay<T>::type>::value &&
			echelon_size<typename std::decay<T>::type::term_type>::value < echelon_size<term_type>::value
			>::type * = piranha_nullptr)
		{
			mixed_multiply(std::forward<T>(x));
		}
		// Overload for series with same echelon size.
		// Multiply by series.
		template <typename T>
		series multiply_by_series(const T &series) const
		{
			series_multiplier<Derived,T> sm(*static_cast<Derived const *>(this),series);
			auto retval = sm();
			tracing::trace("number_of_series_multiplications",[&retval](boost::any &x) -> void {
				if (unlikely(x.empty())) {
					x = 0ull;
				}
				auto ptr = boost::any_cast<unsigned long long>(&x);
				if (likely((bool)ptr && retval.size())) {
					++*ptr;
				}
			});
			tracing::trace("accumulated_sparsity",[this,&series,&retval](boost::any &x) -> void {
				if (unlikely(x.empty())) {
					x = 0.;
				}
				auto ptr = boost::any_cast<double>(&x);
				if (likely((bool)ptr && retval.size())) {
					*ptr += (static_cast<double>(this->size()) * series.size()) / retval.size();
				}
			});
			return retval;
		}
		template <typename T>
		void dispatch_multiply(T &&other, typename std::enable_if<
			std::is_base_of<detail::series_tag,typename std::decay<T>::type>::value &&
			echelon_size<typename std::decay<T>::type::term_type>::value == echelon_size<term_type>::value
			>::type * = piranha_nullptr)
		{
			// NOTE: all this dancing around with base and derived types for series is necessary as the mechanism of
			// specialization of series_multiplier and truncator depends on the derived types - which must then be preserved and
			// not casted away to the base types.
			// Base types of multiplicand series.
			typedef series base_type1;
			typedef series<typename std::decay<T>::type::term_type,typename std::decay<T>::type> base_type2;
			if (likely(m_symbol_set == other.m_symbol_set)) {
				operator=(multiply_by_series(other));
			} else {
				// Let's deal with the first series.
				auto merge = m_symbol_set.merge(other.m_symbol_set);
				piranha_assert(merge == other.m_symbol_set.merge(m_symbol_set));
				if (merge != m_symbol_set) {
					operator=(merge_args(merge));
				}
				// Second series.
				if (merge != other.m_symbol_set) {
					typename std::decay<T>::type other_copy;
					static_cast<base_type2 &>(other_copy) = other.merge_args(merge);
					static_assert(std::is_same<base_type2,decltype(other.merge_args(merge))>::value,"Inconsistent type.");
					operator=(multiply_by_series(other_copy));
				} else {
					operator=(multiply_by_series(other));
				}
			}
		}
	public:
		/// Size type.
		/**
		 * Used to represent the number of terms in the series. Equivalent to piranha::hash_set::size_type.
		 */
		typedef typename container_type::size_type size_type;
		/// Defaulted default constructor.
		series() = default;
		/// Defaulted copy constructor.
		/**
		 * @throws unspecified any exception thrown by the copy constructor of piranha::hash_set.
		 */
		series(const series &) = default;
		/// Defaulted move constructor.
		series(series &&other) piranha_noexcept_spec(true) : m_symbol_set(std::move(other.m_symbol_set)),m_container(std::move(other.m_container)) {}
		/// Generic constructor.
		/**
		 * This template constructor is enabled only if \p T does not derive from piranha::series of \p Term and
		 * \p Derived (so that it does not compete with the copy and move constructors in overload resolution).
		 * The generic construction algorithm works as follows:
		 * 
		 * - if \p T is an instance of piranha::series with the same echelon size as the calling type:
		 *   - if the term type of \p T is the same as that of <tt>this</tt>:
		 *     - the internal terms container and symbol set of \p x are forwarded to construct \p this;
		 *   - else:
		 *     - the symbol set of \p x is forwarded to construct the symbol set of this and all terms from \p x are inserted into \p this
		 *       (after conversion to \p term_type);
		 * - else:
		 *   - \p x is used to construct a new term as follows:
		 *     - \p x is forwarded to construct a coefficient;
		 *     - an empty arguments set will be used to construct a key;
		 *     - coefficient and key are used to construct the new term instance;
		 *   - the new term is inserted into \p this.
		 * 
		 * If \p x is an instance of piranha::series with echelon size larger than the calling type, a compile-time error will be produced.
		 * 
		 * @param[in] x object to construct from.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the copy assignment operators of piranha::symbol_set and piranha::hash_set,
		 * - the construction of a coefficient from \p x or of a key from piranha::symbol_set,
		 * - the construction of a term from a coefficient-key pair,
		 * - insert(),
		 * - the copy constructor of piranha::series.
		 */
		template <typename T>
		explicit series(T &&x, typename std::enable_if<!std::is_base_of<series,typename std::decay<T>::type>::value>::type * = piranha_nullptr)
		{
			dispatch_generic_construction(std::forward<T>(x));
		}
		/// Trivial destructor.
		~series() piranha_noexcept_spec(true)
		{
			BOOST_CONCEPT_ASSERT((concept::ContainerElement<Derived>));
			piranha_assert(destruction_checks());
		}
		/// Copy-assignment operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @throws unspecified any exception thrown by the copy constructor.
		 * 
		 * @return reference to \p this.
		 */
		series &operator=(const series &other)
		{
			if (likely(this != &other)) {
				series tmp(other);
				*this = std::move(tmp);
			}
			return *this;
		}
		/// Move assignment operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 */
		series &operator=(series &&other) piranha_noexcept_spec(true)
		{
			if (likely(this != &other)) {
				m_symbol_set = std::move(other.m_symbol_set);
				m_container = std::move(other.m_container);
			}
			return *this;
		}
		/// Generic assignment operator.
		/**
		 * This template constructor is enabled only if \p T does not derive from piranha::series of \p Term and
		 * \p Derived. Generic assignment is equivalent to assignment to a piranha::series constructed
		 * via the generic constructor.
		 * 
		 * @param[in] x assignment argument.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the generic constructor.
		 */
		template <typename T>
		typename std::enable_if<!std::is_base_of<series,typename std::decay<T>::type>::value,series &>::type
			operator=(T &&x)
		{
			return operator=(series(std::forward<T>(x)));
		}
		/// Symbol set getter.
		/**
		 * @return const reference to the piranha::symbol_set describing the series.
		 */
		const symbol_set &get_symbol_set() const
		{
			return m_symbol_set;
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
		/// Insert generic term.
		/**
		 * This method will insert \p term into the series using internally piranha::hash_set::insert. The method is enabled only
		 * if \p term derives from piranha::base_term.
		 * 
		 * The insertion algorithm proceeds as follows:
		 * 
		 * - if \p term is not of type series::term_type, its coefficient and key are forwarded to construct a series::term_type
		 *   as follows:
		 *     - <tt>term</tt>'s coefficient is forwarded to construct a coefficient of type series::term_type::cf_type;
		 *     - <tt>term</tt>'s key is forwarded, together with the series' piranha::symbol_set,
		 *       to construct a key of type series::term_type::key_type;
		 *     - the newly-constructed coefficient and key are used to construct an instance of series::term_type, which will replace \p term as the
		 *       argument of insertion for the remaining portion of the algorithm;
		 * - if the term is not compatible for insertion, an \p std::invalid_argument exception is thrown;
		 * - if the term is ignorable, the method will return;
		 * - if the term is already in the series, then:
		 *   - its coefficient is added (if \p Sign is \p true) or subtracted (if \p Sign is \p false)
		 *     to the existing term's coefficient;
		 *   - if, after the addition/subtraction the existing term is ignorable, it will be erased;
		 * - else:
		 *   - the term is inserted into the term container and, if \p Sign is \p false, its coefficient is negated.
		 * 
		 * After any modification to an existing term in the series (e.g., via insertion with negative \p Sign or via in-place addition
		 * or subtraction of existing coefficients), the term will be checked again for compatibility and ignorability, and, in case
		 * the term has become incompatible or ignorable, it will be erased from the series.
		 * 
		 * The exception safety guarantee upon insertion is that the series will be left in an undefined but valid state. Such a guarantee
		 * relies on the fact that the addition/subtraction and negation methods of the coefficient type will leave the coefficient in a valid
		 * (possibly undefined) state in face of exceptions.
		 * 
		 * @param[in] term term to be inserted.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the constructors of series::term_type, and of its coefficient and key types,
		 *   invoked by any necessary conversion;
		 * - piranha::hash_set::insert(),
		 * - piranha::hash_set::find(),
		 * - piranha::hash_set::erase(),
		 * - piranha::math::negate(), in-place addition/subtraction on coefficient types.
		 * @throws std::invalid_argument if \p term is incompatible.
		 */
		template <bool Sign, typename T>
		typename std::enable_if<std::is_base_of<detail::base_term_tag,typename std::decay<T>::type>::value,void>::type insert(T &&term)
		{
			dispatch_insertion<Sign>(std::forward<T>(term));
		}
		/// Insert generic term with <tt>Sign = true</tt>.
		/**
		 * Convenience wrapper for the generic insert() method, with \p Sign set to \p true.
		 * 
		 * @param[in] term term to be inserted.
		 * 
		 * @throws unspecified any exception thrown by generic insert().
		 */
		template <typename T>
		void insert(T &&term)
		{
			insert<true>(std::forward<T>(term));
		}
		/// Truncator getter.
		/**
		 * @return an instance of piranha::truncator of \p Derived constructed using \p this.
		 * 
		 * @throws unspecified any exception thrown by the constructor of piranha::truncator.
		 */
		truncator<Derived> get_truncator() const
		{
			BOOST_CONCEPT_ASSERT((concept::Truncator<Derived>));
			return truncator<Derived>(*static_cast<Derived const *>(this));
		}
		/// In-place addition.
		/**
		 * The addition algorithm proceeds as follows:
		 * 
		 * - if \p other is an instance of piranha::series with the same echelon size as <tt>this</tt>:
		 *   - if the symbol sets of \p this and \p other differ, they are merged using piranha::symbol_set::merge(),
		 *     and \p this and \p other are modified as necessary to be compatible with the merged set
		 *     (a copy of \p other might be created if it requires modifications);
		 *   - all terms in \p other (or its copy) will be merged into \p this using piranha::insert();
		 * - else:
		 *   - a \p Term instance will be constructed as follows:
		 *     - \p other will be forwarded to construct the coefficient;
		 *     - the arguments set of \p this will be used to construct the key;
		 *   - the term will be inserted into \p this using insert().
		 * 
		 * If \p other is an instance of piranha::series with echelon size larger than the calling type, a compile-time error will be produced.
		 * 
		 * The exception safety guarantee for this method is the basic one.
		 * 
		 * Please note that in-place addition for series works slightly differently from addition for native C++ types: the coefficients of the terms
		 * to be inserted into the series are, if necessary, first converted to the coefficient type of \p term_type and then added in-place
		 * to exsisting coefficients. This behaviour
		 * is different from the standard mechanism of type promotions for arithmetic C++ types.
		 * 
		 * @param[in] other object to be added to the series.
		 * 
		 * @return reference to \p this, cast to type \p Derived.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - insert(),
		 * - the <tt>merge_args()</tt> method of the key type,
		 * - the constructors of \p term_type, coefficient and key types,
		 * - piranha::symbol_set::merge().
		 */
		template <typename T>
		Derived &operator+=(T &&other)
		{
			dispatch_in_place_add<true>(std::forward<T>(other));
			return *static_cast<Derived *>(this);
		}
		/// Identity operator.
		/**
		 * @return copy of \p this, cast to \p Derived.
		 */
		Derived operator+() const
		{
			return Derived(*static_cast<Derived const *>(this));
		}
		/// In-place addition.
		/**
		 * Analogous to operator+=(), apart from a change in sign.
		 * 
		 * @param[in] other object to be subtracted from the series.
		 * 
		 * @return reference to \p this, cast to type \p Derived.
		 * 
		 * @throws unspecified any exception thrown by operator+=().
		 */
		template <typename T>
		Derived &operator-=(T &&other)
		{
			dispatch_in_place_add<false>(std::forward<T>(other));
			return *static_cast<Derived *>(this);
		}
		/// Negation operator.
		/**
		 * @return a copy of \p this on which negate() has been called.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - negate(),
		 * - the copy constructor of \p Derived.
		 */
		Derived operator-() const
		{
			Derived retval(*static_cast<Derived const *>(this));
			retval.negate();
			return retval;
		}
		/// Negate series in-place.
		/**
		 * This method will call math::negate() on the coefficients of all terms. In case of exceptions,
		 * the basic exception safety guarantee is provided.
		 * 
		 * If any term becomes ignorable or incompatible after negation, it will be erased from the series.
		 * 
		 * @throws unspecified any exception thrown by math::negate() on the coefficient type.
		 */
		void negate()
		{
			try {
				const auto it_f = m_container.end();
				for (auto it = m_container.begin(); it != it_f;) {
					math::negate(it->m_cf);
					if (unlikely(!it->is_compatible(m_symbol_set) || it->is_ignorable(m_symbol_set))) {
						it = m_container.erase(it);
					} else {
						++it;
					}
				}
			} catch (...) {
				m_container.clear();
				throw;
			}
		}
		/// In-place multiplication.
		/**
		 * The multiplication algorithm proceeds as follows:
		 * 
		 * - if \p other is an instance of piranha::series with the same echelon size as <tt>this</tt>:
		 *   - if the symbol sets of \p this and \p other differ, they are merged using piranha::symbol_set::merge(),
		 *     and \p this and \p other are modified as necessary to be compatible with the merged set
		 *     (a copy of \p other might be created if it requires modifications);
		 *   - an instance of piranha::series_multiplier of \p Derived and \p T is created, its function call operator invoked,
		 *     and the result assigned back to \p this using piranha::series::operator=();
		 * - else:
		 *   - the coefficients of all terms of the series are multiplied in-place by \p other. If a
		 *     term is rendered ignorable or incompatible by the multiplication (e.g., multiplication by zero), it will be erased from the series.
		 * 
		 * If \p other is an instance of piranha::series with echelon size larger than the calling type, a compile-time error will be produced.
		 * 
		 * If any exception is thrown when multiplying by a non-series type, \p this will be left in a valid but unspecified state.
		 * 
		 * @param[in] other object by which the series will be multiplied.
		 * 
		 * @return reference to \p this, cast to type \p Derived.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the multiplication operators of the coefficient type(s),
		 * - piranha::hash_set::erase(),
		 * - the constructor and function call operator of piranha::series_multiplier,
		 * - memory allocation errors in standard containers,
		 * - insert(),
		 * - the <tt>merge_args()</tt> method of the key type,
		 * - the constructors of \p term_type, coefficient and key types,
		 * - piranha::symbol_set::merge().
		 * 
		 * @return reference to \p this, cast to type \p Derived.
		 */
		template <typename T>
		Derived &operator*=(T &&other)
		{
			dispatch_multiply(std::forward<T>(other));
			return *static_cast<Derived *>(this);
		}
		/// Evaluate series sparsity.
		/**
		 * Will call piranha::hash_set::evaluate_sparsity() on the internal terms container.
		 * 
		 * @return an \p std::tuple containing information about the sparsity of the internal container.
		 */
		sparsity_info_type evaluate_sparsity() const
		{
			return m_container.evaluate_sparsity();
		}
		/// Overload stream operator for piranha::series.
		/**
		 * Will direct to stream a human-readable representation of the series.
		 * 
		 * The human-readable representation of the series is built as follows:
		 * 
		 * - the coefficient and key of each term are printed adjacent to each other,
		 *   the former via the piranha::print_coefficient() function, the latter via its <tt>print()</tt> method;
		 * - terms are separated by a "+" sign.
		 * 
		 * The following additional transformations take place on the printed output:
		 * 
		 * - if the printed output of a coefficient is the string "1" and the printed output of its key
		 *   is not empty, the coefficient is not printed;
		 * - if the printed output of a coefficient is the string "-1" and the printed output of its key
		 *   is not empty, the printed output of the coefficient is transformed into "-";
		 * - the sequence of characters "+-" is transformed into "-";
		 * - if the total number of characters that would be printed exceeds the limit set in piranha::settings::get_max_char_output(),
		 *   the output is resized to that limit and ellipsis "..." are added at the end of the output.
		 * 
		 * The order in which terms are printed is determined by an instance of
		 * piranha::truncator of \p Derived constructed from \p this, in case the truncator
		 * is sorting and active. Otherwise, the print order will be undefined.
		 * 
		 * @param[in,out] os target stream.
		 * @param[in] s piranha::series argument.
		 * 
		 * @return reference to \p os.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::print_coefficient(),
		 * - the <tt>print()</tt> method of the key type,
		 * - memory allocation errors in standard containers.
		 */
		friend std::ostream &operator<<(std::ostream &os, const series &s)
		{
			// If the series is empty, print zero and exit.
			if (s.empty()) {
				os << "0";
				return os;
			}
			return print_helper_0(os,*static_cast<Derived const *>(&s));
		}
	private:
		template <typename Series>
		static std::ostream &print_helper_0(std::ostream &os, const Series &s, typename std::enable_if<
			truncator_traits<Series>::is_sorting>::type * = piranha_nullptr)
		{
			typedef typename Series::term_type term_type;
			truncator<Series> t(s);
			if (t.is_active()) {
				std::vector<term_type const *> v;
				v.reserve(s.size());
				std::transform(s.m_container.begin(),s.m_container.end(),
					std::back_insert_iterator<decltype(v)>(v),[](const term_type &t) {return &t;});
				std::sort(v.begin(),v.end(),[&t](const term_type *t1, const term_type *t2) {return t.compare_terms(*t1,*t2);});
				return print_helper_1(os,boost::indirect_iterator<decltype(v.begin())>(v.begin()),
					boost::indirect_iterator<decltype(v.end())>(v.end()),s.m_symbol_set);
			} else {
				return print_helper_1(os,s.m_container.begin(),s.m_container.end(),s.m_symbol_set);
			}
		}
		template <typename Series>
		static std::ostream &print_helper_0(std::ostream &os, const Series &s, typename std::enable_if<
			!truncator_traits<Series>::is_sorting>::type * = piranha_nullptr)
		{
			return print_helper_1(os,s.m_container.begin(),s.m_container.end(),s.m_symbol_set);
		}
		template <typename Iterator>
		static std::ostream &print_helper_1(std::ostream &os, Iterator start, Iterator end, const symbol_set &args)
		{
			piranha_assert(start != end);
			const auto limit = settings::get_max_char_output();
			integer count(0);
			std::ostringstream oss;
			for (auto it = start; it != end;) {
				std::ostringstream oss_cf;
				print_coefficient(oss_cf,it->m_cf);
				auto str_cf = oss_cf.str();
				std::ostringstream oss_key;
				it->m_key.print(oss_key,args);
				auto str_key = oss_key.str();
				if (str_cf == "1" && !str_key.empty()) {
					str_cf = "";
				} else if (str_cf == "-1" && !str_key.empty()) {
					str_cf = "-";
				}
				count += str_cf.size();
				oss << str_cf;
				if (count > limit) {
					break;
				}
				count += str_key.size();
				oss << str_key;
				if (count > limit) {
					break;
				}
				++it;
				if (it != end) {
					oss << "+";
				}
			}
			auto str = oss.str();
			if (str.size() > limit) {
				str.resize(limit);
				str += "...";
			}
			std::string::size_type index = 0u;
			while (true) {
				index = str.find("+-",index);
				if (index == std::string::npos) {
					break;
				}
				str.replace(index,2u,"-");
				++index;
			}
			os << str;
			return os;
		}
		// Merge all terms from another series. Works if s is this (in which case a copy is made). Basic exception safety guarantee.
		template <bool Sign, typename T>
		void merge_terms(T &&s,
			typename std::enable_if<std::is_base_of<series_tag,typename std::decay<T>::type>::value>::type * = piranha_nullptr)
		{
			merge_terms_impl0<Sign>(std::forward<T>(s));
		}
		// Merge arguments using new_ss as new symbol set.
		series merge_args(const symbol_set &new_ss) const
		{
			piranha_assert(new_ss.size() > m_symbol_set.size());
			piranha_assert(std::includes(new_ss.begin(),new_ss.end(),m_symbol_set.begin(),m_symbol_set.end()));
			typedef typename term_type::cf_type cf_type;
			typedef typename term_type::key_type key_type;
			series retval;
			retval.m_symbol_set = new_ss;
			for (auto it = m_container.begin(); it != m_container.end(); ++it) {
				cf_type new_cf(it->m_cf);
				key_type new_key(it->m_key.merge_args(m_symbol_set,new_ss));
				retval.insert(term_type(std::move(new_cf),std::move(new_key)));
			}
			return retval;
		}
		// Set of checks to be run on destruction in debug mode.
		bool destruction_checks() const
		{
			for (auto it = m_container.begin(); it != m_container.end(); ++it) {
				if (!it->is_compatible(m_symbol_set)) {
					std::cout << "Term not compatible.\n";
					return false;
				}
				if (it->is_ignorable(m_symbol_set)) {
					std::cout << "Term not ignorable.\n";
					return false;
				}
			}
			return true;
		}
	protected:
		/// Symbol set.
		symbol_set	m_symbol_set;
		/// Terms container.
		container_type	m_container;
};

namespace detail
{

template <typename Series>
struct math_is_zero_impl<Series,typename std::enable_if<
	std::is_base_of<series_tag,Series>::value>::type>
{
	static bool run(const Series &s)
	{
		return s.empty();
	}
};

template <typename Series>
struct math_negate_impl<Series,typename std::enable_if<
	std::is_base_of<series_tag,Series>::value>::type>
{
	static void run(Series &s)
	{
		s.negate();
	}
};

}

/// Specialisation of piranha::print_coefficient_impl for series.
/**
 * This specialisation is enabled if \p Series is an instance of piranha::series.
 */
template <typename Series>
struct print_coefficient_impl<Series,typename std::enable_if<
	std::is_base_of<detail::series_tag,Series>::value>::type>
{
	/// Call operator.
	/**
	 * Equivalent to the stream operator overload of piranha::series, apart from a couple
	 * of parentheses '()' enclosing the coefficient series if its size is larger than 1.
	 * 
	 * @param[in] os target stream.
	 * @param[in] s coefficient series to be printed.
	 * 
	 * @throws unspecified any exception thrown by the stream operator overload of piranha::series.
	 */
	void operator()(std::ostream &os, const Series &s) const
	{
		if (s.size() > 1u) {
			os << '(';
		}
		os << s;
		if (s.size() > 1u) {
			os << ')';
		}
	}
};

}

#endif
