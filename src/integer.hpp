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
#include <boost/functional/hash.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/math/special_functions/trunc.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <cmath>
#include <cstddef>
#include <functional>
// NOTE: GMP docs say gmp.h already includes the extern "C" parts.
#include <gmp.h>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "config.hpp" // For (un)likely.
#include "detail/integer_fwd.hpp"
#include "detail/is_digit.hpp"
#include "detail/rational_fwd.hpp"
#include "detail/real_fwd.hpp"
#include "detail/sfinae_types.hpp"
#include "exceptions.hpp"
#include "math.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

struct integer_private_access;

}

/// Arbitrary precision integer class.
/**
 * This class represents integer numbers of arbitrary size (i.e., the size is limited only by the available memory).
 * The implementation consists of a C++ wrapper around the \p mpz_t type from the multiprecision GMP library.
 * 
 * \section interop Interoperability with fundamental types
 * 
 * Full interoperability with the following C++ types is provided:
 * 
 * - all standard signed integer types,
 * - all standard unsigned integer types,
 * - \p float and \p double,
 * - \p bool and \p char.
 * 
 * Please note that since the GMP API does not directly provide interoperability
 * with <tt>long long</tt> and <tt>unsigned long long</tt>, interaction with these types could be slower due to the extra workload of converting such types
 * to GMP-compatible types. Also, every function interacting with floating-point types will check that the floating-point values are not
 * non-finite: in case of infinities or NaNs, an <tt>std::invalid_argument</tt> exception will be thrown.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the strong exception safety guarantee for all operations. In case of memory allocation errors by GMP routines,
 * the program will terminate.
 * 
 * \section move_semantics Move semantics
 * 
 * Move construction and move assignment will leave the moved-from object in a state that is destructible and assignable.
 * 
 * \section implementation_details Implementation details
 * 
 * This class uses, for certain routines, the internal interface of GMP integers, which is not guaranteed to be stable
 * across different versions. GMP versions 4.x and 5.x are explicitly supported by this class.
 * 
 * @see http://gmplib.org/manual/Integer-Internals.html
 * @see http://gmplib.org/
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo test the swapping arithmetic with a big integer or with operations such as i *= j + k +l
 * \todo test for number of memory allocations: such tests should re-defined GMP memory allocation functions so that they count the allocations, and report
 * their number. It would be useful to check that future GMP versions still behave the way it is assumed in this class regarding allocation requirements
 * for arithmetic operations.
 * \todo investigate implementing *= in terms of *, since it might be slightly faster (create result with constructor from size?)
 * \todo check performance on poly multiplication tests if we replace operator* with straight mpz functions.
 * \todo improve interaction with long long via decomposition of operations in long operands
 * \todo improve performance of binary modulo operation when the second argument is a hardware integer
 * \todo investigate replacing lexical_cast with boost::format, as it seems like boost::format might offer better guarantees
 * on the printed value -> but maybe not: http://www.gotw.ca/publications/mill19.htm it seems it should be ok for int types.
 * \todo think about a better way to establish the limit of factorial input - maybe a test done in the build system?
 */
