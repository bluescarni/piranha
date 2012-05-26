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

#include <boost/integer_traits.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/bounds.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <cstddef>
#include <cstring>
#include <gmp.h>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "config.hpp"
#include "detail/rational_fwd.hpp"
#include "exceptions.hpp"
#include "integer.hpp"
#include "math.hpp"

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
		void construct_from_generic(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			integer::fp_normal_check(x);
			::mpq_init(m_value);
			::mpq_set_d(m_value,static_cast<double>(x));
		}
		template <typename T>
		void construct_from_generic(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			integer::is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			::mpz_init_set_si(mpq_numref(m_value),static_cast<long>(si));
			::mpz_init_set_ui(mpq_denref(m_value),1ul);
		}
		template <typename T>
		void construct_from_generic(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			::mpz_init_set_ui(mpq_numref(m_value),static_cast<unsigned long>(ui));
			::mpz_init_set_ui(mpq_denref(m_value),1ul);
		}
		template <typename T>
		void construct_from_generic(const T &ll, typename std::enable_if<std::is_integral<T>::value &&
			!integer::is_gmp_int<T>::value>::type * = piranha_nullptr)
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
			integer::is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			piranha_assert(den != 0);
			::mpz_init_set_si(mpq_numref(m_value),static_cast<long>(num));
			::mpz_init_set_si(mpq_denref(m_value),static_cast<long>(den));
		}
		template <typename T>
		void construct_from_numden(const T &num, const T &den, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			piranha_assert(den != 0u);
			::mpz_init_set_ui(mpq_numref(m_value),static_cast<unsigned long>(num));
			::mpz_init_set_ui(mpq_denref(m_value),static_cast<unsigned long>(den));
		}
		template <typename T>
		void construct_from_numden(const T &num, const T &den, typename std::enable_if<std::is_same<T,integer>::value>::type * = piranha_nullptr)
		{
			piranha_assert(!math::is_zero(den));
			::mpz_init_set(mpq_numref(m_value),num.m_value);
			::mpz_init_set(mpq_denref(m_value),den.m_value);
		}
		template <typename T>
		void construct_from_numden(const T &num, const T &den, typename std::enable_if<std::is_integral<T>::value &&
			!integer::is_gmp_int<T>::value>::type * = piranha_nullptr)
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
				}
			} catch (...) {
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
		void assign_from_generic(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			integer::fp_normal_check(x);
			::mpq_set_d(m_value,static_cast<double>(x));
		}
		template <typename T>
		void assign_from_generic(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			integer::is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			::mpz_set_si(mpq_numref(m_value),static_cast<long>(si));
			::mpz_set_ui(mpq_denref(m_value),1ul);
		}
		template <typename T>
		void assign_from_generic(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			::mpz_set_ui(mpq_numref(m_value),static_cast<unsigned long>(ui));
			::mpz_set_ui(mpq_denref(m_value),1ul);
		}
		template <typename T>
		void assign_from_generic(const T &ll, typename std::enable_if<std::is_integral<T>::value && !integer::is_gmp_int<T>::value>::type * = piranha_nullptr)
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
			const rational highest(boost::numeric::bounds<T>::highest()),
				lowest(boost::numeric::bounds<T>::lowest());
			if (::mpq_cmp(m_value,lowest.m_value) < 0) {
				if (std::numeric_limits<T>::has_infinity) {
					return -std::numeric_limits<T>::infinity();
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
			integer::is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			if (si >= 0) {
				::mpz_addmul_ui(mpq_numref(m_value),mpq_denref(m_value),static_cast<unsigned long>(si));
			} else {
				// Neat trick here. See:
				// http://stackoverflow.com/questions/4536095/unary-minus-and-signed-to-unsigned-conversion
				::mpz_submul_ui(mpq_numref(m_value),mpq_denref(m_value),-static_cast<unsigned long>(si));
			}
		}
		template <typename T>
		void in_place_add(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			::mpz_addmul_ui(mpq_numref(m_value),mpq_denref(m_value),static_cast<unsigned long>(ui));
		}
		template <typename T>
		void in_place_add(const T &n, typename std::enable_if<std::is_integral<T>::value && !integer::is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			in_place_add(integer(n));
		}
		template <typename T>
		void in_place_add(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			operator=(static_cast<T>(*this) + x);
		}
		// Binary addition.
		template <typename Q, typename T>
		static rational binary_plus(Q &&q, T &&x, typename std::enable_if<
			std::is_same<rational,typename std::decay<Q>::type>::value && (
			are_binary_op_types<Q,T>::value && !std::is_floating_point<typename std::decay<T>::type>::value
			)>::type * = piranha_nullptr)
		{
			rational retval(std::forward<Q>(q));
			retval += std::forward<T>(x);
			return retval;
		}
		template <typename T, typename Q>
		static rational binary_plus(T &&x, Q &&q, typename std::enable_if<
			std::is_same<rational,typename std::decay<Q>::type>::value && (
			are_binary_op_types<Q,T>::value && !std::is_floating_point<typename std::decay<T>::type>::value &&
			// Disambiguate when both operands are rationals.
			!std::is_same<rational,typename std::decay<T>::type>::value
			)>::type * = piranha_nullptr)
		{
			return binary_plus(std::forward<Q>(q),std::forward<T>(x));
		}
		template <typename T>
		static T binary_plus(const rational &q, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			return (static_cast<T>(q) + x);
		}
		template <typename T>
		static T binary_plus(const T &x, const rational &q, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
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
			integer::is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			if (si >= 0) {
				::mpz_submul_ui(mpq_numref(m_value),mpq_denref(m_value),static_cast<unsigned long>(si));
			} else {
				::mpz_addmul_ui(mpq_numref(m_value),mpq_denref(m_value),-static_cast<unsigned long>(si));
			}
		}
		template <typename T>
		void in_place_sub(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			::mpz_submul_ui(mpq_numref(m_value),mpq_denref(m_value),static_cast<unsigned long>(ui));
		}
		template <typename T>
		void in_place_sub(const T &n, typename std::enable_if<std::is_integral<T>::value && !integer::is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			in_place_sub(integer(n));
		}
		template <typename T>
		void in_place_sub(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			operator=(static_cast<T>(*this) - x);
		}
		// Binary subtraction.
		template <typename Q, typename T>
		static rational binary_minus(Q &&q, T &&x, typename std::enable_if<
			std::is_same<rational,typename std::decay<Q>::type>::value && (
			are_binary_op_types<Q,T>::value && !std::is_floating_point<typename std::decay<T>::type>::value
			)>::type * = piranha_nullptr)
		{
			rational retval(std::forward<Q>(q));
			retval -= std::forward<T>(x);
			return retval;
		}
		template <typename T, typename Q>
		static rational binary_minus(T &&x, Q &&q, typename std::enable_if<
			std::is_same<rational,typename std::decay<Q>::type>::value && (
			are_binary_op_types<Q,T>::value && !std::is_floating_point<typename std::decay<T>::type>::value &&
			// Disambiguate when both operands are rationals.
			!std::is_same<rational,typename std::decay<T>::type>::value
			)>::type * = piranha_nullptr)
		{
			auto retval = binary_minus(std::forward<Q>(q),std::forward<T>(x));
			retval.negate();
			return retval;
		}
		template <typename T>
		static T binary_minus(const rational &q, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			return (static_cast<T>(q) - x);
		}
		template <typename T>
		static T binary_minus(const T &x, const rational &q, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			return -binary_minus(q,x);
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
		rational(rational &&other) piranha_noexcept_spec(true)
		{
			auto mover = [](::mpz_t a, ::mpz_t b) {
				// Move b into a.
				a->_mp_size = b->_mp_size;
				a->_mp_d = b->_mp_d;
				a->_mp_alloc = b->_mp_alloc;
				// Erase b.
				b->_mp_size = 0;
				b->_mp_d = piranha_nullptr;
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
			>::type * = piranha_nullptr)
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
			>::type * = piranha_nullptr)
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
		 * Will clear the internal \p mpz_q type.
		 */
		~rational() piranha_noexcept_spec(true)
		{
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
		/// Move assignment operator.
		/**
		 * @param[in] other rational to be moved.
		 * 
		 * @return reference to \p this.
		 */
		rational &operator=(rational &&other) piranha_noexcept_spec(true)
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
		template <typename T, typename std::enable_if<integer::is_interop_type<T>::value || std::is_same<T,integer>::value>::type*& = enabler>
		explicit operator T() const
		{
			return convert_to_impl<T>();
		}
		/// Swap.
		/**
		 * Swap the content of \p this and \p q.
		 * 
		 * @param[in] q swap argument.
		 */
		void swap(rational &q) piranha_noexcept_spec(true)
		{
			if (unlikely(this == &q)) {
			    return;
			}
			::mpq_swap(m_value,q.m_value);
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
		 * @throws unspecified any exception resulting from operating on non-finite floating-point values or from failures in floating-point conversions.
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
		 * @throws unspecified any exception resulting from casting piranha::rational to \p T.
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
		 * @throws unspecified any exception resulting from the conversion of piranha::rational to floating-point types.
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
		 * @throws unspecified any exception resulting from operating on non-finite floating-point values or from failures in floating-point conversions.
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
		 * @throws unspecified any exception resulting from casting piranha::rational to \p T.
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
		 * @throws unspecified any exception resulting from the conversion of piranha::rational to floating-point types.
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
		/// Overload output stream operator for piranha::rational.
		/**
		 * @param[in] os output stream.
		 * @param[in] q piranha::rational to be directed to stream.
		 * 
		 * @return reference to \p os.
		 * 
		 * @throws std::overflow_error if the number of digits is larger than an implementation-defined maximum.
		 * @throws unspecified any exception thrown by memory allocation errors in standard container.
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
	private:
		::mpq_t m_value;
};

}

#endif
