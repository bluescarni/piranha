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

#ifndef PIRANHA_RATIONAL_HPP
#define PIRANHA_RATIONAL_HPP

#include <algorithm>
#include <boost/integer_traits.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <functional>
#include <gmp.h>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "config.hpp"
#include "detail/rational_fwd.hpp"
#include "detail/real_fwd.hpp"
#include "exceptions.hpp"
#include "integer.hpp"
#include "math.hpp"
#include "print_tex_coefficient.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Arbitrary precision rational class.
/**
 * This class represents rational numbers of arbitrary size (i.e., the size is limited only by the available memory).
 * The implementation consists of a C++ wrapper around the \p mpq_t type from the multiprecision GMP library.
 * 
 * \section interop Interoperability with fundamental types
 * 
 * Full interoperability with the same fundamental C++ types as piranha::integer is provided.
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
 */
class rational
{
		// Make friends with real.
		friend class real;
		// Type trait for allowed arguments in arithmetic binary operations.
		template <typename T, typename U>
		struct are_binary_op_types: std::integral_constant<bool,
			(std::is_same<typename std::decay<T>::type,rational>::value && integer::is_interop_type<typename std::decay<U>::type>::value) ||
			(std::is_same<typename std::decay<U>::type,rational>::value && integer::is_interop_type<typename std::decay<T>::type>::value) ||
			(std::is_same<typename std::decay<T>::type,rational>::value && std::is_same<typename std::decay<U>::type,integer>::value) ||
			(std::is_same<typename std::decay<U>::type,rational>::value && std::is_same<typename std::decay<T>::type,integer>::value) ||
			(std::is_same<typename std::decay<T>::type,rational>::value && std::is_same<typename std::decay<U>::type,rational>::value)>
		{};
		// Metaprogramming to establish the return type of binary arithmetic operations involving rationals.
		// Default result type will be rational itself; for consistency with C/C++ when one of the arguments
		// is a floating point type, we will return a value of the same floating point type.
		template <typename T, typename U, typename Enable = void>
		struct deduce_binary_op_result_type
		{
			typedef rational type;
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
		// Construction.
		template <typename T>
		void construct_from_generic(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			integer::fp_normal_check(x);
			::mpq_init(m_value);
			::mpq_set_d(m_value,static_cast<double>(x));
		}
		template <typename T>
		void construct_from_generic(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpz_init_set_si(mpq_numref(m_value),static_cast<long>(si));
			::mpz_init_set_ui(mpq_denref(m_value),1ul);
		}
		template <typename T>
		void construct_from_generic(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpz_init_set_ui(mpq_numref(m_value),static_cast<unsigned long>(ui));
			::mpz_init_set_ui(mpq_denref(m_value),1ul);
		}
		template <typename T>
		void construct_from_generic(const T &ll, typename std::enable_if<std::is_integral<T>::value &&
			!integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			construct_from_string(boost::lexical_cast<std::string>(ll).c_str());
		}
		void construct_from_generic(const integer &n)
		{
			::mpz_init_set(mpq_numref(m_value),n.m_value);
			::mpz_init_set_ui(mpq_denref(m_value),1ul);
		}
		template <typename T>
		void construct_from_numden(const T &num, const T &den, typename std::enable_if<std::is_signed<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			piranha_assert(den != 0);
			::mpz_init_set_si(mpq_numref(m_value),static_cast<long>(num));
			::mpz_init_set_si(mpq_denref(m_value),static_cast<long>(den));
		}
		template <typename T>
		void construct_from_numden(const T &num, const T &den, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			piranha_assert(den != 0u);
			::mpz_init_set_ui(mpq_numref(m_value),static_cast<unsigned long>(num));
			::mpz_init_set_ui(mpq_denref(m_value),static_cast<unsigned long>(den));
		}
		template <typename T>
		void construct_from_numden(const T &num, const T &den, typename std::enable_if<std::is_same<T,integer>::value>::type * = nullptr)
		{
			piranha_assert(!math::is_zero(den));
			::mpz_init_set(mpq_numref(m_value),num.m_value);
			::mpz_init_set(mpq_denref(m_value),den.m_value);
		}
		template <typename T>
		void construct_from_numden(const T &num, const T &den, typename std::enable_if<std::is_integral<T>::value &&
			!integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			construct_from_numden(integer(num),integer(den));
		}
		static void validate_string(const char *str)
		{
			auto ptr = str;
			std::size_t num_size = 0u;
			while (*ptr != '\0' && *ptr != '/') {
				++num_size;
				++ptr;
			}
			try {
				integer::validate_string(str,num_size);
				if (*ptr == '/') {
					integer::validate_string(ptr + 1u,std::strlen(ptr + 1u));
					// Check if denominator is zero.
					integer tmp{ptr + 1u};
					if (unlikely(tmp.sign() == 0)) {
						piranha_throw(zero_division_error,"zero denominator");
					}
				}
			} catch (const std::invalid_argument &) {
				piranha_throw(std::invalid_argument,"invalid string input for rational type");
			}
		}
		void construct_from_string(const char *str)
		{
			validate_string(str);
			// String is OK.
			::mpq_init(m_value);
			const int retval = ::mpq_set_str(m_value,str,10);
			if (retval == -1) {
				// Clear it and throw.
				::mpq_clear(m_value);
				piranha_throw(std::invalid_argument,"invalid string input for rational type");
			}
			piranha_assert(retval == 0);
			// Put in canonical form.
			::mpq_canonicalize(m_value);
		}
		// Assignment.
		void assign_from_string(const char *str)
		{
			validate_string(str);
			// String is OK.
			const int retval = ::mpq_set_str(m_value,str,10);
			if (retval == -1) {
				piranha_throw(std::invalid_argument,"invalid string input for rational type");
			}
			piranha_assert(retval == 0);
			// Put in canonical form.
			::mpq_canonicalize(m_value);
		}
		template <typename T>
		void assign_from_generic(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			integer::fp_normal_check(x);
			::mpq_set_d(m_value,static_cast<double>(x));
		}
		template <typename T>
		void assign_from_generic(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpz_set_si(mpq_numref(m_value),static_cast<long>(si));
			::mpz_set_ui(mpq_denref(m_value),1ul);
		}
		template <typename T>
		void assign_from_generic(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpz_set_ui(mpq_numref(m_value),static_cast<unsigned long>(ui));
			::mpz_set_ui(mpq_denref(m_value),1ul);
		}
		template <typename T>
		void assign_from_generic(const T &ll, typename std::enable_if<std::is_integral<T>::value && !integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			assign_from_string(boost::lexical_cast<std::string>(ll).c_str());
		}
		void assign_from_generic(const integer &n)
		{
			::mpz_set(mpq_numref(m_value),n.m_value);
			::mpz_set_ui(mpq_denref(m_value),1ul);
		}
		// Conversion.
		template <typename T>
		typename std::enable_if<std::is_same<T,integer>::value,T>::type convert_to_impl() const
		{
			// NOTE: here this can probably be optimised, e.g., in case the denominator is 1.
			return integer(mpq_numref(m_value)) / integer(mpq_denref(m_value));
		}
		template <typename T>
		typename std::enable_if<std::is_integral<T>::value && !std::is_same<T,bool>::value,T>::type convert_to_impl() const
		{
			return static_cast<T>(static_cast<integer>(*this));
		}
		template <typename T>
		typename std::enable_if<std::is_same<T,bool>::value,T>::type convert_to_impl() const
		{
			return (mpz_sgn(mpq_numref(m_value)) != 0);
		}
		template <typename T>
		typename std::enable_if<std::is_floating_point<T>::value,T>::type convert_to_impl() const
		{
			// NOTE: this should always be ok, as these are guaranteed to be finite values.
			// See also the corresponding method in integer.
			const rational highest(std::numeric_limits<T>::max()), lowest(std::numeric_limits<T>::lowest());
			if (::mpq_cmp(m_value,lowest.m_value) < 0) {
				if (std::signbit(std::numeric_limits<T>::lowest()) != 0 && std::numeric_limits<T>::has_infinity) {
					return std::copysign(std::numeric_limits<T>::infinity(),std::numeric_limits<T>::lowest());
				} else {
					piranha_throw(std::overflow_error,"cannot convert to floating point type");
				}
			} else if (::mpq_cmp(m_value,highest.m_value) > 0) {
				if (std::numeric_limits<T>::has_infinity) {
					return std::numeric_limits<T>::infinity();
				} else {
					piranha_throw(std::overflow_error,"cannot convert to floating point type");
				}
			} else {
				return static_cast<T>(::mpq_get_d(m_value));
			}
		}
		// In-place addition.
		void in_place_add(const rational &q)
		{
			::mpq_add(m_value,m_value,q.m_value);
		}
		void in_place_add(const integer &n)
		{
			::mpz_addmul(mpq_numref(m_value),mpq_denref(m_value),n.m_value);
		}
		template <typename T>
		void in_place_add(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			in_place_add(integer(si));
		}
		template <typename T>
		void in_place_add(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpz_addmul_ui(mpq_numref(m_value),mpq_denref(m_value),static_cast<unsigned long>(ui));
		}
		template <typename T>
		void in_place_add(const T &n, typename std::enable_if<std::is_integral<T>::value && !integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			in_place_add(integer(n));
		}
		template <typename T>
		void in_place_add(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			operator=(static_cast<T>(*this) + x);
		}
		// Binary addition.
		template <typename T, typename U>
		static rational binary_plus(T &&x, U &&y, typename std::enable_if<
			!std::is_floating_point<typename std::decay<T>::type>::value &&
			!std::is_floating_point<typename std::decay<U>::type>::value
			>::type * = nullptr)
		{
			rational retval(std::forward<T>(x));
			retval += std::forward<U>(y);
			return retval;
		}
		template <typename T>
		static T binary_plus(const rational &q, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return (static_cast<T>(q) + x);
		}
		template <typename T>
		static T binary_plus(const T &x, const rational &q, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return binary_plus(q,x);
		}
		// In-place subtraction.
		void in_place_sub(const rational &q)
		{
			::mpq_sub(m_value,m_value,q.m_value);
		}
		void in_place_sub(const integer &n)
		{
			::mpz_submul(mpq_numref(m_value),mpq_denref(m_value),n.m_value);
		}
		template <typename T>
		void in_place_sub(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			in_place_sub(integer(si));
		}
		template <typename T>
		void in_place_sub(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpz_submul_ui(mpq_numref(m_value),mpq_denref(m_value),static_cast<unsigned long>(ui));
		}
		template <typename T>
		void in_place_sub(const T &n, typename std::enable_if<std::is_integral<T>::value && !integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			in_place_sub(integer(n));
		}
		template <typename T>
		void in_place_sub(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			operator=(static_cast<T>(*this) - x);
		}
		// Binary subtraction.
		template <typename T, typename U>
		static rational binary_minus(T &&x, U &&y, typename std::enable_if<
			!std::is_floating_point<typename std::decay<T>::type>::value &&
			!std::is_floating_point<typename std::decay<U>::type>::value
			>::type * = nullptr)
		{
			rational retval(std::forward<T>(x));
			retval -= std::forward<U>(y);
			return retval;
		}
		template <typename T>
		static T binary_minus(const rational &q, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return (static_cast<T>(q) - x);
		}
		template <typename T>
		static T binary_minus(const T &x, const rational &q, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return -binary_minus(q,x);
		}
		// In-place multiplication.
		void in_place_mul(const rational &q)
		{
			::mpq_mul(m_value,m_value,q.m_value);
		}
		void in_place_mul(const integer &n)
		{
			::mpz_mul(mpq_numref(m_value),mpq_numref(m_value),n.m_value);
			::mpq_canonicalize(m_value);
		}
		template <typename T>
		void in_place_mul(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpz_mul_si(mpq_numref(m_value),mpq_numref(m_value),static_cast<long>(si));
			::mpq_canonicalize(m_value);
		}
		template <typename T>
		void in_place_mul(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpz_mul_ui(mpq_numref(m_value),mpq_numref(m_value),static_cast<unsigned long>(ui));
			::mpq_canonicalize(m_value);
		}
		template <typename T>
		void in_place_mul(const T &n, typename std::enable_if<std::is_integral<T>::value && !integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			in_place_mul(integer(n));
		}
		template <typename T>
		void in_place_mul(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			operator=(static_cast<T>(*this) * x);
		}
		// Binary multiplication.
		template <typename T, typename U>
		static rational binary_mul(T &&x, U &&y, typename std::enable_if<
			!std::is_floating_point<typename std::decay<T>::type>::value &&
			!std::is_floating_point<typename std::decay<U>::type>::value
			>::type * = nullptr)
		{
			rational retval(std::forward<T>(x));
			retval *= std::forward<U>(y);
			return retval;
		}
		template <typename T>
		static T binary_mul(const rational &q, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return (static_cast<T>(q) * x);
		}
		template <typename T>
		static T binary_mul(const T &x, const rational &q, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return binary_mul(q,x);
		}
		// In-place division.
		void in_place_div(const rational &q)
		{
			::mpq_div(m_value,m_value,q.m_value);
		}
		void in_place_div(const integer &n)
		{
			::mpz_mul(mpq_denref(m_value),mpq_denref(m_value),n.m_value);
			::mpq_canonicalize(m_value);
		}
		template <typename T>
		void in_place_div(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpz_mul_si(mpq_denref(m_value),mpq_denref(m_value),static_cast<long>(si));
			::mpq_canonicalize(m_value);
		}
		template <typename T>
		void in_place_div(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpz_mul_ui(mpq_denref(m_value),mpq_denref(m_value),static_cast<unsigned long>(ui));
			::mpq_canonicalize(m_value);
		}
		template <typename T>
		void in_place_div(const T &n, typename std::enable_if<std::is_integral<T>::value && !integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			in_place_div(integer(n));
		}
		template <typename T>
		void in_place_div(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			operator=(static_cast<T>(*this) / x);
		}
		// Binary division.
		template <typename T, typename U>
		static rational binary_div(T &&x, U &&y, typename std::enable_if<
			!std::is_floating_point<typename std::decay<T>::type>::value &&
			!std::is_floating_point<typename std::decay<U>::type>::value
			>::type * = nullptr)
		{
			rational retval(std::forward<T>(x));
			retval /= std::forward<U>(y);
			return retval;
		}
		template <typename T>
		static T binary_div(const rational &q, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			if (unlikely(math::is_zero(x))) {
				piranha_throw(zero_division_error,"division by zero");
			}
			return (static_cast<T>(q) / x);
		}
		template <typename T>
		static T binary_div(const T &x, const rational &q, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			const T q_T = static_cast<T>(q);
			if (unlikely(math::is_zero(q_T))) {
				piranha_throw(zero_division_error,"division by zero");
			}
			return (x / q_T);
		}
		// Binary equality.
		static bool binary_equality(const rational &q1, const rational &q2)
		{
			return ::mpq_equal(q1.m_value,q2.m_value) != 0;
		}
		static bool binary_equality(const rational &q, const integer &n)
		{
			return (mpz_cmp_ui(mpq_denref(q.m_value),1ul) == 0 && ::mpz_cmp(mpq_numref(q.m_value),n.m_value) == 0);
		}
		template <typename T>
		static bool binary_equality(const rational &q, const T &n, typename std::enable_if<std::is_signed<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			return (mpq_cmp_si(q.m_value,static_cast<long>(n),1ul) == 0);
		}
		template <typename T>
		static bool binary_equality(const rational &q, const T &n, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			return (mpq_cmp_ui(q.m_value,static_cast<unsigned long>(n),1ul) == 0);
		}
		template <typename T>
		static bool binary_equality(const rational &q, const T &n, typename std::enable_if<std::is_integral<T>::value &&
			!integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			return binary_equality(q,integer(n));
		}
		template <typename T>
		static bool binary_equality(const rational &q, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return (static_cast<T>(q) == x);
		}
		// NOTE: this is the reverse of above.
		template <typename T>
		static bool binary_equality(const T &x, const rational &q, typename std::enable_if<
			std::is_arithmetic<T>::value || std::is_same<T,integer>::value>::type * = nullptr)
		{
			return binary_equality(q,x);
		}
		// Binary less-than.
		static bool binary_less_than(const rational &q1, const rational &q2)
		{
			return (::mpq_cmp(q1.m_value,q2.m_value) < 0);
		}
		static bool binary_less_than(const rational &q, const integer &n)
		{
			return binary_less_than(q,rational(n));
		}
		template <typename T>
		static bool binary_less_than(const rational &q, const T &n, typename std::enable_if<std::is_signed<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			return (mpq_cmp_si(q.m_value,static_cast<long>(n),1ul) < 0);
		}
		template <typename T>
		static bool binary_less_than(const rational &q, const T &n, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			return (mpq_cmp_ui(q.m_value,static_cast<unsigned long>(n),1ul) < 0);
		}
		template <typename T>
		static bool binary_less_than(const rational &q, const T &n, typename std::enable_if<std::is_integral<T>::value &&
			!integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			return binary_less_than(q,integer(n));
		}
		template <typename T>
		static bool binary_less_than(const rational &q, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return (static_cast<T>(q) < x);
		}
		// Binary less-than or equal.
		static bool binary_leq(const rational &q1, const rational &q2)
		{
			return (::mpq_cmp(q1.m_value,q2.m_value) <= 0);
		}
		static bool binary_leq(const rational &q, const integer &n)
		{
			return binary_leq(q,rational(n));
		}
		template <typename T>
		static bool binary_leq(const rational &q, const T &n, typename std::enable_if<std::is_signed<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			return (mpq_cmp_si(q.m_value,static_cast<long>(n),1ul) <= 0);
		}
		template <typename T>
		static bool binary_leq(const rational &q, const T &n, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			return (mpq_cmp_ui(q.m_value,static_cast<unsigned long>(n),1ul) <= 0);
		}
		template <typename T>
		static bool binary_leq(const rational &q, const T &n, typename std::enable_if<std::is_integral<T>::value &&
			!integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			return binary_leq(q,integer(n));
		}
		template <typename T>
		static bool binary_leq(const rational &q, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return (static_cast<T>(q) <= x);
		}
		// Inverse forms of less-than and leq.
		template <typename T>
		static bool binary_less_than(const T &x, const rational &q, typename std::enable_if<std::is_arithmetic<T>::value ||
			std::is_same<T,integer>::value>::type * = nullptr)
		{
			return !binary_leq(q,x);
		}
		template <typename T>
		static bool binary_leq(const T &x, const rational &q, typename std::enable_if<std::is_arithmetic<T>::value ||
			std::is_same<T,integer>::value>::type * = nullptr)
		{
			return !binary_less_than(q,x);
		}
		// Exponentiation.
		template <typename T>
		rational pow_impl(const T &ui, typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value>::type * = nullptr) const
		{
			unsigned long exp;
			try {
				exp = boost::numeric_cast<unsigned long>(ui);
			} catch (const boost::numeric::bad_numeric_cast &) {
				piranha_throw(std::invalid_argument,"invalid argument for rational exponentiation");
			}
			rational retval;
			::mpz_pow_ui(mpq_numref(retval.m_value),mpq_numref(m_value),exp);
			::mpz_pow_ui(mpq_denref(retval.m_value),mpq_denref(m_value),exp);
			return retval;
		}
		template <typename T>
		rational pow_impl(const T &si, typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type * = nullptr) const
		{
			return pow_impl(integer(si));
		}
		rational pow_impl(const integer &n) const
		{
			if (n.sign() >= 0) {
				unsigned long exp;
				try {
					exp = static_cast<unsigned long>(n);
				} catch (const std::overflow_error &) {
					piranha_throw(std::invalid_argument,"invalid argument for rational exponentiation");
				}
				return pow_impl(exp);
			} else {
				if (sign() == 0) {
					piranha_throw(zero_division_error,"negative exponentiation of zero");
				}
				auto retval = pow_impl(-n);
				::mpz_swap(mpq_numref(retval.m_value),mpq_denref(retval.m_value));
				// Fix signs if needed.
				if (mpz_sgn(mpq_denref(retval.m_value)) < 0) {
					::mpz_neg(mpq_numref(retval.m_value),mpq_numref(retval.m_value));
					::mpz_neg(mpq_denref(retval.m_value),mpq_denref(retval.m_value));
				}
				return retval;
			}
		}
	public:
		/// Default constructor.
		/**
		 * The value will be initialised to zero.
		 */
		rational()
		{
			::mpq_init(m_value);
		}
		/// Copy constructor.
		/**
		 * @param[in] other rational to be deep-copied.
		 */
		rational(const rational &other)
		{
			::mpz_init_set(mpq_numref(m_value),mpq_numref(other.m_value));
			::mpz_init_set(mpq_denref(m_value),mpq_denref(other.m_value));
		}
		/// Move constructor.
		/**
		 * @param[in] other rational to be moved.
		 */
		rational(rational &&other) noexcept(true)
		{
			auto mover = [](::mpz_t a, ::mpz_t b) {
				// Move b into a.
				a->_mp_size = b->_mp_size;
				a->_mp_d = b->_mp_d;
				a->_mp_alloc = b->_mp_alloc;
				// Erase b.
				b->_mp_size = 0;
				b->_mp_d = nullptr;
				b->_mp_alloc = 0;
			};
			mover(mpq_numref(m_value),mpq_numref(other.m_value));
			mover(mpq_denref(m_value),mpq_denref(other.m_value));
		}
		/// Generic constructor.
		/**
		 * The supported types for \p T are the \ref interop "interoperable types" and piranha::integer.
		 * Use of other types will result in a compile-time error. Construction from all supported types is exact.
		 * 
		 * @param[in] x object used to construct \p this.
		 * 
		 * @throws std::invalid_argument if \p x is a non-finite floating-point number.
		 */
		template <typename T>
		explicit rational(const T &x, typename std::enable_if<
			integer::is_interop_type<T>::value ||
			std::is_same<T,integer>::value
			>::type * = nullptr)
		{
			construct_from_generic(x);
		}
		/// Constructor from numerator and denominator.
		/**
		 * This constructor is enabled if \p T is an integral interoperable type or piranha::integer. Numerator and denominator
		 * need not to be in canonical form.
		 * 
		 * @param[in] num numerator.
		 * @param[in] den denominator.
		 * 
		 * @throws piranha::zero_division_error if the denominator is zero.
		 */
		template <typename T>
		explicit rational(const T &num, const T &den, typename std::enable_if<
			(integer::is_interop_type<T>::value && std::is_integral<T>::value) ||
			std::is_same<T,integer>::value
			>::type * = nullptr)
		{
			if (math::is_zero(den)) {
				piranha_throw(zero_division_error,"division by zero");
			}
			construct_from_numden(num,den);
			::mpq_canonicalize(m_value);
		}
		/// Constructor from string.
		/**
		 * The string must consist of either an integer (e.g., "42") or a fraction in the form of two integers
		 * separated by "/" (e.g., "84/2"). In both cases, the rules of integer parsing are the same as for
		 * piranha::integer. Fractions need not to be in canonical form (they will be normalised upon construction).
		 * 
		 * @param[in] str decimal string representation of the number used to initialise the object.
		 * 
		 * @throws std::invalid_argument if the string is malformed.
		 */
		explicit rational(const std::string &str)
		{
			construct_from_string(str.c_str());
		}
		/// Constructor from C string.
		/**
		 * @param[in] str decimal string representation of the number used to initialise the object.
		 * 
		 * @see rational(const std::string &)
		 */
		explicit rational(const char *str)
		{
			construct_from_string(str);
		}
		/// Destructor.
		/**
		 * Will clear the internal \p mpq_t type.
		 */
		~rational() noexcept(true);
		/// Move assignment operator.
		/**
		 * @param[in] other rational to be moved.
		 * 
		 * @return reference to \p this.
		 */
		rational &operator=(rational &&other) noexcept(true)
		{
			// NOTE: swap() already has the check for this.
			swap(other);
			return *this;
		}
		/// Copy assignment operator.
		/**
		 * @param[in] other rational to be assigned.
		 * 
		 * @return reference to \p this.
		 */
		rational &operator=(const rational &other)
		{
			if (likely(this != &other)) {
				// Handle assignment to moved-from objects.
				if (mpq_numref(m_value)->_mp_d) {
					piranha_assert(mpq_denref(m_value)->_mp_d);
					::mpq_set(m_value,other.m_value);
				} else {
					piranha_assert(mpq_numref(m_value)->_mp_size == 0 && mpq_numref(m_value)->_mp_alloc == 0);
					piranha_assert(mpq_denref(m_value)->_mp_size == 0 && mpq_denref(m_value)->_mp_alloc == 0);
					::mpz_init_set(mpq_numref(m_value),mpq_numref(other.m_value));
					::mpz_init_set(mpq_denref(m_value),mpq_denref(other.m_value));
				}
			}
			return *this;
		}
		/// Assignment operator from string.
		/**
		 * The string parsing rules are the same as in the constructor from string.
		 * 
		 * @param[in] str string representation of the rational to be assigned.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws std::invalid_argument if the string is malformed.
		 */
		rational &operator=(const std::string &str)
		{
			return operator=(str.c_str());
		}
		/// Assignment operator from C string.
		/**
		 * @param[in] str string representation of the rational to be assigned.
		 * 
		 * @return reference to \p this.
		 * 
		 * @see operator=(const std::string &)
		 */
		rational &operator=(const char *str)
		{
			if (mpq_numref(m_value)->_mp_d) {
				piranha_assert(mpq_denref(m_value)->_mp_d);
				assign_from_string(str);
			} else {
				piranha_assert(mpq_numref(m_value)->_mp_size == 0 && mpq_numref(m_value)->_mp_alloc == 0);
				piranha_assert(mpq_denref(m_value)->_mp_size == 0 && mpq_denref(m_value)->_mp_alloc == 0);
				construct_from_string(str);
			}
			return *this;
		}
		/// Generic assignment operator.
		/**
		 * The supported types for \p T are the \ref interop "interoperable types" and piranha::integer.
		 * Use of other types will result in a compile-time error.
		 * 
		 * @param[in] x object that will be assigned to \p this.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws std::invalid_argument if \p x is a non-finite floating-point number.
		 */
		template <typename T>
		typename std::enable_if<integer::is_interop_type<T>::value || std::is_same<T,integer>::value,rational &>::type operator=(const T &x)
		{
			if (mpq_numref(m_value)->_mp_d) {
				piranha_assert(mpq_denref(m_value)->_mp_d);
				assign_from_generic(x);
			} else {
				piranha_assert(mpq_numref(m_value)->_mp_size == 0 && mpq_numref(m_value)->_mp_alloc == 0);
				piranha_assert(mpq_denref(m_value)->_mp_size == 0 && mpq_denref(m_value)->_mp_alloc == 0);
				construct_from_generic(x);
			}
			return *this;
		}
		/// Conversion operator.
		/**
		 * Extract an instance of type \p T from \p this. The supported types for \p T are the \ref interop "interoperable types" and piranha::integer.
		 * 
		 * Conversion to \p bool is always successful, and returns <tt>this != 0</tt>.
		 * Conversion to the other integral types is truncated, its success depending on whether or not
		 * the target type can represent the current (truncated) value of the rational.
		 * 
		 * Conversion to floating point types is exact if the target type can represent exactly the current value of the rational.
		 * If that is not the case, the output value will be one of the two adjacents (with an unspecified rounding direction).
		 * 
		 * Return values of +-inf will be produced if the current value of the rational overflows the range of the floating-point type
		 * and the floating-point type can represent infinity. Otherwise, an overflow error will be produced.
		 * 
		 * @return result of the conversion to target type T.
		 * 
		 * @throws std::overflow_error if the conversion to an integral type other than bool results in (negative) overflow, or if
		 * conversion to a floating-point type lacking infinity overflows.
		 */
		template <typename T, typename = typename std::enable_if<integer::is_interop_type<T>::value || std::is_same<T,integer>::value>::type>
		explicit operator T() const
		{
			return convert_to_impl<T>();
		}
		/// Numerator.
		/**
		 * @return copy of the numerator.
		 */
		integer get_numerator() const
		{
			return integer(mpq_numref(m_value));
		}
		/// Denominator.
		/**
		 * @return copy of the denominator.
		 */
		integer get_denominator() const
		{
			return integer(mpq_denref(m_value));
		}
		/// Swap.
		/**
		 * Swap the content of \p this and \p q.
		 * 
		 * @param[in] q swap argument.
		 */
		void swap(rational &q) noexcept(true)
		{
			if (unlikely(this == &q)) {
			    return;
			}
			integer::swap_mpz_t(mpq_numref(m_value),mpq_numref(q.m_value));
			integer::swap_mpz_t(mpq_denref(m_value),mpq_denref(q.m_value));
		}
		/// In-place addition.
		/**
		 * Add \p x to the current value of the rational object. This template operator is activated only if
		 * \p T is either rational, piranha::integer or an \ref interop "interoperable type".
		 * 
		 * If \p T is rational or integral, the result will be exact. If \p T is a floating-point type, the following
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
			integer::is_interop_type<typename std::decay<T>::type>::value ||
			std::is_same<rational,typename std::decay<T>::type>::value ||
			std::is_same<integer,typename std::decay<T>::type>::value,rational &>::type operator+=(T &&x)
		{
			in_place_add(std::forward<T>(x));
			return *this;
		}
		/// Generic in-place addition with piranha::rational.
		/**
		 * Add a piranha::rational in-place. This template operator is activated only if \p T is an \ref interop "interoperable type" or piranha::integer,
		 * and \p Q is piranha::rational.
		 * This method will first compute <tt>q + x</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] q second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception resulting from the binary operator or from casting piranha::rational to \p T.
		 */
		template <typename T, typename Q>
		friend typename std::enable_if<(integer::is_interop_type<T>::value || std::is_same<T,integer>::value) &&
			std::is_same<typename std::decay<Q>::type,rational>::value,T &>::type
			operator+=(T &x, Q &&q)
		{
			x = static_cast<T>(std::forward<Q>(q) + x);
			return x;
		}
		/// Generic binary addition involving piranha::rational.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::rational and \p U is an \ref interop "interoperable type" or piranha::integer,
		 * - \p U is piranha::rational and \p T is an \ref interop "interoperable type" or piranha::integer,
		 * - both \p T and \p U are piranha::rational.
		 * 
		 * If no floating-point types are involved, the exact result of the operation will be returned as a piranha::rational.
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
		rational operator+() const
		{
			return *this;
		}
		/// Prefix increment.
		/**
		 * Increment \p this by one.
		 * 
		 * @return reference to \p this after the increment.
		 */
		rational &operator++()
		{
			return operator+=(1);
		}
		/// Suffix increment.
		/**
		 * Increment \p this by one and return a copy of \p this as it was before the increment.
		 * 
		 * @return copy of \p this before the increment.
		 */
		rational operator++(int)
		{
			const rational retval(*this);
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
			integer::is_interop_type<typename std::decay<T>::type>::value ||
			std::is_same<rational,typename std::decay<T>::type>::value ||
			std::is_same<integer,typename std::decay<T>::type>::value,rational &>::type operator-=(T &&x)
		{
			in_place_sub(std::forward<T>(x));
			return *this;
		}
		/// Generic in-place subtraction with piranha::rational.
		/**
		 * Subtract a piranha::rational in-place. This template operator is activated only if \p T is an \ref interop "interoperable type" or piranha::integer,
		 * and \p Q is piranha::rational.
		 * This method will first compute <tt>x - q</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] q second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception resulting from the binary operator or from casting piranha::rational to \p T.
		 */
		template <typename T, typename Q>
		friend typename std::enable_if<(integer::is_interop_type<T>::value || std::is_same<T,integer>::value) &&
			std::is_same<typename std::decay<Q>::type,rational>::value,T &>::type
			operator-=(T &x, Q &&q)
		{
			x = static_cast<T>(x - std::forward<Q>(q));
			return x;
		}
		/// Generic binary subtraction involving piranha::rational.
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
			::mpq_neg(m_value,m_value);
		}
		/// Negated copy.
		/**
		 * @return copy of \p -this.
		 */
		rational operator-() const
		{
			rational retval(*this);
			retval.negate();
			return retval;
		}
		/// Prefix decrement.
		/**
		 * Decrement \p this by one and return.
		 * 
		 * @return reference to \p this.
		 */
		rational &operator--()
		{
			return operator-=(1);
		}
		/// Suffix decrement.
		/**
		 * Decrement \p this by one and return a copy of \p this as it was before the decrement.
		 * 
		 * @return copy of \p this before the decrement.
		 */
		rational operator--(int)
		{
			const rational retval(*this);
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
			integer::is_interop_type<typename std::decay<T>::type>::value ||
			std::is_same<rational,typename std::decay<T>::type>::value ||
			std::is_same<integer,typename std::decay<T>::type>::value,rational &>::type operator*=(T &&x)
		{
			in_place_mul(std::forward<T>(x));
			return *this;
		}
		/// Generic in-place multiplication with piranha::rational.
		/**
		 * Multiply by a piranha::rational in-place. This template operator is activated only if \p T is an \ref interop "interoperable type" or piranha::integer,
		 * and \p Q is piranha::rational.
		 * This method will first compute <tt>x * q</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] q second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception resulting from the binary operator or from casting piranha::rational to \p T.
		 */
		template <typename T, typename Q>
		friend typename std::enable_if<(integer::is_interop_type<T>::value || std::is_same<T,integer>::value) &&
			std::is_same<typename std::decay<Q>::type,rational>::value,T &>::type
			operator*=(T &x, Q &&q)
		{
			x = static_cast<T>(x * std::forward<Q>(q));
			return x;
		}
		/// Generic binary multiplication involving piranha::rational.
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
		 * The same rules described in operator+=() apply. Trying to divide by zero will throw a piranha::zero_division_error exception.
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
			integer::is_interop_type<typename std::decay<T>::type>::value ||
			std::is_same<rational,typename std::decay<T>::type>::value ||
			std::is_same<integer,typename std::decay<T>::type>::value,rational &>::type operator/=(T &&x)
		{
			if (math::is_zero(x)) {
				piranha_throw(zero_division_error,"division by zero");
			}
			in_place_div(std::forward<T>(x));
			return *this;
		}
		/// Generic in-place division with piranha::rational.
		/**
		 * Divide by a piranha::rational in-place. This template operator is activated only if \p T is an \ref interop "interoperable type" or piranha::integer,
		 * and \p Q is piranha::rational.
		 * This method will first compute <tt>x / q</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] q second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception resulting from the binary operator or from casting piranha::rational to \p T.
		 */
		template <typename T, typename Q>
		friend typename std::enable_if<(integer::is_interop_type<T>::value || std::is_same<T,integer>::value) &&
			std::is_same<typename std::decay<Q>::type,rational>::value,T &>::type
			operator/=(T &x, Q &&q)
		{
			x = static_cast<T>(x / std::forward<Q>(q));
			return x;
		}
		/// Generic binary division involving piranha::rational.
		/**
		 * The implementation is equivalent to the generic binary addition operator.
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
		/// Sign.
		/**
		 * @return 1 if <tt>this > 0</tt>, 0 if <tt>this == 0</tt> and -1 if <tt>this < 0</tt>.
		 */
		int sign() const
		{
			return mpq_sgn(m_value);
		}
		/// Generic equality operator involving piranha::rational.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::rational and \p U is an \ref interop "interoperable type" or piranha::integer,
		 * - \p U is piranha::rational and \p T is an \ref interop "interoperable type" or piranha::integer,
		 * - both \p T and \p U are piranha::rational.
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
		/// Generic inequality operator involving piranha::rational.
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
		/// Generic less-than operator involving piranha::rational.
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
		/// Generic less-than or equal operator involving piranha::rational.
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
		/// Generic greater-than operator involving piranha::rational.
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
		/// Generic greater-than or equal operator involving piranha::rational.
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
		/// Exponentiation.
		/**
		 * Return <tt>this ** exp</tt>. This template method is activated only if \p T is an integral type or integer.
		 * Trying to raise zero to a negative exponent will throw a piranha::zero_division_error exception. <tt>this ** 0</tt> will always return 1.
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
		typename std::enable_if<std::is_integral<T>::value || std::is_same<T,integer>::value,rational>::type pow(const T &exp) const
		{
			return pow_impl(exp);
		}
		/// Absolute value.
		/**
		 * @return absolute value of \p this.
		 */
		rational abs() const
		{
			rational retval(*this);
			::mpz_abs(mpq_numref(retval.m_value),mpq_numref(retval.m_value));
			return retval;
		}
		/// Hash value.
		/**
		 * The value is calculated via \p boost::hash_combine over the limbs of the internal \p mpq_t type.
		 * 
		 * @return a hash value for \p this.
		 * 
		 * @see http://www.boost.org/doc/libs/release/doc/html/hash/reference.html#boost.hash_combine
		 */
		std::size_t hash() const
		{
			auto retval = integer::hash_mpz_t(mpq_numref(m_value));
			boost::hash_combine(retval,integer::hash_mpz_t(mpq_denref(m_value)));
			return retval;
		}
		/// Overload output stream operator for piranha::rational.
		/**
		 * @param[in] os output stream.
		 * @param[in] q piranha::rational to be directed to stream.
		 * 
		 * @return reference to \p os.
		 * 
		 * @throws std::overflow_error if the number of digits is larger than an implementation-defined maximum.
		 * @throws unspecified any exception thrown by memory allocation errors in standard containers.
		 */
		friend std::ostream &operator<<(std::ostream &os, const rational &q)
		{
			const std::size_t size_base10_num = ::mpz_sizeinbase(mpq_numref(q.m_value),10),
				size_base10_den = ::mpz_sizeinbase(mpq_denref(q.m_value),10),
				max = boost::integer_traits<std::size_t>::const_max;
			if (size_base10_den > max - 3u || size_base10_num > max - (size_base10_den + 3u)) {
				piranha_throw(std::overflow_error,"number of digits is too large");
			}
			const auto total_size = size_base10_num + (size_base10_den + 3u);
			// NOTE: here we can optimize, avoiding one allocation, by using a static vector if
			// size is small enough.
			std::vector<char> tmp(static_cast<std::vector<char>::size_type>(total_size));
			if (tmp.size() != total_size) {
				piranha_throw(std::overflow_error,"number of digits is too large");
			}
			os << ::mpq_get_str(&tmp[0u],10,q.m_value);
			return os;
		}
		/// Overload input stream operator for piranha::rational.
		/**
		 * Equivalent to extracting a line from the stream and then assigning it to \p q.
		 * 
		 * @param[in] is input stream.
		 * @param[in,out] q rational to which the contents of the stream will be assigned.
		 * 
		 * @return reference to \p is.
		 * 
		 * @throws unspecified any exception thrown by the constructor from string of piranha::rational.
		 */
		friend std::istream &operator>>(std::istream &is, rational &q)
		{
			std::string tmp_str;
			std::getline(is,tmp_str);
			q = tmp_str;
			return is;
		}
	private:
		::mpq_t m_value;
};

/// Specialisation of the piranha::print_tex_coefficient() for piranha::rational.
template <typename T>
struct print_tex_coefficient_impl<T,typename std::enable_if<std::is_same<rational,T>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] os target stream.
	 * @param[in] cf coefficient to be printed.
	 * 
	 * @throws unspecified any exception thrown by streaming to \p os.
	 */
	void operator()(std::ostream &os, const T &cf) const
	{
		integer num = cf.get_numerator(), den = cf.get_denominator();
		piranha_assert(den.sign() > 0);
		if (num.sign() == 0) {
			os << "0";
			return;
		}
		if (den == 1) {
			os << num;
			return;
		}
		if (num.sign() < 0) {
			os << "-";
			num.negate();
		}
		os << "\\frac{" << num << "}{" << den << "}";
	}
};

