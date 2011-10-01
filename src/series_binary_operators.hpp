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
#include "type_traits.hpp"

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
					(echelon_size<typename T1::term_type>::value > echelon_size<typename T2::term_type>::value),
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
		template <bool Sign, typename Series, typename T>
		static typename result_type<Series,T>::type dispatch_binary_add(Series &&s, T &&x,
			typename std::enable_if<!std::is_base_of<detail::series_tag,typename std::decay<T>::type>::value>::type * = piranha_nullptr)
		{
			return mixed_binary_add<Sign,false>(std::forward<Series>(s),std::forward<T>(x));
		}
		template <bool Sign, typename T, typename Series>
		static typename result_type<T,Series>::type dispatch_binary_add(T &&x, Series &&s,
			typename std::enable_if<!std::is_base_of<detail::series_tag,typename std::decay<T>::type>::value>::type * = piranha_nullptr)
		{
			return mixed_binary_add<Sign,true>(std::forward<Series>(s),std::forward<T>(x));
		}
		template <bool Sign, bool Swapped, typename Series, typename T>
		static typename std::decay<Series>::type mixed_binary_add(Series &&s, T &&x)
		{
			static_assert(std::is_same<typename std::decay<Series>::type,typename result_type<Series,T>::type>::value,"Inconsistent return value type.");
			static_assert(std::is_same<typename std::decay<Series>::type,typename result_type<T,Series>::type>::value,"Inconsistent return value type.");
			typename std::decay<Series>::type retval(std::forward<Series>(s));
			if (Sign) {
				retval += std::forward<T>(x);
			} else {
				retval -= std::forward<T>(x);
				// If the order of operands was swapped in a subtraction, negate the result.
				if (Swapped) {
					retval.negate();
				}
			}
			return retval;
		}
		// NOTE: the idea here is that we need to try to optimize only the creation of the return value, as in-place operations
		// take care of optimizing the actual operation.
		// Overload if either:
		// - return type is first series and series types are not the same,
		// - series type are the same and first series is nonconst rvalue ref.
		template <bool Sign, typename Series1, typename Series2>
		static typename result_type<Series1,Series2>::type series_binary_add(Series1 &&s1, Series2 &&s2, typename std::enable_if<
			(std::is_same<typename result_type<Series1,Series2>::type,typename std::decay<Series1>::type>::value &&
			!std::is_same<typename std::decay<Series1>::type,typename std::decay<Series2>::type>::value) ||
			(std::is_same<typename std::decay<Series1>::type,typename std::decay<Series2>::type>::value &&
			is_nonconst_rvalue_ref<Series1 &&>::value)
			>::type * = piranha_nullptr)
		{
			typename result_type<Series1,Series2>::type retval(std::forward<Series1>(s1));
			if (Sign) {
				retval += std::forward<Series2>(s2);
			} else {
				retval -= std::forward<Series2>(s2);
			}
			return retval;
		}
		// Overload if either:
		// - return type is second series and series types are not the same,
		// - series type are the same and first series is not a nonconst rvalue ref.
		template <bool Sign, typename Series1, typename Series2>
		static typename result_type<Series1,Series2>::type series_binary_add(Series1 &&s1, Series2 &&s2, typename std::enable_if<
			(std::is_same<typename result_type<Series1,Series2>::type,typename std::decay<Series2>::type>::value &&
			!std::is_same<typename std::decay<Series1>::type,typename std::decay<Series2>::type>::value) ||
			(std::is_same<typename std::decay<Series1>::type,typename std::decay<Series2>::type>::value &&
			!is_nonconst_rvalue_ref<Series1 &&>::value)
			>::type * = piranha_nullptr)
		{
			typename result_type<Series1,Series2>::type retval(std::forward<Series2>(s2));
			if (Sign) {
				retval += std::forward<Series1>(s1);
			} else {
				retval -= std::forward<Series1>(s1);
				// We need to change sign, since the order of operands was inverted.
				retval.negate();
			}
			return retval;
		}
		template <bool Sign, typename Series1, typename Series2>
		static typename result_type<Series1,Series2>::type dispatch_binary_add(Series1 &&s1, Series2 &&s2,
			typename std::enable_if<
			std::is_base_of<detail::series_tag,typename std::decay<Series1>::type>::value &&
			std::is_base_of<detail::series_tag,typename std::decay<Series2>::type>::value
			>::type * = piranha_nullptr)
		{
			return series_binary_add<Sign>(std::forward<Series1>(s1),std::forward<Series2>(s2));
		}