class integer
{
		// Make friends with real and rational.
		friend class real;
		friend class rational;
		// Access to interop types, used in pow_impl.
		friend struct detail::integer_private_access;
		// C++ arithmetic types supported for interaction with integer.
		template <typename T>
		struct is_interop_type
		{
			static const bool value = std::is_same<T,char>::value || std::is_same<T,bool>::value ||
				std::is_same<T,float>::value || std::is_same<T,double>::value ||
				std::is_same<T,signed char>::value || std::is_same<T,short>::value || std::is_same<T,int>::value ||
				std::is_same<T,long>::value || std::is_same<T,long long>::value ||
				std::is_same<T,unsigned char>::value || std::is_same<T,unsigned short>::value || std::is_same<T,unsigned>::value ||
				std::is_same<T,unsigned long>::value || std::is_same<T,unsigned long long>::value;
		};
		// Type trait to detect integral types that can interoperate directly with GMP routines.
		template <typename T, typename = void>
		struct is_gmp_int
		{
			static const bool value = false;
		};
		template <typename Int>
		struct is_gmp_int<Int,typename std::enable_if<is_interop_type<Int>::value && std::is_integral<Int>::value &&
			std::is_signed<Int>::value>::type>
		{
			// NOTE: in order to interoperate with GMP, it must not be wider than long.
			static const bool value = boost::integer_traits<Int>::const_max <= boost::integer_traits<long>::const_max &&
				boost::integer_traits<Int>::const_min >= boost::integer_traits<long>::const_min;
		};
		template <typename Uint>
		struct is_gmp_int<Uint,typename std::enable_if<is_interop_type<Uint>::value && std::is_integral<Uint>::value &&
			std::is_unsigned<Uint>::value>::type>
		{
			static const bool value = boost::integer_traits<Uint>::const_max <= boost::integer_traits<unsigned long>::const_max &&
				boost::integer_traits<Uint>::const_min >= boost::integer_traits<unsigned long>::const_min;
		};
		// Function to check that a floating point number is not pathological, in order to shield GMP
		// functions.
		template <typename T>
		static void fp_normal_check(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			if (unlikely(!boost::math::isfinite(x))) {
				piranha_throw(std::invalid_argument,"non-finite floating-point number");
			}
		}
		// Validate the string format for integers.
		static void validate_string(const char *str, const std::size_t &size)
		{
			if (!size) {
				piranha_throw(std::invalid_argument,"invalid string input for integer type");
			}
			const std::size_t has_minus = (str[0] == '-'), signed_size = size - has_minus;
			if (!signed_size) {
				piranha_throw(std::invalid_argument,"invalid string input for integer type");
			}
			// A number starting with zero cannot be multi-digit and cannot have a leading minus sign (no '-0' allowed).
			if (str[has_minus] == '0' && (signed_size > 1u || has_minus)) {
				piranha_throw(std::invalid_argument,"invalid string input for integer type");
			}
			// Check that each character is a digit.
			std::for_each(str + has_minus, str + size,[](char c){if (!detail::is_digit(c)) {piranha_throw(std::invalid_argument,"invalid string input for integer type");}});
		}
		// Construction.
		void construct_from_string(const char *str)
		{
			validate_string(str,std::strlen(str));
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
		void construct_from_arithmetic(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			fp_normal_check(x);
			::mpz_init_set_d(m_value,static_cast<double>(x));
		}
		template <typename T>
		void construct_from_arithmetic(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpz_init_set_si(m_value,static_cast<long>(si));
		}
		template <typename T>
		void construct_from_arithmetic(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpz_init_set_ui(m_value,static_cast<unsigned long>(ui));
		}
		template <typename T>
		void construct_from_arithmetic(const T &ll, typename std::enable_if<std::is_integral<T>::value && !is_gmp_int<T>::value>::type * = nullptr)
		{
			construct_from_string(boost::lexical_cast<std::string>(ll).c_str());
		}
		// Assignment.
		void assign_from_string(const char *str)
		{
			validate_string(str,std::strlen(str));
			// String is OK.
			const int retval = ::mpz_set_str(m_value,str,10);
			if (retval == -1) {
				piranha_throw(std::invalid_argument,"invalid string input for integer type");
			}
			piranha_assert(retval == 0);
		}
		template <typename T>
		void assign_from_arithmetic(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			fp_normal_check(x);
			::mpz_set_d(m_value,static_cast<double>(x));
		}
		template <typename T>
		void assign_from_arithmetic(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpz_set_si(m_value,static_cast<long>(si));
		}
		template <typename T>
		void assign_from_arithmetic(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpz_set_ui(m_value,static_cast<unsigned long>(ui));
		}
		template <typename T>
		void assign_from_arithmetic(const T &ll, typename std::enable_if<std::is_integral<T>::value && !is_gmp_int<T>::value>::type * = nullptr)
		{
			assign_from_string(boost::lexical_cast<std::string>(ll).c_str());
		}
		// Conversion.
		template <typename T>
		typename std::enable_if<std::is_signed<T>::value && is_gmp_int<T>::value &&
			!std::is_same<T,bool>::value,T>::type convert_to_impl() const
		{
			if (::mpz_fits_slong_p(m_value)) {
				try {
					return(boost::numeric_cast<T>(::mpz_get_si(m_value)));
				} catch (const boost::bad_numeric_cast &) {}
			}
			piranha_throw(std::overflow_error,"overflow in conversion to integral type");
		}
		template <typename T>
		typename std::enable_if<std::is_unsigned<T>::value && is_gmp_int<T>::value &&
			!std::is_same<T,bool>::value,T>::type convert_to_impl() const
		{
			if (::mpz_fits_ulong_p(m_value)) {
				try {
					return(boost::numeric_cast<T>(::mpz_get_ui(m_value)));
				} catch (const boost::bad_numeric_cast &) {}
			}
			piranha_throw(std::overflow_error,"overflow in conversion to integral type");
		}
		template <typename T>
		typename std::enable_if<std::is_integral<T>::value && !is_gmp_int<T>::value,T>::type convert_to_impl() const
		{
			try {
				return boost::lexical_cast<T>(*this);
			} catch (const boost::bad_lexical_cast &) {
				piranha_throw(std::overflow_error,"overflow in conversion to integral type");
			}
		}
		template <typename T>
		typename std::enable_if<std::is_floating_point<T>::value,T>::type convert_to_impl() const
		{
			if (::mpz_cmp_d(m_value,static_cast<double>(std::numeric_limits<T>::lowest())) < 0) {
				// NOTE: in order to be able to produce -inf we have to be sure of the following:
				// - the lowest fp number is negative,
				// - we can represent _positive_ infinity.
				// Then we can return positive infinity with the sign copied from lowest().
				if (std::signbit(std::numeric_limits<T>::lowest()) != 0 && std::numeric_limits<T>::has_infinity) {
					return std::copysign(std::numeric_limits<T>::infinity(),std::numeric_limits<T>::lowest());
				} else {
					piranha_throw(std::overflow_error,"cannot convert to floating point type");
				}
			} else if (::mpz_cmp_d(m_value,static_cast<double>(std::numeric_limits<T>::max())) > 0) {
				// NOTE: here we do not have the issues above: if this is greater than the max fp value
				// (be it positive or not) and we can represent positive inf, then return it.
				if (std::numeric_limits<T>::has_infinity) {
					return std::numeric_limits<T>::infinity();
				} else {
					piranha_throw(std::overflow_error,"cannot convert to floating point type");
				}
			} else {
				// NOTE: here the GMP docs warn that this operation can fail in horrid ways,
				// the checks above should keep us safe, but keep this in mind...
				// NOTE: here the static cast is safe if T is float because we made sure in the checks above that
				// the GMP value is within the bounds of float.
				return static_cast<T>(::mpz_get_d(m_value));
			}
		}
		// Special handling for bool.
		template <typename T>
		typename std::enable_if<std::is_same<T,bool>::value,T>::type convert_to_impl() const
		{
			return (sign() != 0);
		}
		// In-place addition.
		void in_place_add(const integer &n)
		{
			::mpz_add(m_value,m_value,n.m_value);
		}
		void in_place_add(integer &&n)
		{
			// NOTE: here we should probably check if the allocated size in this
			// is enough, in that case there is little point in swapping?
			// NOTE: this is never called in the binary operator, only the
			// const & overload is.
			if (n.allocated_size() > allocated_size()) {
				swap(n);
			}
			in_place_add(n);
		}
		template <typename T>
		void in_place_add(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			is_gmp_int<T>::value>::type * = nullptr)
		{
			in_place_add(integer(si));
		}
		template <typename T>
		void in_place_add(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpz_add_ui(m_value,m_value,static_cast<unsigned long>(ui));
		}
		// For non-gmp ints create a temporary integer and add it.
		template <typename T>
		void in_place_add(const T &n, typename std::enable_if<std::is_integral<T>::value && !is_gmp_int<T>::value>::type * = nullptr)
		{
			in_place_add(integer(n));
		}
		template <typename T>
		void in_place_add(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
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
			if (n.allocated_size() > allocated_size()) {
				swap(n);
				::mpz_neg(m_value,m_value);
				in_place_add(n);
			} else {
				in_place_sub(n);
			}
		}
		template <typename T>
		void in_place_sub(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			is_gmp_int<T>::value>::type * = nullptr)
		{
			in_place_sub(integer(si));
		}
		template <typename T>
		void in_place_sub(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpz_sub_ui(m_value,m_value,static_cast<unsigned long>(ui));
		}
		template <typename T>
		void in_place_sub(const T &n, typename std::enable_if<std::is_integral<T>::value && !is_gmp_int<T>::value>::type * = nullptr)
		{
			in_place_sub(integer(n));
		}
		template <typename T>
		void in_place_sub(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
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
			// NOTE: here target size is increased by 1 in order to pre-emptively increase the likelyhood of
			// future additions being able to steal memory.
			const std::size_t s1 = size(), s2 = n.size(), target_size = s1 + s2 + std::size_t(1);
			const auto a2 = n.allocated_size();
			// NOTE: the possible scenario in which swapping is useful is the following:
			// s1's size in limbs is unitary and s2's is not: the GMP docs state the only condition
			// in which a copy of this is avoided is when the second operand has 1 limb. If s2 has enough space
			// to store the result, steal it.
			if (s1 == 1u && a2 >= target_size) {
				swap(n);
			}
			in_place_mul(n);
		}
		template <typename T>
		void in_place_mul(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpz_mul_si(m_value,m_value,static_cast<long>(si));
		}
		template <typename T>
		void in_place_mul(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpz_mul_ui(m_value,m_value,static_cast<unsigned long>(ui));
		}
		template <typename T>
		void in_place_mul(const T &n, typename std::enable_if<std::is_integral<T>::value && !is_gmp_int<T>::value>::type * = nullptr)
		{
			in_place_mul(integer(n));
		}
		template <typename T>
		void in_place_mul(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			operator=(static_cast<T>(*this) * x);
		}
		// In-place division.
		void in_place_div(const integer &n)
		{
			::mpz_tdiv_q(m_value,m_value,n.m_value);
		}
		template <typename T>
		void in_place_div(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			is_gmp_int<T>::value>::type * = nullptr)
		{
			in_place_div(integer(si));
		}
		template <typename T>
		void in_place_div(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpz_tdiv_q_ui(m_value,m_value,static_cast<unsigned long>(ui));
		}
		template <typename T>
		void in_place_div(const T &n, typename std::enable_if<std::is_integral<T>::value && !is_gmp_int<T>::value>::type * = nullptr)
		{
			in_place_div(integer(n));
		}
		template <typename T>
		void in_place_div(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			operator=(static_cast<T>(*this) / x);
		}
		// In-place modulo.
		void in_place_mod(const integer &n)
		{
			if (unlikely(n.sign() <= 0)) {
				piranha_throw(std::invalid_argument,"non-positive divisor");
			}
			::mpz_mod(m_value,m_value,n.m_value);
		}
		template <typename T>
		void in_place_mod(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			is_gmp_int<T>::value>::type * = nullptr)
		{
			in_place_mod(integer(si));
		}
		template <typename T>
		void in_place_mod(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			is_gmp_int<T>::value>::type * = nullptr)
		{
			if (unlikely(ui == 0u)) {
				piranha_throw(std::invalid_argument,"non-positive divisor");
			}
			*this = ::mpz_fdiv_ui(m_value,static_cast<unsigned long>(ui));
		}
		template <typename T>
		void in_place_mod(const T &n, typename std::enable_if<std::is_integral<T>::value && !is_gmp_int<T>::value>::type * = nullptr)
		{
			in_place_mod(integer(n));
		}
		// Binary operations.
		// Type trait for allowed arguments in arithmetic binary operations.
		template <typename T, typename U>
		struct are_binary_op_types: std::integral_constant<bool,
			(std::is_same<typename std::decay<T>::type,integer>::value && is_interop_type<typename std::decay<U>::type>::value) ||
			(std::is_same<typename std::decay<U>::type,integer>::value && is_interop_type<typename std::decay<T>::type>::value) ||
			(std::is_same<typename std::decay<T>::type,integer>::value && std::is_same<typename std::decay<U>::type,integer>::value)>
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
		struct deduce_binary_op_result_type<T,U,typename std::enable_if<std::is_floating_point<typename std::decay<T>::type>::value>::type>
		{
			typedef typename std::decay<T>::type type;
		};
		template <typename T, typename U>
		struct deduce_binary_op_result_type<T,U,typename std::enable_if<std::is_floating_point<typename std::decay<U>::type>::value>::type>
		{
			typedef typename std::decay<U>::type type;
		};
		// Binary addition.
		template <typename T, typename U>
		static integer binary_plus(T &&n1, U &&n2, typename std::enable_if<
			std::is_same<typename std::decay<T>::type,integer>::value && std::is_same<typename std::decay<U>::type,integer>::value
			>::type * = nullptr)
		{
			const auto a1 = n1.allocated_size(), a2 = n2.allocated_size();
			const std::size_t target_size = std::max<std::size_t>(n1.size(),n2.size()) + std::size_t(1);
			if (is_nonconst_rvalue_ref<T &&>::value && is_nonconst_rvalue_ref<U &&>::value) {
				if (a1 >= a2 && a1 >= target_size) {
					return binary_plus_fwd_first(std::forward<T>(n1),n2);
				} else if (a2 >= a1 && a2 >= target_size) {
					return binary_plus_fwd_first(std::forward<U>(n2),n1);
				} else {
					return binary_plus_new(n1,n2,target_size);
				}
			} else if (is_nonconst_rvalue_ref<T &&>::value && a1 >= target_size) {
				return binary_plus_fwd_first(std::forward<T>(n1),n2);
			} else if (is_nonconst_rvalue_ref<U &&>::value && a2 >= target_size) {
				return binary_plus_fwd_first(std::forward<U>(n2),n1);
			} else {
				return binary_plus_new(n1,n2,target_size);
			}
		}
		template <typename T, typename U>
		static integer binary_plus_fwd_first(T &&n, const U &x)
		{
			integer retval(std::forward<T>(n));
			retval.in_place_add(x);
			return retval;
		}
		template <typename T>
		static integer binary_plus_new(const integer &n1, const T &n2, const std::size_t &target_size)
		{
			integer retval{nlimbs(target_size)};
			retval = n1;
			retval.in_place_add(n2);
			return retval;
		}
		template <typename T, typename U>
		static integer binary_plus(T &&n1, U &&n2, typename std::enable_if<
			std::is_same<typename std::decay<T>::type,integer>::value && std::is_integral<typename std::decay<U>::type>::value
			>::type * = nullptr)
		{
			const std::size_t target_size = n1.size() + std::size_t(1);
			if (n1.allocated_size() >= target_size) {
				return binary_plus_fwd_first(std::forward<T>(n1),n2);
			} else {
				return binary_plus_new(n1,n2,target_size);
			}
		}
		template <typename T, typename U>
		static integer binary_plus(T &&n1, U &&n2, typename std::enable_if<
			std::is_same<typename std::decay<U>::type,integer>::value && std::is_integral<typename std::decay<T>::type>::value
			>::type * = nullptr)
		{
			const std::size_t target_size = n2.size() + std::size_t(1);
			if (n2.allocated_size() >= target_size) {
				return binary_plus_fwd_first(std::forward<U>(n2),n1);
			} else {
				return binary_plus_new(n2,n1,target_size);
			}
		}
		template <typename T>
		static T binary_plus(const integer &n, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return (static_cast<T>(n) + x);
		}
		template <typename T>
		static T binary_plus(const T &x, const integer &n, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return binary_plus(n,x);
		}
		// Binary subtraction.
		template <typename T, typename U>
		static integer binary_minus(T &&n1, U &&n2, typename std::enable_if<
			std::is_same<typename std::decay<T>::type,integer>::value && std::is_same<typename std::decay<U>::type,integer>::value
			>::type * = nullptr)
		{
			const auto a1 = n1.allocated_size(), a2 = n2.allocated_size();
			const std::size_t target_size = std::max<std::size_t>(n1.size(),n2.size()) + std::size_t(1);
			if (is_nonconst_rvalue_ref<T &&>::value && is_nonconst_rvalue_ref<U &&>::value) {
				if (a1 >= a2 && a1 >= target_size) {
					return binary_minus_fwd_first<true>(std::forward<T>(n1),n2);
				} else if (a2 >= a1 && a2 >= target_size) {
					return binary_minus_fwd_first<false>(std::forward<U>(n2),n1);
				} else {
					return binary_minus_new<true>(n1,n2,target_size);
				}
			} else if (is_nonconst_rvalue_ref<T &&>::value && a1 >= target_size) {
				return binary_minus_fwd_first<true>(std::forward<T>(n1),n2);
			} else if (is_nonconst_rvalue_ref<U &&>::value && a2 >= target_size) {
				return binary_minus_fwd_first<false>(std::forward<U>(n2),n1);
			} else {
				return binary_minus_new<true>(n1,n2,target_size);
			}
		}
		template <bool Order, typename T, typename U>
		static integer binary_minus_fwd_first(T &&n, const U &x)
		{
			integer retval(std::forward<T>(n));
			retval.in_place_sub(x);
			if (!Order) {
				retval.negate();
			}
			return retval;
		}
		template <bool Order, typename T>
		static integer binary_minus_new(const integer &n1, const T &n2, const std::size_t &target_size)
		{
			integer retval{nlimbs(target_size)};
			retval = n1;
			retval.in_place_sub(n2);
			if (!Order) {
				retval.negate();
			}
			return retval;
		}
		template <typename T, typename U>
		static integer binary_minus(T &&n1, U &&n2, typename std::enable_if<
			std::is_same<typename std::decay<T>::type,integer>::value && std::is_integral<typename std::decay<U>::type>::value
			>::type * = nullptr)
		{
			const std::size_t target_size = n1.size() + std::size_t(1);
			if (n1.allocated_size() >= target_size) {
				return binary_minus_fwd_first<true>(std::forward<T>(n1),n2);
			} else {
				return binary_minus_new<true>(n1,n2,target_size);
			}
		}
		template <typename T, typename U>
		static integer binary_minus(T &&n1, U &&n2, typename std::enable_if<
			std::is_same<typename std::decay<U>::type,integer>::value && std::is_integral<typename std::decay<T>::type>::value
			>::type * = nullptr)
		{
			const std::size_t target_size = n2.size() + std::size_t(1);
			if (n2.allocated_size() >= target_size) {
				return binary_minus_fwd_first<false>(std::forward<U>(n2),n1);
			} else {
				return binary_minus_new<false>(n2,n1,target_size);
			}
		}
		template <typename T>
		static T binary_minus(const integer &n, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return (static_cast<T>(n) - x);
		}
		template <typename T>
		static T binary_minus(const T &x, const integer &n, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return -binary_minus(n,x);
		}
		// Binary multiplication.
		template <typename T, typename U>
		static integer binary_mul(T &&n1, U &&n2, typename std::enable_if<
			std::is_same<typename std::decay<T>::type,integer>::value &&
			std::is_same<typename std::decay<U>::type,integer>::value
			>::type * = nullptr)
		{
			const auto a1 = n1.allocated_size(), a2 = n2.allocated_size();
			const std::size_t s1 = n1.size(), s2 = n2.size(), target_size = s1 + s2 + std::size_t(1);
			// Condition for stealing: one integer has size 1 and the other has allocated enough space.
			if (is_nonconst_rvalue_ref<T &&>::value && is_nonconst_rvalue_ref<U &&>::value) {
				if (s2 == 1u && a1 >= target_size) {
					return binary_mul_fwd_first(std::forward<T>(n1),n2);
				} else if (s1 == 1u && a2 >= target_size) {
					return binary_mul_fwd_first(std::forward<U>(n2),n1);
				} else {
					return binary_mul_new(n1,n2,target_size);
				}
			} else if (is_nonconst_rvalue_ref<T &&>::value && s2 == 1u && a1 >= target_size) {
				return binary_mul_fwd_first(std::forward<T>(n1),n2);
			} else if (is_nonconst_rvalue_ref<U &&>::value && s1 == 1u && a2 >= target_size) {
				return binary_mul_fwd_first(std::forward<U>(n2),n1);
			} else {
				return binary_mul_new(n1,n2,target_size);
			}
		}
		template <typename T, typename U>
		static integer binary_mul_fwd_first(T &&n, const U &x)
		{
			integer retval(std::forward<T>(n));
			retval.in_place_mul(x);
			return retval;
		}
		static integer binary_mul_new(const integer &n1, const integer &n2, const std::size_t &target_size)
		{
			integer retval{nlimbs(target_size)};
			::mpz_mul(retval.m_value,n1.m_value,n2.m_value);
			return retval;
		}
		template <typename T>
		static integer binary_mul_new(const integer &n1, const T &n2, const std::size_t &target_size)
		{
			integer retval{nlimbs(target_size)};
			retval = n1;
			retval.in_place_mul(n2);
			return retval;
		}
		template <typename T, typename U>
		static integer binary_mul(T &&n1, U &&n2, typename std::enable_if<
			std::is_same<typename std::decay<T>::type,integer>::value && std::is_integral<typename std::decay<U>::type>::value
			>::type * = nullptr)
		{
			// NOTE: here the formula would be: n1.size() + size_in_limbs_of_U + 1, but we
			// are assuming that the size of U is no greater than 1 limb.
			const std::size_t target_size = n1.size() + std::size_t(2);
			if (n1.allocated_size() >= target_size) {
				return binary_mul_fwd_first(std::forward<T>(n1),n2);
			} else {
				return binary_mul_new(n1,n2,target_size);
			}
		}
		template <typename T, typename U>
		static integer binary_mul(T &&n1, U &&n2, typename std::enable_if<
			std::is_same<typename std::decay<U>::type,integer>::value && std::is_integral<typename std::decay<T>::type>::value
			>::type * = nullptr)
		{
			const std::size_t target_size = n2.size() + std::size_t(2);
			if (n2.allocated_size() >= target_size) {
				return binary_mul_fwd_first(std::forward<U>(n2),n1);
			} else {
				return binary_mul_new(n2,n1,target_size);
			}
		}
		template <typename T>
		static T binary_mul(const integer &n, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return (static_cast<T>(n) * x);
		}
		template <typename T>
		static T binary_mul(const T &x, const integer &n, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return binary_mul(n,x);
		}
		// Binary division.
		template <typename T, typename U>
		static integer binary_div(T &&n1, U &&n2, typename std::enable_if<
			are_binary_op_types<T,U>::value &&
			!std::is_floating_point<typename std::decay<T>::type>::value && !std::is_floating_point<typename std::decay<U>::type>::value
			>::type * = nullptr)
		{
			// NOTE: here it makes sense only to attempt to steal n1's storage, not n2's, because the operation is not commutative.
			integer retval(std::forward<T>(n1));
			retval /= std::forward<U>(n2);
			return retval;
		}
		template <typename T>
		static T binary_div(const integer &n, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			if (unlikely(x == 0.)) {
				piranha_throw(zero_division_error,"division by zero");
			}
			return (static_cast<T>(n) / x);
		}
		template <typename T>
		static T binary_div(const T &x, const integer &n, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			const T n_T = static_cast<T>(n);
			if (unlikely(n_T == 0.)) {
				piranha_throw(zero_division_error,"division by zero");
			}
			return (x / n_T);
		}
		// Binary modulo operation.
		template <typename T, typename U>
		static integer binary_mod(T &&n1, U &&n2, typename std::enable_if<
			are_binary_op_types<T,U>::value &&
			!std::is_floating_point<typename std::decay<T>::type>::value && !std::is_floating_point<typename std::decay<U>::type>::value
			>::type * = nullptr)
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
		static bool binary_equality(const integer &n1, const T &n2, typename std::enable_if<std::is_signed<T>::value &&
			is_gmp_int<T>::value>::type * = nullptr)
		{
			return (mpz_cmp_si(n1.m_value,static_cast<long>(n2)) == 0);
		}
		template <typename T>
		static bool binary_equality(const integer &n1, const T &n2, typename std::enable_if<std::is_unsigned<T>::value &&
			is_gmp_int<T>::value>::type * = nullptr)
		{
			return (mpz_cmp_ui(n1.m_value,static_cast<unsigned long>(n2)) == 0);
		}
		template <typename T>
		static bool binary_equality(const integer &n1, const T &n2, typename std::enable_if<std::is_integral<T>::value &&
			!is_gmp_int<T>::value>::type * = nullptr)
		{
			return binary_equality(n1,integer(n2));
		}
		template <typename T>
		static bool binary_equality(const integer &n, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return (static_cast<T>(n) == x);
		}
		// NOTE: this is the reverse of above.
		template <typename T>
		static bool binary_equality(const T &x, const integer &n, typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr)
		{
			return binary_equality(n,x);
		}
		// Binary less-than.
		static bool binary_less_than(const integer &n1, const integer &n2)
		{
			return (::mpz_cmp(n1.m_value,n2.m_value) < 0);
		}
		template <typename T>
		static bool binary_less_than(const integer &n1, const T &n2, typename std::enable_if<std::is_signed<T>::value &&
			is_gmp_int<T>::value>::type * = nullptr)
		{
			return (mpz_cmp_si(n1.m_value,static_cast<long>(n2)) < 0);
		}
		template <typename T>
		static bool binary_less_than(const integer &n1, const T &n2, typename std::enable_if<std::is_unsigned<T>::value &&
			is_gmp_int<T>::value>::type * = nullptr)
		{
			return (mpz_cmp_ui(n1.m_value,static_cast<unsigned long>(n2)) < 0);
		}
		template <typename T>
		static bool binary_less_than(const integer &n1, const T &n2, typename std::enable_if<std::is_integral<T>::value &&
			!is_gmp_int<T>::value>::type * = nullptr)
		{
			return binary_less_than(n1,integer(n2));
		}
		template <typename T>
		static bool binary_less_than(const integer &n, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return (static_cast<T>(n) < x);
		}
		// Binary less-than or equal.
		static bool binary_leq(const integer &n1, const integer &n2)
		{
			return (::mpz_cmp(n1.m_value,n2.m_value) <= 0);
		}
		template <typename T>
		static bool binary_leq(const integer &n1, const T &n2, typename std::enable_if<std::is_signed<T>::value &&
			is_gmp_int<T>::value>::type * = nullptr)
		{
			return (mpz_cmp_si(n1.m_value,static_cast<long>(n2)) <= 0);
		}
		template <typename T>
		static bool binary_leq(const integer &n1, const T &n2, typename std::enable_if<std::is_unsigned<T>::value &&
			is_gmp_int<T>::value>::type * = nullptr)
		{
			return (mpz_cmp_ui(n1.m_value,static_cast<unsigned long>(n2)) <= 0);
		}
		template <typename T>
		static bool binary_leq(const integer &n1, const T &n2, typename std::enable_if<std::is_integral<T>::value &&
			!is_gmp_int<T>::value>::type * = nullptr)
		{
			return binary_leq(n1,integer(n2));
		}
		template <typename T>
		static bool binary_leq(const integer &n, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return (static_cast<T>(n) <= x);
		}
		// Inverse forms of less-than and leq.
		template <typename T>
		static bool binary_less_than(const T &x, const integer &n, typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr)
		{
			return !binary_leq(n,x);
		}
		template <typename T>
		static bool binary_leq(const T &x, const integer &n, typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr)
		{
			return !binary_less_than(n,x);
		}
		// Exponentiation.
		template <typename T>
		integer pow_impl(const T &ui, typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value>::type * = nullptr) const
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
		integer pow_impl(const T &si, typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type * = nullptr) const
		{
			return pow_impl(integer(si));
		}
		integer pow_impl(const integer &n) const
		{
			if (n.sign() >= 0) {
				unsigned long exp;
				try {
					exp = static_cast<unsigned long>(n);
				} catch (const std::overflow_error &) {
					piranha_throw(std::invalid_argument,"invalid argument for integer exponentiation");
				}
				return pow_impl(exp);
			} else {
				return 1 / pow_impl(-n);
			}
		}
		// Private constructor for use in rational.
		explicit integer(const ::mpz_t n)
		{
			::mpz_init_set(m_value,n);
		}
		static std::size_t hash_mpz_t(const ::mpz_t n)
		{
			const std::size_t size = ::mpz_size(n);
			// Use the sign as initial seed value.
			std::size_t retval = static_cast<std::size_t>(mpz_sgn(n));
			for (std::size_t i = 0u; i < size; ++i) {
				boost::hash_combine(retval,n->_mp_d[i]);
			}
			return retval;
		}
		// Private implementation of swap.
		static void swap_mpz_t(::mpz_t n1, ::mpz_t n2)
		{
			// NOTE: implement swap manually because this function is used in assignment, and hence when potentially
			// reviving moved-from objects. It seems like we do not have the guarantee that ::mpz_swap() is gonna work
			// on uninitialised objects.
			std::swap(n1->_mp_d,n2->_mp_d);
			std::swap(n1->_mp_size,n2->_mp_size);
			std::swap(n1->_mp_alloc,n2->_mp_alloc);
		}
		template <typename T>
		static unsigned long check_choose_k(const T &k,
			typename std::enable_if<std::is_integral<T>::value>::type * = nullptr)
		{
			try {
				return boost::numeric_cast<unsigned long>(k);
			} catch (...) {
				piranha_throw(std::invalid_argument,"invalid k argument for binomial coefficient");
			}
		}
		template <typename T>
		static unsigned long check_choose_k(const T &k,
			typename std::enable_if<std::is_same<T,integer>::value>::type * = nullptr)
		{
			try {
				return static_cast<unsigned long>(k);
			} catch (...) {
				piranha_throw(std::invalid_argument,"invalid k argument for binomial coefficient");
			}
		}
	public:
		/// Default constructor.
		/**
		 * The value is initialised to zero.
		 */
		integer()
		{
			::mpz_init(m_value);
		}
		/// Class for the representation of the number of limbs in a piranha::integer.
		/**
		 * This class is used in the constructor of piranha::integer from size, and
		 * it represents the number of limbs that will be allocated for the constructed object.
		 */
		class nlimbs
		{
				friend class integer;
			public:
				/// Deleted default constructor.
				nlimbs() = delete;
				/// Deleted copy constructor.
				nlimbs(const nlimbs &) = delete;
				/// Deleted move constructor.
				nlimbs(nlimbs &&) = delete;
				/// Deleted copy-assignment operator.
				nlimbs &operator=(const nlimbs &) = delete;
				/// Deleted move-assignment operator.
				nlimbs &operator=(nlimbs &&) = delete;
				/// Constructor from \p std::size_t.
				/**
				 * @param[in] n number of limbs that will be allocated when constructing a piranha::integer.
				 */
				explicit nlimbs(const std::size_t &n):m_n(n) {}
			private:
				const std::size_t m_n;
		};
		/// Constructor from number of limbs.
		/**
		 * Construct an integer object initialised to zero and with pre-allocated space of \p n limbs.
		 * A limb is, in the jargon of the GMP library, "the part of a multi-precision number that fits into a single
		 * machine word". The GMP global variable \p mp_bits_per_limb represents the number of bits contained in a limb
		 * (typically 32 or 64).
		 * 
		 * Example usage:
		 @code
		 using namespace piranha;
		 integer n(integer::nlimbs(4));
		 @endcode
		 * 
		 * @param[in] n number of limbs to allocate during construction.
		 * 
		 * @see http://gmplib.org/manual/Nomenclature-and-Types.html.
		 */
		explicit integer(const nlimbs &n)
		{
			// NOTE: use unsigned types everywhere, so that in case of overflow we just allocate a different amount of memory.
			::mpz_init2(m_value,static_cast< ::mp_bitcnt_t>(
				n.m_n * std::make_unsigned<decltype(::mp_bits_per_limb)>::type(::mp_bits_per_limb)));
		}
		/// Copy constructor.
		/**
		 * @param[in] other integer to be deep-copied.
		 */
		integer(const integer &other)
		{
			::mpz_init_set(m_value,other.m_value);
		}
		/// Move constructor.
		/**
		 * @param[in] other integer to be moved.
		 */
		integer(integer &&other) noexcept(true)
		{
			m_value->_mp_size = other.m_value->_mp_size;
			m_value->_mp_d = other.m_value->_mp_d;
			m_value->_mp_alloc = other.m_value->_mp_alloc;
			// Erase other.
			other.m_value->_mp_size = 0;
			other.m_value->_mp_d = nullptr;
			other.m_value->_mp_alloc = 0;
		}
		/// Generic constructor.
		/**
		 * The supported types for \p T are the \ref interop "interoperable types".
		 * Use of other types will result in a compile-time error.
		 * In case a floating-point type is used, \p x will be truncated (i.e., rounded towards zero) before being used to construct
		 * the integer object.
		 * 
		 * @param[in] x object used to construct \p this.
		 * 
		 * @throws std::invalid_argument if \p x is a non-finite floating-point number.
		 */
		template <typename T>
		explicit integer(const T &x, typename std::enable_if<is_interop_type<T>::value>::type * = nullptr)
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
		~integer();
		/// Move assignment operator.
		/**
		 * @param[in] other integer to be moved.
		 * 
		 * @return reference to \p this.
		 */
		integer &operator=(integer &&other) noexcept(true)
		{
			// NOTE: swap() already has the check for this.
			swap(other);
			return *this;
		}
		/// Copy assignment operator.
		/**
		 * @param[in] other integer to be assigned.
		 * 
		 * @return reference to \p this.
		 */
		integer &operator=(const integer &other)
		{
			if (likely(this != &other)) {
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
		 * The string parsing rules are the same as in the constructor from string.
		 * 
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
		/// Generic assignment operator.
		/**
		 * The supported types for \p T are the \ref interop "interoperable types".
		 * Use of other types will result in a compile-time error.
		 * In case a floating-point type is used, \p x will be truncated (i.e., rounded towards zero) before being assigned
		 * to the object.
		 * 
		 * @param[in] x object that will be assigned to \p this.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws std::invalid_argument if \p x is a non-finite floating-point number.
		 */
		template <typename T>
		typename std::enable_if<is_interop_type<T>::value,integer &>::type operator=(const T &x)
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
		void swap(integer &n) noexcept(true)
		{
			if (unlikely(this == &n)) {
			    return;
			}
			swap_mpz_t(m_value,n.m_value);
		}
		/// Conversion to interoperable types.
		/**
		 * Extract an instance of type \p T from \p this. The supported types for \p T are the \ref interop "interoperable types".
		 * 
		 * Conversion to \p bool is always successful, and returns <tt>this != 0</tt>.
		 * Conversion to the other integral types is exact, its success depending on whether or not
		 * the target type can represent the current value of the integer.
		 * 
		 * Conversion to floating point types is exact if the target type can represent exactly the current value of the integer.
		 * If that is not the case, the output value will be one of the two adjacents (with an unspecified rounding direction).
		 * 
		 * Return values of +-inf will be produced if the current value of the piranha::integer overflows the range of the floating-point type
		 * and the floating-point type can represent infinity. Otherwise, an overflow error will be produced.
		 * 
		 * @return result of the conversion to target type T.
		 * 
		 * @throws std::overflow_error if the conversion to an integral type other than bool results in (negative) overflow, or if
		 * conversion to a floating-point type lacking infinity overflows.
		 */
		// NOTE: the reason why conversion to floating point has unspecified rounding direction (despite the fact that for double
		// the rounding direction is well defined) is that we don't know what happens in the conversion from double to float (there
		// the rounding direction is unspecified). So let's keep it vague.
		template <typename T, typename = typename std::enable_if<is_interop_type<T>::value>::type>
		explicit operator T() const
		{
			return convert_to_impl<T>();
		}
		/// In-place addition.
		/**
		 * Add \p x to the current value of the integer object. This template operator is activated only if
		 * \p T is either integer or an \ref interop "interoperable type".
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
		 * @throws unspecified any exception resulting from interoperating with floating-point types.
		 */
		template <typename T>
		typename std::enable_if<
			is_interop_type<typename std::decay<T>::type>::value ||
			std::is_same<integer,typename std::decay<T>::type>::value,integer &>::type operator+=(T &&x)
		{
			in_place_add(std::forward<T>(x));
			return *this;
		}
		/// Generic in-place addition with piranha::integer.
		/**
		 * Add a piranha::integer in-place. This template operator is activated only if \p T is an \ref interop "interoperable type" and \p I is piranha::integer.
		 * This method will first compute <tt>n + x</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] n second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception resulting from the binary operator or from casting piranha::integer to \p T.
		 */
		template <typename T, typename I>
		friend typename std::enable_if<is_interop_type<T>::value && std::is_same<typename std::decay<I>::type,integer>::value,T &>::type
			operator+=(T &x, I &&n)
		{
			x = static_cast<T>(std::forward<I>(n) + x);
			return x;
		}
		/// Generic binary addition involving piranha::integer.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::integer and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::integer and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::integer.
		 * 
		 * If no floating-point types are involved, the exact result of the operation will be returned as a piranha::integer.
		 * 
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and added to \p f to generate the return value, which will then be of type \p F.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x + y</tt>.
		 * 
		 * @throws unspecified any exception resulting from interoperating with floating-point types.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,typename deduce_binary_op_result_type<T,U>::type>::type
			operator+(T &&x, U &&y)
		{
			return binary_plus(std::forward<T>(x),std::forward<U>(y));
		}
		/// Identity operation.
		/**
		 * @return copy of \p this.
		 */
		integer operator+() const
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
		 * 
		 * @throws unspecified any exception resulting from interoperating with floating-point types.
		 */
		template <typename T>
		typename std::enable_if<
			is_interop_type<typename std::decay<T>::type>::value ||
			std::is_same<integer,typename std::decay<T>::type>::value,integer &>::type operator-=(T &&x)
		{
			in_place_sub(std::forward<T>(x));
			return *this;
		}
		/// Generic in-place subtraction with piranha::integer.
		/**
		 * Subtract a piranha::integer in-place. This template operator is activated only if \p T is an \ref interop "interoperable type" and \p I is piranha::integer.
		 * This method will first compute <tt>x - n</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] n second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception resulting from the binary operator or from casting piranha::integer to \p T.
		 */
		template <typename T, typename I>
		friend typename std::enable_if<is_interop_type<T>::value && std::is_same<typename std::decay<I>::type,integer>::value,T &>::type
			operator-=(T &x, I &&n)
		{
			x = static_cast<T>(x - std::forward<I>(n));
			return x;
		}
		/// Generic binary subtraction involving piranha::integer.
		/**
		 * The implementation is equivalent to the generic binary addition operator.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x - y</tt>.
		 * 
		 * @throws unspecified any exception resulting from interoperating with floating-point types.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,typename deduce_binary_op_result_type<T,U>::type>::type
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
		 * 
		 * @throws unspecified any exception resulting from interoperating with floating-point types.
		 */
		template <typename T>
		typename std::enable_if<
			is_interop_type<typename std::decay<T>::type>::value ||
			std::is_same<integer,typename std::decay<T>::type>::value,integer &>::type operator*=(T &&x)
		{
			in_place_mul(std::forward<T>(x));
			return *this;
		}
		/// Generic in-place multiplication with piranha::integer.
		/**
		 * Multiply by a piranha::integer in-place. This template operator is activated only if \p T is an \ref interop "interoperable type" and \p I is piranha::integer.
		 * This method will first compute <tt>n * x</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] n second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception resulting from the binary operator or from casting piranha::integer to \p T.
		 */
		template <typename T, typename I>
		friend typename std::enable_if<is_interop_type<T>::value && std::is_same<typename std::decay<I>::type,integer>::value,T &>::type
			operator*=(T &x, I &&n)
		{
			x = static_cast<T>(std::forward<I>(n) * x);
			return x;
		}
		/// Generic binary multiplication involving piranha::integer.
		/**
		 * The implementation is equivalent to the generic binary addition operator.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x * y</tt>.
		 * 
		 * @throws unspecified any exception resulting from interoperating with floating-point types.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,typename deduce_binary_op_result_type<T,U>::type>::type
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
		 * @throws piranha::zero_division_error if piranha::math::is_zero() returns \p true on \p x.
		 * @throws unspecified any exception resulting from interoperating with floating-point types.
		 */
		template <typename T>
		typename std::enable_if<
			is_interop_type<typename std::decay<T>::type>::value ||
			std::is_same<integer,typename std::decay<T>::type>::value,integer &>::type operator/=(T &&x)
		{
			if (math::is_zero(x)) {
				piranha_throw(zero_division_error,"division by zero");
			}
			in_place_div(std::forward<T>(x));
			return *this;
		}
		/// Generic in-place division with piranha::integer.
		/**
		 * Divide by a piranha::integer in-place. This template operator is activated only if \p T is an \ref interop "interoperable type" and \p I is piranha::integer.
		 * This method will first compute <tt>x / n</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] n second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception resulting from the binary operator or from casting piranha::integer to \p T.
		 */
		template <typename T, typename I>
		friend typename std::enable_if<is_interop_type<T>::value && std::is_same<typename std::decay<I>::type,integer>::value,T &>::type
			operator/=(T &x, I &&n)
		{
			x = static_cast<T>(x / std::forward<I>(n));
			return x;
		}
		/// Generic binary division involving piranha::integer.
		/**
		 * The implementation is equivalent to the generic binary addition operator. When integral operands are involved, the division
		 * will be truncated.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x / y</tt>.
		 * 
		 * @throws piranha::zero_division_error if piranha::math::is_zero() returns \p true on \p y.
		 * @throws unspecified any exception resulting from interoperating with floating-point types.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,typename deduce_binary_op_result_type<T,U>::type>::type
			operator/(T &&x, U &&y)
		{
			return binary_div(std::forward<T>(x),std::forward<U>(y));
		}
		/// In-place modulo operation.
		/**
		 * Sets \p this to <tt>this % n</tt>. This template operator is enabled if \p T is piranha::integer or an integral type among the \ref interop "interoperable types".
		 * \p this must be non-negative and \p n strictly positive, otherwise an \p std::invalid_argument exception will be thrown.
		 * 
		 * @param[in] n argument for the modulo operation.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws std::invalid_argument if <tt>n <= 0</tt> or <tt>this < 0</tt>.
		 */
		template <typename T>
		typename std::enable_if<
			(std::is_integral<typename std::decay<T>::type>::value && is_interop_type<typename std::decay<T>::type>::value) ||
			std::is_same<integer,typename std::decay<T>::type>::value,integer &>::type operator%=(T &&n)
		{
			if (unlikely(sign() < 0)) {
				piranha_throw(std::invalid_argument,"negative dividend");
			}
			in_place_mod(std::forward<T>(n));
			return *this;
		}
		/// Generic in-place modulo operation with piranha::integer.
		/**
		 * Apply the modulo operation by a piranha::integer in-place. This template operator is activated only if \p T is an integral type
		 * among the \ref interop "interoperable types"
		 * and \p I is piranha::integer. This method will first compute <tt>x % n</tt>,
		 * cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] n second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws std::invalid_argument if <tt>n <= 0</tt> or <tt>x < 0</tt>.
		 * @throws unspecified any exception resulting from the binary operator or from casting piranha::integer to \p T.
		 */
		template <typename T, typename I>
		friend typename std::enable_if<std::is_integral<T>::value && is_interop_type<T>::value && std::is_same<typename std::decay<I>::type,integer>::value,T &>::type
			operator%=(T &x, I &&n)
		{
			x = static_cast<T>(x % std::forward<I>(n));
			return x;
		}
		/// Generic binary modulo operation involving piranha::integer.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::integer and \p U is an integral type among the \ref interop "interoperable types",
		 * - \p U is piranha::integer and \p T is an integral type among the \ref interop "interoperable types",
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
		friend typename std::enable_if<
			are_binary_op_types<T,U>::value && !std::is_floating_point<typename std::decay<T>::type>::value && !std::is_floating_point<typename std::decay<U>::type>::value,
			typename deduce_binary_op_result_type<T,U>::type>::type
			operator%(T &&x, U &&y)
		{
			return binary_mod(std::forward<T>(x),std::forward<U>(y));
		}
		/// Generic equality operator involving piranha::integer.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::integer and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::integer and \p T is an \ref interop "interoperable type",
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
		 * 
		 * @throws unspecified any exception resulting from interoperating with floating-point types.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,bool>::type operator==(const T &x, const U &y)
		{
			return binary_equality(x,y);
		}
		/// Generic inequality operator involving piranha::integer.
		/**
		 * The implementation is equivalent to the generic equality operator.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x != y</tt>, \p false otherwise.
		 * 
		 * @throws unspecified any exception resulting from the equality operator.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,bool>::type operator!=(const T &x, const U &y)
		{
			return !(x == y);
		}
		/// Generic less-than operator involving piranha::integer.
		/**
		 * The implementation is equivalent to the generic equality operator.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x < y</tt>, \p false otherwise.
		 * 
		 * @throws unspecified any exception resulting from interoperating with floating-point types.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,bool>::type operator<(const T &x, const U &y)
		{
			return binary_less_than(x,y);
		}
		/// Generic less-than or equal operator involving piranha::integer.
		/**
		 * The implementation is equivalent to the generic equality operator.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x <= y</tt>, \p false otherwise.
		 * 
		 * @throws unspecified any exception resulting from interoperating with floating-point types.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,bool>::type operator<=(const T &x, const U &y)
		{
			return binary_leq(x,y);
		}
		/// Generic greater-than operator involving piranha::integer.
		/**
		 * The implementation is equivalent to the generic equality operator.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x > y</tt>, \p false otherwise.
		 * 
		 * @throws unspecified any exception resulting from the less-than operator.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,bool>::type operator>(const T &x, const U &y)
		{
			return (y < x);
		}
		/// Generic greater-than or equal operator involving piranha::integer.
		/**
		 * The implementation is equivalent to the generic equality operator.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x >= y</tt>, \p false otherwise.
		 * 
		 * @throws unspecified any exception resulting from the less-than or equal operator.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,bool>::type operator>=(const T &x, const U &y)
		{
			return (y <= x);
		}
		/// Combined multiply-add.
		/**
		 * Sets \p this to <tt>this + (n1 * n2)</tt>.
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
		 * Return <tt>this ** exp</tt>. This template method is activated only if \p T is an integral type or integer. Negative
		 * powers are calculated as <tt>(1 / this) ** exp</tt>. Trying to raise zero to a negative exponent will throw a
		 * piranha::zero_division_error exception. <tt>this ** 0</tt> will always return 1.
		 * 
		 * The value of \p exp cannot exceed in absolute value the maximum value representable by the <tt>unsigned long</tt> type, otherwise an
		 * \p std::invalid_argument exception will be thrown.
		 * 
		 * @param[in] exp exponent.
		 * 
		 * @return <tt>this ** exp</tt>.
		 * 
		 * @throws std::invalid_argument if <tt>exp</tt>'s magnitude exceeds the range of the <tt>unsigned long</tt> type.
		 * @throws piranha::zero_division_error if \p this is zero and \p exp is negative.
		 */
		template <typename T>
		typename std::enable_if<std::is_integral<T>::value || std::is_same<T,integer>::value,integer>::type pow(const T &exp) const
		{
			return pow_impl(exp);
		}
		/// Absolute value.
		/**
		 * @return absolute value of \p this.
		 */
		integer abs() const
		{
			integer retval(*this);
			::mpz_abs(retval.m_value,retval.m_value);
			return retval;
		}
		/// Compute next prime number.
		/**
		 * Compute the next prime number after \p this. The method uses the GMP function <tt>mpz_nextprime()</tt>.
		 * 
		 * @return next prime.
		 * 
		 * @throws std::invalid_argument if the sign of this is negative.
		 */
		integer nextprime() const
		{
			if (unlikely(sign() < 0)) {
				piranha_throw(std::invalid_argument,"cannot compute the next prime of a negative number");
			}
			integer retval;
			::mpz_nextprime(retval.m_value,m_value);
			return retval;
		}
		/// Check if \p this is a prime number
		/**
		 * The method uses the GMP function <tt>mpz_probab_prime_p()</tt>.
		 * 
		 * @param[in] reps number of primality tests to be run.
		 * 
		 * @return 2 if \p this is definitely a prime, 1 if \p this is probably prime, 0 if \p this is definitely composite.
		 * 
		 * @throws std::invalid_argument if \p reps is negative.
		 */
		int probab_prime_p(int reps = 8) const
		{
			if (unlikely(reps < 0)) {
				piranha_throw(std::invalid_argument,"invalid number of primality tests");
			}
			return ::mpz_probab_prime_p(m_value,reps);
		}
		/// Integer square root.
		/**
		 * @return the truncated integer part of the square root of \p this.
		 * 
		 * @throws std::invalid_argument if \p this is negative.
		 */
		integer sqrt() const
		{
			if (unlikely(sign() < 0)) {
				piranha_throw(std::invalid_argument,"cannot calculate square root of negative integer");
			}
			integer retval(*this);
			::mpz_sqrt(retval.m_value,m_value);
			return retval;
		}
		/// Hash value.
		/**
		 * The value is calculated via \p boost::hash_combine over the limbs of the internal \p mpz_t type. The sign of \p this
		 * is used as initial seed value.
		 * 
		 * @return a hash value for \p this.
		 * 
		 * @see http://www.boost.org/doc/libs/release/doc/html/hash/reference.html#boost.hash_combine
		 */
		std::size_t hash() const
		{
			return hash_mpz_t(m_value);
		}
		/// Integer size.
		/**
		 * @return number of GMP limbs which the number represented by \p this is made of. If \p this represents 0, the return value will be 0.
		 */
		std::size_t size() const
		{
			return ::mpz_size(m_value);
		}
		/// Number of allocated limbs.
		/**
		 * @return number of GMP limbs currently allocated in \p this. The return type is the unsigned counterpart of the integer
		 * type used to represent the allocated size in GMP's integer type.
		 */
		auto allocated_size() const -> typename std::decay<std::make_unsigned<decltype(std::declval< ::mpz_t>()->_mp_alloc)>::type>::type
		{
			typedef typename std::decay<std::make_unsigned<decltype(std::declval< ::mpz_t>()->_mp_alloc)>::type>::type return_type;
			return return_type(m_value->_mp_alloc);
		}
		/// Sign.
		/**
		 * @return 1 if <tt>this > 0</tt>, 0 if <tt>this == 0</tt> and -1 if <tt>this < 0</tt>.
		 */
		int sign() const
		{
			return mpz_sgn(m_value);
		}
		/// Factorial.
		/**
		 * The GMP function <tt>mpz_fac_ui()</tt> will be used.
		 * 
		 * @return the factorial of \p this.
		 * 
		 * @throws std::invalid_argument if \p this is negative or larger than an implementation-defined value.
		 */
		integer factorial() const
		{
			if (*this > 100000L || sign() < 0) {
				piranha_throw(std::invalid_argument,"invalid input for factorial()");
			}
			integer retval;
			::mpz_fac_ui(retval.m_value,static_cast<unsigned long>(*this));
			return retval;
		}
		/// Binomial coefficient.
		/**
		 * \note
		 * This method is enabled only if \p T is an integral type or piranha::integer.
		 *
		 * Will return \p this choose \p k using the GMP <tt>mpz_bin_ui</tt> function.
		 * 
		 * @param[in] k bottom argument for the binomial coefficient.
		 * 
		 * @return \p this choose \p k.
		 * 
		 * @throws std::invalid_argument if \p k cannot be converted successfully to <tt>unsigned int</tt>.
		 */
		template <typename T>
		integer binomial(const T &k, typename std::enable_if<std::is_integral<T>::value ||
			std::is_same<integer,T>::value>::type * = nullptr) const
		{
			integer retval;
			::mpz_bin_ui(retval.m_value,m_value,check_choose_k(k));
			return retval;
		}
		/// Overload output stream operator for piranha::integer.
		/**
		 * @param[in] os output stream.
		 * @param[in] n piranha::integer to be directed to stream.
		 * 
		 * @return reference to \p os.
		 * 
		 * @throws std::overflow_error if the number of digits is larger than an implementation-defined maximum.
		 * @throws unspecified any exception thrown by memory allocation errors in standard containers.
		 */
		friend std::ostream &operator<<(std::ostream &os, const integer &n)
		{
			const std::size_t size_base10 = ::mpz_sizeinbase(n.m_value,10);
			if (size_base10 > boost::integer_traits<std::size_t>::const_max - static_cast<std::size_t>(2)) {
				piranha_throw(std::overflow_error,"number of digits is too large");
			}
			const auto total_size = size_base10 + 2u;
			// NOTE: here we can optimize, avoiding one allocation, by using a static vector if
			// size_base10 is small enough.
			std::vector<char> tmp(static_cast<std::vector<char>::size_type>(total_size));
			if (tmp.size() != total_size) {
				piranha_throw(std::overflow_error,"number of digits is too large");
			}
			os << ::mpz_get_str(&tmp[0u],10,n.m_value);
			return os;
		}
		/// Overload input stream operator for piranha::integer.
		/**
		 * Equivalent to extracting a line from the stream and then assigning it to \p n.
		 * 
		 * @param[in] is input stream.
		 * @param[in,out] n integer to which the contents of the stream will be assigned.
		 * 
		 * @return reference to \p is.
		 * 
		 * @throws unspecified any exception thrown by the constructor from string of piranha::integer.
		 */
		friend std::istream &operator>>(std::istream &is, integer &n)
		{
			std::string tmp_str;
			std::getline(is,tmp_str);
			n = tmp_str;
			return is;
		}
	private:
		::mpz_t m_value;
};


namespace math
{

/// Specialisation of the implementation of piranha::math::multiply_accumulate() for piranha::integer.
template <typename T>
struct multiply_accumulate_impl<T,T,T,typename std::enable_if<std::is_same<T,integer>::value>::type>
{
	/// Call operator.
 	/**
	 * This implementation will use piranha::integer::multiply_accumulate().
	 * 
	 * @param[in,out] x target value for accumulation.
	 * @param[in] y first argument.
	 * @param[in] z second argument.
	 * 
	 * @return <tt>x.multiply_accumulate(y,z)</tt>.
	 */
	auto operator()(T &x, const T &y, const T &z) const -> decltype(x.multiply_accumulate(y,z))
	{
		return x.multiply_accumulate(y,z);
	}
};

/// Specialisation of the piranha::math::negate() functor for piranha::integer.
template <typename T>
struct negate_impl<T,typename std::enable_if<std::is_same<T,integer>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in,out] n piranha::integer to be negated.
	 */
	void operator()(integer &n) const
	{
		n.negate();
	}
};

/// Specialisation of the piranha::math::is_zero() functor for piranha::integer.
template <typename T>
struct is_zero_impl<T,typename std::enable_if<std::is_same<T,integer>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] n piranha::integer to be tested.
	 * 
	 * @return \p true if \p n is zero, \p false otherwise.
	 */
	bool operator()(const T &n) const
	{
		return n.sign() == 0;
	}
};

}