namespace math
{

/// Specialisation of the piranha::math::negate() functor for piranha::rational.
template <typename T>
struct negate_impl<T,typename std::enable_if<std::is_same<T,rational>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in,out] q piranha::rational to be negated.
	 */
	void operator()(rational &q) const
	{
		q.negate();
	}
};

/// Specialisation of the piranha::math::is_zero() functor for piranha::rational.
template <typename T>
struct is_zero_impl<T,typename std::enable_if<std::is_same<T,rational>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] q piranha::rational to be tested.
	 * 
	 * @return \p true if \p q is zero, \p false otherwise.
	 */
	bool operator()(const T &q) const
	{
		return q.sign() == 0;
	}
};

/// Specialisation of the piranha::math::pow() functor for piranha::rational.
/**
 * This specialisation is activated when \p T is piranha::rational.
 * The result will be computed via piranha::rational::pow().
 */
template <typename T, typename U>
struct pow_impl<T,U,typename std::enable_if<std::is_same<T,rational>::value>::type>
{
	/// Call operator.
	/**
	 * The exponentiation will be computed via piranha::rational::pow().
	 * 
	 * @param[in] q base.
	 * @param[in] x exponent.
	 * 
	 * @return \p q to the power of \p x.
	 * 
	 * @throws unspecified any exception resulting from piranha::rational::pow().
	 */
	template <typename T2, typename U2>
	auto operator()(const T2 &q, const U2 &x) const -> decltype(q.pow(x))
	{
		return q.pow(x);
	}
};

