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

#ifndef PIRANHA_INTEGER_HPP
#define PIRANHA_INTEGER_HPP

#include <algorithm>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/math/special_functions/trunc.hpp>
#include <boost/numeric/conversion/bounds.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/utility.hpp>
#include <cctype> // For std::isdigit().
#include <cstddef>
#include <cstring>
#include <gmp.h>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "config.hpp" // For (un)likely.
#include "exceptions.hpp"

namespace piranha
{

// TODO: move this in math namespace.
// void negate(integer &n);

/// Arbitrary precision integer class.
/**
 * This class can represent integer numbers of arbitrary size (i.e., the size is limited only by the available memory).
 * The implementation employs the \p mpz_t type from the multiprecision GMP library.
 * 
 * Interoperability with all builtin C++ arithmetic types is provided. Please note that since the GMP API does not provide interoperability
 * with <tt>long long</tt> and <tt>long double</tt>, interaction with this types will be somewhat slow due to the extra workload of converting such types
 * to GMP-compatible types. Also, every function interacting with floating-point types will check that the floating-point values are not
 * non-finite: in case of infinities or NaNs, an <tt>std::invalid_argument</tt> exception will be thrown.
 * 
 * @see http://gmplib.org/
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo implementation notes: use of internal GMP implementation details
 * \todo implement hash via mpz_getlimbn and mpz_size: http://gmplib.org/manual/Integer-Special-Functions.html#Integer-Special-Functions
 * \todo move safety
 * \todo exception safety
 * \todo fix use of noexcept
 * \todo test the swapping arithmetic with a big integer or with operations such as i *= j + k +l
 * \todo test for number of memory allocations
 * \todo exception specifications for in-place operations with integers: document the possible overflow errors.
 * \todo improve interaction with long long via decomposition of operations in long operands
 * \todo improve performance of binary modulo operation when the second argument is a hardware integer
 */
class integer
{
		// Strip T of reference, const and volatile attributes.
		template <typename T>
		struct strip_cv_ref
		{
			typedef typename std::remove_cv<typename std::remove_reference<T>::type>::type type;
		};
		// Function to check that a floating point number is not pathological, in order to shield GMP
		// functions.
		template <typename T>
		static void fp_normal_check(const T &x, typename boost::enable_if<std::is_floating_point<T>>::type * = 0)
		{
			if (!boost::math::isfinite(x)) {
				piranha_throw(std::invalid_argument,"non-finite floating-point number");
			}
		}
		// Convert input long double into text representation of the corresponding truncated integer.
		static std::string ld_to_string(const long double &x)
		{
			piranha_assert(boost::math::isfinite(x));
			boost::format f("%.0Lf");
			f % x;
			return f.str();
		}
		// Validate the string format for integers.
		static void validate_string(const char *str)
		{
			const std::size_t size = std::strlen(str);
			if (!size) {
				piranha_throw(std::invalid_argument,"invalid string input for integer type");
			}
			const std::size_t has_minus = (str[0] == '-'), signed_size = size - has_minus;
			if (!signed_size) {
				piranha_throw(std::invalid_argument,"invalid string input for integer type");
			}
			// A number starting with zero cannot be multi-digit and cannot have a leading minus sign (no '-0' allowed).
			if (str[has_minus] == '0' && (signed_size > 1 || has_minus)) {
				piranha_throw(std::invalid_argument,"invalid string input for integer type");
			}
			// Check that each character is a digit.
			std::for_each(str + has_minus, str + size,[](char c){if (!std::isdigit(c)) {piranha_throw(std::invalid_argument,"invalid string input for integer type");}});
		}
		// Construction.
		void construct_from_string(const char *str)
		{
			validate_string(str);
			// String is OK.
			const int retval = ::mpz_init_set_str(m_value,str,10);
			if (retval == -1) {
				// Clear it and throw.
				::mpz_clear(m_value);
				piranha_throw(std::invalid_argument,"invalid string input for integer type");
			}
			piranha_assert(retval == 0);
		}
		template <typename T>
		void construct_from_arithmetic(const T &x, typename boost::enable_if_c<std::is_floating_point<T>::value && !std::is_same<T,long double>::value>::type * = 0)
		{
			fp_normal_check(x);
			::mpz_init_set_d(m_value,static_cast<double>(x));
		}
		template <typename T>
		void construct_from_arithmetic(const T &si, typename boost::enable_if_c<std::is_integral<T>::value
			&& std::is_signed<T>::value && !std::is_same<T,long long>::value>::type * = 0)
		{
			::mpz_init_set_si(m_value,static_cast<long>(si));
		}
		template <typename T>
		void construct_from_arithmetic(const T &ui, typename boost::enable_if_c<std::is_integral<T>::value
			&& !std::is_signed<T>::value && !std::is_same<T,unsigned long long>::value>::type * = 0)
		{
			::mpz_init_set_ui(m_value,static_cast<unsigned long>(ui));
		}
		template <typename T>
		void construct_from_arithmetic(const T &ll, typename boost::enable_if_c<std::is_same<long long,T>::value || std::is_same<unsigned long long,T>::value>::type * = 0)
		{
			construct_from_string(boost::lexical_cast<std::string>(ll).c_str());
		}
		void construct_from_arithmetic(const long double &x)
		{
			fp_normal_check(x);
			construct_from_string(ld_to_string(x).c_str());
		}
		// Assignment.
		void assign_from_string(const char *str)
		{
			validate_string(str);
			// String is OK.
			const int retval = ::mpz_set_str(m_value,str,10);
			if (retval == -1) {
				piranha_throw(std::invalid_argument,"invalid string input for integer type");
			}
			piranha_assert(retval == 0);
		}
		template <typename T>
		void assign_from_arithmetic(const T &x, typename boost::enable_if_c<std::is_floating_point<T>::value && !std::is_same<T,long double>::value>::type * = 0)
		{
			fp_normal_check(x);
			::mpz_set_d(m_value,static_cast<double>(x));
		}
		template <typename T>
		void assign_from_arithmetic(const T &si, typename boost::enable_if_c<std::is_integral<T>::value
			&& std::is_signed<T>::value && !std::is_same<T,long long>::value>::type * = 0)
		{
			::mpz_set_si(m_value,static_cast<long>(si));
		}
		template <typename T>
		void assign_from_arithmetic(const T &ui, typename boost::enable_if_c<std::is_integral<T>::value
			&& !std::is_signed<T>::value && !std::is_same<T,unsigned long long>::value>::type * = 0)
		{
			::mpz_set_ui(m_value,static_cast<unsigned long>(ui));
		}
		template <typename T>
		void assign_from_arithmetic(const T &ll, typename boost::enable_if_c<std::is_same<long long,T>::value || std::is_same<unsigned long long,T>::value>::type * = 0)
		{
			assign_from_string(boost::lexical_cast<std::string>(ll).c_str());
		}
		void assign_from_arithmetic(const long double &x)
		{
			fp_normal_check(x);
			assign_from_string(ld_to_string(x).c_str());
		}
		// Conversion.
		template <typename T>
		typename boost::enable_if_c<std::is_integral<T>::value && std::is_signed<T>::value &&
			!std::is_same<T,long long>::value && !std::is_same<T,bool>::value,T>::type convert_to() const
		{
			if (::mpz_fits_slong_p(m_value)) {
				try {
					return(boost::numeric_cast<T>(::mpz_get_si(m_value)));
				} catch (const boost::bad_numeric_cast &) {}
			}
			piranha_throw(std::overflow_error,"overflow in conversion to integral type");
		}
		template <typename T>
		typename boost::enable_if_c<std::is_integral<T>::value && !std::is_signed<T>::value &&
			!std::is_same<T,unsigned long long>::value && !std::is_same<T,bool>::value,T>::type convert_to() const
		{
			if (::mpz_fits_ulong_p(m_value)) {
				try {
					return(boost::numeric_cast<T>(::mpz_get_ui(m_value)));
				} catch (const boost::bad_numeric_cast &) {}
			}
			piranha_throw(std::overflow_error,"overflow in conversion to integral type");
		}
		template <typename T>
		typename boost::enable_if_c<std::is_same<long long,T>::value || std::is_same<unsigned long long,T>::value,T>::type convert_to() const
		{
			try {
				return boost::lexical_cast<T>(*this);
			} catch (const boost::bad_lexical_cast &) {
				piranha_throw(std::overflow_error,"overflow in conversion to integral type");
			}
		}
		template <typename T>
		typename boost::enable_if_c<std::is_floating_point<T>::value && !std::is_same<T,long double>::value,T>::type convert_to() const
		{
			// Extract always the double-precision value, and cast as needed.
			// NOTE: here the GMP docs warn that this operation can fail in horrid ways,
			// so far never had problems, but if this becomes an issue we can resort to
			// the good old lexical casting.
			if (::mpz_cmp_d(m_value,static_cast<double>(boost::numeric::bounds<T>::lowest())) < 0) {
				return -std::numeric_limits<T>::infinity();
			} else if (::mpz_cmp_d(m_value,static_cast<double>(boost::numeric::bounds<T>::highest())) > 0) {
				return std::numeric_limits<T>::infinity();
			} else {
				return static_cast<T>(::mpz_get_d(m_value));
			}
		}
		template <typename T>
		typename boost::enable_if_c<std::is_same<T,long double>::value,T>::type convert_to() const
		{
			try {
				return boost::lexical_cast<long double>(*this);
			} catch (const boost::bad_lexical_cast &) {
				// If the conversion fails, it means we are at +-Inf.
				piranha_assert(mpz_sgn(m_value) != 0);
				if (mpz_sgn(m_value) > 0) {
					return std::numeric_limits<long double>::infinity();
				} else {
					return -std::numeric_limits<long double>::infinity();
				}
			}
		}
		// Special handling for bool.
		template <typename T>
		typename boost::enable_if_c<std::is_same<T,bool>::value,T>::type convert_to() const
		{
			return (mpz_sgn(m_value) != 0);
		}
		// In-place addition.
		void in_place_add(const integer &n)
		{
			::mpz_add(m_value,m_value,n.m_value);
		}
		void in_place_add(integer &&n)
		{
			if (n.m_value->_mp_alloc > m_value->_mp_alloc) {
				swap(n);
			}
			in_place_add(n);
		}
		template <typename T>
		void in_place_add(const T &si, typename boost::enable_if_c<std::is_integral<T>::value
			&& std::is_signed<T>::value && !std::is_same<T,long long>::value>::type * = 0)
		{
			if (si >= 0) {
				::mpz_add_ui(m_value,m_value,static_cast<unsigned long>(si));
			} else {
				// Neat trick here. See:
				// http://stackoverflow.com/questions/4536095/unary-minus-and-signed-to-unsigned-conversion
				::mpz_sub_ui(m_value,m_value,-static_cast<unsigned long>(si));
			}
		}
		template <typename T>
		void in_place_add(const T &ui, typename boost::enable_if_c<std::is_integral<T>::value
			&& !std::is_signed<T>::value && !std::is_same<T,unsigned long long>::value>::type * = 0)
		{
			::mpz_add_ui(m_value,m_value,static_cast<unsigned long>(ui));
		}
		// For (unsigned) long long create a temporary integer and add it.
		template <typename T>
		void in_place_add(const T &n, typename boost::enable_if_c<std::is_same<T,long long>::value || std::is_same<T,unsigned long long>::value>::type * = 0)
		{
			in_place_add(integer(n));
		}
		template <typename T>
		void in_place_add(const T &x, typename boost::enable_if<std::is_floating_point<T>>::type * = 0)
		{
			operator=(static_cast<T>(*this) + x);
		}
		// In-place subtraction.
		void in_place_sub(const integer &n)
		{
			::mpz_sub(m_value,m_value,n.m_value);
		}
		void in_place_sub(integer &&n)
		{
			if (n.m_value->_mp_alloc > m_value->_mp_alloc) {
				swap(n);
				::mpz_neg(m_value,m_value);
				in_place_add(n);
			} else {
				in_place_sub(n);
			}
		}
		template <typename T>
		void in_place_sub(const T &si, typename boost::enable_if_c<std::is_integral<T>::value
			&& std::is_signed<T>::value && !std::is_same<T,long long>::value>::type * = 0)
		{
			if (si >= 0) {
				::mpz_sub_ui(m_value,m_value,static_cast<unsigned long>(si));
			} else {
				::mpz_add_ui(m_value,m_value,-static_cast<unsigned long>(si));
			}
		}
		template <typename T>
		void in_place_sub(const T &ui, typename boost::enable_if_c<std::is_integral<T>::value
			&& !std::is_signed<T>::value && !std::is_same<T,unsigned long long>::value>::type * = 0)
		{
			::mpz_sub_ui(m_value,m_value,static_cast<unsigned long>(ui));
		}
		template <typename T>
		void in_place_sub(const T &n, typename boost::enable_if_c<std::is_same<T,long long>::value || std::is_same<T,unsigned long long>::value>::type * = 0)
		{
			in_place_sub(integer(n));
		}
		template <typename T>
		void in_place_sub(const T &x, typename boost::enable_if<std::is_floating_point<T>>::type * = 0)
		{
			operator=(static_cast<T>(*this) - x);
		}
		// In-place multiplication.
		void in_place_mul(const integer &n)
		{
			::mpz_mul(m_value,m_value,n.m_value);
		}
		void in_place_mul(integer &&n)
		{
			if (n.m_value->_mp_alloc > m_value->_mp_alloc) {
				swap(n);
			}
			in_place_mul(n);
		}
		template <typename T>
		void in_place_mul(const T &si, typename boost::enable_if_c<std::is_integral<T>::value
			&& std::is_signed<T>::value && !std::is_same<T,long long>::value>::type * = 0)
		{
			::mpz_mul_si(m_value,m_value,static_cast<long>(si));
		}
		template <typename T>
		void in_place_mul(const T &ui, typename boost::enable_if_c<std::is_integral<T>::value
			&& !std::is_signed<T>::value && !std::is_same<T,unsigned long long>::value>::type * = 0)
		{
			::mpz_mul_ui(m_value,m_value,static_cast<unsigned long>(ui));
		}
		template <typename T>
		void in_place_mul(const T &n, typename boost::enable_if_c<std::is_same<T,long long>::value || std::is_same<T,unsigned long long>::value>::type * = 0)
		{
			in_place_mul(integer(n));
		}
		template <typename T>
		void in_place_mul(const T &x, typename boost::enable_if<std::is_floating_point<T>>::type * = 0)
		{
			operator=(static_cast<T>(*this) * x);
		}
		// In-place division.
		void in_place_div(const integer &n)
		{
			if (unlikely(mpz_sgn(n.m_value) == 0)) {
				piranha_throw(piranha::zero_division_error,"division by zero");
			}
			::mpz_tdiv_q(m_value,m_value,n.m_value);
		}
		template <typename T>
		void in_place_div(const T &si, typename boost::enable_if_c<std::is_integral<T>::value
			&& std::is_signed<T>::value && !std::is_same<T,long long>::value>::type * = 0)
		{
			if (unlikely(si == 0)) {
				piranha_throw(piranha::zero_division_error,"division by zero");
			}
			if (si > 0) {
				::mpz_tdiv_q_ui(m_value,m_value,static_cast<unsigned long>(si));
			} else {
				::mpz_tdiv_q_ui(m_value,m_value,-static_cast<unsigned long>(si));
				::mpz_neg(m_value,m_value);
			}
		}
		template <typename T>
		void in_place_div(const T &ui, typename boost::enable_if_c<std::is_integral<T>::value
			&& !std::is_signed<T>::value && !std::is_same<T,unsigned long long>::value>::type * = 0)
		{
			if (unlikely(ui == 0)) {
				piranha_throw(piranha::zero_division_error,"division by zero");
			}
			::mpz_tdiv_q_ui(m_value,m_value,static_cast<unsigned long>(ui));
		}
		template <typename T>
		void in_place_div(const T &n, typename boost::enable_if_c<std::is_same<T,long long>::value || std::is_same<T,unsigned long long>::value>::type * = 0)
		{
			in_place_div(integer(n));
		}
		template <typename T>
		void in_place_div(const T &x, typename boost::enable_if<std::is_floating_point<T>>::type * = 0)
		{
			if (unlikely(x == 0)) {
				piranha_throw(piranha::zero_division_error,"division by zero");
			}
			operator=(static_cast<T>(*this) / x);
		}
		// In-place modulo.
		void in_place_mod(const integer &n)
		{
			if (unlikely(mpz_sgn(n.m_value) <= 0)) {
				piranha_throw(std::invalid_argument,"non-positive divisor");
			}
			::mpz_mod(m_value,m_value,n.m_value);
		}
		template <typename T>
		void in_place_mod(const T &si, typename boost::enable_if_c<std::is_integral<T>::value
			&& std::is_signed<T>::value && !std::is_same<T,long long>::value>::type * = 0)
		{
			if (unlikely(si <= 0)) {
				piranha_throw(std::invalid_argument,"non-positive divisor");
			}
			*this = ::mpz_fdiv_ui(m_value,static_cast<unsigned long>(si));
		}
		template <typename T>
		void in_place_mod(const T &ui, typename boost::enable_if_c<std::is_integral<T>::value
			&& !std::is_signed<T>::value && !std::is_same<T,unsigned long long>::value>::type * = 0)
		{
			if (unlikely(ui == 0)) {
				piranha_throw(std::invalid_argument,"non-positive divisor");
			}
			*this = ::mpz_fdiv_ui(m_value,static_cast<unsigned long>(ui));
		}
		template <typename T>
		void in_place_mod(const T &n, typename boost::enable_if_c<std::is_same<T,long long>::value || std::is_same<T,unsigned long long>::value>::type * = 0)
		{
			in_place_mod(integer(n));
		}
		// Binary operations.
		// Type trait for allowed arguments in arithmetic binary operations.
		template <typename T, typename U>
		struct are_binary_op_types: std::integral_constant<bool,
			(std::is_same<typename strip_cv_ref<T>::type,integer>::value && std::is_arithmetic<typename strip_cv_ref<U>::type>::value) ||
			(std::is_same<typename strip_cv_ref<U>::type,integer>::value && std::is_arithmetic<typename strip_cv_ref<T>::type>::value) ||
			(std::is_same<typename strip_cv_ref<T>::type,integer>::value && std::is_same<typename strip_cv_ref<U>::type,integer>::value)>
		{};
		// Metaprogramming to establish the return type of binary arithmetic operations involving integers.
		// Default result type will be integer itself; for consistency with C/C++ when one of the arguments
		// is a floating point type, we will return a value of the same floating point type.
		template <typename T, typename U, typename Enable = void>
		struct deduce_binary_op_result_type
		{
			typedef integer type;
		};
		template <typename T, typename U>
		struct deduce_binary_op_result_type<T,U,typename boost::enable_if<std::is_floating_point<typename strip_cv_ref<T>::type>>::type>
		{
			typedef typename strip_cv_ref<T>::type type;
		};
		template <typename T, typename U>
		struct deduce_binary_op_result_type<T,U,typename boost::enable_if<std::is_floating_point<typename strip_cv_ref<U>::type>>::type>
		{
			typedef typename strip_cv_ref<U>::type type;
		};
		// Binary addition.
		template <typename T, typename U>
		static integer binary_plus(T &&n1, U &&n2, typename boost::enable_if_c<
			are_binary_op_types<T,U>::value &&
			!std::is_floating_point<typename strip_cv_ref<T>::type>::value && !std::is_floating_point<typename strip_cv_ref<U>::type>::value &&
			std::is_same<typename strip_cv_ref<T>::type,integer>::value && std::is_rvalue_reference<T &&>::value
			>::type * = 0)
		{
			// NOTE: the logic here is that we can "steal" from n1 we do it here, otherwise we
			// attempt the steal from n2 in the overload below.
			integer retval(std::forward<T>(n1));
			retval += std::forward<U>(n2);
			return retval;
		}
		template <typename T, typename U>
		static integer binary_plus(T &&n1, U &&n2, typename boost::enable_if_c<
			are_binary_op_types<T,U>::value &&
			!std::is_floating_point<typename strip_cv_ref<T>::type>::value && !std::is_floating_point<typename strip_cv_ref<U>::type>::value &&
			!(std::is_same<typename strip_cv_ref<T>::type,integer>::value && std::is_rvalue_reference<T &&>::value)
			>::type * = 0)
		{
			integer retval(std::forward<U>(n2));
			retval += std::forward<T>(n1);
			return retval;
		}
		template <typename T>
		static T binary_plus(const integer &n, const T &x, typename boost::enable_if<std::is_floating_point<T>>::type * = 0)
		{
			return (static_cast<T>(n) + x);
		}
		template <typename T>
		static T binary_plus(const T &x, const integer &n, typename boost::enable_if<std::is_floating_point<T>>::type * = 0)
		{
			return binary_plus(n,x);
		}
		// Binary subtraction.
		template <typename T, typename U>
		static integer binary_minus(T &&n1, U &&n2, typename boost::enable_if_c<
			are_binary_op_types<T,U>::value &&
			!std::is_floating_point<typename strip_cv_ref<T>::type>::value && !std::is_floating_point<typename strip_cv_ref<U>::type>::value &&
			std::is_same<typename strip_cv_ref<T>::type,integer>::value && std::is_rvalue_reference<T &&>::value
			>::type * = 0)
		{
			integer retval(std::forward<T>(n1));
			retval -= std::forward<U>(n2);
			return retval;
		}
		template <typename T, typename U>
		static integer binary_minus(T &&n1, U &&n2, typename boost::enable_if_c<
			are_binary_op_types<T,U>::value &&
			!std::is_floating_point<typename strip_cv_ref<T>::type>::value && !std::is_floating_point<typename strip_cv_ref<U>::type>::value &&
			!(std::is_same<typename strip_cv_ref<T>::type,integer>::value && std::is_rvalue_reference<T &&>::value)
			>::type * = 0)
		{
			integer retval(std::forward<U>(n2));
			::mpz_neg(retval.m_value,retval.m_value);
			retval += std::forward<T>(n1);
			return retval;
		}
		template <typename T>
		static T binary_minus(const integer &n, const T &x, typename boost::enable_if<std::is_floating_point<T>>::type * = 0)
		{
			return (static_cast<T>(n) - x);
		}
		template <typename T>
		static T binary_minus(const T &x, const integer &n, typename boost::enable_if<std::is_floating_point<T>>::type * = 0)
		{
			return -binary_minus(n,x);
		}
		// Binary multiplication.
		template <typename T, typename U>
		static integer binary_mul(T &&n1, U &&n2, typename boost::enable_if_c<
			are_binary_op_types<T,U>::value &&
			!std::is_floating_point<typename strip_cv_ref<T>::type>::value && !std::is_floating_point<typename strip_cv_ref<U>::type>::value &&
			std::is_same<typename strip_cv_ref<T>::type,integer>::value && std::is_rvalue_reference<T &&>::value
			>::type * = 0)
		{
			integer retval(std::forward<T>(n1));
			retval *= std::forward<U>(n2);
			return retval;
		}
		template <typename T, typename U>
		static integer binary_mul(T &&n1, U &&n2, typename boost::enable_if_c<
			are_binary_op_types<T,U>::value &&
			!std::is_floating_point<typename strip_cv_ref<T>::type>::value && !std::is_floating_point<typename strip_cv_ref<U>::type>::value &&
			!(std::is_same<typename strip_cv_ref<T>::type,integer>::value && std::is_rvalue_reference<T &&>::value)
			>::type * = 0)
		{
			integer retval(std::forward<U>(n2));
			retval *= std::forward<T>(n1);
			return retval;
		}
		template <typename T>
		static T binary_mul(const integer &n, const T &x, typename boost::enable_if<std::is_floating_point<T>>::type * = 0)
		{
			return (static_cast<T>(n) * x);
		}
		template <typename T>
		static T binary_mul(const T &x, const integer &n, typename boost::enable_if<std::is_floating_point<T>>::type * = 0)
		{
			return binary_mul(n,x);
		}
		// Binary division.
		template <typename T, typename U>
		static integer binary_div(T &&n1, U &&n2, typename boost::enable_if_c<
			are_binary_op_types<T,U>::value &&
			!std::is_floating_point<typename strip_cv_ref<T>::type>::value && !std::is_floating_point<typename strip_cv_ref<U>::type>::value
			>::type * = 0)
		{
			// NOTE: here it makes sense only to attempt to steal n1's storage, not n2's, because the operation is not commutative.
			integer retval(std::forward<T>(n1));
			retval /= std::forward<U>(n2);
			return retval;
		}
		template <typename T>
		static T binary_div(const integer &n, const T &x, typename boost::enable_if<std::is_floating_point<T>>::type * = 0)
		{
			if (unlikely(x == 0)) {
				piranha_throw(piranha::zero_division_error,"division by zero");
			}
			return (static_cast<T>(n) / x);
		}
		template <typename T>
		static T binary_div(const T &x, const integer &n, typename boost::enable_if<std::is_floating_point<T>>::type * = 0)
		{
			const T n_T = static_cast<T>(n);
			if (unlikely(n_T == 0)) {
				piranha_throw(piranha::zero_division_error,"division by zero");
			}
			return (x / n_T);
		}
		// Binary modulo operation.
		template <typename T, typename U>
		static integer binary_mod(T &&n1, U &&n2, typename boost::enable_if_c<
			are_binary_op_types<T,U>::value &&
			!std::is_floating_point<typename strip_cv_ref<T>::type>::value && !std::is_floating_point<typename strip_cv_ref<U>::type>::value
			>::type * = 0)
		{
			integer retval(std::forward<T>(n1));
			retval %= std::forward<U>(n2);
			return retval;
		}
		// Binary equality.
		static bool binary_equality(const integer &n1, const integer &n2)
		{
			return (::mpz_cmp(n1.m_value,n2.m_value) == 0);
		}
		template <typename T>
		static bool binary_equality(const integer &n1, const T &n2,typename boost::enable_if_c<std::is_integral<T>::value &&
			std::is_signed<T>::value && !std::is_same<T,long long>::value>::type * = 0)
		{
			return (mpz_cmp_si(n1.m_value,static_cast<long>(n2)) == 0);
		}
		template <typename T>
		static bool binary_equality(const integer &n1, const T &n2,typename boost::enable_if_c<std::is_integral<T>::value &&
			!std::is_signed<T>::value && !std::is_same<T,unsigned long long>::value>::type * = 0)
		{
			return (mpz_cmp_ui(n1.m_value,static_cast<unsigned long>(n2)) == 0);
		}
		template <typename T>
		static bool binary_equality(const integer &n1, const T &n2, typename boost::enable_if_c<std::is_same<T,long long>::value ||
			std::is_same<T,unsigned long long>::value>::type * = 0)
		{
			return binary_equality(n1,integer(n2));
		}
		template <typename T>
		static bool binary_equality(const integer &n, const T &x,typename boost::enable_if<std::is_floating_point<T>>::type * = 0)
		{
			return (static_cast<T>(n) == x);
		}
		template <typename T>
		static bool binary_equality(const T &x, const integer &n, typename boost::enable_if<std::is_arithmetic<T>>::type * = 0)
		{
			return binary_equality(n,x);
		}
		// Binary less-than.
		static bool binary_less_than(const integer &n1, const integer &n2)
		{
			return (::mpz_cmp(n1.m_value,n2.m_value) < 0);
		}
		template <typename T>
		static bool binary_less_than(const integer &n1, const T &n2,typename boost::enable_if_c<std::is_integral<T>::value &&
			std::is_signed<T>::value && !std::is_same<T,long long>::value>::type * = 0)
		{
			return (mpz_cmp_si(n1.m_value,static_cast<long>(n2)) < 0);
		}
		template <typename T>
		static bool binary_less_than(const integer &n1, const T &n2,typename boost::enable_if_c<std::is_integral<T>::value &&
			!std::is_signed<T>::value && !std::is_same<T,unsigned long long>::value>::type * = 0)
		{
			return (mpz_cmp_ui(n1.m_value,static_cast<unsigned long>(n2)) < 0);
		}
		template <typename T>
		static bool binary_less_than(const integer &n1, const T &n2, typename boost::enable_if_c<std::is_same<T,long long>::value ||
			std::is_same<T,unsigned long long>::value>::type * = 0)
		{
			return binary_less_than(n1,integer(n2));
		}
		template <typename T>
		static bool binary_less_than(const integer &n, const T &x,typename boost::enable_if<std::is_floating_point<T>>::type * = 0)
		{
			return (static_cast<T>(n) < x);
		}
		// Binary less-than or equal.
		static bool binary_leq(const integer &n1, const integer &n2)
		{
			return (::mpz_cmp(n1.m_value,n2.m_value) <= 0);
		}
		template <typename T>
		static bool binary_leq(const integer &n1, const T &n2,typename boost::enable_if_c<std::is_integral<T>::value &&
			std::is_signed<T>::value && !std::is_same<T,long long>::value>::type * = 0)
		{
			return (mpz_cmp_si(n1.m_value,static_cast<long>(n2)) <= 0);
		}
		template <typename T>
		static bool binary_leq(const integer &n1, const T &n2,typename boost::enable_if_c<std::is_integral<T>::value &&
			!std::is_signed<T>::value && !std::is_same<T,unsigned long long>::value>::type * = 0)
		{
			return (mpz_cmp_ui(n1.m_value,static_cast<unsigned long>(n2)) <= 0);
		}
		template <typename T>
		static bool binary_leq(const integer &n1, const T &n2, typename boost::enable_if_c<std::is_same<T,long long>::value ||
			std::is_same<T,unsigned long long>::value>::type * = 0)
		{
			return binary_leq(n1,integer(n2));
		}
		template <typename T>
		static bool binary_leq(const integer &n, const T &x,typename boost::enable_if<std::is_floating_point<T>>::type * = 0)
		{
			return (static_cast<T>(n) <= x);
		}
		// Inverse forms of less-than and leq.
		template <typename T>
		static bool binary_less_than(const T &x, const integer &n, typename boost::enable_if<std::is_arithmetic<T>>::type * = 0)
		{
			return !binary_leq(n,x);
		}
		template <typename T>
		static bool binary_leq(const T &x, const integer &n, typename boost::enable_if<std::is_arithmetic<T>>::type * = 0)
		{
			return !binary_less_than(n,x);
		}
		// Exponentiation.
		template <typename T>
		integer pow_impl(const T &ui, typename boost::enable_if_c<std::is_integral<T>::value && !std::is_signed<T>::value>::type * = 0) const
		{
			unsigned long exp;
			try {
				exp = boost::numeric_cast<unsigned long>(ui);
			} catch (const boost::numeric::bad_numeric_cast &) {
				piranha_throw(std::invalid_argument,"invalid argument for integer exponentiation");
			}
			integer retval;
			::mpz_pow_ui(retval.m_value,m_value,exp);
			return retval;
		}
		template <typename T>
		integer pow_impl(const T &si, typename boost::enable_if_c<std::is_integral<T>::value && std::is_signed<T>::value>::type * = 0) const
		{
			if (si >= 0) {
				return pow_impl(static_cast<typename std::make_unsigned<T>::type>(si));
			} else {
				if (*this == 0) {
					piranha_throw(piranha::zero_division_error,"negative exponentiation of zero");
				}
				return (1 / *this).pow(-static_cast<typename std::make_unsigned<T>::type>(si));
			}
		}
		integer pow_impl(const integer &n) const
		{
			unsigned long exp;
			try {
				exp = static_cast<unsigned long>(n);
			} catch (const std::overflow_error &) {
				piranha_throw(std::invalid_argument,"invalid argument for integer exponentiation");
			}
			return pow_impl(exp);
		}
		template <typename T>
		integer pow_impl(const T &x, typename boost::enable_if<std::is_floating_point<T>>::type * = 0) const
		{
			if (!boost::math::isfinite(x) || boost::math::trunc(x) != x) {
				piranha_throw(std::invalid_argument,"invalid argument for integer exponentiation");
			}
			unsigned long exp;
			try {
				exp = boost::numeric_cast<unsigned long>((x >= 0) ? x : -x);
			} catch (const boost::numeric::bad_numeric_cast &) {
				piranha_throw(std::invalid_argument,"invalid argument for integer exponentiation");
			}
			if (x >= 0) {
				return pow_impl(exp);
			} else {
				if (*this == 0) {
					piranha_throw(piranha::zero_division_error,"negative exponentiation of zero");
				}
				return (1 / *this).pow(exp);
			}
		}
	public:
		/// Default constructor.
		/**
		 * The value is initialised to zero.
		 */
		integer() piranha_noexcept(true)
		{
			::mpz_init(m_value);
		}
		/// Copy constructor.
		/**
		 * @param[in] other integer to be deep-copied.
		 */
		integer(const integer &other) piranha_noexcept(true)
		{
			::mpz_init_set(m_value,other.m_value);
		}
		/// Move constructor.
		/**
		 * @param[in] other integer to be moved.
		 */
		integer(integer &&other) piranha_noexcept(true)
		{
			m_value->_mp_size = other.m_value->_mp_size;
			m_value->_mp_d = other.m_value->_mp_d;
			m_value->_mp_alloc = other.m_value->_mp_alloc;
			// Erase other.
			other.m_value->_mp_size = 0;
			other.m_value->_mp_d = 0;
			other.m_value->_mp_alloc = 0;
		}
		/// Generic constructor from arithmetic types.
		/**
		 * The supported types for \p T are all arithmetic types.
		 * Use of other types will result in a compile-time error.
		 * In case a floating-point type is used, \p x will be truncated (i.e., rounded towards zero) before being used to construct
		 * the integer object.
		 * 
		 * @param[in] x arithmetic type used to construct \p this.
		 * 
		 * @throw std::invalid_argument if \p x is a non-finite floating-point number.
		 */
		template <typename T>
		explicit integer(const T &x, typename boost::enable_if<std::is_arithmetic<T>>::type * = 0)
		{
			construct_from_arithmetic(x);
		}
		/// Constructor from string.
		/**
		 * The string must be a sequence of decimal digits, preceded by a minus sign for
		 * strictly negative numbers. The first digit of a non-zero number must not be zero. A malformed string will throw an \p std::invalid_argument
		 * exception.
		 * 
		 * @param[in] str decimal string representation of the number used to initialise the integer object.
		 * 
		 * @throws std::invalid_argument if the string is malformed.
		 */
		explicit integer(const std::string &str)
		{
			construct_from_string(str.c_str());
		}
		/// Constructor from C string.
		/**
		 * @param[in] str decimal string representation of the number used to initialise the integer object.
		 * 
		 * @see integer(const std::string &)
		 */
		explicit integer(const char *str)
		{
			construct_from_string(str);
		}
		/// Destructor.
		/**
		 * Will clear the internal \p mpz_t type.
		 */
		~integer()
		{
			if (m_value->_mp_d) {
				::mpz_clear(m_value);
			} else {
				piranha_assert(m_value->_mp_size == 0 && m_value->_mp_alloc == 0);
			}
		}
		/// Move assignment operator.
		/**
		 * @param[in] other integer to be moved.
		 * 
		 * @return reference to \p this.
		 */
		integer &operator=(integer &&other) piranha_noexcept(true)
		{
			swap(other);
			return *this;
		}
		/// Assignment operator.
		/**
		 * @param[in] other integer to be assigned.
		 * 
		 * @return reference to \p this.
		 */
		integer &operator=(const integer &other) piranha_noexcept(true)
		{
			if (this != boost::addressof(other)) {
				// Handle assignment to moved-from objects.
				if (m_value->_mp_d) {
					::mpz_set(m_value,other.m_value);
				} else {
					piranha_assert(m_value->_mp_size == 0 && m_value->_mp_alloc == 0);
					::mpz_init_set(m_value,other.m_value);
				}
			}
			return *this;
		}
		/// Assignment operator from string.
		/**
		 * @param[in] str string representation of the integer to be assigned.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws std::invalid_argument if the string is malformed.
		 */
		integer &operator=(const std::string &str)
		{
			return operator=(str.c_str());
		}
		/// Assignment operator from C string.
		/**
		 * @param[in] str string representation of the integer to be assigned.
		 * 
		 * @return reference to \p this.
		 * 
		 * @see operator=(const std::string &)
		 */
		integer &operator=(const char *str)
		{
			if (m_value->_mp_d) {
				assign_from_string(str);
			} else {
				piranha_assert(m_value->_mp_size == 0 && m_value->_mp_alloc == 0);
				construct_from_string(str);
			}
			return *this;
		}
		/// Generic assignment from arithmetic types.
		/**
		 * The supported types for \p T are all arithmetic types.
		 * Use of other types will result in a compile-time error.
		 * In case a floating-point type is used, \p x will be truncated (i.e., rounded towards zero) before being used to construct
		 * the integer object.
		 * 
		 * @param[in] x arithmetic type that will be assigned to \p this.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws std::invalid_argument if \p x is a non-finite floating-point number.
		 */
		template <typename T>
		typename boost::enable_if_c<std::is_arithmetic<T>::value,integer &>::type operator=(const T &x) piranha_noexcept(!std::is_floating_point<T>::value)
		{
			if (m_value->_mp_d) {
				assign_from_arithmetic(x);
			} else {
				piranha_assert(m_value->_mp_size == 0 && m_value->_mp_alloc == 0);
				construct_from_arithmetic(x);
			}
			return *this;
		}
		/// Swap.
		/**
		 * Swap the content of \p this and \p n.
		 * 
		 * @param[in] n swap argument.
		 */
		void swap(integer &n) piranha_noexcept(true)
		{
			::mpz_swap(m_value,n.m_value);
		}
		/// Conversion operator to arithmetic type.
		/**
		 * Extract an instance of arithmetic type \p T from \p this.
		 * 
		 * Conversion to \p bool is always successful, and returns <tt>this != 0</tt>.
		 * Conversion to the other integral types is exact, its success depending on whether or not
		 * the target type can represent the current value of the integer.
		 * 
		 * Conversion to floating point types is exact if the target type can represent exactly the current value of the integer.
		 * If that is not the case, the output value will be one of the two adjacents (with an unspecified rounding direction).
		 * Return values of +-inf will be produced if the current value of the integer overflows the range of the floating-point type.
		 * 
		 * If \p T is not an arithmetic type, a compile-time error will be produced.
		 * 
		 * @return result of the conversion to target type T.
		 * 
		 * @throws std::overflow_error if the conversion to an integral type other than bool results in (negative) overflow.
		 */
		template <typename T>
		explicit operator T() const piranha_noexcept((std::is_same<typename std::remove_cv<T>::type,bool>::value ||
			std::is_floating_point<typename std::remove_cv<T>::type>::value))
		{
			static_assert(std::is_arithmetic<typename std::remove_cv<T>::type>::value,"Cannot convert to non-arithmetic type.");
			return convert_to<typename std::remove_cv<T>::type>();
		}
		/// In-place addition.
		/**
		 * Add \p x to the current value of the integer object. This template operator is activated only if
		 * \p T is either integer or an arithmetic type.
		 * 
		 * If \p T is integer or an integral type, the result will be exact. If \p T is a floating-point type, the following
		 * sequence of operations takes place:
		 * 
		 * - \p this is converted to an instance \p f of type \p T via the conversion operator,
		 * - \p f is added to \p x,
		 * - the result is assigned back to \p this.
		 * 
		 * @param[in] x argument for the addition.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws std::invalid_argument if \p T is a floating-point type and the result of the operation generates a non-finite value.
		 */
		template <typename T>
		typename boost::enable_if_c<
			std::is_arithmetic<typename strip_cv_ref<T>::type>::value ||
			std::is_same<integer,typename strip_cv_ref<T>::type>::value,integer &>::type operator+=(T &&x)
			piranha_noexcept(!std::is_floating_point<typename strip_cv_ref<T>::type>::value)
		{
			in_place_add(std::forward<T>(x));
			return *this;
		}
		/// Generic in-place addition with piranha::integer.
		/**
		 * Add a piranha::integer in-place. This template operator is activated only if \p T is an arithmetic type and \p I is piranha::integer.
		 * This method will first compute <tt>n + x</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] n second argument.
		 * 
		 * @return reference to \p x.
		 */
		template <typename T, typename I>
		friend inline typename boost::enable_if_c<std::is_arithmetic<T>::value && std::is_same<typename strip_cv_ref<I>::type,integer>::value,T &>::type
			operator+=(T &x, I &&n)
		{
			x = static_cast<T>(std::forward<I>(n) + x);
			return x;
		}
		/// Generic binary addition involving piranha::integer.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::integer and \p U is an arithmetic type,
		 * - \p U is piranha::integer and \p T is an arithmetic type,
		 * - both \p T and \p U are piranha::integer.
		 * 
		 * If no floating-point types are involved, the exact result of the operation will be returned as a piranha::integer.
		 * 
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and combined with \p f to generate the return value, wich will then be of type \p F.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x + y</tt>.
		 */
		template <typename T, typename U>
		friend inline typename boost::enable_if_c<are_binary_op_types<T,U>::value,typename deduce_binary_op_result_type<T,U>::type>::type
			operator+(T &&x, U &&y)
		{
			return binary_plus(std::forward<T>(x),std::forward<U>(y));
		}
		/// Identity operation.
		/**
		 * @return reference to \p this.
		 */
		integer &operator+()
		{
			return *this;
		}
		/// Identity operation (const version).
		/**
		 * @return const reference to \p this.
		 */
		const integer &operator+() const
		{
			return *this;
		}
		/// Prefix increment.
		/**
		 * Increment \p this by one.
		 * 
		 * @return reference to \p this after the increment.
		 */
		integer &operator++()
		{
			return operator+=(1);
		}
		/// Suffix increment.
		/**
		 * Increment \p this by one and return a copy of \p this as it was before the increment.
		 * 
		 * @return copy of \p this before the increment.
		 */
		integer operator++(int)
		{
			const integer retval(*this);
			++(*this);
			return retval;
		}
		/// In-place subtraction.
		/**
		 * The same rules described in operator+=() apply.
		 * 
		 * @param[in] x argument for the subtraction.
		 * 
		 * @return reference to \p this.
		 */
		template <typename T>
		typename boost::enable_if_c<
			std::is_arithmetic<typename strip_cv_ref<T>::type>::value ||
			std::is_same<integer,typename strip_cv_ref<T>::type>::value,integer &>::type operator-=(T &&x)
			piranha_noexcept(!std::is_floating_point<typename strip_cv_ref<T>::type>::value)
		{
			in_place_sub(std::forward<T>(x));
			return *this;
		}
		/// Generic in-place subtraction with piranha::integer.
		/**
		 * Subtract a piranha::integer in-place. This template operator is activated only if \p T is an arithmetic type and \p I is piranha::integer.
		 * This method will first compute <tt>x - n</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] n second argument.
		 * 
		 * @return reference to \p x.
		 */
		template <typename T, typename I>
		friend inline typename boost::enable_if_c<std::is_arithmetic<T>::value && std::is_same<typename strip_cv_ref<I>::type,integer>::value,T &>::type
			operator-=(T &x, I &&n)
		{
			x = static_cast<T>(x - std::forward<I>(n));
			return x;
		}
		/// Generic binary subtraction involving piranha::integer.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::integer and \p U is an arithmetic type,
		 * - \p U is piranha::integer and \p T is an arithmetic type,
		 * - both \p T and \p U are piranha::integer.
		 * 
		 * If no floating-point types are involved, the exact result of the operation will be returned as a piranha::integer.
		 * 
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and combined with \p f to generate the return value, wich will then be of type \p F.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x - y</tt>.
		 */
		template <typename T, typename U>
		friend inline typename boost::enable_if_c<are_binary_op_types<T,U>::value,typename deduce_binary_op_result_type<T,U>::type>::type
			operator-(T &&x, U &&y)
		{
			return binary_minus(std::forward<T>(x),std::forward<U>(y));
		}
		/// In-place negation.
		/**
		 * Set \p this to \p -this.
		 */
		void negate()
		{
			::mpz_neg(m_value,m_value);
		}
		/// Negated copy.
		/**
		 * @return copy of \p -this.
		 */
		integer operator-() const
		{
			integer retval(*this);
			retval.negate();
			return retval;
		}
		/// Prefix decrement.
		/**
		 * Decrement \p this by one and return.
		 * 
		 * @return reference to \p this.
		 */
		integer &operator--()
		{
			return operator-=(1);
		}
		/// Suffix decrement.
		/**
		 * Decrement \p this by one and return a copy of \p this as it was before the decrement.
		 * 
		 * @return copy of \p this before the decrement.
		 */
		integer operator--(int)
		{
			const integer retval(*this);
			--(*this);
			return retval;
		}
		/// In-place multiplication.
		/**
		 * The same rules described in operator+=() apply.
		 * 
		 * @param[in] x argument for the multiplication.
		 * 
		 * @return reference to \p this.
		 */
		template <typename T>
		typename boost::enable_if_c<
			std::is_arithmetic<typename strip_cv_ref<T>::type>::value ||
			std::is_same<integer,typename strip_cv_ref<T>::type>::value,integer &>::type operator*=(T &&x)
			piranha_noexcept(!std::is_floating_point<typename strip_cv_ref<T>::type>::value)
		{
			in_place_mul(std::forward<T>(x));
			return *this;
		}
		/// Generic in-place multiplication with piranha::integer.
		/**
		 * Multiply by a piranha::integer in-place. This template operator is activated only if \p T is an arithmetic type and \p I is piranha::integer.
		 * This method will first compute <tt>n * x</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] n second argument.
		 * 
		 * @return reference to \p x.
		 */
		template <typename T, typename I>
		friend inline typename boost::enable_if_c<std::is_arithmetic<T>::value && std::is_same<typename strip_cv_ref<I>::type,integer>::value,T &>::type
			operator*=(T &x, I &&n)
		{
			x = static_cast<T>(std::forward<I>(n) * x);
			return x;
		}
		/// Generic binary multiplication involving piranha::integer.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::integer and \p U is an arithmetic type,
		 * - \p U is piranha::integer and \p T is an arithmetic type,
		 * - both \p T and \p U are piranha::integer.
		 * 
		 * If no floating-point types are involved, the exact result of the operation will be returned as a piranha::integer.
		 * 
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and combined with \p f to generate the return value, wich will then be of type \p F.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x * y</tt>.
		 */
		template <typename T, typename U>
		friend inline typename boost::enable_if_c<are_binary_op_types<T,U>::value,typename deduce_binary_op_result_type<T,U>::type>::type
			operator*(T &&x, U &&y)
		{
			return binary_mul(std::forward<T>(x),std::forward<U>(y));
		}
		/// In-place division.
		/**
		 * The same rules described in operator+=() apply.
		 * Division by integer or by an integral type is truncated.
		 * Trying to divide by zero will throw a piranha::zero_division_error exception.
		 * 
		 * @param[in] x argument for the division.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws piranha::zero_division_error if <tt>x == 0</tt>.
		 */
		template <typename T>
		typename boost::enable_if_c<
			std::is_arithmetic<typename strip_cv_ref<T>::type>::value ||
			std::is_same<integer,typename strip_cv_ref<T>::type>::value,integer &>::type operator/=(T &&x)
			piranha_noexcept(!std::is_floating_point<typename strip_cv_ref<T>::type>::value)
		{
			in_place_div(std::forward<T>(x));
			return *this;
		}
		/// Generic in-place division with piranha::integer.
		/**
		 * Divide by a piranha::integer in-place. This template operator is activated only if \p T is an arithmetic type and \p I is piranha::integer.
		 * This method will first compute <tt>x / n</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] n second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws piranha::zero_division_error if <tt>n == 0</tt>.
		 */
		template <typename T, typename I>
		friend inline typename boost::enable_if_c<std::is_arithmetic<T>::value && std::is_same<typename strip_cv_ref<I>::type,integer>::value,T &>::type
			operator/=(T &x, I &&n)
		{
			x = static_cast<T>(x / std::forward<I>(n));
			return x;
		}
		/// Generic binary division involving piranha::integer.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::integer and \p U is an arithmetic type,
		 * - \p U is piranha::integer and \p T is an arithmetic type,
		 * - both \p T and \p U are piranha::integer.
		 * 
		 * If no floating-point types are involved, the result of the operation, truncated to zero, will be returned as a piranha::integer.
		 * 
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and combined with \p f to generate the return value, wich will then be of type \p F.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x / y</tt>.
		 * 
		 * @throws piranha::zero_division_error if <tt>y == 0</tt>.
		 */
		template <typename T, typename U>
		friend inline typename boost::enable_if_c<are_binary_op_types<T,U>::value,typename deduce_binary_op_result_type<T,U>::type>::type
			operator/(T &&x, U &&y)
		{
			return binary_div(std::forward<T>(x),std::forward<U>(y));
		}
		/// In-place modulo operation.
		/**
		 * Sets \p this to <tt>this % n</tt>. This template operator is enabled if \p T is piranha::integer or an integral type.
		 * \p this must be non-negative and \p n strictly positive, otherwise an \p std::invalid_argument exception will be thrown.
		 * 
		 * @param[in] n argument for the modulo operation.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws std::invalid_argument if <tt>n <= 0</tt> or <tt>this < 0</tt>.
		 */
		template <typename T>
		typename boost::enable_if_c<
			std::is_integral<typename strip_cv_ref<T>::type>::value ||
			std::is_same<integer,typename strip_cv_ref<T>::type>::value,integer &>::type operator%=(T &&n)
		{
			if (unlikely(mpz_sgn(m_value) < 0)) {
				piranha_throw(std::invalid_argument,"negative dividend");
			}
			in_place_mod(std::forward<T>(n));
			return *this;
		}
		/// Generic in-place modulo operation with piranha::integer.
		/**
		 * Apply the modulo operation by a piranha::integer in-place. This template operator is activated only if \p T is an integral type and \p I is piranha::integer.
		 * This method will first compute <tt>x % n</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] n second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws std::invalid_argument if <tt>n <= 0</tt> or <tt>x < 0</tt>.
		 */
		template <typename T, typename I>
		friend inline typename boost::enable_if_c<std::is_integral<T>::value && std::is_same<typename strip_cv_ref<I>::type,integer>::value,T &>::type
			operator%=(T &x, I &&n)
		{
			x = static_cast<T>(x % std::forward<I>(n));
			return x;
		}
		/// Generic binary modulo operation involving piranha::integer.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::integer and \p U is an integral type,
		 * - \p U is piranha::integer and \p T is an integral type,
		 * - both \p T and \p U are piranha::integer.
		 * 
		 * The result is always a non-negative piranha::integer.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x % y</tt>.
		 * 
		 * @throws std::invalid_argument if <tt>y <= 0</tt> or <tt>x < 0</tt>.
		 */
		template <typename T, typename U>
		friend inline typename boost::enable_if_c<
			are_binary_op_types<T,U>::value && !std::is_floating_point<typename strip_cv_ref<T>::type>::value && !std::is_floating_point<typename strip_cv_ref<U>::type>::value,
			typename deduce_binary_op_result_type<T,U>::type>::type
			operator%(T &&x, U &&y)
		{
			return binary_mod(std::forward<T>(x),std::forward<U>(y));
		}
		/// Generic equality operator involving piranha::integer.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::integer and \p U is an arithmetic type,
		 * - \p U is piranha::integer and \p T is an arithmetic type,
		 * - both \p T and \p U are piranha::integer.
		 * 
		 * If no floating-point types are involved, the exact result of the comparison will be returned.
		 * 
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and compared to \p f to generate the return value.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x == y</tt>, \p false otherwise.
		 */
		template <typename T, typename U>
		friend inline typename boost::enable_if_c<are_binary_op_types<T,U>::value,bool>::type operator==(const T &x, const U &y)
		{
			return binary_equality(x,y);
		}
		/// Generic inequality operator involving piranha::integer.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::integer and \p U is an arithmetic type,
		 * - \p U is piranha::integer and \p T is an arithmetic type,
		 * - both \p T and \p U are piranha::integer.
		 * 
		 * If no floating-point types are involved, the exact result of the comparison will be returned.
		 * 
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and compared to \p f to generate the return value.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x != y</tt>, \p false otherwise.
		 */
		template <typename T, typename U>
		friend inline typename boost::enable_if_c<are_binary_op_types<T,U>::value,bool>::type operator!=(const T &x, const U &y)
		{
			return !(x == y);
		}
		/// Generic less-than operator involving piranha::integer.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::integer and \p U is an arithmetic type,
		 * - \p U is piranha::integer and \p T is an arithmetic type,
		 * - both \p T and \p U are piranha::integer.
		 * 
		 * If no floating-point types are involved, the exact result of the comparison will be returned.
		 * 
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and compared to \p f to generate the return value.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x < y</tt>, \p false otherwise.
		 */
		template <typename T, typename U>
		friend inline typename boost::enable_if_c<are_binary_op_types<T,U>::value,bool>::type operator<(const T &x, const U &y)
		{
			return binary_less_than(x,y);
		}
		/// Generic less-than or equal operator involving piranha::integer.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::integer and \p U is an arithmetic type,
		 * - \p U is piranha::integer and \p T is an arithmetic type,
		 * - both \p T and \p U are piranha::integer.
		 * 
		 * If no floating-point types are involved, the exact result of the comparison will be returned.
		 * 
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and compared to \p f to generate the return value.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x <= y</tt>, \p false otherwise.
		 */
		template <typename T, typename U>
		friend inline typename boost::enable_if_c<are_binary_op_types<T,U>::value,bool>::type operator<=(const T &x, const U &y)
		{
			return binary_leq(x,y);
		}
		/// Generic greater-than operator involving piranha::integer.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::integer and \p U is an arithmetic type,
		 * - \p U is piranha::integer and \p T is an arithmetic type,
		 * - both \p T and \p U are piranha::integer.
		 * 
		 * If no floating-point types are involved, the exact result of the comparison will be returned.
		 * 
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and compared to \p f to generate the return value.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x > y</tt>, \p false otherwise.
		 */
		template <typename T, typename U>
		friend inline typename boost::enable_if_c<are_binary_op_types<T,U>::value,bool>::type operator>(const T &x, const U &y)
		{
			return (y < x);
		}
		/// Generic greater-than or equal operator involving piranha::integer.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::integer and \p U is an arithmetic type,
		 * - \p U is piranha::integer and \p T is an arithmetic type,
		 * - both \p T and \p U are piranha::integer.
		 * 
		 * If no floating-point types are involved, the exact result of the comparison will be returned.
		 * 
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and compared to \p f to generate the return value.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x >= y</tt>, \p false otherwise.
		 */
		template <typename T, typename U>
		friend inline typename boost::enable_if_c<are_binary_op_types<T,U>::value,bool>::type operator>=(const T &x, const U &y)
		{
			return (y <= x);
		}
		/// Combined multiply-add.
		/**
		 * Sets \p this to <tt>this + (x * y)</tt>.
		 * 
		 * @param[in] n1 first argument.
		 * @param[in] n2 second argument.
		 * 
		 * @return reference to \p this.
		 */
		integer &multiply_accumulate(const integer &n1, const integer &n2)
		{
			::mpz_addmul(m_value,n1.m_value,n2.m_value);
			return *this;
		}
		/// Exponentiation.
		/**
		 * Return <tt>this ** exp</tt>. This template method is activated only if \p T is an arithmetic type or integer.
		 * 
		 * If \p T is an integral type or integer, the result will be exact, with negative powers calculated as <tt>(1 / this) ** exp</tt>.
		 * 
		 * If \p T is a floating-point type, the result will be exact if \p exp can be converted exactly to an integer value,
		 * otherwise an \p std::invalid_argument exception will be thrown.
		 * 
		 * Trying to raise zero to a negative exponent will throw a piranha::zero_division_error exception. <tt>this ** 0</tt> will always return 1.
		 * 
		 * In any case, the value of \p exp cannot exceed in magnitude the maximum value representable by the <tt>unsigned long</tt> type, otherwise an
		 * \p std::invalid_argument exception will be thrown.
		 * 
		 * @param[in] exp exponent.
		 * 
		 * @return <tt>this ** exp</tt>.
		 * 
		 * @throws std::invalid_argument if \p T is a floating-point type and \p exp is not an exact integer, or if <tt>exp</tt>'s magnitude exceeds
		 * the range of the <tt>unsigned long</tt> type.
		 * @throws piranha::zero_division_error if \p this is zero and \p exp is negative.
		 */
		template <typename T>
		typename boost::enable_if_c<std::is_arithmetic<T>::value || std::is_same<T,integer>::value,integer>::type pow(const T &exp) const
		{
			return pow_impl(exp);
		}
		/// Overload output stream operator for piranha::integer.
		/**
		 * @param[in] os output stream.
		 * @param[in] n piranha::integer to be sent to stream.
		 * 
		 * @return reference to output stream.
		 */
		friend inline std::ostream &operator<<(std::ostream &os, const integer &n)
		{
			const std::size_t size_base10 = ::mpz_sizeinbase(n.m_value,10);
			if (size_base10 > boost::integer_traits<std::size_t>::const_max - static_cast<std::size_t>(2)) {
				piranha_throw(std::overflow_error,"number of digits is too large");
			}
			// NOTE: here we can optimize, avoiding one allocation, by using a std::array if
			// size_base10 is small enough.
			std::vector<char> tmp(boost::numeric_cast<std::vector<char>::size_type>(size_base10 + static_cast<std::size_t>(2)));
			os << ::mpz_get_str(&tmp[0],10,n.m_value);
			return os;
		}
		/// Overload input stream operator for piranha::integer.
		/**
		 * Equivalent to extracting a string from the stream and then using it to construct an integer that will be assigned to n.
		 * 
		 * @param[in] is input stream.
		 * @param[in,out] n integer to which the contents of the stream will be assigned.
		 * 
		 * @return reference to input stream.
		 */
		friend inline std::istream &operator>>(std::istream &is, integer &n)
		{
			std::string tmp_str;
			std::getline(is,tmp_str);
			// NOTE: here this can probably be optimized via mpz_set_str,
			// thus avoiding one allocation.
			n = integer(tmp_str);
			return is;
		}
	private:
		mpz_t m_value;
};


// TODO: move this in math namespace.
// Overload multiply_accumulate.
// inline void negate(integer &n)
// {
// 	n.negate();
// }

}

namespace std
{

/// Specialization of std::swap for piranha::integer.
/**
 * @param[in] n1 first argument.
 * @param[in] n2 second argument.
 * 
 * @see piranha::integer::swap()
 */
template <>
inline void swap(piranha::integer &n1, piranha::integer &n2)
{
	n1.swap(n2);
}

/// Specialization of std::numeric_limits for piranha::integer.
template <>
class numeric_limits<piranha::integer>: public numeric_limits<long>
{
	public:
		/// Override is_bounded attribute to false.
		static const bool is_bounded = false;
		/// Override is_modulo attribute to false.
		static const bool is_modulo = false;
};

}

#endif