namespace detail
{

struct integer_private_access
{
	template <typename T>
	using is_interop = integer::is_interop_type<T>;
};

template <typename T, typename U>
using integer_pow_enabler = typename std::enable_if<
	(std::is_same<T,integer>::value && integer_private_access::is_interop<U>::value) ||
	(std::is_same<U,integer>::value && integer_private_access::is_interop<T>::value) ||
	(std::is_same<T,integer>::value && std::is_same<U,integer>::value) ||
	(std::is_integral<T>::value && std::is_integral<U>::value)
>::type;

}

namespace math
{

/// Specialisation of the piranha::math::pow() functor for piranha::integer and integral types.
/**
 * This specialisation is activated when:
 * - one of the arguments is piranha::integer and the other is either
 *   piranha::integer or an interoperable type for piranha::integer,
 * - both arguments are integral types.
 * 
 * The implementation uses a schema similar to the arithmetic operators of piranha::integer. Specifically:
 * - if the arguments are both piranha::integer, or a piranha::integer and an integral type, then piranha::integer::pow() is used
 *   to compute the result (after any necessary conversion),
 * - if both arguments are integral types, piranha::integer::pow() is used after the conversion of the base
 *   to piranha::integer,
 * - otherwise, the piranha::integer argument is converted to the floating-point type and \p std::pow() is
 *   used to compute the result.
 */
template <typename T, typename U>
struct pow_impl<T,U,detail::integer_pow_enabler<T,U>>
{
	/// Call operator, integer--integer overload.
	/**
	 * @param[in] b base
	 * @param[in] e exponent.
	 * 
	 * @returns <tt>b**e</tt>.
	 * 
	 * @throws unspecified any exception thrown by piranha::integer::pow().
	 */
	integer operator()(const integer &b, const integer &e) const
	{
		return b.pow(e);
	}
	/// Call operator, integer--integral overload.
	/**
	 * @param[in] n base
	 * @param[in] x exponent.
	 * 
	 * @returns <tt>n**x</tt>.
	 * 
	 * @throws unspecified any exception thrown by piranha::integer::pow().
	 */
	template <typename T2, typename std::enable_if<std::is_integral<T2>::value,int>::type = 0>
	integer operator()(const integer &n, const T2 &x) const
	{
		return n.pow(x);
	}
	/// Call operator, integral--integer overload.
	/**
	 * @param[in] x base
	 * @param[in] n exponent.
	 * 
	 * @returns <tt>x**n</tt>.
	 * 
	 * @throws unspecified any exception thrown by piranha::integer::pow() or by the constructor of piranha::integer
	 * from integral types.
	 */
	template <typename T2, typename std::enable_if<std::is_integral<T2>::value,int>::type = 0>
	integer operator()(const T2 &x, const integer &n) const
	{
		return integer(x).pow(n);
	}
	/// Call operator, integral--integral overload.
	/**
	 * @param[in] x base
	 * @param[in] y exponent.
	 * 
	 * @returns <tt>x**y</tt>.
	 * 
	 * @throws unspecified any exception thrown by piranha::integer::pow() or by the constructor of piranha::integer
	 * from integral types.
	 */
	template <typename T2, typename U2, typename std::enable_if<std::is_integral<T2>::value &&
		std::is_integral<U2>::value,int>::type = 0>
	integer operator()(const T2 &x, const U2 &y) const
	{
		return integer(x).pow(y);
	}
	/// Call operator, floating-point--integer overload.
	/**
	 * @param[in] x base
	 * @param[in] n exponent.
	 * 
	 * @returns <tt>x**n</tt>.
	 * 
	 * @throws unspecified any exception thrown by converting piranha::integer to a floating-point type.
	 */
	template <typename T2, typename std::enable_if<std::is_floating_point<T2>::value,int>::type = 0>
	auto operator()(const T2 &x, const integer &n) const -> decltype(std::pow(x,static_cast<T2>(n)))
	{
		return std::pow(x,static_cast<T2>(n));
	}
	/// Call operator, integer--floating-point overload.
	/**
	 * @param[in] n base
	 * @param[in] x exponent.
	 * 
	 * @returns <tt>n**x</tt>.
	 * 
	 * @throws unspecified any exception thrown by converting piranha::integer to a floating-point type.
	 */
	template <typename T2, typename std::enable_if<std::is_floating_point<T2>::value,int>::type = 0>
	auto operator()(const integer &n, const T2 &x) const -> decltype(std::pow(static_cast<T2>(n),x))
	{
		return std::pow(static_cast<T2>(n),x);
	}
};

/// Specialisation of the piranha::math::sin() functor for piranha::integer.
template <typename T>
struct sin_impl<T,typename std::enable_if<std::is_same<T,integer>::value>::type>
{
	/// Call operator.
	/**
	 * The operation will return zero if \p n is also zero, otherwise an
	 * exception will be thrown.
	 * 
	 * @param[in] n argument.
	 * 
	 * @return sine of \p n.
	 */
	integer operator()(const T &n) const
	{
		if (n.sign() == 0) {
			return integer{};
		}
		piranha_throw(std::invalid_argument,"cannot calculate the sine of a nonzero integer");
	}
};

/// Specialisation of the piranha::math::cos() functor for piranha::integer.
template <typename T>
struct cos_impl<T,typename std::enable_if<std::is_same<T,integer>::value>::type>
{
	/// Call operator.
	/**
	 * The operation will return one if \p n is zero, otherwise an
	 * exception will be thrown.
	 * 
	 * @param[in] n argument.
	 * 
	 * @return cosine of \p n.
	 */
	integer operator()(const T &n) const
	{
		if (n.sign() == 0) {
			return integer{1};
		}
		piranha_throw(std::invalid_argument,"cannot calculate the cosine of a nonzero integer");
	}
};

/// Specialisation of the piranha::math::partial() functor for piranha::integer.
template <typename T>
struct partial_impl<T,typename std::enable_if<std::is_same<T,integer>::value>::type>
{
	/// Call operator.
	/**
	 * @return an instance of piranha::integer constructed from zero.
	 */
	integer operator()(const integer &, const std::string &) const
	{
		return integer(0);
	}
};

/// Specialisation of the piranha::math::evaluate() functor for piranha::integer.
template <typename T>
struct evaluate_impl<T,typename std::enable_if<std::is_same<T,integer>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] n evaluation argument.
	 * 
	 * @return copy of \p n.
	 */
	template <typename U>
	T operator()(const T &n, const std::unordered_map<std::string,U> &) const
	{
		return n;
	}
};