/// Specialisation of the piranha::math::sin() functor for piranha::rational.
template <typename T>
struct sin_impl<T,typename std::enable_if<std::is_same<T,rational>::value>::type>
{
	/// Call operator.
	/**
	 * The operation will return zero if \p q is also zero, otherwise an
	 * exception will be thrown.
	 * 
	 * @param[in] q argument.
	 * 
	 * @return sine of \p q.
	 */
	rational operator()(const T &q) const
	{
		if (q.sign() == 0) {
			return rational{};
		}
		piranha_throw(std::invalid_argument,"cannot calculate the sine of a nonzero rational");
	}
};

/// Specialisation of the piranha::math::cos() functor for piranha::rational.
template <typename T>
struct cos_impl<T,typename std::enable_if<std::is_same<T,rational>::value>::type>
{
	/// Call operator.
	/**
	 * The operation will return one if \p q is zero, otherwise an
	 * exception will be thrown.
	 * 
	 * @param[in] q argument.
	 * 
	 * @return cosine of \p q.
	 */
	rational operator()(const T &q) const
	{
		if (q.sign() == 0) {
			return rational{1};
		}
		piranha_throw(std::invalid_argument,"cannot calculate the cosine of a nonzero rational");
	}
};

/// Specialisation of the piranha::math::abs() functor for piranha::rational.
template <typename T>
struct abs_impl<T,typename std::enable_if<std::is_same<T,rational>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] q input parameter.
	 * 
	 * @return absolute value of \p q.
	 */
	T operator()(const T &q) const
	{
		return q.abs();
	}
};

