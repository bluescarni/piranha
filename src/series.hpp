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
#include <boost/integer_traits.hpp>
#include <boost/iterator/indirect_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/math/special_functions/trunc.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <functional>
#include <memory>
#include <iostream>
#include <iterator>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base_term.hpp"
#include "config.hpp"
#include "debug_access.hpp"
#include "detail/sfinae_types.hpp"
#include "detail/series_fwd.hpp"
#include "echelon_size.hpp"
#include "environment.hpp"
#include "hash_set.hpp"
#include "math.hpp" // For negate() and math specialisations.
#include "mp_integer.hpp"
#include "print_coefficient.hpp"
#include "print_tex_coefficient.hpp"
#include "series_multiplier.hpp"
#include "series_binary_operators.hpp"
#include "settings.hpp"
#include "symbol_set.hpp"
#include "symbol.hpp"
#include "tracing.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

// Hash functor for term type in series.
template <typename Term>
struct term_hasher
{
	std::size_t operator()(const Term &term) const noexcept
	{
		return term.hash();
	}
};

// NOTE: this needs to go here, instead of in the series class as private method,
// because of a bug in GCC 4.7:
// http://gcc.gnu.org/bugzilla/show_bug.cgi?id=53137
template <typename Term, typename Derived>
inline std::pair<typename Term::cf_type,Derived> pair_from_term(const symbol_set &s, const Term &t)
{
	typedef typename Term::cf_type cf_type;
	Derived retval;
	retval.m_symbol_set = s;
	retval.insert(Term(cf_type(1),t.m_key));
	return std::make_pair(t.m_cf,std::move(retval));
}

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
 * - \p Term must satisfy piranha::is_term.
 * - \p Derived must derive from piranha::series of \p Term and \p Derived.
 * - \p Derived must satisfy piranha::is_container_element.
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
 * \todo review the handling of incompatible terms: it seems like in some places we are considering the possibility that operations on
 * coefficients could make the term incompatible, but this is not the case as in base_term only the key matters for compatibility. Propagating
 * this through the code would solve the issue of what to do if a term becomes incompatible during multiplication/merging/insertion etc. (now
 * it cannot become incompatible any more if it is already in series).
 */