/// Specialisation of the piranha::math::subs() functor for piranha::integer.
template <typename T>
struct subs_impl<T,typename std::enable_if<std::is_same<T,integer>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] n substitution argument.
	 * 
	 * @return copy of \p n.
	 */
	template <typename U>
	T operator()(const T &n, const std::string &, const U &) const
	{
		return n;
	}
};

/// Specialisation of the piranha::math::abs() functor for piranha::integer.
template <typename T>
struct abs_impl<T,typename std::enable_if<std::is_same<T,integer>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] n input parameter.
	 * 
	 * @return absolute value of \p n.
	 */
	T operator()(const T &n) const
	{
		return n.abs();
	}
};

/// Default implementation of the piranha::math::integral_cast functor.
/**
 * This functor should be specialised using the \p std::enable_if mechanism.
 * Default implementation will not define any call operator.
 */
template <typename T, typename Enable = void>
struct integral_cast_impl
{};

/// Specialisation of the piranha::math::integral_cast functor for floating-point types.
/**
 * The supported floating-point types are those interoperable with piranha::integer.
 */
template <typename T>
struct integral_cast_impl<T,typename std::enable_if<std::is_floating_point<T>::value &&
	std::is_constructible<integer,T>::value>::type>
{
	/// Call operator.
	/**
	 * The call will be successful if \p x is finite and representing an integer value.
	 * 
	 * @param[in] x cast argument.
	 * 
	 * @return result of the cast operation.
	 *
	 * @throws std::invalid_argument if the conversion is not successful.
	 */
	integer operator()(const T &x) const
	{
		if (!boost::math::isfinite(x) || boost::math::trunc(x) != x) {
			piranha_throw(std::invalid_argument,"invalid floating point value");
		}
		return integer{x};
	}
};

