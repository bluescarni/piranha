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

#ifndef PIRANHA_MP_INTEGER_HPP
#define PIRANHA_MP_INTEGER_HPP

#include <algorithm>
#include <boost/functional/hash.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/math/special_functions/trunc.hpp>
#include <boost/numeric/conversion/bounds.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <cctype> // For std::isdigit().
#include <cstddef>
// NOTE: GMP docs say gmp.h already includes the extern "C" parts.
#include <gmp.h>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_set> // For hash specialisation.
#include <vector>

#include "config.hpp" // For (un)likely.
#include "detail/integer_fwd.hpp"
#include "exceptions.hpp"
#include "math.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Multiprecision integer class.
/**
 * This class represents integer numbers of arbitrary length.
 * The implementation consists of a C++ wrapper around the \p mpz_t type from the multiprecision GMP library.
 * The values are always represented in multiprecision mode, that is, always using dynamically-allocated memory.
 * For operations on small operands, the piranha::integer class will likely offer better performance and should be
 * preferred in general to this class.
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
 * with <tt>long long</tt> and <tt>unsigned long long</tt>, interaction with this types could be slower due to the extra workload of converting such types
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
 * \todo memory optimisations for division (same as those for mult and add)
 * \todo test the swapping arithmetic with a big integer or with operations such as i *= j + k +l
 * \todo test for number of memory allocations: such tests should re-defined GMP memory allocation functions so that they count the allocations, and report
 * their number. It would be useful to check that future GMP versions still behave the way it is assumed in this class regarding allocation requirements
 * for arithmetic operations.
 * \todo investigate implementing *= in terms of *, since it might be slightly faster (create result with constructor from size?)
 * \todo improve interaction with long long via decomposition of operations in long operands
 * \todo improve performance of binary modulo operation when the second argument is a hardware integer
 * \todo investigate replacing lexical_cast with boost::format, as it seems like boost::format might offer better guarantees
 * on the printed value -> but maybe not: http://www.gotw.ca/publications/mill19.htm it seems it should be ok for int types.
 */