template <typename Term, typename Derived>
class series: series_binary_operators, detail::series_tag
{
		PIRANHA_TT_CHECK(is_term,Term);
	public:
		/// Alias for term type.
		typedef Term term_type;
	private:
		// Make friend with all series.
		template <typename, typename>
		friend class series;
		// Make friend with debugging class.
		template <typename>
		friend class debug_access;
		// Make friend with series multiplier class.
		template <typename, typename, typename>
		friend class series_multiplier;
		// Make friend with series binary operators class.
		friend class series_binary_operators;
		// Partial need access to the custom derivatives.
		template <typename, typename>
		friend struct math::partial_impl;
		// NOTE: this friendship is related to the bug workaround above.
		template <typename Term2, typename Derived2>
		friend std::pair<typename Term2::cf_type,Derived2> detail::pair_from_term(const symbol_set &, const Term2 &);
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
			>::type * = nullptr)
		{
			dispatch_insertion<Sign>(term_type(typename term_type::cf_type(term.m_cf),
				typename term_type::key_type(term.m_key,m_symbol_set)));
		}
		// Overload for completely different term type: move-convert to term_type and proceed.
		template <bool Sign, typename T>
		void dispatch_insertion(T &&term, typename std::enable_if<
			!std::is_same<typename std::decay<T>::type,term_type>::value &&
			is_nonconst_rvalue_ref<T &&>::value
			>::type * = nullptr)
		{
			dispatch_insertion<Sign>(term_type(typename term_type::cf_type(std::move(term.m_cf)),
				typename term_type::key_type(std::move(term.m_key),m_symbol_set)));
		}
		// Overload for term_type.
		template <bool Sign, typename T>
		void dispatch_insertion(T &&term, typename std::enable_if<
			std::is_same<typename std::decay<T>::type,term_type>::value
			>::type * = nullptr)
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
			>::type * = nullptr)
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
			auto cleanup = [this](const typename container_type::const_iterator &it) {
				if (unlikely(!it->is_compatible(this->m_symbol_set) || it->is_ignorable(this->m_symbol_set))) {
					this->m_container.erase(it);
				}
			};
			if (it == m_container.end()) {
				if (unlikely(m_container.size() == boost::integer_traits<size_type>::const_max)) {
					piranha_throw(std::overflow_error,"maximum number of elements reached");
				}
				// Term is new. Handle the case in which we need to rehash because of load factor.
				if (unlikely(static_cast<double>(m_container.size() + size_type(1u)) / static_cast<double>(m_container.bucket_count()) >
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
			typename std::enable_if<std::is_base_of<series<Term,Derived>,typename std::decay<T>::type>::value>::type * = nullptr)
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
			typename std::enable_if<!std::is_base_of<series<Term,Derived>,typename std::decay<T>::type>::value>::type * = nullptr)
		{
			// No worries about same object, just forward.
			merge_terms_impl1<Sign>(std::forward<T>(s));
		}
		// Overload if we cannot move objects from series.
		template <bool Sign, typename T>
		void merge_terms_impl1(T &&s,
			typename std::enable_if<!is_nonconst_rvalue_ref<T &&>::value>::type * = nullptr)
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
				max_n_buckets = boost::numeric_cast<size_type>(boost::math::trunc(static_cast<double>(max_size) / c1.max_load_factor()));
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
			typename std::enable_if<is_nonconst_rvalue_ref<T &&>::value>::type * = nullptr)
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
		// TMP for the generic constructor.
		// NOTE: this logic is a slight repetition of the helper methods below. However, since the enabling
		// conditions below are already quite complicated and split in lvalue/rvalue,
		// it might be better to leave this as is.
		template <typename T, typename = void>
		struct generic_ctor_enabler
		{
			static const bool value = std::is_constructible<typename term_type::cf_type,T>::value;
		};
		template <typename T>
		struct generic_ctor_enabler<T,typename std::enable_if<
			is_instance_of<typename std::decay<T>::type,piranha::series>::value &&
			(echelon_size<term_type>::value == echelon_size<typename std::decay<T>::type::term_type>::value)
			>::type>
		{
			typedef typename std::decay<T>::type::term_type other_term_type;
			typedef typename other_term_type::cf_type other_cf_type;
			typedef typename other_term_type::key_type other_key_type;
			// NOTE: this essentially is an is_term_insertable check. Keep it in mind if we implement
			// enable/disable of insertion method in the future.
			static const bool value = std::is_same<term_type,other_term_type>::value ||
				(std::is_constructible<typename term_type::cf_type,other_cf_type>:: value &&
				std::is_constructible<typename term_type::key_type,other_key_type,symbol_set>:: value);
		};
		template <typename T>
		struct generic_ctor_enabler<T,typename std::enable_if<
			is_instance_of<typename std::decay<T>::type,piranha::series>::value &&
			(echelon_size<term_type>::value < echelon_size<typename std::decay<T>::type::term_type>::value)
			>::type>
		{
			static const bool value = false;
		};
		// Series with same echelon size, same term type, move.
		template <typename Series>
		void dispatch_generic_construction(Series &&s,
			typename std::enable_if<is_instance_of<typename std::decay<Series>::type,piranha::series>::value &&
			echelon_size<term_type>::value == echelon_size<typename std::decay<Series>::type::term_type>::value &&
			std::is_same<term_type,typename std::decay<Series>::type::term_type>::value &&
			is_nonconst_rvalue_ref<Series &&>::value
			>::type * = nullptr)
		{
			static_assert(!std::is_same<series,typename std::decay<Series>::type>::value,"Invalid series type for generic construction.");
			m_symbol_set = std::move(s.m_symbol_set);
			m_container = std::move(s.m_container);
		}
		// Series with same echelon size, same term type, copy.
		template <typename Series>
		void dispatch_generic_construction(Series &&s,
			typename std::enable_if<is_instance_of<typename std::decay<Series>::type,piranha::series>::value &&
			echelon_size<term_type>::value == echelon_size<typename std::decay<Series>::type::term_type>::value &&
			std::is_same<term_type,typename std::decay<Series>::type::term_type>::value &&
			!is_nonconst_rvalue_ref<Series &&>::value
			>::type * = nullptr)
		{
			static_assert(!std::is_same<series,typename std::decay<Series>::type>::value,"Invalid series type for generic construction.");
			m_symbol_set = s.m_symbol_set;
			m_container = s.m_container;
		}
		// Series with same echelon size and different term type, move.
		template <typename Series>
		void dispatch_generic_construction(Series &&s,
			typename std::enable_if<is_instance_of<typename std::decay<Series>::type,piranha::series>::value &&
			echelon_size<term_type>::value == echelon_size<typename std::decay<Series>::type::term_type>::value &&
			!std::is_same<term_type,typename std::decay<Series>::type::term_type>::value &&
			is_nonconst_rvalue_ref<Series &&>::value
			>::type * = nullptr)
		{
			m_symbol_set = std::move(s.m_symbol_set);
			merge_terms<true>(std::forward<Series>(s));
		}
		// Series with same echelon size and different term type, copy.
		template <typename Series>
		void dispatch_generic_construction(Series &&s,
			typename std::enable_if<is_instance_of<typename std::decay<Series>::type,piranha::series>::value &&
			echelon_size<term_type>::value == echelon_size<typename std::decay<Series>::type::term_type>::value &&
			!std::is_same<term_type,typename std::decay<Series>::type::term_type>::value &&
			!is_nonconst_rvalue_ref<Series &&>::value
			>::type * = nullptr)
		{
			m_symbol_set = s.m_symbol_set;
			merge_terms<true>(s);
		}
		// Series with smaller echelon size.
		template <typename Series>
		void dispatch_generic_construction(Series &&s,
			typename std::enable_if<is_instance_of<typename std::decay<Series>::type,piranha::series>::value &&
			echelon_size<typename std::decay<Series>::type::term_type>::value < echelon_size<term_type>::value
			>::type * = nullptr)
		{
			dispatch_generic_construction_from_cf(std::forward<Series>(s));
		}
		// Non-series.
		template <typename T>
		void dispatch_generic_construction(T &&x,
			typename std::enable_if<!is_instance_of<typename std::decay<T>::type,piranha::series>::value>::type * = nullptr)
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
		void dispatch_in_place_add(T &&x, typename std::enable_if<!is_instance_of<typename std::decay<T>::type,piranha::series>::value>::type * = nullptr)
		{
			term_type tmp(typename term_type::cf_type(std::forward<T>(x)),typename term_type::key_type(m_symbol_set));
			insert<Sign>(std::move(tmp));
		}
		// Overload for series type with smaller echelon size.
		template <bool Sign, typename T>
		void dispatch_in_place_add(T &&series, typename std::enable_if<is_instance_of<typename std::decay<T>::type,piranha::series>::value &&
			echelon_size<typename std::decay<T>::type::term_type>::value < echelon_size<term_type>::value>::type * = nullptr)
		{
			term_type tmp(typename term_type::cf_type(std::forward<T>(series)),typename term_type::key_type(m_symbol_set));
			insert<Sign>(std::move(tmp));
		}
		// Overload for series with same echelon size.
		template <bool Sign, typename T>
		void dispatch_in_place_add(T &&other, typename std::enable_if<is_instance_of<typename std::decay<T>::type,piranha::series>::value &&
			echelon_size<typename std::decay<T>::type::term_type>::value == echelon_size<term_type>::value>::type * = nullptr)
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
		void dispatch_multiply(T &&x, typename std::enable_if<!is_instance_of<typename std::decay<T>::type,piranha::series>::value>::type * = nullptr)
		{
			mixed_multiply(std::forward<T>(x));
		}
		template <typename T>
		void dispatch_multiply(T &&x, typename std::enable_if<
			is_instance_of<typename std::decay<T>::type,piranha::series>::value &&
			echelon_size<typename std::decay<T>::type::term_type>::value < echelon_size<term_type>::value
			>::type * = nullptr)
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
			tracing::trace("number_of_series_multiplications",[&retval](boost::any &x) {
				if (unlikely(x.empty())) {
					x = 0ull;
				}
				auto ptr = boost::any_cast<unsigned long long>(&x);
				if (likely((bool)ptr && retval.size())) {
					++*ptr;
				}
			});
			tracing::trace("accumulated_sparsity",[this,&series,&retval](boost::any &x) {
				if (unlikely(x.empty())) {
					x = 0.;
				}
				auto ptr = boost::any_cast<double>(&x);
				if (likely((bool)ptr && retval.size())) {
					*ptr += (static_cast<double>(this->size()) * static_cast<double>(series.size())) / static_cast<double>(retval.size());
				}
			});
			return retval;
		}
		template <typename T>
		void dispatch_multiply(T &&other, typename std::enable_if<
			is_instance_of<typename std::decay<T>::type,piranha::series>::value &&
			echelon_size<typename std::decay<T>::type::term_type>::value == echelon_size<term_type>::value
			>::type * = nullptr)
		{
			// NOTE: all this dancing around with base and derived types for series is necessary as the mechanism of
			// specialization of series_multiplier depends on the derived types - which must then be preserved and
			// not casted away to the base types.
			// Base types of multiplicand series.
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
		// In-place divide
		// =================
		template <typename T>
		void in_place_divide(T &&x)
		{
			const auto it_f = m_container.end();
			try {
				for (auto it = m_container.begin(); it != it_f;) {
					it->m_cf /= x;
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
		// Exponentiation.
		template <typename T>
		Derived pow_impl(const T &x) const
		{
			integer n;
			try {
				n = math::integral_cast(x);
			} catch (const std::invalid_argument &) {
				piranha_throw(std::invalid_argument,"invalid argument for series exponentiation: non-integral value");
			}
			if (n.sign() < 0) {
				piranha_throw(std::invalid_argument,"invalid argument for series exponentiation: negative integral value");
			}
			// NOTE: for series it seems like it is better to run the dumb algorithm instead of, e.g.,
			// exponentiation by squaring - the growth in number of terms seems to be slower.
			Derived retval(*static_cast<Derived const *>(this));
			for (integer i(1); i < n; ++i) {
				retval *= *static_cast<Derived const *>(this);
			}
			return retval;
		}
		// Iterator utilities.
		typedef boost::transform_iterator<std::function<std::pair<typename term_type::cf_type,Derived>(const term_type &)>,
			typename container_type::const_iterator> const_iterator_impl;
		// Evaluation utilities.
		// NOTE: here we need the Series template because otherwise, in case of missing eval on key or coefficient, the decltype
		// will fail as series is not a template parameter.
		template <typename Series, typename U, typename = void>
		struct eval_type {};
		template <typename Series, typename U>
		using e_type = decltype(math::evaluate(std::declval<typename Series::term_type::cf_type const &>(),std::declval<std::unordered_map<std::string,U> const &>()) *
			std::declval<const typename Series::term_type::key_type &>().evaluate(std::declval<const std::unordered_map<symbol,U> &>(),
			std::declval<const symbol_set &>()));
		template <typename Series, typename U>
		struct eval_type<Series,U,typename std::enable_if<has_multiply_accumulate<e_type<Series,U>,
			decltype(math::evaluate(std::declval<typename Series::term_type::cf_type const &>(),std::declval<std::unordered_map<std::string,U> const &>())),
			decltype(std::declval<const typename Series::term_type::key_type &>().evaluate(std::declval<const std::unordered_map<symbol,U> &>(),std::declval<const symbol_set &>()))>::value &&
			std::is_constructible<e_type<Series,U>,int>::value
			>::type>
		{
			using type = e_type<Series,U>;
		};
		// Print utilities.
		template <bool TexMode, typename Iterator>
		static std::ostream &print_helper_1(std::ostream &os, Iterator start, Iterator end, const symbol_set &args)
		{
			piranha_assert(start != end);
			const auto limit = settings::get_max_term_output();
			size_type count = 0u;
			std::ostringstream oss;
			auto it = start;
			for (; it != end;) {
				if (limit && count == limit) {
					break;
				}
				std::ostringstream oss_cf;
				if (TexMode) {
					print_tex_coefficient(oss_cf,it->m_cf);
				} else {
					print_coefficient(oss_cf,it->m_cf);
				}
				auto str_cf = oss_cf.str();
				std::ostringstream oss_key;
				if (TexMode) {
					it->m_key.print_tex(oss_key,args);
				} else {
					it->m_key.print(oss_key,args);
				}
				auto str_key = oss_key.str();
				if (str_cf == "1" && !str_key.empty()) {
					str_cf = "";
				} else if (str_cf == "-1" && !str_key.empty()) {
					str_cf = "-";
				}
				oss << str_cf;
				if (str_cf != "" && str_cf != "-" && !str_key.empty() && !TexMode) {
					oss << "*";
				}
				oss << str_key;
				++it;
				if (it != end) {
					oss << "+";
				}
				++count;
			}
			auto str = oss.str();
			// If we reached the limit without printing all terms in the series, print the ellipsis.
			if (limit && count == limit && it != end) {
				if (TexMode) {
					str += "\\ldots";
				} else {
					str += "...";
				}
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
			typename std::enable_if<is_instance_of<typename std::decay<T>::type,piranha::series>::value>::type * = nullptr)
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
			// Run destruction checks only if we are not in shutdown.
			if (environment::shutdown()) {
				return true;
			}
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
		template <typename T>
		static T trim_cf_impl(const T &s, typename std::enable_if<
			is_instance_of<T,piranha::series>::value>::type * = nullptr)
		{
			return s.trim();
		}
		template <typename T>
		static const T &trim_cf_impl(const T &s, typename std::enable_if<
			!is_instance_of<T,piranha::series>::value>::type * = nullptr)
		{
			return s;
		}
		// Typedef for pow().
		template <typename T, typename U>
		using pow_ret_type = typename std::enable_if<std::is_same<decltype(math::pow(std::declval<typename term_type::cf_type const &>(),std::declval<T const &>())),
			typename term_type::cf_type>::value && has_is_zero<T>::value && has_integral_cast<T>::value && is_multipliable_in_place<U>::value,U
			>::type;
	public:
		/// Size type.
		/**
		 * Used to represent the number of terms in the series. Equivalent to piranha::hash_set::size_type.
		 */
		typedef typename container_type::size_type size_type;
		/// Const iterator.
		/**
		 * Iterator type that can be used to iterate over the terms of the series.
		 * The object returned upon dereferentiation is an \p std::pair in which the first element
		 * is a copy of the coefficient of the term, the second element a single-term instance of \p Derived constructed from
		 * the term's key and a unitary coefficient.
		 *
		 * This iterator is an input iterator which additionally offers the multi-pass guarantee.
		 * 
		 * @see piranha::series::begin() and piranha::series::end().
		 */
		typedef const_iterator_impl const_iterator;
		/// Defaulted default constructor.
		series() = default;
		/// Defaulted copy constructor.
		/**
		 * @throws unspecified any exception thrown by the copy constructor of piranha::hash_set.
		 */
		series(const series &) = default;
		/// Defaulted move constructor.
		series(series &&) = default;
		/// Generic constructor.
		/**
		 * \note
		 * This constructor is enabled only if the decayed type of \p T is different from piranha::series and
		 * the algorithm outlined below is supported by the involved types.
		 *
		 * The generic construction algorithm works as follows:
		 * - if \p T is an instance of piranha::series with the same echelon size as the calling type:
		 *   - if the term type of \p T is the same as that of <tt>this</tt>:
		 *     - the internal terms container and symbol set of \p x are forwarded to construct \p this;
		 *   - else:
		 *     - the symbol set of \p x is forwarded to construct the symbol set of this and all terms from \p x are inserted into \p this
		 *       (after conversion to \p term_type via binary constructor from coefficient and key);
		 * - else:
		 *   - \p x is used to construct a new term as follows:
		 *     - \p x is forwarded to construct a coefficient;
		 *     - an empty arguments set will be used to construct a key;
		 *     - coefficient and key are used to construct the new term instance;
		 *   - the new term is inserted into \p this.
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
		template <typename T, typename std::enable_if<!std::is_same<series,typename std::decay<T>::type>::value &&
			generic_ctor_enabler<T>::value,int>::type = 0>
		explicit series(T &&x)
		{
			dispatch_generic_construction(std::forward<T>(x));
		}
		/// Trivial destructor.
		~series()
		{
			PIRANHA_TT_CHECK(std::is_base_of,series,Derived);
			PIRANHA_TT_CHECK(is_container_element,Derived);
			// Static checks on the iterator types.
			PIRANHA_TT_CHECK(is_input_iterator,const_iterator);
			piranha_assert(destruction_checks());
		}
		/// Copy-assignment operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the copy constructor.
		 */
		series &operator=(const series &other)
		{
			if (likely(this != &other)) {
				series tmp(other);
				*this = std::move(tmp);
			}
			return *this;
		}
		/// Defaulted move assignment operator.
		series &operator=(series &&) = default;
		/// Generic assignment operator.
		/**
		 * This template constructor is enabled only if \p T is different from the type of \p this,
		 * and \p x can be used to construct piranha::series.
		 * That is, generic assignment is equivalent to assignment to a piranha::series constructed
		 * via the generic constructor.
		 * 
		 * @param[in] x assignment argument.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the generic constructor.
		 */
		template <typename T>
		typename std::enable_if<!std::is_same<series,typename std::decay<T>::type>::value &&
			std::is_constructible<series,T>::value,
			series &>::type operator=(T &&x)
		{
			return operator=(series(std::forward<T>(x)));
		}
		/// Symbol set getter.
		/**
		 * @return const reference to the piranha::symbol_set associated to the series.
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
		/// Test for single-coefficient series.
		/**
		 * A series is considered to be <em>single-coefficient</em> when it is symbolically equivalent to a coefficient.
		 * That is, the series is either empty (in which case it is considered to be equivalent to a coefficient constructed
		 * from zero) or consisting of a single term with unitary key (in which case the series is considered equivalent to
		 * its only coefficient).
		 * 
		 * @return \p true in case of single-coefficient series, \p false otherwise.
		 * 
		 * @throws unspecified any exception thrown by the <tt>is_unitary()</tt> method of the key type.
		 */
		bool is_single_coefficient() const
		{
			return (empty() || (size() == 1u && m_container.begin()->m_key.is_unitary(m_symbol_set)));
		}
		/// Insert generic term.
		/**
		 * This method will insert \p term into the series using internally piranha::hash_set::insert. The method is enabled only
		 * if \p term derives from piranha::base_term.
		 * 
		 * The insertion algorithm proceeds as follows:
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
		typename std::enable_if<is_instance_of<typename std::decay<T>::type,base_term>::value,void>::type insert(T &&term)
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
		/// In-place addition.
		/**
		 * The addition algorithm proceeds as follows:
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
		 * 
		 * @throws unspecified any exception thrown by the copy constructor.
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
		 */
		template <typename T>
		Derived &operator*=(T &&other)
		{
			dispatch_multiply(std::forward<T>(other));
			return *static_cast<Derived *>(this);
		}
		/// In-place division.
		/**
		 * This template operator is activated only if \p T is not an instance of piranha::series.
		 * The coefficients of all terms of the series are divided in-place by \p other. If a
		 * term is rendered ignorable or incompatible by the division (e.g., division by infinity), it will be erased from the series.
		 * 
		 * If any exception is thrown, \p this will be left in a valid but unspecified state.
		 * 
		 * @param[in] other object by which the series will be divided.
		 * 
		 * @return reference to \p this, cast to type \p Derived.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the division operators of the coefficient type(s),
		 * - piranha::hash_set::erase().
		 */
		template <typename T>
		typename std::enable_if<
			!is_instance_of<typename std::decay<T>::type,piranha::series>::value,Derived &>::type operator/=(T &&other)
		{
			in_place_divide(std::forward<T>(other));
			return *static_cast<Derived *>(this);
		}
		/** @name Table-querying methods
		 * Methods to query the properties of the internal container used to store the terms.
		 */
		//@{
		/// Table sparsity.
		/**
		 * Will call piranha::hash_set::evaluate_sparsity() on the internal terms container
		 * and return the result.
		 * 
		 * @return the output of piranha::hash_set::evaluate_sparsity().
		 * 
		 * @throws unspecified any exception thrown by piranha::hash_set::evaluate_sparsity().
		 */
		sparsity_info_type table_sparsity() const
		{
			return m_container.evaluate_sparsity();
		}
		/// Table load factor.
		/**
		 * Will call piranha::hash_set::load_factor() on the internal terms container
		 * and return the result.
		 * 
		 * @return the load factor of the internal container.
		 */
		double table_load_factor() const
		{
			return m_container.load_factor();
		}
		/// Table bucket count.
		/**
		 * @return the bucket count of the internal container.
		 */
		size_type table_bucket_count() const
		{
			return m_container.bucket_count();
		}
		//@}
		/// Exponentiation.
		/**
		 * \note
		 * This method is enabled only if:
		 * - the coefficient type is exponentiable to the power of \p x, and the return type is the coefficient type itself,
		 * - \p T can be used as argument for piranha::math::is_zero() and piranha::math::integral_cast(),
		 * - \p Derived is multipliable in place.
		 *
		 * Return \p this raised to the <tt>x</tt>-th power.
		 * 
		 * The exponentiation algorithm proceeds as follows:
		 * - if the series is single-coefficient, a call to apply_cf_functor() is attempted, using a functor that calls piranha::math::pow() on
		 *   the coefficient. Otherwise, the algorithm proceeds;
		 * - if \p x is zero (as established by piranha::math::is_zero()), a series with a single term
		 *   with unitary key and coefficient constructed from the integer numeral "1" is returned (i.e., any series raised to the power of zero
		 *   is 1 - including empty series);
		 * - if \p x represents a non-negative integral value, the return value is constructed via repeated multiplications;
		 * - otherwise, an exception will be raised.
		 * 
		 * @param[in] x exponent.
		 * 
		 * @return \p this raised to the power of \p x.
		 * 
		 * @throws std::invalid_argument if exponentiation is computed via repeated series multiplications and
		 * \p x does not represent a non-negative integer.
		 * @throws unspecified any exception thrown by:
		 * - series, term, coefficient and key construction,
		 * - insert(),
		 * - is_single_coefficient(),
		 * - apply_cf_functor(),
		 * - piranha::math::pow(), piranha::math::is_zero() and piranha::math::integral_cast(),
		 * - series multiplication.
		 */
		template <typename T, typename U = Derived>
		pow_ret_type<T,U> pow(const T &x) const
		{
			typedef typename term_type::cf_type cf_type;
			typedef typename term_type::key_type key_type;
			if (is_single_coefficient()) {
				auto pow_functor = [&x](const cf_type &cf) {return math::pow(cf,x);};
				return apply_cf_functor(pow_functor);
			}
			if (math::is_zero(x)) {
				Derived retval;
				retval.insert(term_type(cf_type(1),key_type(symbol_set{})));
				return retval;
			}
			return pow_impl(x);
		}
		/// Apply functor to single-coefficient series.
		/**
		 * This method can be called successfully only on single-coefficient series.
		 * 
		 * If the series is empty, the return value will be a series with single term and unitary key in which the coefficient
		 * is the result of applying the functor \p f on a coefficient instance constructed from the integral constant "0".
		 * 
		 * If the series has a single term with unitary key, the return value will be a series with single term and unitary key in which the coefficient
		 * is the result of applying the functor \p f to the only coefficient of \p this.
		 * 
		 * @param[in] f coefficient functor.
		 * 
		 * @return a series constructed via the application of \p f to a coefficient instance as described above.
		 * 
		 * @throws std::invalid_argument if the series is not single-coefficient.
		 * @throws unspecified any exception thrown by:
		 * - the call operator of \p f,
		 * - construction of coefficient, key and term instances,
		 * - insert(),
		 * - is_single_coefficient().
		 */
		Derived apply_cf_functor(std::function<typename term_type::cf_type (const typename term_type::cf_type &)> f) const
		{
			if (!is_single_coefficient()) {
				piranha_throw(std::invalid_argument,"cannot apply functor, series is not single-coefficient");
			}
			typedef typename term_type::cf_type cf_type;
			typedef typename term_type::key_type key_type;
			Derived retval;
			if (empty()) {
				retval.insert(term_type(f(cf_type(0)),key_type(symbol_set{})));
			} else {
				retval.insert(term_type(f(m_container.begin()->m_cf),key_type(symbol_set{})));
			}
			return retval;
		}
		/// Partial derivative.
		/**
		 * \note
		 * This method is enabled only if the term type satisfies piranha::term_is_differentiable.
		 * 
		 * Will return the partial derivative of \p this with respect to the argument called \p name. The method will construct
		 * the return value series from the output of the term's differentiation method. Note that, contrary to the specialisation
		 * of piranha::math::partial() for series types, this method will not take into account custom derivatives registered
		 * via register_custom_derivative().
		 * 
		 * @param[in] name name of the argument with respect to which the derivative will be calculated.
		 * 
		 * @return partial derivative of \p this with respect to the symbol named \p name.
		 * 
		 * @throws unspecified any exception thrown by the differentiation method of the term type or by insert().
		 */
		template <typename T = term_type, typename = typename std::enable_if<term_is_differentiable<T>::value>::type>
		Derived partial(const std::string &name) const
		{
			Derived retval;
			retval.m_symbol_set = this->m_symbol_set;
			const auto it_f = this->m_container.end();
			const symbol s(name);
			for (auto it = this->m_container.begin(); it != it_f; ++it) {
				auto tmp_partial = it->partial(s,this->m_symbol_set);
				const auto size = tmp_partial.size();
				for (decltype(tmp_partial.size()) i = 0u; i < size; ++i) {
					retval.insert(std::move(tmp_partial[i]));
				}
			}
			return retval;
		}
		/// Register custom partial derivative.
		/**
		 * Register a copy of \p func associated to the symbol called \p name for use by piranha::math::partial().
		 * \p func will be used to compute the partial derivative of instances of type \p Derived with respect to
		 * \p name in place of the default partial differentiation algorithm.
		 * 
		 * It is safe to call this method from multiple threads.
		 * 
		 * @param[in] name symbol for which the custom partial derivative function will be registered.
		 * @param[in] func custom partial derivative function.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - failure(s) in threading primitives,
		 * - lookup and insertion operations on \p std::unordered_map,
		 * - copy-assignment of \p func.
		 */
		static void register_custom_derivative(const std::string &name, std::function<Derived(const Derived &)> func)
		{
			std::lock_guard<std::mutex> lock(cp_mutex);
			(*cp_map)[name] = func;
		}
		/// Unregister custom partial derivative.
		/**
		 * Unregister the custom partial derivative function associated to the symbol called \p name. If no custom
		 * partial derivative was previously registered using register_custom_derivative(), calling this function will be a no-op.
		 * 
		 * It is safe to call this method from multiple threads.
		 * 
		 * @param[in] name symbol for which the custom partial derivative function will be unregistered.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - failure(s) in threading primitives,
		 * - lookup and erase operations on \p std::unordered_map.
		 */
		static void unregister_custom_derivative(const std::string &name)
		{
			std::lock_guard<std::mutex> lock(cp_mutex);
			auto it = cp_map->find(name);
			if (it != cp_map->end()) {
				cp_map->erase(it);
			}
		}
		/// Unregister all custom partial derivatives.
		/**
		 * Will unregister all custom derivatives currently registered via register_custom_derivative().
		 * It is safe to call this method from multiple threads.
		 * 
		 * @throws unspecified any exception thrown by failure(s) in threading primitives.
		 */
		static void unregister_all_custom_derivatives()
		{
			std::lock_guard<std::mutex> lock(cp_mutex);
			cp_map->clear();
		}
		/// Begin iterator.
		/**
		 * Return an iterator to the first term of the series. The returned iterator will
		 * provide, when dereferenced, an \p std::pair in which the first element is a copy of the coefficient of
		 * the term, whereas the second element is a single-term instance of \p Derived built from the term's key
		 * and a unitary coefficient.
		 * 
		 * Note that terms are stored unordered in the series, hence it is not defined which particular
		 * term will be returned by calling this method. The only guarantee is that the iterator can be used to transverse
		 * all the series' terms until piranha::series::end() is eventually reached.
		 * 
		 * Calling any non-const method on the series will invalidate the iterators obtained via piranha::series::begin() and piranha::series::end().
		 * 
		 * @return an iterator to the first term of the series.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - construction and assignment of piranha::symbol_set,
		 * - insert(),
		 * - construction of term, coefficient and key instances.
		 */
		const_iterator begin() const
		{
			typedef std::function<std::pair<typename term_type::cf_type,Derived>(const term_type &)>
				func_type;
			auto func = [this](const term_type &t) {
				return detail::pair_from_term<term_type,Derived>(this->m_symbol_set,t);
			};
			return boost::make_transform_iterator(m_container.begin(),func_type(std::move(func)));
		}
		/// End iterator.
		/**
		 * Return an iterator one past the last term of the series. See the documentation of piranha::series::begin()
		 * on how the returned iterator can be used.
		 * 
		 * @return an iterator to the end of the series.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - construction and assignment of piranha::symbol_set,
		 * - insert(),
		 * - construction of term, coefficient and key instances.
		 */
		const_iterator end() const
		{
			typedef std::function<std::pair<typename term_type::cf_type,Derived>(const term_type &)>
				func_type;
			auto func = [this](const term_type &t) {
				return detail::pair_from_term<term_type,Derived>(this->m_symbol_set,t);
			};
			return boost::make_transform_iterator(m_container.end(),func_type(std::move(func)));
		}
		/// Term filtering.
		/**
		 * This method will apply the functor \p func to each term in the series, and produce a return series
		 * containing all terms in \p this for which \p func returns \p true.
		 * Terms are passed to \p func in the format resulting from dereferencing the iterators obtained
		 * via piranha::series::begin().
		 * 
		 * @param[in] func filtering functor.
		 * 
		 * @return filtered series.
		 * 
		 * @throw unspecified any exception thrown by:
		 * - the call operator of \p func,
		 * - insert(),
		 * - the assignment operator of piranha::symbol_set,
		 * - term, coefficient, key construction.
		 */
		Derived filter(std::function<bool(const std::pair<typename term_type::cf_type,Derived> &)> func) const
		{
			Derived retval;
			retval.m_symbol_set = m_symbol_set;
			const auto it_f = this->m_container.end();
			for (auto it = this->m_container.begin(); it != it_f; ++it) {
				if (func(detail::pair_from_term<term_type,Derived>(m_symbol_set,*it))) {
					retval.insert(*it);
				}
			}
			return retval;
		}
		/// Term transformation.
		/**
		 * This method will apply the functor \p func to each term in the series, and will use the return
		 * value of the functor to construct a new series. Terms are passed to \p func in the same format
		 * resulting from dereferencing the iterators obtained via piranha::series::begin(), and \p func is expected to produce
		 * a return value of the same type.
		 * 
		 * The return series is first initialised as an empty series. For each input term \p t, the return value
		 * of \p func is used to construct a new temporary series from the multiplication of \p t.first and
		 * \p t.second. Each temporary series is then added to the return value series.
		 * 
		 * This method requires the coefficient type to be multipliable by \p Derived.
		 * 
		 * @param[in] func transforming functor.
		 * 
		 * @return transformed series.
		 *
		 * @throw unspecified any exception thrown by:
		 * - the call operator of \p func,
		 * - insert(),
		 * - the assignment operator of piranha::symbol_set,
		 * - term, coefficient, key construction,
		 * - series multiplication and addition.
		 * 
		 * \todo require multipliability of cf * Derived and addability of the result to Derived in place.
		 */
		Derived transform(std::function<std::pair<typename term_type::cf_type,Derived>
			(const std::pair<typename term_type::cf_type,Derived> &)> func) const
		{
			Derived retval;
			std::pair<typename term_type::cf_type,Derived> tmp;
			const auto it_f = this->m_container.end();
			for (auto it = this->m_container.begin(); it != it_f; ++it) {
				tmp = func(detail::pair_from_term<term_type,Derived>(m_symbol_set,*it));
				// NOTE: here we could use multadd, but it seems like there's not
				// much benefit (plus, the types involved are different).
				retval += tmp.first * tmp.second;
			}
			return retval;
		}
		/// Evaluation.
		/**
		 * \note
		 * This method is enabled only if:
		 * - both the coefficient and the key types are evaluable,
		 * - the evaluated types are suitable for use in piranha::math::multiply_accumulate(),
		 * - the return type is constructible from \p int.
		 *
		 * Series evaluation starts with a zero-initialised instance of the return type, which is determined
		 * according to the evaluation types of coefficient and key. The return value accumulates the evaluation
		 * of all terms in the series via the product of the evaluations of the coefficient-key pairs in each term.
		 * The input dictionary \p dict specifies with which value each symbolic quantity will be evaluated.
		 * 
		 * @param[in] dict dictionary of that will be used for evaluation.
		 * 
		 * @return evaluation of the series according to the evaluation dictionary \p dict.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - coefficient and key evaluation,
		 * - insertion operations on \p std::unordered_map,
		 * - piranha::math::multiply_accumulate().
		 */
		template <typename T, typename Series = series>
		typename eval_type<Series,T>::type evaluate(const std::unordered_map<std::string,T> &dict) const
		{
			// NOTE: possible improvement: if the evaluation type is less-than comparable,
			// build a vector of evaluated terms, sort it and accumulate (to minimise accuracy loss
			// with fp types and maybe improve performance - e.g., for integers).
			typedef typename eval_type<Series,T>::type return_type;
			// Transform the string dict into symbol dict for use in keys.
			std::unordered_map<symbol,T> s_dict;
			for (auto it = dict.begin(); it != dict.end(); ++it) {
				s_dict[symbol(it->first)] = it->second;
			}
			return_type retval = return_type(0);
			const auto it_f = this->m_container.end();
			for (auto it = this->m_container.begin(); it != it_f; ++it) {
				math::multiply_accumulate(retval,math::evaluate(it->m_cf,dict),it->m_key.evaluate(s_dict,m_symbol_set));
			}
			return retval;
		}
		/// Trim.
		/**
		 * This method will return a series mathematically equivalent to \p this in which discardable arguments
		 * have been removed from the internal set of symbols. Which symbols are removed depends on the trimming
		 * method \p trim_identify() of the key type (e.g., in a polynomial a symbol can be discarded if its exponent
		 * is zero in all monomials).
		 * 
		 * If the coefficient type is an instance of piranha::series, trim() will be called recursively on the coefficients
		 * while building the return value.
		 * 
		 * @return trimmed version of \p this.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - operations on piranha::symbol_set,
		 * - the trimming methods of coefficient and/or key,
		 * - insert(),
		 * - term, coefficient and key type construction.
		 */
		Derived trim() const
		{
			// Build the set of symbols that can be removed.
			const auto it_f = this->m_container.end();
			symbol_set trim_ss(m_symbol_set);
			for (auto it = this->m_container.begin(); it != it_f; ++it) {
				it->m_key.trim_identify(trim_ss,m_symbol_set);
			}
			// Determine the new set.
			Derived retval;
			retval.m_symbol_set = m_symbol_set.diff(trim_ss);
			for (auto it = this->m_container.begin(); it != it_f; ++it) {
				retval.insert(term_type(trim_cf_impl(it->m_cf),it->m_key.trim(trim_ss,m_symbol_set)));
			}
			return retval;
		}
		/// Print in TeX mode.
		/**
		 * Print series to stream \p os in TeX mode. The representation is constructed in the same way as explained in
		 * piranha::series::operator<<(), but using piranha::print_tex_coefficient() and the key's TeX printing method instead of the plain
		 * printing functions.
		 * 
		 * @param os target stream.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::print_tex_coefficient(),
		 * - the TeX printing method of the key type,
		 * - memory allocation errors in standard containers,
		 * - piranha::settings::get_max_term_output(),
		 * - streaming to \p os or to instances of \p std::ostringstream.
		 * 
		 * @see operator<<().
		 */
		void print_tex(std::ostream &os) const
		{
			// If the series is empty, print zero and exit.
			if (empty()) {
				os << "0";
				return;
			}
			print_helper_1<true>(os,m_container.begin(),m_container.end(),m_symbol_set);
		}
		/// Overloaded stream operator for piranha::series.
		/**
		 * Will direct to stream a human-readable representation of the series.
		 * 
		 * The human-readable representation of the series is built as follows:
		 * 
		 * - the coefficient and key of each term are printed adjacent to each other separated by the character "*",
		 *   the former via the piranha::print_coefficient() function, the latter via its <tt>print()</tt> method;
		 * - terms are separated by a "+" sign.
		 * 
		 * The following additional transformations take place on the printed output:
		 * 
		 * - if the printed output of a coefficient is the string "1" and the printed output of its key
		 *   is not empty, the coefficient and the "*" sign are not printed;
		 * - if the printed output of a coefficient is the string "-1" and the printed output of its key
		 *   is not empty, the printed output of the coefficient is transformed into "-" and the sign "*" is not printed;
		 * - if the key output is empty, the sign "*" is not printed;
		 * - the sequence of characters "+-" is transformed into "-";
		 * - at most piranha::settings::get_max_term_output() terms are printed, and terms in excess are
		 *   represented with ellipsis "..." at the end of the output; if piranha::settings::get_max_term_output()
		 *   is zero, all the terms will be printed.
		 * 
		 * Note that the print order of the terms will be undefined.
		 * 
		 * @param[in,out] os target stream.
		 * @param[in] s piranha::series argument.
		 * 
		 * @return reference to \p os.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::print_coefficient(),
		 * - the <tt>print()</tt> method of the key type,
		 * - memory allocation errors in standard containers,
		 * - piranha::settings::get_max_term_output(),
		 * - streaming to \p os or to instances of \p std::ostringstream.
		 */
		friend std::ostream &operator<<(std::ostream &os, const series &s)
		{
			// If the series is empty, print zero and exit.
			if (s.empty()) {
				os << "0";
				return os;
			}
			return print_helper_1<false>(os,s.m_container.begin(),s.m_container.end(),s.m_symbol_set);
		}
	protected:
		/// Symbol set.
		symbol_set	m_symbol_set;
		/// Terms container.
		container_type	m_container;
	private:
		// NOTE: Derived is not a complete type here, so we need to wrap everything in a unique_ptr.
		typedef std::unique_ptr<std::unordered_map<std::string,std::function<Derived(const Derived &)>>> cp_map_type;
		static std::mutex	cp_mutex;
		static cp_map_type	cp_map;

};

// Static initialisation.
template <typename Term, typename Derived>
std::mutex series<Term,Derived>::cp_mutex;

template <typename Term, typename Derived>
typename series<Term,Derived>::cp_map_type series<Term,Derived>::cp_map =
	series<Term,Derived>::cp_map_type(new std::unordered_map<std::string,std::function<Derived(const Derived &)>>);

/// Specialisation of piranha::print_coefficient_impl for series.
/**
 * This specialisation is enabled if \p Series is an instance of piranha::series.
 */
template <typename Series>
struct print_coefficient_impl<Series,typename std::enable_if<
	is_instance_of<Series,series>::value>::type>
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

/// Specialisation of piranha::print_tex_coefficient_impl for series.
/**
 * This specialisation is enabled if \p Series is an instance of piranha::series.
 */
template <typename Series>
struct print_tex_coefficient_impl<Series,typename std::enable_if<
	is_instance_of<Series,series>::value>::type>
{
	/// Call operator.
	/**
	 * Equivalent to piranha::series::print_tex(), apart from a couple
	 * of parentheses '()' enclosing the coefficient series if its size is larger than 1.
	 * 
	 * @param[in] os target stream.
	 * @param[in] s coefficient series to be printed.
	 * 
	 * @throws unspecified any exception thrown by piranha::series::print_tex().
	 */
	void operator()(std::ostream &os, const Series &s) const
	{
		if (s.size() > 1u) {
			os << "\\left(";
		}
		s.print_tex(os);
		if (s.size() > 1u) {
			os << "\\right)";
		}
	}
};

namespace math
{

/// Specialisation of the piranha::math::negate() functor for piranha::series.
/**
 * This specialisation is enabled if \p T is an instance of piranha::series.
 */
template <typename T>
struct negate_impl<T,typename std::enable_if<is_instance_of<T,series>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in,out] s piranha::series to be negated.
	 * 
	 * @return the return value of piranha::series::negate()..
	 * 
	 * @throws unspecified any exception thrown by piranha::series::negate().
	 */
	template <typename U>
	auto operator()(U &s) const -> decltype(s.negate())
	{
		return s.negate();
	}
};

/// Specialisation of the piranha::math::is_zero() functor for piranha::series.
/**
 * This specialisation is activated when \p Series is an instance of piranha::series.
 * The result will be computed via the series' <tt>empty()</tt> method.
 */
template <typename Series>
struct is_zero_impl<Series,typename std::enable_if<is_instance_of<Series,series>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] s piranha::series to be tested.
	 * 
	 * @return \p true if \p s is empty, \p false otherwise.
	 */
	bool operator()(const Series &s) const
	{
		return s.empty();
	}
};

/// Specialisation of the piranha::math::pow() functor for piranha::series.
/**
 * This specialisation is activated when \p Series is an instance of piranha::series.
 * The result will be computed via the series' <tt>pow()</tt> method.
 */
template <typename Series, typename T>
struct pow_impl<Series,T,typename std::enable_if<is_instance_of<Series,series>::value>::type>
{
	/// Call operator.
	/**
	 * The exponentiation will be computed via the series' <tt>pow()</tt> method.
	 * 
	 * @param[in] s base.
	 * @param[in] x exponent.
	 * 
	 * @return \p s to the power of \p x.
	 * 
	 * @throws unspecified any exception resulting from the series' <tt>pow()</tt> method.
	 */
	template <typename S, typename U>
	auto operator()(const S &s, const U &x) const -> decltype(s.pow(x))
	{
		return s.pow(x);
	}
};

}