/// Specialisation of the piranha::math::integral_cast functor for integral types.
/**
 * The supported integral types are those interoperable with piranha::integer and piranha::integer itself.
 */
template <typename T>
struct integral_cast_impl<T,typename std::enable_if<(std::is_integral<T>::value &&
	std::is_constructible<integer,T>::value) || std::is_same<T,integer>::value>::type>
{
	/// Call operator.
	/**
	 * The call will always be successful and will return a piranha::integer constructed from \p x.
	 * 
	 * @param[in] x cast argument.
	 * 
	 * @return a piranha::integer constructed from \p x.
	 */
	integer operator()(const T &x) const
	{
		return integer(x);
	}
};

/// Integral cast.
/**
 * Will cast input value to piranha::integer if the conversion is exact.
 * The actual implementation of this function is in the piranha::math::integral_cast_impl functor's call operator.
 * Any exception thrown by the implementation will be ignored and recast as std::invalid_argument.
 * 
 * @param[in] x cast argument.
 * 
 * @return \p x cast to piranha::integer if the conversion is exact.
 * 
 * @throws std::invalid_argument if the call operator of piranha::math::integral_cast_impl throws any exception.
 */
template <typename T>
inline auto integral_cast(const T &x) -> decltype(integral_cast_impl<T>()(x))
{
	try {
		return integral_cast_impl<T>()(x);
	} catch (...) {
		piranha_throw(std::invalid_argument,"integral_cast failure");
	}
}

