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

#ifndef PIRANHA_SERIES_BINARY_OPERATORS_HPP
#define PIRANHA_SERIES_BINARY_OPERATORS_HPP

#include <type_traits>

#include "config.hpp"
#include "detail/series_fwd.hpp"
#include "echelon_size.hpp"
#include "math.hpp"

namespace piranha
{

/// Binary operators for piranha::series.
/**
 * This class provides binary arithmetic and relational operators overloads for piranha::series instances. The operators
 * are implemented as inline friend functions, enabled only when at least one of the two operands derives from piranha::series.
 * The class has no data members, and hence has trivial move semantics and provides the strong exception safety guarantee.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
// NOTE: the existence of this class, as opposed to defining the operators directly in series,
// is that it provides a non-templated scope for the operators - which are declared as inline friends.
// If they were declared as such in series, ambiguities would arise due to the presence of an extra
// template parameter.
class series_binary_operators
{
		template <typename T, typename U>
		struct result_type
		{
			// Series with same echelon size and different coefficient types.
			template <typename T1, typename T2, typename Enable = void>
			struct deduction_impl
			{
				typedef typename T1::term_type::cf_type cf_type1;
				typedef typename T2::term_type::cf_type cf_type2;
				typedef decltype(std::declval<cf_type1>() + std::declval<cf_type2>()) _r_type;
				static_assert(std::is_same<_r_type,cf_type1>::value || std::is_same<_r_type,cf_type2>::value,
					"Invalid result type for binary operator.");
				typedef typename std::conditional<
					std::is_same<_r_type,cf_type1>::value,
					T1,
					T2
				>::type type;
			};
			// Series with same echelon size and coefficient types.
			template <typename T1, typename T2>
			struct deduction_impl<T1,T2,
				typename std::enable_if<
				std::is_base_of<detail::series_tag,T1>::value &&
				std::is_base_of<detail::series_tag,T2>::value &&
				std::is_same<typename T1::term_type::cf_type,typename T2::term_type::cf_type>::value &&
				echelon_size<typename T1::term_type>::value == echelon_size<typename T2::term_type>::value>::type>
			{
				typedef T1 type;
			};
			// Series with different echelon sizes.
			template <typename T1, typename T2>
			struct deduction_impl<T1,T2,typename std::enable_if<
				std::is_base_of<detail::series_tag,T1>::value &&
				std::is_base_of<detail::series_tag,T2>::value &&
				echelon_size<typename T1::term_type>::value != echelon_size<typename T1::term_type>::value
				>::type>
			{
				typedef typename std::conditional<
					((echelon_size<typename T1::term_type>::value) > (echelon_size<typename T2::term_type>::value)),
					T1,
					T2
				>::type type;
			};
			// Series and non-series.
			template <typename T1, typename T2>
			struct deduction_impl<T1,T2,typename std::enable_if<!std::is_base_of<detail::series_tag,T1>::value || !std::is_base_of<detail::series_tag,T2>::value>::type>
			{
				typedef typename std::conditional<
					std::is_base_of<detail::series_tag,T1>::value,
					T1,
					T2
				>::type type;
			};
			typedef typename std::decay<T>::type type1;
			typedef typename std::decay<U>::type type2;
			typedef typename deduction_impl<type1,type2>::type type;
		};
		template <typename T, typename U>
		struct are_series_operands
		{
			static const bool value = std::is_base_of<detail::series_tag,typename std::decay<T>::type>::value ||
				std::is_base_of<detail::series_tag,typename std::decay<U>::type>::value;
		};
		// NOTE: here we can probably optimize better: use +=/-= only if we can steal enough resources for return value. Otherwsise,
		// create return value with enough buckets - possibly copying it from one of the input lvalues to perform a copy instead of
		// repeated insertions - and insert the rest. Or something like it. Preparing the return value in a similar way might also
		// be appropriate for the implementation of +=/-=, even if though we need some kind of cost model for that.
		// NOTE: the idea here is that we need to try to optimize only the creation of the return value, as in-place operations
		// take care of optimizing the actual operation.
		// Overload if either:
		// - return type is first one and types are not the same,
		// - series types are the same and first series is nonconst rvalue ref,
		// - series types are the same and no type is nonconst rvalue ref.
		template <bool Sign, typename Series1, typename U>
		static typename result_type<Series1,U>::type dispatch_binary_add(Series1 &&s1, U &&other, typename std::enable_if<
			(std::is_same<typename result_type<Series1,U>::type,typename std::decay<Series1>::type>::value &&
			!std::is_same<typename std::decay<Series1>::type,typename std::decay<U>::type>::value) ||
			(std::is_same<typename std::decay<Series1>::type,typename std::decay<U>::type>::value &&
			is_nonconst_rvalue_ref<Series1 &&>::value) ||
			(std::is_same<typename std::decay<Series1>::type,typename std::decay<U>::type>::value &&
			!is_nonconst_rvalue_ref<Series1 &&>::value && !is_nonconst_rvalue_ref<U &&>::value)
			>::type * = piranha_nullptr)
		{
			typename result_type<Series1,U>::type retval(std::forward<Series1>(s1));
			if (Sign) {
				retval += std::forward<U>(other);
			} else {
				retval -= std::forward<U>(other);
			}
			return retval;
		}
		// Overload if either:
		// - return type is second one and types are not the same,
		// - series type are the same and first series is not a nonconst rvalue ref, while second one is.
		template <bool Sign, typename U, typename Series2>
		static typename result_type<U,Series2>::type dispatch_binary_add(U &&other, Series2 &&s2, typename std::enable_if<
			(std::is_same<typename result_type<U,Series2>::type,typename std::decay<Series2>::type>::value &&
			!std::is_same<typename std::decay<U>::type,typename std::decay<Series2>::type>::value) ||
			(std::is_same<typename std::decay<U>::type,typename std::decay<Series2>::type>::value &&
			!is_nonconst_rvalue_ref<U &&>::value && is_nonconst_rvalue_ref<Series2 &&>::value)
			>::type * = piranha_nullptr)
		{
			typename result_type<U,Series2>::type retval(std::forward<Series2>(s2));
			if (Sign) {
				retval += std::forward<U>(other);
			} else {
				retval -= std::forward<U>(other);
				// We need to change sign, since the order of operands was inverted.
				retval.negate();
			}
			return retval;
		}
		// Binary multiplication.
		// NOTE: for binary mult we want to diversify the cases between series multiplication and mixed multiplication,
		// as in the latter case we implement in terms of *= while in the former we do not.
		// Binary multiplication.
		// Series vs non-series.
		template <typename Series, typename T>
		static typename result_type<Series,T>::type dispatch_binary_multiply(Series &&s, T &&x,
			typename std::enable_if<!std::is_base_of<detail::series_tag,typename std::decay<T>::type>::value>::type * = piranha_nullptr)
		{
			return mixed_binary_multiply(std::forward<Series>(s),std::forward<T>(x));
		}
		// Series with larger echelon size vs series.
		template <typename Series, typename T>
		static typename result_type<Series,T>::type dispatch_binary_multiply(Series &&s, T &&x,
			typename std::enable_if<
			std::is_base_of<detail::series_tag,typename std::decay<Series>::type>::value &&
			std::is_base_of<detail::series_tag,typename std::decay<T>::type>::value &&
			(echelon_size<typename std::decay<T>::type::term_type>::value < echelon_size<typename std::decay<Series>::type::term_type>::value)>::type * = piranha_nullptr)
		{
			return mixed_binary_multiply(std::forward<Series>(s),std::forward<T>(x));
		}
		// Non-series vs series.
		template <typename T, typename Series>
		static typename result_type<T,Series>::type dispatch_binary_multiply(T &&x, Series &&s,
			typename std::enable_if<!std::is_base_of<detail::series_tag,typename std::decay<T>::type>::value>::type * = piranha_nullptr)
		{
			return mixed_binary_multiply(std::forward<Series>(s),std::forward<T>(x));
		}
		// Series with smaller echelon size vs series.
		template <typename T, typename Series>
		static typename result_type<T,Series>::type dispatch_binary_multiply(T &&x, Series &&s,
			typename std::enable_if<
			std::is_base_of<detail::series_tag,typename std::decay<Series>::type>::value &&
			std::is_base_of<detail::series_tag,typename std::decay<T>::type>::value &&
			(echelon_size<typename std::decay<T>::type::term_type>::value < echelon_size<typename std::decay<Series>::type::term_type>::value)>::type * = piranha_nullptr)
		{
			return mixed_binary_multiply(std::forward<Series>(s),std::forward<T>(x));
		}
		template <typename Series, typename T>
		static typename std::decay<Series>::type mixed_binary_multiply(Series &&s, T &&x)
		{
			// NOTE: here it might be potentially possible to improve performance in certain cases as follows:
			// - create empty series,
			// - insert terms constructed by multiplying coefficient by x and same keys as in s.
			// This way in case of, e.g., integer GMP coefficients, we can avoid allocating space in the copy of the series
			// below and again allocation for the multiplication.
			typename std::decay<Series>::type retval(std::forward<Series>(s));
			retval *= std::forward<T>(x);
			return retval;
		}
		template <typename Series1, typename Series2>
		static Series1 series_multiply_first(const Series1 &s1, const Series2 &s2)
		{
			// NOTE: all this dancing around with base and derived types for series is necessary as the mechanism of
			// specialization of series_multiplier and truncator depends on the derived types - which must then be preserved and
			// not casted away to the base types.
			// Base series types.
			typedef series<typename Series1::term_type,Series1> base_type1;
			typedef series<typename Series2::term_type,Series2> base_type2;
			Series1 retval;
			if (likely(s1.m_symbol_set == s2.m_symbol_set)) {
				retval.m_symbol_set = s1.m_symbol_set;
				static_cast<base_type1 &>(retval) = s1.multiply_by_series(s2);
			} else {
				// Let's deal with the first series.
				auto merge = s1.m_symbol_set.merge(s2.m_symbol_set);
				piranha_assert(merge == s2.m_symbol_set.merge(s1.m_symbol_set));
				piranha_assert(merge == s2.m_symbol_set.merge(merge));
				const bool need_copy1 = (merge != s1.m_symbol_set);
				const bool need_copy2 = (merge != s2.m_symbol_set);
				piranha_assert(need_copy1 || need_copy2);
				retval.m_symbol_set = merge;
				if (need_copy1) {
					Series1 s1_copy;
					static_cast<base_type1 &>(s1_copy) = s1.merge_args(merge);
					if (need_copy2) {
						Series2 s2_copy;
						static_cast<base_type2 &>(s2_copy) = s2.merge_args(merge);
						static_cast<base_type1 &>(retval) = s1_copy.multiply_by_series(s2_copy);
					} else {
						static_cast<base_type1 &>(retval) = s1_copy.multiply_by_series(s2);
					}

				} else {
					piranha_assert(need_copy2);
					Series2 s2_copy;
					static_cast<base_type2 &>(s2_copy) = s2.merge_args(merge);
					static_cast<base_type1 &>(retval) = s1.multiply_by_series(s2_copy);
				}
			}
			return retval;
		}
		// Overload for same echelon size series, and types are the same or return type is Series1.
		template <typename Series1, typename Series2>
		static typename result_type<Series1,Series2>::type dispatch_binary_multiply(const Series1 &s1, const Series2 &s2, typename std::enable_if<
			(std::is_same<typename result_type<Series1,Series2>::type,Series1>::value || std::is_same<Series1,Series2>::value) &&
			std::is_base_of<detail::series_tag,typename std::decay<Series1>::type>::value &&
			std::is_base_of<detail::series_tag,typename std::decay<Series2>::type>::value &&
			echelon_size<typename Series1::term_type>::value == echelon_size<typename Series2::term_type>::value
			>::type * = piranha_nullptr)
		{
			return series_multiply_first(s1,s2);
		}
		// Overload for same echelon size series if types are not the the same and return type is Series2.
		template <typename Series1, typename Series2>
		static typename result_type<Series1,Series2>::type dispatch_binary_multiply(const Series1 &s1, const Series2 &s2, typename std::enable_if<
			std::is_same<typename result_type<Series1,Series2>::type,Series2>::value && !std::is_same<Series1,Series2>::value &&
			std::is_base_of<detail::series_tag,typename std::decay<Series1>::type>::value &&
			std::is_base_of<detail::series_tag,typename std::decay<Series2>::type>::value &&
			echelon_size<typename Series1::term_type>::value == echelon_size<typename Series2::term_type>::value
			>::type * = piranha_nullptr)
		{
			return series_multiply_first(s2,s1);
		}
		// Equality.
		template <typename Series, typename T>
		static bool mixed_equality(const Series &s, const T &x)
		{
			static_assert(std::is_base_of<detail::series_tag,Series>::value,"Non-series type during mixed comparison.");
			const auto size = s.size();
			if (size > 1u) {
				return false;
			}
			if (!size) {
				return math::is_zero(x);
			}
			const auto it = s.m_container.begin();
			return (it->m_cf == x && it->m_key.is_unitary(s.m_symbol_set));
		}
		// Overload for series vs non-series.
		template <typename Series, typename T>
		static bool dispatch_equality(const Series &s, const T &x,
			typename std::enable_if<!std::is_base_of<detail::series_tag,T>::value>::type * = piranha_nullptr)
		{
			return mixed_equality(s,x);
		}
		// Overload for series vs series with lesser echelon size.
		template <typename Series, typename T>
		static bool dispatch_equality(const Series &s, const T &x,
			typename std::enable_if<
			std::is_base_of<detail::series_tag,Series>::value &&
			std::is_base_of<detail::series_tag,T>::value &&
			(echelon_size<typename T::term_type>::value < echelon_size<typename Series::term_type>::value)>::type * = piranha_nullptr)
		{
			return mixed_equality(s,x);
		}
		// Overload for non-series vs series.
		template <typename T, typename Series>
		static bool dispatch_equality(const T &x, const Series &s,
			typename std::enable_if<!std::is_base_of<detail::series_tag,T>::value>::type * = piranha_nullptr)
		{
			return mixed_equality(s,x);
		}
		// Overload for series with lesser echelon size vs series.
		template <typename T, typename Series>
		static bool dispatch_equality(const T &x, const Series &s,
			typename std::enable_if<
			std::is_base_of<detail::series_tag,Series>::value &&
			std::is_base_of<detail::series_tag,T>::value &&
			(echelon_size<typename T::term_type>::value < echelon_size<typename Series::term_type>::value)>::type * = piranha_nullptr)
		{
			return mixed_equality(s,x);
		}
		// Overload with series with same term types.
		template <typename Series1, typename Series2>
		static bool series_equality_impl(const Series1 &s1, const Series2 &s2, typename std::enable_if<
			std::is_same<typename Series1::term_type,typename Series2::term_type>::value
			>::type * = piranha_nullptr)
		{
			if (s1.size() != s2.size()) {
				return false;
			}
			piranha_assert(s1.m_symbol_set == s2.m_symbol_set);
			const auto it_f = s1.m_container.end();
			const auto it_end = s2.m_container.end();
			for (auto it = s1.m_container.begin(); it != it_f; ++it) {
				const auto tmp_it = s2.m_container.find(*it);
				if (tmp_it == it_end || tmp_it->m_cf != it->m_cf) {
					return false;
				}
			}
			return true;
		}
		// Overload with series with different term types. Series1 term type is promoted to
		// Series2 term type.
		template <typename Series1, typename Series2>
		static bool series_equality_impl(const Series1 &s1, const Series2 &s2, typename std::enable_if<
			!std::is_same<typename Series1::term_type,typename Series2::term_type>::value
			>::type * = piranha_nullptr)
		{
			const auto it_f = s1.m_container.end();
			const auto it_end = s2.m_container.end();
			typedef typename std::decay<decltype(*it_end)>::type term_type;
			typedef typename term_type::cf_type cf_type;
			typedef typename term_type::key_type key_type;
			decltype(s1.size()) count = 0u;
			for (auto it = s1.m_container.begin(); it != it_f; ++it) {
				// NOTE: here there is quite a corner case: namely we cannot exclude
				// that two different source terms do not generate the same converted term,
				// which would cause the same term in s2 to be counted twice.
				// Construct the temporary converted term.
				term_type tmp(cf_type(it->m_cf),
					key_type(it->m_key,s2.m_symbol_set));
				// Locate the tmp term.
				const auto tmp_it = s2.m_container.find(tmp);
				const bool in_series = (tmp_it != it_end);
				// If a converted term is not in the series, it must be ignorable,
				// otherwise the series are different.
				if (!in_series && !tmp.is_ignorable(s2.m_symbol_set)) {
					return false;
				}
				if (in_series) {
					// The existing term must not be ignorable.
					piranha_assert(!tmp_it->is_ignorable(s2.m_symbol_set));
					if (tmp_it->m_cf != tmp.m_cf) {
						// If the converted term is in the series, but its coefficient differs,
						// then the series are different.
						return false;
					}
					// The converted term and the term located in s2 have the same coefficient:
					// update the count of converted terms from s1 found in s2.
					++count;
				}
			}
			return (count == s2.size());
		}
		// Helper method used to merge arguments when comparing series with different
		// arguments.
		template <typename Series, typename Merge>
		static Series merge_series_for_equality(const Series &s, Merge &merge)
		{
			Series retval;
			// This is the base type of the series.
			typedef decltype(s.merge_args(merge)) base_type;
			// Take care of assigning the merged base class.
			(*static_cast<base_type *>(&retval)) = s.merge_args(merge);
			return retval;
		}
		// Series equality, with promotion rule returning Series1.
		template <typename Series1, typename Series2>
		static bool series_equality(const Series1 &s1, const Series2 &s2)
		{
			// Arguments merging.
			if (likely(s1.m_symbol_set == s2.m_symbol_set)) {
				return series_equality_impl(s1,s2);
			} else {
				// Let's deal with the first series.
				auto merge = s1.m_symbol_set.merge(s2.m_symbol_set);
				const bool s1_needs_copy = (merge != s1.m_symbol_set);
				const bool s2_needs_copy = (merge != s2.m_symbol_set);
				piranha_assert(s1_needs_copy || s2_needs_copy);
				return series_equality_impl(
					s1_needs_copy ? merge_series_for_equality(s1,merge) : s1,
					s2_needs_copy ? merge_series_for_equality(s2,merge) : s2
				);
			}
		}
		// Series with same echelon size, and type promotion rule returns the first type.
		template <typename Series1, typename Series2>
		static bool dispatch_equality(const Series1 &s1, const Series2 &s2, typename std::enable_if<
			std::is_base_of<detail::series_tag,Series1>::value &&
			std::is_base_of<detail::series_tag,Series2>::value &&
			echelon_size<typename Series1::term_type>::value == echelon_size<typename Series2::term_type>::value &&
			std::is_same<decltype(s1 + s2),Series1>::value
			>::type * = piranha_nullptr)
		{
			return series_equality(s2,s1);
		}
		// Series with same echelon size, and type promotion rule does not return first type.
		template <typename Series1, typename Series2>
		static bool dispatch_equality(const Series1 &s1, const Series2 &s2, typename std::enable_if<
			std::is_base_of<detail::series_tag,Series1>::value &&
			std::is_base_of<detail::series_tag,Series2>::value &&
			echelon_size<typename Series1::term_type>::value == echelon_size<typename Series2::term_type>::value &&
			!std::is_same<decltype(s1 + s2),Series1>::value
			>::type * = piranha_nullptr)
		{
			return series_equality(s1,s2);
		}
	public:
		/// Binary addition involving piranha::series.
		/**
		 * This template operator is activated iff at least one operand is an instance of piranha::series.
		 * The binary addition algorithm proceeds as follows:
		 * 
		 * - if operands are series with same echelon size, the return type is determined by the coefficient types
		 *   \p c1 and \p c2 of \p T and \p U respectively:
		 *   - the return type is \p T if \p c1 and \p c2 are the same type or <tt>decltype(c1 + c2)</tt> is \p c1,
		 *     it is \p c2 if <tt>decltype(c1 + c2)</tt> is \p c2. If \p c1 and \p c2 are different types and <tt>decltype(c1 + c2)</tt>
		 *     is neither \p c1 or \p c2, a compile-time error will be produced;
		 *   - the return value is built from either \p s1 or \p s2 (depending on its type);
		 * - else:
		 *   - the return type is the type of the series operand with largest echelon size;
		 *   - the return value is built from the series operand with largest echelon size;
		 * - piranha::series::operator+=() is called on the return value;
		 * - the return value is returned.
		 * 
		 * Note that the return type is determined solely by the coefficient types, and that the rules determining the return type and
		 * the implementation of the operator might differ from those of built-in C++ types.
		 * 
		 * @param[in] s1 first operand.
		 * @param[in] s2 second operand.
		 * 
		 * @return <tt>s1 + s2</tt>.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - copy-construction of return value type,
		 * - piranha::series::operator+=().
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_series_operands<T,U>::value,typename result_type<T,U>::type>::type operator+(T &&s1, U &&s2)
		{
			return dispatch_binary_add<true>(std::forward<T>(s1),std::forward<U>(s2));
		}
		/// Binary subtraction involving piranha::series.
		/**
		 * This template operator is activated iff at least one operand is an instance of piranha::series. The algorithm proceeds in the same
		 * way as operator+(), with a change in sign and possibly a call to piranha::series::negate().
		 * 
		 * @param[in] s1 first operand.
		 * @param[in] s2 second operand.
		 * 
		 * @return <tt>s1 - s2</tt>.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - copy-construction of return value type,
		 * - piranha::series::operator-=(),
		 * - piranha::series::negate().
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_series_operands<T,U>::value,typename result_type<T,U>::type>::type operator-(T &&s1, U &&s2)
		{
			return dispatch_binary_add<false>(std::forward<T>(s1),std::forward<U>(s2));
		}
		/// Binary multiplication involving piranha::series.
		/**
		 * This template operator is activated iff at least one operand is an instance of piranha::series.
		 * The binary multiplication algorithm proceeds as follows:
		 * 
		 * - if both operands are series with same echelon size:
		 *   - the return type is determined in the same way as for the binary addition operator;
		 *   - the same sequence of operations described in piranha::series::operator*=()
		 *     is performed;
		 * - else:
		 *   - the return type is the type of the series operand with largest echelon size;
		 *   - the return value is built from the series operand with largest echelon size;
		 *   - piranha::series::operator*=() is called on the return value with the other operand as argument;
		 * - the return value is returned.
		 * 
		 * @param[in] s1 first operand.
		 * @param[in] s2 second operand.
		 * 
		 * @return <tt>s1 * s2</tt>.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - copy-construction of return value type,
		 * - piranha::series::operator*=().
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_series_operands<T,U>::value,typename result_type<T,U>::type>::type operator*(T &&s1, U &&s2)
		{
			return dispatch_binary_multiply(std::forward<T>(s1),std::forward<U>(s2));
		}
		/// Equality operator involving piranha::series.
		/**
		 * This template operator is activated iff at least one operand is an instance of piranha::series.
		 * 
		 * The comparison algorithm operates as follows:
		 * 
		 * - if both operands are instances of piranha::series with the same echelon size:
		 *   - if the symbol sets of the two series differ, copies of the series
		 *     are created as necessary, and the algorithm proceeds to the comparison of such
		 *     copies instead;
		 *   - if the term types of the two series are the same:
		 *     - the series are equal if their sizes coincide, and if for each term \p t1
		 *       of one series there exist a term \p t2 with the same key in the second series
		 *       and the coefficients of \p t1 and \p t2 are equal. Otherwise, the operator will return \p false;
		 *   - else:
		 *     - one of the two series types is chosen for type promotion following the same rules of the binary addition operator;
		 *     - each term \p t1 of the series to be promoted is converted to an instance \p t1c of the other series'
		 *       term type, and a term \p t2 equivalent to \p t1c is located into the
		 *       other series. If each non-ignorable converted term \p t1c is successfully identified in the other series
		 *       (meaning that \p t2 is found with same key and coefficient as \p t1c) and the total count of \p t2 terms
		 *       located in this way is equivalent to the size of the other series, the operator returns \p true.
		 *       Otherwise, the operator will return \p false;
		 * - else:
		 *   - if the size of the series operand with the largest echelon size is greater than one,\p false is returned;
		 *   - if the size of the series operand with the largest echelon size is zero, then the return value of
		 *     piranha::math::is_zero() on the other operand is returned;
		 *   - if the coefficient of the only term of the series operand with the largest echelon size is equal to the other
		 *     operand and the key is unitary (as established by the key type's <tt>is_unitary()</tt> method),
		 *     then \p true will be returned, otherwise \p false will be returned.
		 * 
		 * Note that in series comparisons involving series with same coefficient types but different term types,
		 * the promotion rule during term comparisons will depend on the order of the operands.
		 * 
		 * @param[in] s1 first operand.
		 * @param[in] s2 second operand.
		 * 
		 * @return \p true if <tt>s1 == s2</tt>, \p false otherwise.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::symbol_set::merge(),
		 * - piranha::series::insert(),
		 * - the <tt>merge_args()</tt> and <tt>is_unitary()</tt> methods of the key type,
		 * - the equality operator of coefficient types,
		 * - piranha::hash_set::find(),
		 * - the constructors of term, coefficient and key types,
		 * - piranha::math::is_zero().
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_series_operands<T,U>::value,bool>::type operator==(const T &s1, const U &s2)
		{
			return dispatch_equality(s1,s2);
		}
		/// Inequality operator involving piranha::series.
		/**
		 * Logical negation of the equality operator.
		 * 
		 * @param[in] s1 first operand.
		 * @param[in] s2 second operand.
		 * 
		 * @return \p true if <tt>s1 != s2</tt>, \p false otherwise.
		 * 
		 * @throws unspecified any exception thrown by the equality operator.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_series_operands<T,U>::value,bool>::type operator!=(const T &s1, const U &s2)
		{
			return !(s1 == s2);
		}
};

}

#endif