/// Specialisation of the piranha::math::partial() functor for piranha::rational.
template <typename T>
struct partial_impl<T,typename std::enable_if<std::is_same<T,rational>::value>::type>
{
	/// Call operator.
	/**
	 * @return an instance of piranha::rational constructed from zero.
	 */
	rational operator()(const rational &, const std::string &) const
	{
		return rational(0);
	}
};

/// Specialisation of the piranha::math::evaluate() functor for piranha::rational.
template <typename T>
struct evaluate_impl<T,typename std::enable_if<std::is_same<T,rational>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] q evaluation argument.
	 * 
	 * @return copy of \p q.
	 */
	template <typename U>
	T operator()(const T &q, const std::unordered_map<std::string,U> &) const
	{
		return q;
	}
};

/// Specialisation of the piranha::math::subs() functor for piranha::rational.
template <typename T>
struct subs_impl<T,typename std::enable_if<std::is_same<T,rational>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] q substitution argument.
	 * 
	 * @return copy of \p q.
	 */
	template <typename U>
	T operator()(const T &q, const std::string &, const U &) const
	{
		return q;
	}
};

/// Specialisation of the piranha::math::integral_cast functor for piranha::rational.
template <typename T>
struct integral_cast_impl<T,typename std::enable_if<std::is_same<T,rational>::value>::type>
{
	/// Call operator.
	/**
	 * The call will be successful if the denominator of \p x is unitary.
	 * 
	 * @param[in] x cast argument.
	 * 
	 * @return result of the cast operation.
	 *
	 * @throws std::invalid_argument if the call is unsuccessful.
	 */
	integer operator()(const T &x) const
	{
		if (x.get_denominator() == 1) {
			return x.get_numerator();
		}
		piranha_throw(std::invalid_argument,"invalid rational");
	}
};