namespace detail
{

// All this gook can go back into the private impl methods once we switch to GCC 4.8.
template <typename T>
class series_has_sin: sfinae_types
{
		template <typename U>
		static auto test(const U *t) -> decltype(t->sin());
		static no test(...);
	public:
		static const bool value = std::is_same<decltype(test((const T *)nullptr)),T>::value;
};

template <typename T>
inline T series_sin_call_impl(const T &s, typename std::enable_if<series_has_sin<T>::value>::type * = nullptr)
{
	return s.sin();
}

// NOTE: this is similar to the approach that we use in trigonometric series. It is one possible way of accomplishing this,
// but this particular form seems to work ok across a variety of compilers - especially in GCC, the interplay between template
// aliases, decltype() and sfinae seems to be brittle in early versions. Variations of this are used also in power_series
// and t_subs series.
// NOTE: template aliases are dispatched immediately where they are used, after that normal SFINAE rules apply. See, e.g., the usage
// in this example:
// http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_active.html#1558
template <typename T>
using sin_cf_enabler = typename std::enable_if<!series_has_sin<T>::value &&
	std::is_same<typename T::term_type::cf_type,decltype(piranha::math::sin(std::declval<typename T::term_type::cf_type>()))>::value>::type;

template <typename T>
inline T series_sin_call_impl(const T &s, sin_cf_enabler<T> * = nullptr)
{
	typedef typename T::term_type::cf_type cf_type;
	auto f = [](const cf_type &cf) {return piranha::math::sin(cf);};
	try {
		return s.apply_cf_functor(f);
	} catch (const std::invalid_argument &) {
		piranha_throw(std::invalid_argument,"series is unsuitable for the calculation of sine");
	}
}

template <typename T>
class series_has_cos: sfinae_types
{
		template <typename U>
		static auto test(const U *t) -> decltype(t->cos());
		static no test(...);
	public:
		static const bool value = std::is_same<decltype(test((const T *)nullptr)),T>::value;
};

template <typename T>
inline T series_cos_call_impl(const T &s, typename std::enable_if<series_has_cos<T>::value>::type * = nullptr)
{
	return s.cos();
}

template <typename T>
using cos_cf_enabler = typename std::enable_if<!series_has_cos<T>::value &&
	std::is_same<typename T::term_type::cf_type,decltype(piranha::math::cos(std::declval<typename T::term_type::cf_type>()))>::value>::type;

template <typename T>
inline T series_cos_call_impl(const T &s, cos_cf_enabler<T> * = nullptr)
{
	typedef typename T::term_type::cf_type cf_type;
	auto f = [](const cf_type &cf) {return piranha::math::cos(cf);};
	try {
		return s.apply_cf_functor(f);
	} catch (const std::invalid_argument &) {
		piranha_throw(std::invalid_argument,"series is unsuitable for the calculation of cosine");
	}
}

}