/// Default functor for the implementation of piranha::math::ipow_subs().
/**
 * This functor should be specialised via the \p std::enable_if mechanism. Default implementation will not define
 * the call operator, and will hence result in a compilation error when used.
 */
template <typename T, typename Enable = void>
struct ipow_subs_impl
{};

/// Specialisation of the piranha::math::ipow_subs() functor for arithmetic types.
/**
 * This specialisation is activated when \p T is a C++ arithmetic type.
 * The result will be the input value unchanged.
 */
template <typename T>
struct ipow_subs_impl<T,typename std::enable_if<std::is_arithmetic<T>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] x substitution argument.
	 * 
	 * @return copy of \p x.
	 */
	template <typename U>
	T operator()(const T &x, const std::string &, const integer &, const U &) const
	{
		return x;
	}
};

/// Specialisation of the piranha::math::ipow_subs() functor for piranha::integer.
/**
 * This specialisation is activated when \p T is piranha::integer.
 * The result will be the input value unchanged.
 */
template <typename T>
struct ipow_subs_impl<T,typename std::enable_if<std::is_same<T,integer>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] n substitution argument.
	 * 
	 * @return copy of \p n.
	 */
	template <typename U>
	T operator()(const T &n, const std::string &, const integer &, const U &) const
	{
		return n;
	}
};