/// Specialisation of the piranha::math::ipow_subs() functor for piranha::rational.
/**
 * This specialisation is activated when \p T is piranha::rational.
 * The result will be the input value unchanged.
 */
template <typename T>
struct ipow_subs_impl<T,typename std::enable_if<std::is_same<T,rational>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] q substitution argument.
	 * 
	 * @return copy of \p q.
	 */
	template <typename U>
	T operator()(const T &q, const std::string &, const integer &, const U &) const
	{
		return q;
	}
};

/// Specialisation of the piranha::math::binomial() functor for piranha::rational.
/**
 * This specialisation is enabled if \p T is piranha::rational and \p U is an integral type or piranha::integer.
 */
template <typename T, typename U>
struct binomial_impl<T,U,typename std::enable_if<
	std::is_same<rational,T>::value &&
	(std::is_integral<U>::value || std::is_same<integer,U>::value)
	>::type>
{
	/// Call operator.
	/**
	 * @param[in] q top number.
	 * @param[in] k bottom number.
	 * 
	 * @return \p q choose \p k.
	 * 
	 * @throws std::invalid_argument if \p k is negative.
	 * @throws unspecified any exception resulting from arithmetic operations involving piranha::rational.
	 */
	rational operator()(const rational &q, const U &k) const
	{
		return detail::generic_binomial(q,k);
	}
};

}