namespace math
{

/// Specialisation of the piranha::math::sin() functor for piranha::series.
/**
 * This specialisation is activated when \p Series is an instance of piranha::series.
 * If the series type provides a const <tt>Series::sin()</tt> method returning an object of type \p Series,
 * it will be used for the computation of the result.
 * 
 * Otherwise, a call to piranha::series::apply_cf_functor() will be attempted with a functor that
 * calculates piranha::math::sin().
 */
template <typename Series>
struct sin_impl<Series,typename std::enable_if<is_instance_of<Series,series>::value>::type>
{
	/// Call operator.
	/**
	 * \note
	 * This operator is enabled if one of these conditions apply:
	 * - the input series type has a const <tt>sin()</tt> method, or
	 * - the coefficient type of the series satisfies piranha::has_sine, returning an instance of the
	 *   coefficient type as result.
	 *
	 * @param[in] s argument.
	 *
	 * @return sine of \p s.
	 *
	 * @throws unspecified any exception thrown by:
	 * - the <tt>Series::sin()</tt> method,
	 * - piranha::math::sin(),
	 * - piranha::series::apply_cf_functor().
	 */
	template <typename T>
	auto operator()(const T &s) const -> decltype(detail::series_sin_call_impl(s))
	{
		return detail::series_sin_call_impl(s);
	}
};

/// Specialisation of the piranha::math::cos() functor for piranha::series.
/**
 * This specialisation is activated when \p Series is an instance of piranha::series.
 * If the series type provides a const <tt>Series::cos()</tt> method returning an object of type \p Series,
 * it will be used for the computation of the result.
 * 
 * Otherwise, a call to piranha::series::apply_cf_functor() will be attempted with a functor that
 * calculates piranha::math::cos().
 */
template <typename Series>
struct cos_impl<Series,typename std::enable_if<is_instance_of<Series,series>::value>::type>
{
	/// Call operator.
	/**
	 * \note
	 * This operator is enabled if one of these conditions apply:
	 * - the input series type has a const <tt>cos()</tt> method, or
	 * - the coefficient type of the series satisfies piranha::has_cosine, returning an instance of the
	 *   coefficient type as result.
	 *
	 * @param[in] s argument.
	 *
	 * @return cosine of \p s.
	 *
	 * @throws unspecified any exception thrown by:
	 * - the <tt>Series::cos()</tt> method,
	 * - piranha::math::cos(),
	 * - piranha::series::apply_cf_functor().
	 */
	template <typename T>
	auto operator()(const T &s) const -> decltype(detail::series_cos_call_impl(s))
	{
		return detail::series_cos_call_impl(s);
	}
};

/// Specialisation of the piranha::math::partial() functor for series types.
/**
 * This specialisation is activated when \p Series is an instance of piranha::series.
 */
template <typename Series>
struct partial_impl<Series,typename std::enable_if<is_instance_of<Series,series>::value>::type>
{
	/// Call operator.
	/**
	 * The call operator will first check whether a custom partial derivative for \p Series was registered
	 * via piranha::series::register_custom_derivative(). In such a case, the custom derivative function will be used
	 * to compute the return value. Otherwise, the output of piranha::series::partial() will be returned.
	 * 
	 * @param[in] s input series.
	 * @param[in] name name of the argument with respect to which the differentiation will be calculated.
	 * 
	 * @return the partial derivative of \p s with respect to \p name.
	 * 
	 * @throws unspecified any exception thrown by:
	 * - piranha::series::partial(),
	 * - failure(s) in threading primitives,
	 * - lookup operations on \p std::unordered_map,
	 * - the copy assignment and call operators of the registered custom partial derivative function.
	 */
	template <typename T>
	auto operator()(const T &s, const std::string &name) -> decltype(s.partial(name))
	{
		bool custom = false;
		std::function<T(const T &)> func;
		// Try to locate a custom partial derivative and copy it into func, if found.
		{
			std::lock_guard<std::mutex> lock(T::cp_mutex);
			auto it = T::cp_map->find(name);
			if (it != T::cp_map->end()) {
				func = it->second;
				custom = true;
			}
		}
		if (custom) {
			return func(s);
		}
		return s.partial(name);
	}
};

/// Specialisation of the piranha::math::evaluate() functor for series types.
/**
 * This specialisation is activated when \p Series is an instance of piranha::series.
 */
template <typename Series>
struct evaluate_impl<Series,typename std::enable_if<is_instance_of<Series,series>::value>::type>
{
	/// Call operator.
	/**
	 * The implementation will use piranha::series::evaluate().
	 *
	 * @param[in] s evaluation argument.
	 * @param[in] dict evaluation dictionary.
	 *
	 * @return output of piranha::series::evaluate().
	 *
	 * @throws unspecified any exception thrown by piranha::series::evaluate().
	 */
	template <typename T>
	auto operator()(const Series &s, const std::unordered_map<std::string,T> &dict) const -> decltype(s.evaluate(dict))
	{
		return s.evaluate(dict);
	}
};

}

/// Type trait to detect series types.
/**
 * This type trait will be true if \p T is an instance of piranha::series and it satisfies piranha::is_container_element.
 */
template <typename T>
class is_series
{
	public:
		/// Value of the type trait.
		static const bool value = is_instance_of<T,series>::value && is_container_element<T>::value;
};

template <typename T>
const bool is_series<T>::value;

}

#endif
