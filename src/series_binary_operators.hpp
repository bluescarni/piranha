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
				// Check that switching the operands does not change result type.
				static_assert(std::is_same<
					result_type,
					typename std::conditional<
					binary_op_promotion_rule<cf_type2,cf_type1>::value,
					cf_type1,
					cf_type2
					>::type>::value,
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
	public:
		/// Binary addition involving piranha::top_level_series.
		/**
		 * This template operator is activated iff at least one operand derives from piranha::top_level_series.
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
		 * Note that in case of two series operands of different type but with same coefficient types the return type will depend on the order
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
		 * This template operator is activated iff at least one operand derives from piranha::top_level_series. The algorithm proceeds in the same
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
};

}

#endif