class mp_integer
{
		// Make friend with integer.
		friend class integer;
		// C++ arithmetic types supported for interaction with mp_integer.
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
		// Type-trait to detect integral types that can interoperate directly with GMP routines.
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
		static void fp_normal_check(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			if (unlikely(!boost::math::isfinite(x))) {
				piranha_throw(std::invalid_argument,"non-finite floating-point number");
			}
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
			if (str[has_minus] == '0' && (signed_size > 1u || has_minus)) {
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
		void construct_from_arithmetic(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			fp_normal_check(x);
			::mpz_init_set_d(m_value,static_cast<double>(x));
		}
		template <typename T>
		void construct_from_arithmetic(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			::mpz_init_set_si(m_value,static_cast<long>(si));
		}
		template <typename T>
		void construct_from_arithmetic(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			::mpz_init_set_ui(m_value,static_cast<unsigned long>(ui));
		}
		template <typename T>
		void construct_from_arithmetic(const T &ll, typename std::enable_if<std::is_integral<T>::value && !is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			construct_from_string(boost::lexical_cast<std::string>(ll).c_str());
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
		void assign_from_arithmetic(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			fp_normal_check(x);
			::mpz_set_d(m_value,static_cast<double>(x));
		}
		template <typename T>
		void assign_from_arithmetic(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			::mpz_set_si(m_value,static_cast<long>(si));
		}
		template <typename T>
		void assign_from_arithmetic(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			::mpz_set_ui(m_value,static_cast<unsigned long>(ui));
		}
		template <typename T>
		void assign_from_arithmetic(const T &ll, typename std::enable_if<std::is_integral<T>::value && !is_gmp_int<T>::value>::type * = piranha_nullptr)
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
			if (::mpz_cmp_d(m_value,static_cast<double>(boost::numeric::bounds<T>::lowest())) < 0) {
				if (std::numeric_limits<T>::has_infinity) {
					return -std::numeric_limits<T>::infinity();
				} else {
					piranha_throw(std::overflow_error,"cannot convert to floating point type");
				}
			} else if (::mpz_cmp_d(m_value,static_cast<double>(boost::numeric::bounds<T>::highest())) > 0) {
				if (std::numeric_limits<T>::has_infinity) {
					return std::numeric_limits<T>::infinity();
				} else {
					piranha_throw(std::overflow_error,"cannot convert to floating point type");
				}
			} else {
				// NOTE: here the GMP docs warn that this operation can fail in horrid ways,
				// so far never had problems, but if this becomes an issue we can resort to
				// the good old lexical casting.
				return static_cast<T>(::mpz_get_d(m_value));
			}
		}
		// Special handling for bool.
		template <typename T>
		typename std::enable_if<std::is_same<T,bool>::value,T>::type convert_to_impl() const
		{
			return (mpz_sgn(m_value) != 0);
		}
		// In-place addition.
		void in_place_add(const mp_integer &n)
		{
			::mpz_add(m_value,m_value,n.m_value);
		}
		void in_place_add(mp_integer &&n)
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
			is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			if (si >= 0) {
				// NOTE: here we know that whatever type si is, it can be represented by long,
				// and thus also by unsigned long (as si is positive).
				::mpz_add_ui(m_value,m_value,static_cast<unsigned long>(si));
			} else {
				// Neat trick here. See:
				// http://stackoverflow.com/questions/4536095/unary-minus-and-signed-to-unsigned-conversion
				::mpz_sub_ui(m_value,m_value,-static_cast<unsigned long>(si));
			}
		}
		template <typename T>
		void in_place_add(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			::mpz_add_ui(m_value,m_value,static_cast<unsigned long>(ui));
		}
		// For non-gmp ints create a temporary mp_integer and add it.
		template <typename T>
		void in_place_add(const T &n, typename std::enable_if<std::is_integral<T>::value && !is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			in_place_add(mp_integer(n));
		}
		template <typename T>
		void in_place_add(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			operator=(static_cast<T>(*this) + x);
		}
		// In-place subtraction.
		void in_place_sub(const mp_integer &n)
		{
			::mpz_sub(m_value,m_value,n.m_value);
		}
		void in_place_sub(mp_integer &&n)
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
			is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			if (si >= 0) {
				::mpz_sub_ui(m_value,m_value,static_cast<unsigned long>(si));
			} else {
				::mpz_add_ui(m_value,m_value,-static_cast<unsigned long>(si));
			}
		}
		template <typename T>
		void in_place_sub(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			::mpz_sub_ui(m_value,m_value,static_cast<unsigned long>(ui));
		}
		template <typename T>
		void in_place_sub(const T &n, typename std::enable_if<std::is_integral<T>::value && !is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			in_place_sub(mp_integer(n));
		}
		template <typename T>
		void in_place_sub(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			operator=(static_cast<T>(*this) - x);
		}
		// In-place multiplication.
		void in_place_mul(const mp_integer &n)
		{
			::mpz_mul(m_value,m_value,n.m_value);
		}
		void in_place_mul(mp_integer &&n)
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
			is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			::mpz_mul_si(m_value,m_value,static_cast<long>(si));
		}
		template <typename T>
		void in_place_mul(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			::mpz_mul_ui(m_value,m_value,static_cast<unsigned long>(ui));
		}
		template <typename T>
		void in_place_mul(const T &n, typename std::enable_if<std::is_integral<T>::value && !is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			in_place_mul(mp_integer(n));
		}
		template <typename T>
		void in_place_mul(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			operator=(static_cast<T>(*this) * x);
		}
		// In-place division.
		void in_place_div(const mp_integer &n)
		{
			if (unlikely(mpz_sgn(n.m_value) == 0)) {
				piranha_throw(piranha::zero_division_error,"division by zero");
			}
			::mpz_tdiv_q(m_value,m_value,n.m_value);
		}
		template <typename T>
		void in_place_div(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			is_gmp_int<T>::value>::type * = piranha_nullptr)
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
		void in_place_div(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			if (unlikely(ui == 0u)) {
				piranha_throw(piranha::zero_division_error,"division by zero");
			}
			::mpz_tdiv_q_ui(m_value,m_value,static_cast<unsigned long>(ui));
		}
		template <typename T>
		void in_place_div(const T &n, typename std::enable_if<std::is_integral<T>::value && !is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			in_place_div(mp_integer(n));
		}
		template <typename T>
		void in_place_div(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			if (unlikely(x == 0.)) {
				piranha_throw(piranha::zero_division_error,"division by zero");
			}
			operator=(static_cast<T>(*this) / x);
		}
		// In-place modulo.
		void in_place_mod(const mp_integer &n)
		{
			if (unlikely(mpz_sgn(n.m_value) <= 0)) {
				piranha_throw(std::invalid_argument,"non-positive divisor");
			}
			::mpz_mod(m_value,m_value,n.m_value);
		}
		template <typename T>
		void in_place_mod(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			if (unlikely(si <= 0)) {
				piranha_throw(std::invalid_argument,"non-positive divisor");
			}
			*this = ::mpz_fdiv_ui(m_value,static_cast<unsigned long>(si));
		}
		template <typename T>
		void in_place_mod(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			if (unlikely(ui == 0u)) {
				piranha_throw(std::invalid_argument,"non-positive divisor");
			}
			*this = ::mpz_fdiv_ui(m_value,static_cast<unsigned long>(ui));
		}
		template <typename T>
		void in_place_mod(const T &n, typename std::enable_if<std::is_integral<T>::value && !is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			in_place_mod(mp_integer(n));
		}
		// Binary operations.
		// Type trait for allowed arguments in arithmetic binary operations.
		template <typename T, typename U>
		struct are_binary_op_types: std::integral_constant<bool,
			(std::is_same<typename std::decay<T>::type,mp_integer>::value && is_interop_type<typename std::decay<U>::type>::value) ||
			(std::is_same<typename std::decay<U>::type,mp_integer>::value && is_interop_type<typename std::decay<T>::type>::value) ||
			(std::is_same<typename std::decay<T>::type,mp_integer>::value && std::is_same<typename std::decay<U>::type,mp_integer>::value)>
		{};
		// Metaprogramming to establish the return type of binary arithmetic operations involving mp_integers.
		// Default result type will be mp_integer itself; for consistency with C/C++ when one of the arguments
		// is a floating point type, we will return a value of the same floating point type.
		template <typename T, typename U, typename Enable = void>
		struct deduce_binary_op_result_type
		{
			typedef mp_integer type;
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
		static mp_integer binary_plus(T &&n1, U &&n2, typename std::enable_if<
			std::is_same<typename std::decay<T>::type,mp_integer>::value && std::is_same<typename std::decay<U>::type,mp_integer>::value
			>::type * = piranha_nullptr)
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
		static mp_integer binary_plus_fwd_first(T &&n, const U &x)
		{
			mp_integer retval(std::forward<T>(n));
			retval.in_place_add(x);
			return retval;
		}
		template <typename T>
		static mp_integer binary_plus_new(const mp_integer &n1, const T &n2, const std::size_t &target_size)
		{
			mp_integer retval{nlimbs(target_size)};
			retval = n1;
			retval.in_place_add(n2);
			return retval;
		}
		template <typename T, typename U>
		static mp_integer binary_plus(T &&n1, U &&n2, typename std::enable_if<
			std::is_same<typename std::decay<T>::type,mp_integer>::value && std::is_integral<typename std::decay<U>::type>::value
			>::type * = piranha_nullptr)
		{
			const std::size_t target_size = n1.size() + std::size_t(1);
			if (n1.allocated_size() >= target_size) {
				return binary_plus_fwd_first(std::forward<T>(n1),n2);
			} else {
				return binary_plus_new(n1,n2,target_size);
			}
		}
		template <typename T, typename U>
		static mp_integer binary_plus(T &&n1, U &&n2, typename std::enable_if<
			std::is_same<typename std::decay<U>::type,mp_integer>::value && std::is_integral<typename std::decay<T>::type>::value
			>::type * = piranha_nullptr)
		{
			const std::size_t target_size = n2.size() + std::size_t(1);
			if (n2.allocated_size() >= target_size) {
				return binary_plus_fwd_first(std::forward<U>(n2),n1);
			} else {
				return binary_plus_new(n2,n1,target_size);
			}
		}
		template <typename T>
		static T binary_plus(const mp_integer &n, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			return (static_cast<T>(n) + x);
		}
		template <typename T>
		static T binary_plus(const T &x, const mp_integer &n, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			return binary_plus(n,x);
		}
		// Binary subtraction.
		template <typename T, typename U>
		static mp_integer binary_minus(T &&n1, U &&n2, typename std::enable_if<
			std::is_same<typename std::decay<T>::type,mp_integer>::value && std::is_same<typename std::decay<U>::type,mp_integer>::value
			>::type * = piranha_nullptr)
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
		static mp_integer binary_minus_fwd_first(T &&n, const U &x)
		{
			mp_integer retval(std::forward<T>(n));
			retval.in_place_sub(x);
			if (!Order) {
				retval.negate();
			}
			return retval;
		}
		template <bool Order, typename T>
		static mp_integer binary_minus_new(const mp_integer &n1, const T &n2, const std::size_t &target_size)
		{
			mp_integer retval{nlimbs(target_size)};
			retval = n1;
			retval.in_place_sub(n2);
			if (!Order) {
				retval.negate();
			}
			return retval;
		}
		template <typename T, typename U>
		static mp_integer binary_minus(T &&n1, U &&n2, typename std::enable_if<
			std::is_same<typename std::decay<T>::type,mp_integer>::value && std::is_integral<typename std::decay<U>::type>::value
			>::type * = piranha_nullptr)
		{
			const std::size_t target_size = n1.size() + std::size_t(1);
			if (n1.allocated_size() >= target_size) {
				return binary_minus_fwd_first<true>(std::forward<T>(n1),n2);
			} else {
				return binary_minus_new<true>(n1,n2,target_size);
			}
		}
		template <typename T, typename U>
		static mp_integer binary_minus(T &&n1, U &&n2, typename std::enable_if<
			std::is_same<typename std::decay<U>::type,mp_integer>::value && std::is_integral<typename std::decay<T>::type>::value
			>::type * = piranha_nullptr)
		{
			const std::size_t target_size = n2.size() + std::size_t(1);
			if (n2.allocated_size() >= target_size) {
				return binary_minus_fwd_first<false>(std::forward<U>(n2),n1);
			} else {
				return binary_minus_new<false>(n2,n1,target_size);
			}
		}
		template <typename T>
		static T binary_minus(const mp_integer &n, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			return (static_cast<T>(n) - x);
		}
		template <typename T>
		static T binary_minus(const T &x, const mp_integer &n, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			return -binary_minus(n,x);
		}
		// Binary multiplication.
		template <typename T, typename U>
		static mp_integer binary_mul(T &&n1, U &&n2, typename std::enable_if<
			std::is_same<typename std::decay<T>::type,mp_integer>::value &&
			std::is_same<typename std::decay<U>::type,mp_integer>::value
			>::type * = piranha_nullptr)
		{
			const auto a1 = n1.allocated_size(), a2 = n2.allocated_size();
			const std::size_t s1 = n1.size(), s2 = n2.size(), target_size = s1 + s2 + std::size_t(1);
			// Condition for stealing: one mp_integer has size 1 and the other has allocated enough space.
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
		static mp_integer binary_mul_fwd_first(T &&n, const U &x)
		{
			mp_integer retval(std::forward<T>(n));
			retval.in_place_mul(x);
			return retval;
		}
		static mp_integer binary_mul_new(const mp_integer &n1, const mp_integer &n2, const std::size_t &target_size)
		{
			mp_integer retval{nlimbs(target_size)};
			::mpz_mul(retval.m_value,n1.m_value,n2.m_value);
			return retval;
		}
		template <typename T>
		static mp_integer binary_mul_new(const mp_integer &n1, const T &n2, const std::size_t &target_size)
		{
			mp_integer retval{nlimbs(target_size)};
			retval = n1;
			retval.in_place_mul(n2);
			return retval;
		}
		template <typename T, typename U>
		static mp_integer binary_mul(T &&n1, U &&n2, typename std::enable_if<
			std::is_same<typename std::decay<T>::type,mp_integer>::value && std::is_integral<typename std::decay<U>::type>::value
			>::type * = piranha_nullptr)
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
		static mp_integer binary_mul(T &&n1, U &&n2, typename std::enable_if<
			std::is_same<typename std::decay<U>::type,mp_integer>::value && std::is_integral<typename std::decay<T>::type>::value
			>::type * = piranha_nullptr)
		{
			const std::size_t target_size = n2.size() + std::size_t(2);
			if (n2.allocated_size() >= target_size) {
				return binary_mul_fwd_first(std::forward<U>(n2),n1);
			} else {
				return binary_mul_new(n2,n1,target_size);
			}
		}
		template <typename T>
		static T binary_mul(const mp_integer &n, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			return (static_cast<T>(n) * x);
		}
		template <typename T>
		static T binary_mul(const T &x, const mp_integer &n, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			return binary_mul(n,x);
		}
		// Binary division.
		template <typename T, typename U>
		static mp_integer binary_div(T &&n1, U &&n2, typename std::enable_if<
			are_binary_op_types<T,U>::value &&
			!std::is_floating_point<typename std::decay<T>::type>::value && !std::is_floating_point<typename std::decay<U>::type>::value
			>::type * = piranha_nullptr)
		{
			// NOTE: here it makes sense only to attempt to steal n1's storage, not n2's, because the operation is not commutative.
			mp_integer retval(std::forward<T>(n1));
			retval /= std::forward<U>(n2);
			return retval;
		}
		template <typename T>
		static T binary_div(const mp_integer &n, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			if (unlikely(x == 0.)) {
				piranha_throw(piranha::zero_division_error,"division by zero");
			}
			return (static_cast<T>(n) / x);
		}
		template <typename T>
		static T binary_div(const T &x, const mp_integer &n, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			const T n_T = static_cast<T>(n);
			if (unlikely(n_T == 0.)) {
				piranha_throw(piranha::zero_division_error,"division by zero");
			}
			return (x / n_T);
		}
		// Binary modulo operation.
		template <typename T, typename U>
		static mp_integer binary_mod(T &&n1, U &&n2, typename std::enable_if<
			are_binary_op_types<T,U>::value &&
			!std::is_floating_point<typename std::decay<T>::type>::value && !std::is_floating_point<typename std::decay<U>::type>::value
			>::type * = piranha_nullptr)
		{
			mp_integer retval(std::forward<T>(n1));
			retval %= std::forward<U>(n2);
			return retval;
		}
		// Binary equality.
		static bool binary_equality(const mp_integer &n1, const mp_integer &n2)
		{
			return (::mpz_cmp(n1.m_value,n2.m_value) == 0);
		}
		template <typename T>
		static bool binary_equality(const mp_integer &n1, const T &n2,typename std::enable_if<std::is_signed<T>::value &&
			is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			return (mpz_cmp_si(n1.m_value,static_cast<long>(n2)) == 0);
		}
		template <typename T>
		static bool binary_equality(const mp_integer &n1, const T &n2,typename std::enable_if<std::is_unsigned<T>::value &&
			is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			return (mpz_cmp_ui(n1.m_value,static_cast<unsigned long>(n2)) == 0);
		}
		template <typename T>
		static bool binary_equality(const mp_integer &n1, const T &n2, typename std::enable_if<std::is_integral<T>::value && !is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			return binary_equality(n1,mp_integer(n2));
		}
		template <typename T>
		static bool binary_equality(const mp_integer &n, const T &x,typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			return (static_cast<T>(n) == x);
		}
		template <typename T>
		static bool binary_equality(const T &x, const mp_integer &n, typename std::enable_if<std::is_arithmetic<T>::value>::type * = piranha_nullptr)
		{
			return binary_equality(n,x);
		}
		// Binary less-than.
		static bool binary_less_than(const mp_integer &n1, const mp_integer &n2)
		{
			return (::mpz_cmp(n1.m_value,n2.m_value) < 0);
		}
		template <typename T>
		static bool binary_less_than(const mp_integer &n1, const T &n2,typename std::enable_if<std::is_signed<T>::value &&
			is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			return (mpz_cmp_si(n1.m_value,static_cast<long>(n2)) < 0);
		}
		template <typename T>
		static bool binary_less_than(const mp_integer &n1, const T &n2,typename std::enable_if<std::is_unsigned<T>::value &&
			is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			return (mpz_cmp_ui(n1.m_value,static_cast<unsigned long>(n2)) < 0);
		}
		template <typename T>
		static bool binary_less_than(const mp_integer &n1, const T &n2, typename std::enable_if<std::is_integral<T>::value &&
			!is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			return binary_less_than(n1,mp_integer(n2));
		}
		template <typename T>
		static bool binary_less_than(const mp_integer &n, const T &x,typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			return (static_cast<T>(n) < x);
		}
		// Binary less-than or equal.
		static bool binary_leq(const mp_integer &n1, const mp_integer &n2)
		{
			return (::mpz_cmp(n1.m_value,n2.m_value) <= 0);
		}
		template <typename T>
		static bool binary_leq(const mp_integer &n1, const T &n2,typename std::enable_if<std::is_signed<T>::value &&
			is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			return (mpz_cmp_si(n1.m_value,static_cast<long>(n2)) <= 0);
		}
		template <typename T>
		static bool binary_leq(const mp_integer &n1, const T &n2,typename std::enable_if<std::is_unsigned<T>::value &&
			is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			return (mpz_cmp_ui(n1.m_value,static_cast<unsigned long>(n2)) <= 0);
		}
		template <typename T>
		static bool binary_leq(const mp_integer &n1, const T &n2, typename std::enable_if<std::is_integral<T>::value &&
			!is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			return binary_leq(n1,mp_integer(n2));
		}
		template <typename T>
		static bool binary_leq(const mp_integer &n, const T &x,typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			return (static_cast<T>(n) <= x);
		}
		// Inverse forms of less-than and leq.
		template <typename T>
		static bool binary_less_than(const T &x, const mp_integer &n, typename std::enable_if<std::is_arithmetic<T>::value>::type * = piranha_nullptr)
		{
			return !binary_leq(n,x);
		}
		template <typename T>
		static bool binary_leq(const T &x, const mp_integer &n, typename std::enable_if<std::is_arithmetic<T>::value>::type * = piranha_nullptr)
		{
			return !binary_less_than(n,x);
		}
		// Exponentiation.
		template <typename T>
		mp_integer pow_impl(const T &ui, typename std::enable_if<std::is_integral<T>::value && !std::is_signed<T>::value>::type * = piranha_nullptr) const
		{
			unsigned long exp;
			try {
				exp = boost::numeric_cast<unsigned long>(ui);
			} catch (const boost::numeric::bad_numeric_cast &) {
				piranha_throw(std::invalid_argument,"invalid argument for integer exponentiation");
			}
			mp_integer retval;
			::mpz_pow_ui(retval.m_value,m_value,exp);
			return retval;
		}
		template <typename T>
		mp_integer pow_impl(const T &si, typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type * = piranha_nullptr) const
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
		mp_integer pow_impl(const mp_integer &n) const
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
		mp_integer pow_impl(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr) const
		{
			if (!boost::math::isfinite(x)) {
				piranha_throw(std::invalid_argument,"invalid argument for integer exponentiation: non-finite floating-point");
			}
			if (boost::math::trunc(x) != x) {
				piranha_throw(std::invalid_argument,"invalid argument for integer exponentiation: floating-point with non-zero fractional part");
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
		mp_integer()
		{
			::mpz_init(m_value);
		}
		/// Class for the representation of the number of limbs in a piranha::mp_integer.
		/**
		 * This class is used in the constructor of piranha::mp_integer from size, and
		 * it represents the number of limbs that will be allocated for the constructed object.
		 */
		class nlimbs
		{
				friend class mp_integer;
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
				 * @param[in] n number of limbs that will be allocated when constructing a piranha::mp_integer.
				 */
				explicit nlimbs(const std::size_t &n):m_n(n) {}
			private:
				const std::size_t m_n;
		};
		/// Constructor from number of limbs.
		/**
		 * Construct a piranha::mp_integer object initialised to zero and with pre-allocated space of \p n limbs.
		 * A limb is, in the jargon of the GMP library, "the part of a multi-precision number that fits into a single
		 * machine word". The GMP global variable \p mp_bits_per_limb represents the number of bits contained in a limb
		 * (typically 32 or 64).
		 * 
		 * Example usage:
		 * @code
		 * using namespace piranha;
		 * mp_integer n(mp_integer::nlimbs(4));
		 * @endcode
		 * 
		 * @param[in] n number of limbs to allocate during construction.
		 * 
		 * @see http://gmplib.org/manual/Nomenclature-and-Types.html.
		 */
		explicit mp_integer(const nlimbs &n)
		{
			// NOTE: use unsigned types everywhere, so that in case of overflow we just allocate a different amount of memory.
			::mpz_init2(m_value,n.m_n * std::make_unsigned<decltype(::mp_bits_per_limb)>::type(::mp_bits_per_limb));
		}
		/// Copy constructor.
		/**
		 * @param[in] other piranha::mp_integer to be deep-copied.
		 */
		mp_integer(const mp_integer &other)
		{
			::mpz_init_set(m_value,other.m_value);
		}
		/// Move constructor.
		/**
		 * @param[in] other piranha::mp_integer to be moved.
		 */
		mp_integer(mp_integer &&other) piranha_noexcept_spec(true)
		{
			m_value->_mp_size = other.m_value->_mp_size;
			m_value->_mp_d = other.m_value->_mp_d;
			m_value->_mp_alloc = other.m_value->_mp_alloc;
			// Erase other.
			other.m_value->_mp_size = 0;
			other.m_value->_mp_d = 0;
			other.m_value->_mp_alloc = 0;
		}
		/// Generic constructor.
		/**
		 * The supported types for \p T are the \ref interop "interoperable types".
		 * Use of other types will result in a compile-time error.
		 * In case a floating-point type is used, \p x will be truncated (i.e., rounded towards zero) before being used to construct
		 * the object.
		 * 
		 * @param[in] x object used to construct \p this.
		 * 
		 * @throws std::invalid_argument if \p x is a non-finite floating-point number.
		 */
		template <typename T>
		explicit mp_integer(const T &x, typename std::enable_if<is_interop_type<T>::value>::type * = piranha_nullptr)
		{
			construct_from_arithmetic(x);
		}
		/// Constructor from string.
		/**
		 * The string must be a sequence of decimal digits, preceded by a minus sign for
		 * strictly negative numbers. The first digit of a non-zero number must not be zero. A malformed string will throw an \p std::invalid_argument
		 * exception.
		 * 
		 * @param[in] str decimal string representation of the number used to initialise the piranha::mp_integer object.
		 * 
		 * @throws std::invalid_argument if the string is malformed.
		 */
		explicit mp_integer(const std::string &str)
		{
			construct_from_string(str.c_str());
		}
		/// Constructor from C string.
		/**
		 * @param[in] str decimal string representation of the number used to initialise the piranha::mp_integer object.
		 * 
		 * @see mp_integer(const std::string &)
		 */
		explicit mp_integer(const char *str)
		{
			construct_from_string(str);
		}
		/// Destructor.
		/**
		 * Will clear the internal \p mpz_t type.
		 */
		~mp_integer() piranha_noexcept_spec(true)
		{
			piranha_assert(m_value->_mp_alloc >= 0);
			// The rationale for using unlikely here is that mpz_clear is an expensive operation
			// which is going to dominate anyway the branch penalty.
			if (unlikely(m_value->_mp_d != 0)) {
				::mpz_clear(m_value);
			} else {
				piranha_assert(m_value->_mp_size == 0 && m_value->_mp_alloc == 0);
			}
		}
		/// Move assignment operator.
		/**
		 * @param[in] other piranha::mp_integer to be moved.
		 * 
		 * @return reference to \p this.
		 */
		mp_integer &operator=(mp_integer &&other) piranha_noexcept_spec(true)
		{
			// NOTE: swap() already has the check for this.
			swap(other);
			return *this;
		}
		/// Assignment operator.
		/**
		 * @param[in] other piranha::mp_integer to be assigned.
		 * 
		 * @return reference to \p this.
		 */
		mp_integer &operator=(const mp_integer &other)
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
		 * @param[in] str string representation of the piranha::mp_integer to be assigned.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws std::invalid_argument if the string is malformed.
		 */
		mp_integer &operator=(const std::string &str)
		{
			return operator=(str.c_str());
		}
		/// Assignment operator from C string.
		/**
		 * @param[in] str string representation of the piranha::mp_integer to be assigned.
		 * 
		 * @return reference to \p this.
		 * 
		 * @see operator=(const std::string &)
		 */
		mp_integer &operator=(const char *str)
		{
			if (m_value->_mp_d) {
				assign_from_string(str);
			} else {
				piranha_assert(m_value->_mp_size == 0 && m_value->_mp_alloc == 0);
				construct_from_string(str);
			}
			return *this;
		}
		/// Generic assignment from interoperable types.
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
		typename std::enable_if<is_interop_type<T>::value,mp_integer &>::type operator=(const T &x)
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
		void swap(mp_integer &n) piranha_noexcept_spec(true)
		{
			if (unlikely(this == &n)) {
			    return;
			}
			::mpz_swap(m_value,n.m_value);
		}
		/// Conversion to interoperable types.
		/**
		 * Extract an instance of type \p T from \p this. The supported types for \p T are the \ref interop "interoperable types".
		 * 
		 * Conversion to \p bool is always successful, and returns <tt>this != 0</tt>.
		 * Conversion to the other integral types is exact, its success depending on whether or not
		 * the target type can represent the current value of the piranha::mp_integer.
		 * 
		 * Conversion to floating point types is exact if the target type can represent exactly the current value of the piranha::mp_integer.
		 * If that is not the case, the output value will be one of the two adjacents (with an unspecified rounding direction).
		 * 
		 * Return values of +-inf will be produced if the current value of the piranha::mp_integer overflows the range of the floating-point type
		 * and the floating-point type can represent infinity. Otherwise, an overflow error will be produced.
		 * 
		 * @return result of the conversion to target type T.
		 * 
		 * @throws std::overflow_error if the conversion to an integral type other than bool results in (negative) overflow, or if
		 * conversion to a floating-point type lacking infinity overflows.
		 */
		template <typename T, typename std::enable_if<is_interop_type<T>::value>::type*& = enabler>
		explicit operator T() const
		{
			return convert_to_impl<T>();
		}
		/// In-place addition.
		/**
		 * Add \p x to the current value of the piranha::mp_integer object. This template operator is activated only if
		 * \p T is either piranha::mp_integer or an \ref interop "interoperable type".
		 * 
		 * If \p T is piranha::mp_integer or an integral type, the result will be exact. If \p T is a floating-point type, the following
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
		 * @throws unspecified any exception resulting from operating on non-finite floating-point values or from failures in floating-point conversions.
		 */
		template <typename T>
		typename std::enable_if<
			is_interop_type<typename std::decay<T>::type>::value ||
			std::is_same<mp_integer,typename std::decay<T>::type>::value,mp_integer &>::type operator+=(T &&x)
		{
			in_place_add(std::forward<T>(x));
			return *this;
		}
		/// Generic in-place addition with piranha::mp_integer.
		/**
		 * Add a piranha::mp_integer in-place. This template operator is activated only if \p T is an \ref interop "interoperable type" and \p I is piranha::mp_integer.
		 * This method will first compute <tt>n + x</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] n second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception resulting from casting piranha::mp_integer to \p T.
		 */
		template <typename T, typename I>
		friend typename std::enable_if<is_interop_type<T>::value && std::is_same<typename std::decay<I>::type,mp_integer>::value,T &>::type
			operator+=(T &x, I &&n)
		{
			x = static_cast<T>(std::forward<I>(n) + x);
			return x;
		}
		/// Generic binary addition involving piranha::mp_integer.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::mp_integer and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::mp_integer and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::mp_integer.
		 * 
		 * If no floating-point types are involved, the exact result of the operation will be returned as a piranha::mp_integer.
		 * 
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and added to \p f to generate the return value, which will then be of type \p F.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x + y</tt>.
		 * 
		 * @throws unspecified any exception resulting from the conversion of piranha::mp_integer to floating-point types.
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
		mp_integer operator+() const
		{
			return *this;
		}
		/// Prefix increment.
		/**
		 * Increment \p this by one.
		 * 
		 * @return reference to \p this after the increment.
		 */
		mp_integer &operator++()
		{
			return operator+=(1);
		}
		/// Suffix increment.
		/**
		 * Increment \p this by one and return a copy of \p this as it was before the increment.
		 * 
		 * @return copy of \p this before the increment.
		 */
		mp_integer operator++(int)
		{
			const mp_integer retval(*this);
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
		 * @throws unspecified any exception resulting from operating on non-finite floating-point values or from failures in floating-point conversions.
		 */
		template <typename T>
		typename std::enable_if<
			is_interop_type<typename std::decay<T>::type>::value ||
			std::is_same<mp_integer,typename std::decay<T>::type>::value,mp_integer &>::type operator-=(T &&x)
		{
			in_place_sub(std::forward<T>(x));
			return *this;
		}
		/// Generic in-place subtraction with piranha::mp_integer.
		/**
		 * Subtract a piranha::mp_integer in-place. This template operator is activated only if \p T is an \ref interop "interoperable type" and \p I is piranha::mp_integer.
		 * This method will first compute <tt>x - n</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] n second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception resulting from casting piranha::mp_integer to \p T.
		 */
		template <typename T, typename I>
		friend typename std::enable_if<is_interop_type<T>::value && std::is_same<typename std::decay<I>::type,mp_integer>::value,T &>::type
			operator-=(T &x, I &&n)
		{
			x = static_cast<T>(x - std::forward<I>(n));
			return x;
		}
		/// Generic binary subtraction involving piranha::mp_integer.
		/**
		 * The implementation is equivalent to the generic binary addition operator.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x - y</tt>.
		 * 
		 * @throws unspecified any exception resulting from the conversion of piranha::mp_integer to floating-point types.
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
		mp_integer operator-() const
		{
			mp_integer retval(*this);
			retval.negate();
			return retval;
		}
		/// Prefix decrement.
		/**
		 * Decrement \p this by one and return.
		 * 
		 * @return reference to \p this.
		 */
		mp_integer &operator--()
		{
			return operator-=(1);
		}
		/// Suffix decrement.
		/**
		 * Decrement \p this by one and return a copy of \p this as it was before the decrement.
		 * 
		 * @return copy of \p this before the decrement.
		 */
		mp_integer operator--(int)
		{
			const mp_integer retval(*this);
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
		 * @throws unspecified any exception resulting from operating on non-finite floating-point values or from failures in floating-point conversions.
		 */
		template <typename T>
		typename std::enable_if<
			is_interop_type<typename std::decay<T>::type>::value ||
			std::is_same<mp_integer,typename std::decay<T>::type>::value,mp_integer &>::type operator*=(T &&x)
		{
			in_place_mul(std::forward<T>(x));
			return *this;
		}
		/// Generic in-place multiplication with piranha::mp_integer.
		/**
		 * Multiply by a piranha::mp_integer in-place. This template operator is activated only if \p T is an \ref interop "interoperable type" and \p I is piranha::mp_integer.
		 * This method will first compute <tt>n * x</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] n second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception resulting from casting piranha::mp_integer to \p T.
		 */
		template <typename T, typename I>
		friend typename std::enable_if<is_interop_type<T>::value && std::is_same<typename std::decay<I>::type,mp_integer>::value,T &>::type
			operator*=(T &x, I &&n)
		{
			x = static_cast<T>(std::forward<I>(n) * x);
			return x;
		}
		/// Generic binary multiplication involving piranha::mp_integer.
		/**
		 * The implementation is equivalent to the generic binary addition operator.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x * y</tt>.
		 * 
		 * @throws unspecified any exception resulting from the conversion of piranha::mp_integer to floating-point types.
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
		 * Division by piranha::mp_integer or by an integral type is truncated.
		 * Trying to divide by zero will throw a piranha::zero_division_error exception.
		 * 
		 * @param[in] x argument for the division.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws piranha::zero_division_error if <tt>x == 0</tt>.
		 * @throws unspecified any exception resulting from operating on non-finite floating-point values or from failures in floating-point conversions.
		 */
		template <typename T>
		typename std::enable_if<
			is_interop_type<typename std::decay<T>::type>::value ||
			std::is_same<mp_integer,typename std::decay<T>::type>::value,mp_integer &>::type operator/=(T &&x)
		{
			in_place_div(std::forward<T>(x));
			return *this;
		}
		/// Generic in-place division with piranha::mp_integer.
		/**
		 * Divide by a piranha::mp_integer in-place. This template operator is activated only if \p T is an \ref interop "interoperable type" and \p I is piranha::mp_integer.
		 * This method will first compute <tt>x / n</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] n second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws piranha::zero_division_error if <tt>n == 0</tt>.
		 * @throws unspecified any exception resulting from casting piranha::mp_integer to \p T.
		 */
		template <typename T, typename I>
		friend typename std::enable_if<is_interop_type<T>::value && std::is_same<typename std::decay<I>::type,mp_integer>::value,T &>::type
			operator/=(T &x, I &&n)
		{
			x = static_cast<T>(x / std::forward<I>(n));
			return x;
		}
		/// Generic binary division involving piranha::mp_integer.
		/**
		 * The implementation is equivalent to the generic binary addition operator. When integral operands are involved, the division
		 * will be truncated.
		 * 
		 * @return <tt>x / y</tt>.
		 * 
		 * @throws piranha::zero_division_error if <tt>y == 0</tt>.
		 * @throws unspecified any exception resulting from the conversion of piranha::mp_integer to floating-point types.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,typename deduce_binary_op_result_type<T,U>::type>::type
			operator/(T &&x, U &&y)
		{
			return binary_div(std::forward<T>(x),std::forward<U>(y));
		}
		/// In-place modulo operation.
		/**
		 * Sets \p this to <tt>this % n</tt>. This template operator is enabled if \p T is piranha::mp_integer or an integral type among the \ref interop "interoperable types".
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
			std::is_same<mp_integer,typename std::decay<T>::type>::value,mp_integer &>::type operator%=(T &&n)
		{
			if (unlikely(mpz_sgn(m_value) < 0)) {
				piranha_throw(std::invalid_argument,"negative dividend");
			}
			in_place_mod(std::forward<T>(n));
			return *this;
		}
		/// Generic in-place modulo operation with piranha::mp_integer.
		/**
		 * Apply the modulo operation by a piranha::mp_integer in-place. This template operator is activated only if \p T is an integral type
		 * among the \ref interop "interoperable types"
		 * and \p I is piranha::mp_integer. This method will first compute <tt>x % n</tt>,
		 * cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] n second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws std::invalid_argument if <tt>n <= 0</tt> or <tt>x < 0</tt>.
		 * @throws unspecified any exception resulting from casting piranha::mp_integer to \p T.
		 */
		template <typename T, typename I>
		friend typename std::enable_if<std::is_integral<T>::value && is_interop_type<T>::value && std::is_same<typename std::decay<I>::type,mp_integer>::value,T &>::type
			operator%=(T &x, I &&n)
		{
			x = static_cast<T>(x % std::forward<I>(n));
			return x;
		}
		/// Generic binary modulo operation involving piranha::mp_integer.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::mp_integer and \p U is an integral type among the \ref interop "interoperable types",
		 * - \p U is piranha::mp_integer and \p T is an integral type among the \ref interop "interoperable types",
		 * - both \p T and \p U are piranha::mp_integer.
		 * 
		 * The result is always a non-negative piranha::mp_integer.
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
		/// Generic equality operator involving piranha::mp_integer.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::mp_integer and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::mp_integer and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::mp_integer.
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
		 * @throws unspecified any exception resulting from the conversion of piranha::mp_integer to floating-point types.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,bool>::type operator==(const T &x, const U &y)
		{
			return binary_equality(x,y);
		}
		/// Generic inequality operator involving piranha::mp_integer.
		/**
		 * The implementation is equivalent to the generic equality operator.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x != y</tt>, \p false otherwise.
		 * 
		 * @throws unspecified any exception resulting from the conversion of piranha::mp_integer to floating-point types.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,bool>::type operator!=(const T &x, const U &y)
		{
			return !(x == y);
		}
		/// Generic less-than operator involving piranha::mp_integer.
		/**
		 * The implementation is equivalent to the generic equality operator.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x < y</tt>, \p false otherwise.
		 * 
		 * @throws unspecified any exception resulting from the conversion of piranha::mp_integer to floating-point types.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,bool>::type operator<(const T &x, const U &y)
		{
			return binary_less_than(x,y);
		}
		/// Generic less-than or equal operator involving piranha::mp_integer.
		/**
		 * The implementation is equivalent to the generic equality operator.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x <= y</tt>, \p false otherwise.
		 * 
		 * @throws unspecified any exception resulting from the conversion of piranha::mp_integer to floating-point types.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,bool>::type operator<=(const T &x, const U &y)
		{
			return binary_leq(x,y);
		}
		/// Generic greater-than operator involving piranha::mp_integer.
		/**
		 * The implementation is equivalent to the generic equality operator.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x > y</tt>, \p false otherwise.
		 * 
		 * @throws unspecified any exception resulting from the conversion of piranha::mp_integer to floating-point types.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,bool>::type operator>(const T &x, const U &y)
		{
			return (y < x);
		}
		/// Generic greater-than or equal operator involving piranha::mp_integer.
		/**
		 * The implementation is equivalent to the generic equality operator.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x >= y</tt>, \p false otherwise.
		 * 
		 * @throws unspecified any exception resulting from the conversion of piranha::mp_integer to floating-point types.
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
		mp_integer &multiply_accumulate(const mp_integer &n1, const mp_integer &n2)
		{
			::mpz_addmul(m_value,n1.m_value,n2.m_value);
			return *this;
		}
		/// Exponentiation.
		/**
		 * Return <tt>this ** exp</tt>. This template method is activated only if \p T is an \ref interop "interoperable type" or piranha::mp_integer.
		 * 
		 * If \p T is an integral type or piranha::mp_integer, the result will be exact, with negative powers calculated as <tt>(1 / this) ** exp</tt>.
		 * 
		 * If \p T is a floating-point type, the result will be exact if \p exp can be converted exactly to a piranha::mp_integer value,
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
		 * @throws std::invalid_argument if \p T is a floating-point type and \p exp is not an exact piranha::mp_integer, or if <tt>exp</tt>'s magnitude exceeds
		 * the range of the <tt>unsigned long</tt> type.
		 * @throws piranha::zero_division_error if \p this is zero and \p exp is negative.
		 */
		template <typename T>
		typename std::enable_if<is_interop_type<T>::value || std::is_same<T,mp_integer>::value,mp_integer>::type pow(const T &exp) const
		{
			return pow_impl(exp);
		}
		/// Compute next prime number.
		/**
		 * Compute the next prime number after \p this. The method uses the GMP function <tt>mpz_nextprime()</tt>.
		 * 
		 * @return next prime.
		 * 
		 * @throws std::invalid_argument if the sign of this is negative.
		 */
		mp_integer nextprime() const
		{
			if (unlikely(sign() < 0)) {
				piranha_throw(std::invalid_argument,"cannot compute the next prime of a negative number");
			}
			mp_integer retval;
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
		mp_integer sqrt() const
		{
			if (unlikely(sign() < 0)) {
				piranha_throw(std::invalid_argument,"cannot calculate square root of negative integer");
			}
			mp_integer retval(*this);
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
		 * 
		 * \todo fix this not to throw.
		 */
		std::size_t hash() const
		{
			const ::mp_size_t size = boost::numeric_cast< ::mp_size_t>(this->size());
			// Use the sign as initial seed value.
			std::size_t retval = static_cast<std::size_t>(mpz_sgn(m_value));
			for (::mp_size_t i = 0; i < size; ++i) {
				boost::hash_combine(retval,::mpz_getlimbn(m_value,i));
			}
			return retval;
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
		 * @return number of GMP limbs currently allocated in \p this. The return type is the unsigned counterpart of the piranha::mp_integer
		 * type used to represent the allocated size in GMP's integer type.
		 */
		auto allocated_size() const -> typename std::decay<std::make_unsigned<decltype(mpz_t{}->_mp_alloc)>::type>::type
		{
			typedef typename std::decay<std::make_unsigned<decltype(mpz_t{}->_mp_alloc)>::type>::type return_type;
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
		/// Overload output stream operator for piranha::mp_integer.
		/**
		 * @param[in] os output stream.
		 * @param[in] n piranha::mp_integer to be directed to stream.
		 * 
		 * @return reference to \p os.
		 * 
		 * @throws std::overflow_error if the number of digits is larger than an implementation-defined maximum.
		 * @throws unspecified any exception thrown by memory allocation errors in standard container.
		 */
		friend std::ostream &operator<<(std::ostream &os, const mp_integer &n)
		{
			const std::size_t size_base10 = ::mpz_sizeinbase(n.m_value,10);
			if (size_base10 > boost::integer_traits<std::size_t>::const_max - static_cast<std::size_t>(2)) {
				piranha_throw(std::overflow_error,"number of digits is too large");
			}
			const auto total_size = size_base10 + 2u;
			// TODO: use static vector here.
			// NOTE: here we can optimize, avoiding one allocation, by using a std::array if
			// size_base10 is small enough.
			std::vector<char> tmp(static_cast<std::vector<char>::size_type>(total_size));
			if (tmp.size() != total_size) {
				piranha_throw(std::overflow_error,"number of digits is too large");
			}
			os << ::mpz_get_str(&tmp[0u],10,n.m_value);
			return os;
		}
		/// Overload input stream operator for piranha::mp_integer.
		/**
		 * Equivalent to extracting a string from the stream and then using it to construct a piranha::mp_integer that will be assigned to \p n.
		 * 
		 * @param[in] is input stream.
		 * @param[in,out] n piranha::mp_integer to which the contents of the stream will be assigned.
		 * 
		 * @return reference to \p is.
		 * 
		 * @throws unspecified any exception thrown by the constructor from string of piranha::mp_integer.
		 */
		friend std::istream &operator>>(std::istream &is, mp_integer &n)
		{
			std::string tmp_str;
			std::getline(is,tmp_str);
			// NOTE: here this can probably be optimized via mpz_set_str,
			// thus avoiding one allocation.
			n = mp_integer(tmp_str);
			return is;
		}
	private:
		mpz_t m_value;
};


namespace detail
{

// Specialise implementation of math::is_zero for mp_integer.
template <typename T>
struct math_is_zero_impl<T,typename std::enable_if<std::is_same<T,mp_integer>::value>::type>
{
	static bool run(const T &n)
	{
		return n.sign() == 0;
	}
};

// Specialise implementation of math::negate for mp_integer.
template <typename T>
struct math_negate_impl<T,typename std::enable_if<std::is_same<T,mp_integer>::value>::type>
{
	static void run(T &n)
	{
		n.negate();
	}
};

// Specialise multadd for mp_integer.
template <typename T>
struct math_multiply_accumulate_impl<T,T,T,typename std::enable_if<std::is_same<T,mp_integer>::value>::type>
{
	static void run(T &x, const T &y, const T &z)
	{
		x.multiply_accumulate(y,z);
	}
};

}

}

namespace std
{

/// Specialization of \p std::swap for piranha::mp_integer.
/**
 * @param[in] n1 first argument.
 * @param[in] n2 second argument.
 * 
 * @see piranha::mp_integer::swap()
 */
template <>
inline void swap(piranha::mp_integer &n1, piranha::mp_integer &n2)
{
	n1.swap(n2);
}

#if 0
/// Specialization of \p std::numeric_limits for piranha::integer.
template <>
class numeric_limits<piranha::integer>
{
	public:
		/// Specialisation flag.
		static piranha_constexpr bool is_specialized = true;
		/// Minimum representable value.
		/**
		 * @return default-constructed piranha::integer.
		 */
		static const piranha::integer min() piranha_noexcept_spec(true)
		{
			return piranha::integer();
		}
		/// Maximum representable value.
		/**
		 * @return default-constructed piranha::integer.
		 */
		static const piranha::integer max() piranha_noexcept_spec(true)
		{
			return piranha::integer();
		}
		/// Lowest finite value.
		/**
		 * @return default-constructed piranha::integer.
		 */
		static const piranha::integer lowest() piranha_noexcept_spec(true)
		{
			return piranha::integer();
		}
		static piranha_constexpr int digits = 0;
		static piranha_constexpr int digits10 = 0;
		static piranha_constexpr bool is_signed = true;
		static piranha_constexpr bool is_integer = true;
		static piranha_constexpr bool is_exact = true;
		static piranha_constexpr int radix = 0;
		static piranha::integer epsilon() throw()
		{
			return piranha::integer(1);
		}
		static piranha::integer round_error() throw()
		{
			return piranha::integer();
		}
		static const int min_exponent = 0;
		static const int min_exponent10 = 0;
		static const int max_exponent = 0;
		static const int max_exponent10 = 0;
		static const bool has_infinity = false;
		static const bool has_quiet_NaN = false;
		static const bool has_signaling_NaN = false;
		static const float_denorm_style has_denorm = denorm_absent;
		static const bool has_denorm_loss = false;
		static piranha::integer infinity() throw()
		{
			return piranha::integer();
		}
		static piranha::integer quiet_NaN() throw()
		{
			return piranha::integer();
		}
		static piranha::integer signaling_NaN() throw()
		{
			return piranha::integer();
		}
		static piranha::integer denorm_min() throw()
		{
			return piranha::integer();
		}
		static const bool is_iec559 = false;
		/// Override is_bounded attribute to false.
		static const bool is_bounded = false;
		/// Override is_modulo attribute to false.
		static const bool is_modulo = false;
		static const bool traps = false;
		static const bool tinyness_before = false;
		static const float_round_style round_style = round_toward_zero;
};
#endif

/// Specialisation of \p std::hash for piranha::mp_integer.
template <>
struct hash<piranha::mp_integer>
{
	/// Result type.
	typedef size_t result_type;
	/// Argument type.
	typedef piranha::mp_integer argument_type;
	/// Hash operator.
	/**
	 * @param[in] n piranha::mp_integer whose hash value will be returned.
	 * 
	 * @return <tt>n.hash()</tt>.
	 * 
	 * @see piranha::mp_integer::hash()
	 */
	result_type operator()(const argument_type &n) const
	{
		return n.hash();
	}
};

}

#endif