/// Substitution of integral power.
/**
 * Substitute the integral power of a symbolic variable with a generic object.
 * The actual implementation of this function is in the piranha::math::ipow_subs_impl functor.
 * 
 * @param[in] x quantity that will be subject to substitution.
 * @param[in] name name of the symbolic variable that will be substituted.
 * @param[in] n power of \p name that will be substituted.
 * @param[in] y object that will substitute the variable.
 * 
 * @return \p x after substitution  of \p name to the power of \p n with \p y.
 * 
 * @throws unspecified any exception thrown by the call operator of piranha::math::subs_impl.
 */
template <typename T, typename U>
inline auto ipow_subs(const T &x, const std::string &name, const integer &n, const U &y) -> decltype(ipow_subs_impl<T>()(x,name,n,y))
{
	return ipow_subs_impl<T>()(x,name,n,y);
}

/// Factorial.
/**
 * @param[in] n factorial argument.
 * 
 * @return the output of piranha::integer::factorial().
 * 
 * @throws unspecified any exception thrown by piranha::integer::factorial().
 */
inline integer factorial(const integer &n)
{
	return n.factorial();
}

/// Specialisation of the piranha::math::binomial() functor for piranha::integer.
/**
 * This specialisation is enabled if \p T is piranha::integer.
 */
