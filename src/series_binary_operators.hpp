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
#include "detail/top_level_series_fwd.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Binary operators for piranha::top_level_series.
/**
 * This class provides binary arithmetic and logical operator overloads for piranha::top_level_series instances. The operators
 * are implemented as inline friend functions, enabled only when at least one of the two operands derives from piranha::top_level_series.
 * The class has no data members, and hence has trivial move semantics and provides the strong exception safety guarantee.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
// NOTE: the existence of this class, as opposed to defining the operators directly in top_level_series,
// is that it provides a non-templated scope for the operators - which are declared as inline friends.
// If they were declared as such in top_level_series, ambiguities would arise due to the presence of an extra
// template parameter.
class series_binary_operators
{
		template <typename T, typename U>
		struct series_op_return_type
		{
			typedef typename strip_cv_ref<T>::type type1;
			typedef typename strip_cv_ref<U>::type type2;
			template <typename T1, typename T2, typename Enable = void>
			struct deduction_impl
			{
				typedef typename T1::term_type::cf_type cf_type1;
				typedef typename T2::term_type::cf_type cf_type2;
				typedef typename std::conditional<
					binary_op_promotion_rule<cf_type1,cf_type2>::value,
					cf_type2,
					cf_type1
				>::type result_type;
				// Check that result type is one of the two types.
				static_assert(std::is_same<result_type,cf_type1>::value || std::is_same<result_type,cf_type2>::value,
					"Invalid result type for binary operation on coefficients.");
				typedef typename std::conditional<
					std::is_same<result_type,cf_type1>::value,
					T1,
					T2
				>::type type;
			};
			template <typename T1, typename T2>
			struct deduction_impl<T1,T2,typename std::enable_if<!std::is_base_of<detail::top_level_series_tag,T1>::value || !std::is_base_of<detail::top_level_series_tag,T2>::value>::type>
			{
				typedef typename std::conditional<
					std::is_base_of<detail::top_level_series_tag,T1>::value,
					T1,
					T2
				>::type type;
			};
			typedef typename deduction_impl<type1,type2>::type type;
		};
		template <typename T, typename U>
		struct are_series_operands
		{
			static const bool value = std::is_base_of<detail::top_level_series_tag,typename strip_cv_ref<T>::type>::value ||
				std::is_base_of<detail::top_level_series_tag,typename strip_cv_ref<U>::type>::value;
		};
		template <bool Sign, typename Series, typename T>
		static typename series_op_return_type<Series,T>::type dispatch_binary_add(Series &&s, T &&x,
			typename std::enable_if<!std::is_base_of<detail::top_level_series_tag,typename strip_cv_ref<T>::type>::value>::type * = piranha_nullptr)
		{
std::cout << "MIXED1!!!!\n";
			return mixed_binary_add<Sign,false>(std::forward<Series>(s),std::forward<T>(x));
		}
		template <bool Sign, typename T, typename Series>
		static typename series_op_return_type<T,Series>::type dispatch_binary_add(T &&x, Series &&s,
			typename std::enable_if<!std::is_base_of<detail::top_level_series_tag,typename strip_cv_ref<T>::type>::value>::type * = piranha_nullptr)
		{
std::cout << "MIXED2!!!!\n";
			return mixed_binary_add<Sign,true>(std::forward<Series>(s),std::forward<T>(x));
		}
		template <bool Sign, bool Swapped, typename Series, typename T>
		static typename strip_cv_ref<Series>::type mixed_binary_add(Series &&s, T &&x)
		{
			static_assert(std::is_same<typename strip_cv_ref<Series>::type,typename series_op_return_type<Series,T>::type>::value,"Inconsistent return value type.");
			static_assert(std::is_same<typename strip_cv_ref<Series>::type,typename series_op_return_type<T,Series>::type>::value,"Inconsistent return value type.");
			typename strip_cv_ref<Series>::type retval(std::forward<Series>(s));
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
		// Overload if either:
		// - return type is first series and series types are not the same,
		// - series type are the same and first series is nonconst rvalue ref.
		template <bool Sign, typename Series1, typename Series2>
		static typename series_op_return_type<Series1,Series2>::type series_binary_add(Series1 &&s1, Series2 &&s2, typename std::enable_if<
			(std::is_same<typename series_op_return_type<Series1,Series2>::type,typename strip_cv_ref<Series1>::type>::value &&
			!std::is_same<typename strip_cv_ref<Series1>::type,typename strip_cv_ref<Series2>::type>::value) ||
			(std::is_same<typename strip_cv_ref<Series1>::type,typename strip_cv_ref<Series2>::type>::value &&
			is_nonconst_rvalue_ref<Series1 &&>::value)
			>::type * = piranha_nullptr)
		{
			typename series_op_return_type<Series1,Series2>::type retval(std::forward<Series1>(s1));
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
		static typename series_op_return_type<Series1,Series2>::type series_binary_add(Series1 &&s1, Series2 &&s2, typename std::enable_if<
			(std::is_same<typename series_op_return_type<Series1,Series2>::type,typename strip_cv_ref<Series2>::type>::value &&
			!std::is_same<typename strip_cv_ref<Series1>::type,typename strip_cv_ref<Series2>::type>::value) ||
			(std::is_same<typename strip_cv_ref<Series1>::type,typename strip_cv_ref<Series2>::type>::value &&
			!is_nonconst_rvalue_ref<Series1 &&>::value)
			>::type * = piranha_nullptr)
		{
			typename series_op_return_type<Series1,Series2>::type retval(std::forward<Series2>(s2));
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
		static typename series_op_return_type<Series1,Series2>::type dispatch_binary_add(Series1 &&s1, Series2 &&s2,
			typename std::enable_if<
			std::is_base_of<detail::top_level_series_tag,typename strip_cv_ref<Series1>::type>::value &&
			std::is_base_of<detail::top_level_series_tag,typename strip_cv_ref<Series2>::type>::value
			>::type * = piranha_nullptr)
		{
			return series_binary_add<Sign>(std::forward<Series1>(s1),std::forward<Series2>(s2));
		}
		template <typename Series, typename T>
		static bool mixed_equality(const Series &s, const T &x)
		{
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
			typename std::enable_if<!std::is_base_of<detail::top_level_series_tag,typename strip_cv_ref<T>::type>::value>::type * = piranha_nullptr)
		{
std::cout << "LOL bool1\n";
			return mixed_equality(s,x);
		}
		// Overload for non-series vs series.
		template <typename T, typename Series>
		static bool dispatch_equality(const T &x, const Series &s,
			typename std::enable_if<!std::is_base_of<detail::top_level_series_tag,typename strip_cv_ref<T>::type>::value>::type * = piranha_nullptr)
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
			std::is_base_of<detail::top_level_series_tag,typename strip_cv_ref<Series1>::type>::value &&
			std::is_base_of<detail::top_level_series_tag,typename strip_cv_ref<Series2>::type>::value &&
			binary_op_promotion_rule<typename strip_cv_ref<Series1>::type::term_type::cf_type,
			typename strip_cv_ref<Series2>::type::term_type::cf_type>::value>::type * = piranha_nullptr)
		{
std::cout << "LOL series equality 1\n";
			return series_equality(s1,s2);
		}
		// Overload for: both series, and coefficient promotion rule does not promote first coefficient type to second.
		template <typename Series1, typename Series2>
		static bool dispatch_equality(const Series1 &s1, const Series2 &s2, typename std::enable_if<
			std::is_base_of<detail::top_level_series_tag,typename strip_cv_ref<Series1>::type>::value &&
			std::is_base_of<detail::top_level_series_tag,typename strip_cv_ref<Series2>::type>::value &&
			!binary_op_promotion_rule<typename strip_cv_ref<Series1>::type::term_type::cf_type,
			typename strip_cv_ref<Series2>::type::term_type::cf_type>::value>::type * = piranha_nullptr)
		{
std::cout << "LOL series equality 2\n";
			return series_equality(s2,s1);
		}
	public:
		/// Binary addition involving piranha::top_level_series.
		/**
		 * This template operator is activated iff at least one operand is an instance of piranha::top_level_series.
		 * The binary addition algorithm proceeds as follow:
		 * 
		 * - if both operands are series:
		 *   - the return type is \p T if the value of piranha::binary_op_promotion_rule of the coefficient types of \p T and \p U is \p false,
		 *     \p U otherwise;
		 *   - the return value is built from either \p s1 or \p s2 (depending on its type);
		 *   - piranha::top_level_series::operator+=() is called on the return value, with either \p s1 or \p s2 as argument;
		 * - else:
		 *   - the return type is the type of the series operand;
		 *   - the return value is built from the series operand;
		 *   - piranha::top_level_series::operator+=() is called on the return value, the non-series operand as argument;
		 * - the return value is returned.
		 * 
		 * Note that in case of two series operands of different type but with same coefficient types, the return type will depend on the order
		 * of the operands.
		 * 
		 * @param[in] s1 first operand.
		 * @param[in] s2 second operand.
		 * 
		 * @return <tt>s1 + s2</tt>.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - copy-construction of return value type,
		 * - piranha::top_level_series::operator+=().
		 */
		template <typename T, typename U>
		friend inline typename std::enable_if<are_series_operands<T,U>::value,typename series_op_return_type<T,U>::type>::type operator+(T &&s1, U &&s2)
		{
			return dispatch_binary_add<true>(std::forward<T>(s1),std::forward<U>(s2));
		}
		/// Binary subtraction involving piranha::top_level_series.
		/**
		 * This template operator is activated iff at least one operand is an instance of piranha::top_level_series. The algorithm proceeds in the same
		 * way as operator+(), with a change in sign and possibly a call to piranha::top_level_series::negate().
		 * 
		 * @param[in] s1 first operand.
		 * @param[in] s2 second operand.
		 * 
		 * @return <tt>s1 - s2</tt>.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - copy-construction of return value type,
		 * - piranha::top_level_series::operator-=(),
		 * - piranha::top_level_series::negate().
		 */
		template <typename T, typename U>
		friend inline typename std::enable_if<are_series_operands<T,U>::value,typename series_op_return_type<T,U>::type>::type operator-(T &&s1, U &&s2)
		{
			return dispatch_binary_add<false>(std::forward<T>(s1),std::forward<U>(s2));
		}
		/// Equality operator involving piranha::top_level_series.
		/**
		 * This template operator is activated iff at least one operand is an instance of piranha::top_level_series.
		 * 
		 * The comparison algorithm operates as follow:
		 * 
		 * - if both operands are instances of piranha::top_level_series:
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
		friend inline typename std::enable_if<are_series_operands<T,U>::value,bool>::type operator==(const T &s1, const U &s2)
		{
			return dispatch_equality(s1,s2);
		}
		/// Inequality operator involving piranha::top_level_series.
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
		friend inline typename std::enable_if<are_series_operands<T,U>::value,bool>::type operator!=(const T &s1, const U &s2)
		{
			return !(s1 == s2);
		}
};

}

#endif