inline rational::~rational() noexcept(true)
{
	PIRANHA_TT_CHECK(is_cf,rational);
	piranha_assert(mpq_numref(m_value)->_mp_alloc >= 0);
	piranha_assert(mpq_denref(m_value)->_mp_alloc >= 0);
	if (mpq_numref(m_value)->_mp_d != 0) {
		piranha_assert(mpq_denref(m_value)->_mp_d != 0);
		::mpq_clear(m_value);
	} else {
		piranha_assert(mpq_denref(m_value)->_mp_d == 0);
		piranha_assert(mpq_numref(m_value)->_mp_size == 0 && mpq_numref(m_value)->_mp_alloc == 0);
		piranha_assert(mpq_denref(m_value)->_mp_size == 0 && mpq_denref(m_value)->_mp_alloc == 0);
	}
}

}

namespace std
{

/// Specialization of \p std::swap for piranha::rational.
/**
 * @param[in] q1 first argument.
 * @param[in] q2 second argument.
 * 
 * @see piranha::rational::swap()
 */
template <>
inline void swap(piranha::rational &q1, piranha::rational &q2) noexcept(true)
{
	q1.swap(q2);
}

/// Specialisation of \p std::hash for piranha::rational.
template <>
struct hash<piranha::rational>
{
	/// Result type.
	typedef size_t result_type;
	/// Argument type.
	typedef piranha::rational argument_type;
	/// Hash operator.
	/**
	 * @param[in] q piranha::rational whose hash value will be returned.
	 * 
	 * @return <tt>q.hash()</tt>.
	 * 
	 * @see piranha::rational::hash()
	 */
	result_type operator()(const argument_type &q) const noexcept(true)
	{
		return q.hash();
	}
};

}

#endif