template <typename T, typename U>
struct binomial_impl<T,U,typename std::enable_if<std::is_same<T,integer>::value>::type>
{
	/// Call operator.
	/**
	 * \note
	 * This call operator is enabled only if \p k can be used as argument to piranha::integer::binomial().
	 *
	 * @param[in] n top number.
	 * @param[in] k bottom number.
	 * 
	 * @return \p n choose \p k.
	 * 
	 * @throws unspecified any exception thrown by piranha::integer::binomial().
	 */
	template <typename U1>
	auto operator()(const integer &n, const U1 &k) const -> decltype(n.binomial(k))
	{
		return n.binomial(k);
	}
};

/// Specialisation of the piranha::math::binomial() functor for integral types.
/**
 * This specialisation is enabled if \p T is an integral type that can be used to construct piranha::integer.
 */
template <typename T, typename U>
struct binomial_impl<T,U,typename std::enable_if<std::is_integral<T>::value && std::is_constructible<integer,T>::value>::type>
{
	/// Call operator.
	/**
	 * \note
	 * This call operator is enabled only if \p k can be used as argument to piranha::integer::binomial().
	 *
	 * @param[in] n top number.
	 * @param[in] k bottom number.
	 * 
	 * @return \p n choose \p k.
	 * 
	 * @throws unspecified any exception thrown by constructing piranha::integer from \p n or by piranha::integer::binomial().
	 */
	template <typename U1>
	auto operator()(const T &n, const U1 &k) const -> decltype(integer(n).binomial(k))
	{
		return integer(n).binomial(k);
	}
};

}

// NOTE: this needs to be moved after the specialisations for the math functors as
// the static type trait check needs to see them.
inline integer::~integer()
{
	PIRANHA_TT_CHECK(is_cf,integer);
	piranha_assert(m_value->_mp_alloc >= 0);
	if (m_value->_mp_d != 0) {
		::mpz_clear(m_value);
	} else {
		piranha_assert(m_value->_mp_size == 0 && m_value->_mp_alloc == 0);
	}
}

/// Type trait to detect piranha::math::integral_cast().
/**
 * The type trait will be \p true if piranha::math::integral_cast() can be used on instances of type \p T,
 * \p false otherwise.
 */
template <typename T>
class has_integral_cast: detail::sfinae_types
{
		template <typename T1>
		static auto test(const T1 &x) -> decltype(math::integral_cast(x),void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test(std::declval<T>())),yes>::value;
};

template <typename T>
const bool has_integral_cast<T>::value;

/// Type trait to detect the presence of the piranha::math::ipow_subs function.
/**
 * The type trait will be \p true if piranha::math::ipow_subs can be successfully called on instances
 * of type \p T, with an instance of type \p U as substitution argument.
 */
template <typename T, typename U = T>
class has_ipow_subs: detail::sfinae_types
{
		typedef typename std::decay<T>::type Td;
		typedef typename std::decay<U>::type Ud;
		template <typename T1, typename U1>
		static auto test(const T1 &t, const U1 &u) -> decltype(math::ipow_subs(t,std::declval<std::string const &>(),
			std::declval<integer const &>(),u),void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test(std::declval<Td>(),std::declval<Ud>())),yes>::value;
};

template <typename T, typename U>
const bool has_ipow_subs<T,U>::value;

}

namespace std
{

/// Specialization of \p std::swap for piranha::integer.
/**
 * @param[in] n1 first argument.
 * @param[in] n2 second argument.
 * 
 * @see piranha::integer::swap()
 */
template <>
inline void swap(piranha::integer &n1, piranha::integer &n2) noexcept(true)
{
	n1.swap(n2);
}

/// Specialisation of \p std::hash for piranha::integer.
template <>
struct hash<piranha::integer>
{
	/// Result type.
	typedef size_t result_type;
	/// Argument type.
	typedef piranha::integer argument_type;
	/// Hash operator.
	/**
	 * @param[in] n piranha::integer whose hash value will be returned.
	 * 
	 * @return <tt>n.hash()</tt>.
	 * 
	 * @see piranha::integer::hash()
	 */
	result_type operator()(const argument_type &n) const noexcept(true)
	{
		return n.hash();
	}
};

}

#endif