#if 0
		// Binary multiplication.
		template <typename Series, typename T>
		static typename series_op_return_type<Series,T>::type dispatch_binary_multiply(Series &&s, T &&x,
			typename std::enable_if<!std::is_base_of<detail::series_tag,typename strip_cv_ref<T>::type>::value>::type * = piranha_nullptr)
		{
			return mixed_binary_multiply(std::forward<Series>(s),std::forward<T>(x));
		}
		template <typename T, typename Series>
		static typename series_op_return_type<T,Series>::type dispatch_binary_multiply(T &&x, Series &&s,
			typename std::enable_if<!std::is_base_of<detail::series_tag,typename strip_cv_ref<T>::type>::value>::type * = piranha_nullptr)
		{
			return mixed_binary_multiply(std::forward<Series>(s),std::forward<T>(x));
		}
		template <typename Series, typename T>
		static typename strip_cv_ref<Series>::type mixed_binary_multiply(Series &&s, T &&x)
		{
			// NOTE: here it might be potentially possible to improve performance in certain cases as follows:
			// - create empty series,
			// - insert terms constructed by multiplying coefficient by x and same keys as in s.
			// This way in case of, e.g., integer GMP coefficients, we can avoid allocating space in the copy of the series
			// below and again allocation for the multiplication.
			static_assert(std::is_same<typename strip_cv_ref<Series>::type,typename series_op_return_type<Series,T>::type>::value,"Inconsistent return value type.");
			static_assert(std::is_same<typename strip_cv_ref<Series>::type,typename series_op_return_type<T,Series>::type>::value,"Inconsistent return value type.");
			typename strip_cv_ref<Series>::type retval(std::forward<Series>(s));
			retval *= std::forward<T>(x);
			return retval;
		}
		template <typename Series1, typename Series2>
		static Series1 series_multiply_first(const Series1 &s1, const Series2 &s2)
		{
			static_assert(echelon_size<typename Series1::term_type>::value == echelon_size<typename Series2::term_type>::value,
				"Cannot multiply series with different echelon sizes.");
			// Determine base series types using the multiply_by_series() method.
			typedef decltype(s1.multiply_by_series(s2,s1.m_ed)) base_type1;
			typedef decltype(s2.multiply_by_series(s1,s2.m_ed)) base_type2;
			Series1 retval;
			if (likely(s1.m_ed.get_args_tuple() == s2.m_ed.get_args_tuple())) {
				retval.m_ed = s1.m_ed;
				static_cast<base_type1 &>(retval) = s1.multiply_by_series(s2,s1.m_ed);
			} else {
				// Let's deal with the first series.
				auto merge1 = s1.m_ed.merge(s2.m_ed);
				const bool need_copy1 = merge1.first.get_args_tuple() != s1.m_ed.get_args_tuple();
				auto merge2 = s2.m_ed.merge(merge1.first);
				const bool need_copy2 = merge2.first.get_args_tuple() != s2.m_ed.get_args_tuple();
				retval.m_ed = merge1.first;
				if (need_copy1) {
					static_cast<base_type1 &>(retval) = s1.merge_args(s1.m_ed,merge1.first).multiply_by_series(
						(need_copy2 ? s2.merge_args(s2.m_ed,merge2.first) : static_cast<base_type2 const &>(s2)),merge1.first
					);
				} else {
					static_cast<base_type1 &>(retval) = s1.multiply_by_series(
						(need_copy2 ? s2.merge_args(s2.m_ed,merge2.first) : static_cast<base_type2 const &>(s2)),merge1.first
					);
				}
			}
			return retval;
		}
		// Overload if types are the same or return type is Series1.
		template <typename Series1, typename Series2>
		static typename series_op_return_type<Series1,Series2>::type dispatch_binary_multiply(const Series1 &s1, const Series2 &s2, typename std::enable_if<
			std::is_same<typename series_op_return_type<Series1,Series2>::type,Series1>::value ||
			std::is_same<Series1,Series2>::value
			>::type * = piranha_nullptr)
		{
std::cout << "BLUH BLUH\n";
			return series_multiply_first(s1,s2);
		}
		// Overload if types are not the the same and return type is Series2.
		template <typename Series1, typename Series2>
		static typename series_op_return_type<Series1,Series2>::type dispatch_binary_multiply(const Series1 &s1, const Series2 &s2, typename std::enable_if<
			std::is_same<typename series_op_return_type<Series1,Series2>::type,Series2>::value &&
			!std::is_same<Series1,Series2>::value
			>::type * = piranha_nullptr)
		{
std::cout << "BLAH BLAH\n";
			return series_multiply_first(s2,s1);
		}
		// Equality.
		template <typename Series, typename T>
		static bool mixed_equality(const Series &s, const T &x)
		{
			// TODO: what about this?
			static_assert(!std::is_base_of<detail::base_series_tag,typename strip_cv_ref<T>::type>::value,
				"Cannot compare non top level series.");
			const auto size = s.size();
			if (size > 1) {
				return false;
			}
			if (!size) {
				return typename Series::term_type::cf_type(x,s.m_ed).is_ignorable(s.m_ed);
			}
			const auto it = s.m_container.begin();
			return (it->m_cf.is_equal_to(x,s.m_ed) && it->m_key.is_unitary(s.m_ed.template get_args<typename Series::term_type>()));
		}
		// Overload for series vs non-series.
		template <typename Series, typename T>
		static bool dispatch_equality(const Series &s, const T &x,
			typename std::enable_if<!std::is_base_of<detail::series_tag,typename strip_cv_ref<T>::type>::value>::type * = piranha_nullptr)
		{
std::cout << "LOL bool1\n";
			return mixed_equality(s,x);
		}
		// Overload for non-series vs series.
		template <typename T, typename Series>
		static bool dispatch_equality(const T &x, const Series &s,
			typename std::enable_if<!std::is_base_of<detail::series_tag,typename strip_cv_ref<T>::type>::value>::type * = piranha_nullptr)
		{
std::cout << "LOL bool2\n";
			return mixed_equality(s,x);
		}
		// Overload with series with same term types.
		template <typename Series1, typename Series2>
		static bool series_equality_impl(const Series1 &s1, const Series2 &s2, typename std::enable_if<
			std::is_same<typename strip_cv_ref<Series1>::type::term_type,typename strip_cv_ref<Series2>::type::term_type>::value
			>::type * = piranha_nullptr)
		{
std::cout << "LOL same term\n";
			if (s1.size() != s2.size()) {
std::cout << "LOL different sizes\n";
				return false;
			}
			piranha_assert(s1.m_ed.get_args_tuple() == s2.m_ed.get_args_tuple());
			const auto it_f = s1.m_container.end();
			const auto it_end = s2.m_container.end();
			for (auto it = s1.m_container.begin(); it != it_f; ++it) {
				const auto tmp_it = s2.m_container.find(*it);
				if (tmp_it == it_end || !tmp_it->m_cf.is_equal_to(it->m_cf,s2.m_ed)) {
					return false;
				}
			}
			return true;
		}
		// Overload with series with different term types. Series1 term type is promoted to
		// Series2 term type.
		template <typename Series1, typename Series2>
		static bool series_equality_impl(const Series1 &s1, const Series2 &s2, typename std::enable_if<
			!std::is_same<typename strip_cv_ref<Series1>::type::term_type,typename strip_cv_ref<Series2>::type::term_type>::value
			>::type * = piranha_nullptr)
		{
std::cout << "LOL different terms\n";
			const auto it_f = s1.m_container.end();
			const auto it_end = s2.m_container.end();
			typedef typename strip_cv_ref<decltype(*it_end)>::type term_type;
			typedef typename term_type::cf_type cf_type;
			typedef typename term_type::key_type key_type;
			decltype(s1.size()) count = 0;
			for (auto it = s1.m_container.begin(); it != it_f; ++it) {
				// NOTE: here there is quite a corner case: namely we cannot exclude
				// that two different source terms do not generate the same converted term,
				// which would cause the same term in s2 to be counted twice.
				// Construct the temporary converted term.
				term_type tmp(cf_type(it->m_cf,s2.m_ed),
					key_type(it->m_key,s2.m_ed.template get_args<term_type>()));
				// Locate the tmp term.
				const auto tmp_it = s2.m_container.find(tmp);
				const bool in_series = (tmp_it != it_end);
				// If a converted term is not in the series, it must be ignorable,
				// otherwise the series are different.
				if (!in_series && !tmp.is_ignorable(s2.m_ed)) {
					return false;
				}
				if (in_series) {
					// The existing term must not be ignorable.
					piranha_assert(!tmp_it->is_ignorable(s2.m_ed));
					if (!tmp_it->m_cf.is_equal_to(tmp.m_cf,s2.m_ed)) {
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
			typedef decltype(s.merge_args(s.m_ed,merge.first)) base_type;
			// Take care of assigning the merged base class.
			(*static_cast<base_type *>(&retval)) = s.merge_args(s.m_ed,merge.first);
			// Assign the echelon descriptor.
			retval.m_ed = std::move(merge.first);
			return retval;
		}
		// Series equality, with coefficient promotion rule promoting first coefficient type to second
		// or coefficients are the same type.
		template <typename Series1, typename Series2>
		static bool series_equality(const Series1 &s1, const Series2 &s2)
		{
			static_assert(echelon_size<typename Series1::term_type>::value == echelon_size<typename Series2::term_type>::value,
				"Cannot compare series with different echelon sizes.");
std::cout << "LOL series comparison\n";
			// Arguments merging.
			if (likely(s1.m_ed.get_args_tuple() == s2.m_ed.get_args_tuple())) {
std::cout << "LOL same args\n";
				return series_equality_impl(s1,s2);
			} else {
std::cout << "LOL different args\n";
				// Let's deal with the first series.
				auto merge1 = s1.m_ed.merge(s2.m_ed);
				const bool s1_needs_copy = merge1.first.get_args_tuple() != s1.m_ed.get_args_tuple();
				auto merge2 = s2.m_ed.merge(merge1.first);
				piranha_assert(merge1.first.get_args_tuple() == merge2.first.get_args_tuple());
				const bool s2_needs_copy = merge2.first.get_args_tuple() != s2.m_ed.get_args_tuple();
				return series_equality_impl(
					s1_needs_copy ? merge_series_for_equality(s1,merge1) : s1,
					s2_needs_copy ? merge_series_for_equality(s2,merge2) : s2
				);
			}
		}
		// Overload for: both series, and coefficient promotion rule promotes first coefficient type to second
		// or coefficients are the same.
		template <typename Series1, typename Series2>
		static bool dispatch_equality(const Series1 &s1, const Series2 &s2, typename std::enable_if<
			std::is_base_of<detail::series_tag,typename strip_cv_ref<Series1>::type>::value &&
			std::is_base_of<detail::series_tag,typename strip_cv_ref<Series2>::type>::value &&
			binary_op_promotion_rule<typename strip_cv_ref<Series1>::type::term_type::cf_type,
			typename strip_cv_ref<Series2>::type::term_type::cf_type>::value>::type * = piranha_nullptr)
		{
std::cout << "LOL series equality 1\n";
			return series_equality(s1,s2);
		}
		// Overload for: both series, and coefficient promotion rule does not promote first coefficient type to second.
		template <typename Series1, typename Series2>
		static bool dispatch_equality(const Series1 &s1, const Series2 &s2, typename std::enable_if<
			std::is_base_of<detail::series_tag,typename strip_cv_ref<Series1>::type>::value &&
			std::is_base_of<detail::series_tag,typename strip_cv_ref<Series2>::type>::value &&
			!binary_op_promotion_rule<typename strip_cv_ref<Series1>::type::term_type::cf_type,
			typename strip_cv_ref<Series2>::type::term_type::cf_type>::value>::type * = piranha_nullptr)
		{
std::cout << "LOL series equality 2\n";
			return series_equality(s2,s1);
		}
#endif
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
		 *   - piranha::series::operator+=() is called on the return value, with either \p s1 or \p s2 forwarded as argument;
		 * - else:
		 *   - the return type is the type of the series operand with largest echelon size;
		 *   - the return value is built from the series operand with largest echelon size;
		 *   - piranha::series::operator+=() is called on the return value;
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
#if 0
		/// Binary multiplication involving piranha::series.
		/**
		 * This template operator is activated iff at least one operand is an instance of piranha::series.
		 * The binary addition algorithm proceeds as follows:
		 * 
		 * - if both operands are series:
		 *   - the return type is \p T if the value of piranha::binary_op_promotion_rule of the coefficient types of \p T and \p U is \p false,
		 *     \p U otherwise;
		 *   - base_series::multiply_by_series() is called on \p s1 or \p s2, depending on the promotion rule above,
		 *     and the result used to build the return value after the echelon descriptors of the two series have been merged as necessary;
		 * - else:
		 *   - the return type is the type of the series operand;
		 *   - the return value is built from the series operand;
		 *   - piranha::series::operator*=() is called on the return value, the non-series operand as argument;
		 * - the return value is returned.
		 * 
		 * Note that in case of two series operands of different type but with same coefficient types, the return type will depend on the order
		 * of the operands.
		 * 
		 * @param[in] s1 first operand.
		 * @param[in] s2 second operand.
		 * 
		 * @return <tt>s1 * s2</tt>.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - copy-construction of return value type,
		 * - piranha::series::operator*=(),
		 * - the assignment operator of piranha::echelon_descriptor,
		 * - piranha::echelon_descriptor::merge_args(),
		 * - piranha::base_series::multiply_by_series(),
		 * - piranha::base_series::merge_args().
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_series_operands<T,U>::value,typename series_op_return_type<T,U>::type>::type operator*(T &&s1, U &&s2)
		{
			return dispatch_binary_multiply(std::forward<T>(s1),std::forward<U>(s2));
		}
		/// Equality operator involving piranha::series.
		/**
		 * This template operator is activated iff at least one operand is an instance of piranha::series.
		 * 
		 * The comparison algorithm operates as follows:
		 * 
		 * - if both operands are instances of piranha::series:
		 *   - if the echelon descriptors of the two series differ, copies of the series
		 *     are created as necessary, and the algorithm proceeds to the comparison of such
		 *     copies instead;
		 *   - if the term types of the two series are the same:
		 *     - the series are equal if their sizes coincide, and if for each term \p t1
		 *       of one series there exist a term \p t2 with the same key in the second series
		 *       and the coefficients of \p t1 and \p t2 are equal (according to the <tt>is_equal_to()</tt>
		 *       method of the coefficient type). Otherwise, the operator will return \p false;
		 *   - else:
		 *     - one of the two series types is chosen for type promotion according to the value of
		 *       piranha::binary_op_promotion_rule on the series' coefficient types;
		 *     - each term \p t1 of the series to be promoted is converted to an instance \p t1c of the other series'
		 *       term type, and a term \p t2 equivalent to \p t1c is located into the
		 *       other series. If each non-ignorable converted term \p t1c is successfully identified in the other series
		 *       (meaning that \p t2 is found with same key and coefficient as \p t1c) and the total count of \p t2 terms
		 *       located in this way is equivalent to the size of the other series, the operator returns \p true.
		 *       Otherwise, the operator will return \p false;
		 * - else:
		 *   - if the size of the series operand is greater than one,\p false is returned;
		 *   - if the size of the series operand is zero, then a coefficient is constructed from
		 *     the non-series operand. If this coefficient is ignorable, then \p true is returned,
		 *     otherwise \p false is returned;
		 *   - if the coefficient of the only term of the series operand is equal to the non-series
		 *     operand (as established by the <tt>is_equal_to()</tt> method of the coefficient type)
		 *     and the key is unitary (as established by the key type's <tt>is_unitary()</tt> method),
		 *     then \p true will be returned, otherwise \p false will be returned.
		 * 
		 * Note that in series comparisons involving series with same coefficient types but different term types,
		 * the promotion rule during term comparisons will depend on the order of the operands.
		 * 
		 * If the series being compared have different echelon sizes, or are piranha::base_series without being
		 * piranha::series, compile-time error messages will be produced.
		 * 
		 * @param[in] s1 first operand.
		 * @param[in] s2 second operand.
		 * 
		 * @return \p true if <tt>s1 == s2</tt>, \p false otherwise.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::echelon_descriptor::merge(),
		 * - piranha::base_series::merge_args(),
		 * - the copy constructors of the series types involved in the comparison,
		 * - the two-arguments constructor (from coefficient and key) of the series' term types,
		 *   and the generic constructors of the coefficient and key types,
		 * - the <tt>is_equal_to()</tt> method of the coefficient types,
		 * - the <tt>is_unitary()</tt> method of the key types,
		 * - the <tt>is_ignorable()</tt> method of the term types,
		 * - piranha::hop_table::find().
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
#endif
};

}

#endif
