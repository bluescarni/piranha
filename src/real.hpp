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

#ifndef PIRANHA_REAL_HPP
#define PIRANHA_REAL_HPP

#include <algorithm>
#include <boost/integer_traits.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "config.hpp"
#include "detail/is_digit.hpp"
#include "detail/mpfr.hpp"
#include "detail/real_fwd.hpp"
#include "exceptions.hpp"
#include "integer.hpp"
#include "math.hpp"
#include "rational.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

template <typename = int>
struct real_base
{
	// Default rounding mode.
	// All operations will use the MPFR_RNDN (round to nearest) rounding mode.
	static const ::mpfr_rnd_t default_rnd = MPFR_RNDN;
	// Default significand precision.
	// The precision is the number of bits used to represent the significand of a floating-point number.
	// This default value is equivalent to the IEEE 754 quadruple-precision binary floating-point format.
	static const ::mpfr_prec_t default_prec = 113;
};

template <typename T>
const ::mpfr_rnd_t real_base<T>::default_rnd;

template <typename T>
const ::mpfr_prec_t real_base<T>::default_prec;

}

/// Arbitrary precision floating-point class.
/**
 * This class represents floating-point ("real") numbers of arbitrary size (i.e., the size is limited only by the available memory).
 * The implementation consists of a C++ wrapper around the \p mpfr_t type from the multiprecision MPFR library. Real numbers
 * are represented in binary format and they consist of an arbitrary-size significand coupled to a fixed-size exponent.
 * 
 * Unless noted otherwise, this implementation always uses the \p MPFR_RNDN (round to nearest) rounding mode for all operations.
 * 
 * \section interop Interoperability with fundamental types
 * 
 * Full interoperability with the same fundamental C++ types as piranha::integer is provided.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * Unless noted otherwise, this class provides the strong exception safety guarantee for all operations.
 * In case of memory allocation errors by GMP/MPFR routines, the program will terminate.
 * 
 * \section move_semantics Move semantics
 * 
 * Move construction and move assignment will leave the moved-from object in a state that is destructible and assignable.
 * 
 * @see http://www.mpfr.org
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
// TODO:
// - fix the move semantics if possible (i.e., valid but unspecified state),
// - add interoperability with long double and long long, avoiding the is_gmp_int stuff ->
//   look into using the intmax_t overloads from MPFR,
// - maybe we can replace the raii str holder with a unique_ptr with custom deleter,
// - fix use of isdigit.
// - should probably review all the precision handling stuff, it's really easy to forget about something :/
// - here we are using a straight mpfr_t as underlying member, which is an array. Doest it matter? Should we use
//   the corresponding struct for consistency? Does it matter for performance?
// - if we overhaul the tests, put random precision values as well.
class real: public detail::real_base<>
{
		// Type trait for allowed arguments in arithmetic binary operations.
		template <typename T, typename U>
		struct are_binary_op_types: std::integral_constant<bool,
			(std::is_same<typename std::decay<T>::type,real>::value && integer::is_interop_type<typename std::decay<U>::type>::value) ||
			(std::is_same<typename std::decay<U>::type,real>::value && integer::is_interop_type<typename std::decay<T>::type>::value) ||
			(std::is_same<typename std::decay<T>::type,real>::value && std::is_same<typename std::decay<U>::type,integer>::value) ||
			(std::is_same<typename std::decay<U>::type,real>::value && std::is_same<typename std::decay<T>::type,integer>::value) ||
			(std::is_same<typename std::decay<T>::type,real>::value && std::is_same<typename std::decay<U>::type,rational>::value) ||
			(std::is_same<typename std::decay<U>::type,real>::value && std::is_same<typename std::decay<T>::type,rational>::value) ||
			(std::is_same<typename std::decay<T>::type,real>::value && std::is_same<typename std::decay<U>::type,real>::value)>
		{};
		// RAII struct to manage strings received via ::mpfr_get_str().
		struct mpfr_str_manager
		{
			explicit mpfr_str_manager(char *str):m_str(str) {}
			~mpfr_str_manager()
			{
				if (m_str) {
					::mpfr_free_str(m_str);
				}
			}
			char *m_str;
		};
		static void prec_check(const ::mpfr_prec_t &prec)
		{
			if (prec < MPFR_PREC_MIN || prec > MPFR_PREC_MAX) {
				piranha_throw(std::invalid_argument,"invalid significand precision requested");
			}
		}
		// Construction.
		void construct_from_string(const char *str, const ::mpfr_prec_t &prec)
		{
			prec_check(prec);
			::mpfr_init2(m_value,prec);
			const int retval = ::mpfr_set_str(m_value,str,10,default_rnd);
			if (retval != 0) {
				::mpfr_clear(m_value);
				piranha_throw(std::invalid_argument,"invalid string input for real");
			}
		}
		template <typename T>
		void construct_from_generic(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			::mpfr_set_d(m_value,static_cast<double>(x),default_rnd);
		}
		template <typename T>
		void construct_from_generic(const T &si, typename std::enable_if<std::is_signed<T>::value && integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpfr_set_si(m_value,static_cast<long>(si),default_rnd);
		}
		template <typename T>
		void construct_from_generic(const T &ui, typename std::enable_if<std::is_unsigned<T>::value && integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpfr_set_ui(m_value,static_cast<unsigned long>(ui),default_rnd);
		}
		template <typename T>
		void construct_from_generic(const T &ll, typename std::enable_if<std::is_integral<T>::value &&
			!integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			construct_from_generic(integer(ll));
		}
		void construct_from_generic(const integer &n)
		{
			::mpfr_set_z(m_value,n.m_value,default_rnd);
		}
		void construct_from_generic(const rational &q)
		{
			::mpfr_set_q(m_value,q.m_value,default_rnd);
		}
		// Assignment.
		void assign_from_string(const char *str)
		{
			piranha_assert(m_value->_mpfr_d);
			const int retval = ::mpfr_set_str(m_value,str,10,default_rnd);
			if (retval != 0) {
				// Reset the internal value, as it might have been changed by ::mpfr_set_str().
				::mpfr_set_zero(m_value,0);
				piranha_throw(std::invalid_argument,"invalid string input for real");
			}
		}
		// Conversion.
		template <typename T>
		typename std::enable_if<std::is_same<T,bool>::value,T>::type convert_to_impl() const
		{
			return (sign() != 0);
		}
		template <typename T>
		typename std::enable_if<std::is_same<T,integer>::value,T>::type convert_to_impl() const
		{
			if (is_nan() || is_inf()) {
				piranha_throw(std::overflow_error,"cannot convert non-finite real to an integral value");
			}
			integer retval;
			// Explicitly request rounding to zero in this case.
			::mpfr_get_z(retval.m_value,m_value,MPFR_RNDZ);
			return retval;
		}
		template <typename T>
		typename std::enable_if<std::is_integral<T>::value && !std::is_same<T,bool>::value,T>::type convert_to_impl() const
		{
			// NOTE: of course, this can be optimised by avoiding going through the integer conversion and
			// using directly the MPFR functions.
			return static_cast<T>(static_cast<integer>(*this));
		}
		template <typename T>
		typename std::enable_if<std::is_floating_point<T>::value,T>::type convert_to_impl() const
		{
			if (is_nan()) {
				if (std::numeric_limits<T>::has_quiet_NaN) {
					return std::numeric_limits<T>::quiet_NaN();
				} else {
					piranha_throw(std::overflow_error,"cannot convert NaN to floating-point type");
				}
			}
			if (is_inf()) {
				piranha_assert(sign() != 0);
				if (std::numeric_limits<T>::has_infinity && sign() > 0) {
					return std::numeric_limits<T>::infinity();
				} else if (std::numeric_limits<T>::has_infinity && sign() < 0 && std::signbit(std::numeric_limits<T>::lowest()) != 0) {
					return std::copysign(std::numeric_limits<T>::infinity(),std::numeric_limits<T>::lowest());
				} else {
					piranha_throw(std::overflow_error,"cannot convert infinity to floating-point type");
				}
			}
			if (std::is_same<T,double>::value) {
				return static_cast<T>(::mpfr_get_d(m_value,default_rnd));
			}
			return ::mpfr_get_flt(m_value,default_rnd);
		}
		template <typename T>
		typename std::enable_if<std::is_same<T,rational>::value,T>::type convert_to_impl() const
		{
			if (is_nan()) {
				piranha_throw(std::overflow_error,"cannot convert NaN to rational");
			}
			if (is_inf()) {
				piranha_throw(std::overflow_error,"cannot convert infinity to rational");
			}
			if (sign() == 0) {
				return rational{};
			}
			// Get string representation.
			::mpfr_exp_t exp(0);
			mpfr_str_manager m(::mpfr_get_str(nullptr,&exp,10,0,m_value,default_rnd));
			auto str = m.m_str;
			if (!str) {
				piranha_throw(std::overflow_error,"error in conversion of real to rational: the call to the MPFR function failed");
			}
			// Transform into fraction.
			std::size_t digits = 0u;
			for (; *str != '\0'; ++str) {
				if (detail::is_digit(*str)) {
					++digits;
				}
			}
			if (!digits) {
				piranha_throw(std::overflow_error,"error in conversion of real to rational: invalid number of digits");
			}
			// NOTE: here the only exception that can be thrown is when raising to a power
			// that cannot be represented by unsigned long.
			try {
				rational retval(m.m_str);
				// NOTE: possible optimizations here include going through direct GMP routines.
				retval *= rational(1,10).pow(digits);
				retval *= rational(10).pow(exp);
				return retval;
			} catch (...) {
				piranha_throw(std::overflow_error,"error in conversion of real to rational: exponent is too large");
			}
		}
		// In-place addition.
		// NOTE: all sorts of optimisations, here and in binary add, are possible (e.g., steal from rvalue ref, 
		// avoid setting precision twice in binary operators, etc.). For the moment we keep it basic.
		void in_place_add(const real &r)
		{
			if (r.get_prec() > get_prec()) {
				// Re-init this with the prec of r.
				*this = real{*this,r.get_prec()};
			}
			::mpfr_add(m_value,m_value,r.m_value,default_rnd);
		}
		void in_place_add(const rational &q)
		{
			::mpfr_add_q(m_value,m_value,q.m_value,default_rnd);
		}
		void in_place_add(const integer &n)
		{
			::mpfr_add_z(m_value,m_value,n.m_value,default_rnd);
		}
		template <typename T>
		void in_place_add(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpfr_add_si(m_value,m_value,static_cast<long>(si),default_rnd);
		}
		template <typename T>
		void in_place_add(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpfr_add_ui(m_value,m_value,static_cast<unsigned long>(ui),default_rnd);
		}
		template <typename T>
		void in_place_add(const T &n, typename std::enable_if<std::is_integral<T>::value && !integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			in_place_add(integer(n));
		}
		template <typename T>
		void in_place_add(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			static_assert(std::numeric_limits<T>::radix > 0,"Invalid radix");
			const unsigned radix = static_cast<unsigned>(std::numeric_limits<T>::radix);
			if ((radix & (radix - 1u)) == 0u) {
				::mpfr_add_d(m_value,m_value,static_cast<double>(x),default_rnd);
			} else {
				in_place_add(real(x,get_prec()));
			}
		}
		// Binary add.
		// NOTE: here we need to distinguish between the two cases because we want to avoid the case in which
		// we are constructing a real with a non-real. In that case we might mess up the precision of the result,
		// as default_prec would be used during construction and it would not be equal in general to the precision
		// of the other operand.
		template <typename T, typename U>
		static real binary_add(T &&a, U &&b, typename std::enable_if<
			// NOTE: T == U means they have both to be real.
			std::is_same<typename std::decay<T>::type,typename std::decay<U>::type>::value ||
			std::is_same<typename std::decay<T>::type,real>::value
			>::type * = nullptr)
		{
			real retval(std::forward<T>(a));
			retval += std::forward<U>(b);
			return retval;
		}
		template <typename T, typename U>
		static real binary_add(T &&a, U &&b, typename std::enable_if<
			!std::is_same<typename std::decay<T>::type,real>::value
			>::type * = nullptr)
		{
			return binary_add(std::forward<U>(b),std::forward<T>(a));
		}
		// In-place subtraction.
		void in_place_sub(const real &r)
		{
			if (r.get_prec() > get_prec()) {
				*this = real{*this,r.get_prec()};
			}
			::mpfr_sub(m_value,m_value,r.m_value,default_rnd);
		}
		void in_place_sub(const rational &q)
		{
			::mpfr_sub_q(m_value,m_value,q.m_value,default_rnd);
		}
		void in_place_sub(const integer &n)
		{
			::mpfr_sub_z(m_value,m_value,n.m_value,default_rnd);
		}
		template <typename T>
		void in_place_sub(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpfr_sub_si(m_value,m_value,static_cast<long>(si),default_rnd);
		}
		template <typename T>
		void in_place_sub(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpfr_sub_ui(m_value,m_value,static_cast<unsigned long>(ui),default_rnd);
		}
		template <typename T>
		void in_place_sub(const T &n, typename std::enable_if<std::is_integral<T>::value && !integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			in_place_sub(integer(n));
		}
		template <typename T>
		void in_place_sub(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			static_assert(std::numeric_limits<T>::radix > 0,"Invalid radix");
			const unsigned radix = static_cast<unsigned>(std::numeric_limits<T>::radix);
			if ((radix & (radix - 1u)) == 0u) {
				::mpfr_sub_d(m_value,m_value,static_cast<double>(x),default_rnd);
			} else {
				in_place_sub(real(x,get_prec()));
			}
		}
		// Binary subtraction.
		template <typename T, typename U>
		static real binary_sub(T &&a, U &&b, typename std::enable_if<
			std::is_same<typename std::decay<T>::type,typename std::decay<U>::type>::value ||
			std::is_same<typename std::decay<T>::type,real>::value
			>::type * = nullptr)
		{
			real retval(std::forward<T>(a));
			retval -= std::forward<U>(b);
			return retval;
		}
		template <typename T, typename U>
		static real binary_sub(T &&a, U &&b, typename std::enable_if<
			!std::is_same<typename std::decay<T>::type,real>::value
			>::type * = nullptr)
		{
			real retval(binary_sub(std::forward<U>(b),std::forward<T>(a)));
			retval.negate();
			return retval;
		}
		// In-place multiplication.
		void in_place_mul(const real &r)
		{
			if (r.get_prec() > get_prec()) {
				*this = real{*this,r.get_prec()};
			}
			::mpfr_mul(m_value,m_value,r.m_value,default_rnd);
		}
		void in_place_mul(const rational &q)
		{
			::mpfr_mul_q(m_value,m_value,q.m_value,default_rnd);
		}
		void in_place_mul(const integer &n)
		{
			::mpfr_mul_z(m_value,m_value,n.m_value,default_rnd);
		}
		template <typename T>
		void in_place_mul(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpfr_mul_si(m_value,m_value,static_cast<long>(si),default_rnd);
		}
		template <typename T>
		void in_place_mul(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpfr_mul_ui(m_value,m_value,static_cast<unsigned long>(ui),default_rnd);
		}
		template <typename T>
		void in_place_mul(const T &n, typename std::enable_if<std::is_integral<T>::value && !integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			in_place_mul(integer(n));
		}
		template <typename T>
		void in_place_mul(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			static_assert(std::numeric_limits<T>::radix > 0,"Invalid radix");
			const unsigned radix = static_cast<unsigned>(std::numeric_limits<T>::radix);
			if ((radix & (radix - 1u)) == 0u) {
				::mpfr_mul_d(m_value,m_value,static_cast<double>(x),default_rnd);
			} else {
				in_place_mul(real(x,get_prec()));
			}
		}
		// Binary multiplication.
		template <typename T, typename U>
		static real binary_mul(T &&a, U &&b, typename std::enable_if<
			std::is_same<typename std::decay<T>::type,typename std::decay<U>::type>::value ||
			std::is_same<typename std::decay<T>::type,real>::value
			>::type * = nullptr)
		{
			real retval(std::forward<T>(a));
			retval *= std::forward<U>(b);
			return retval;
		}
		template <typename T, typename U>
		static real binary_mul(T &&a, U &&b, typename std::enable_if<
			!std::is_same<typename std::decay<T>::type,real>::value
			>::type * = nullptr)
		{
			return binary_mul(std::forward<U>(b),std::forward<T>(a));
		}
		// In-place division.
		void in_place_div(const real &r)
		{
			if (r.get_prec() > get_prec()) {
				*this = real{*this,r.get_prec()};
			}
			::mpfr_div(m_value,m_value,r.m_value,default_rnd);
		}
		void in_place_div(const rational &q)
		{
			::mpfr_div_q(m_value,m_value,q.m_value,default_rnd);
		}
		void in_place_div(const integer &n)
		{
			::mpfr_div_z(m_value,m_value,n.m_value,default_rnd);
		}
		template <typename T>
		void in_place_div(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpfr_div_si(m_value,m_value,static_cast<long>(si),default_rnd);
		}
		template <typename T>
		void in_place_div(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			::mpfr_div_ui(m_value,m_value,static_cast<unsigned long>(ui),default_rnd);
		}
		template <typename T>
		void in_place_div(const T &n, typename std::enable_if<std::is_integral<T>::value && !integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			in_place_div(integer(n));
		}
		template <typename T>
		void in_place_div(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			static_assert(std::numeric_limits<T>::radix > 0,"Invalid radix");
			const unsigned radix = static_cast<unsigned>(std::numeric_limits<T>::radix);
			if ((radix & (radix - 1u)) == 0u) {
				::mpfr_div_d(m_value,m_value,static_cast<double>(x),default_rnd);
			} else {
				in_place_div(real(x,get_prec()));
			}
		}
		// Binary division.
		template <typename T, typename U>
		static real binary_div(T &&a, U &&b, typename std::enable_if<
			std::is_same<typename std::decay<T>::type,typename std::decay<U>::type>::value ||
			std::is_same<typename std::decay<T>::type,real>::value
			>::type * = nullptr)
		{
			real retval(std::forward<T>(a));
			retval /= std::forward<U>(b);
			return retval;
		}
		template <typename T, typename U>
		static real binary_div(T &&a, U &&b, typename std::enable_if<
			!std::is_same<typename std::decay<T>::type,real>::value
			>::type * = nullptr)
		{
			// Create retval from a, with same precision as b.
			real retval(std::forward<T>(a),b.get_prec());
			retval /= std::forward<U>(b);
			return retval;
		}
		// Equality.
		static bool binary_equality(const real &r1, const real &r2)
		{
			return (::mpfr_equal_p(r1.m_value,r2.m_value) != 0);
		}
		static bool binary_equality(const real &r, const integer &n)
		{
			if (r.is_nan()) {
				return false;
			}
			return (::mpfr_cmp_z(r.m_value,n.m_value) == 0);
		}
		static bool binary_equality(const real &r, const rational &q)
		{
			if (r.is_nan()) {
				return false;
			}
			return (::mpfr_cmp_q(r.m_value,q.m_value) == 0);
		}
		template <typename T>
		static bool binary_equality(const real &r, const T &n, typename std::enable_if<std::is_signed<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			if (r.is_nan()) {
				return false;
			}
			return (::mpfr_cmp_si(r.m_value,static_cast<long>(n)) == 0);
		}
		template <typename T>
		static bool binary_equality(const real &r, const T &n, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			if (r.is_nan()) {
				return false;
			}
			return (::mpfr_cmp_ui(r.m_value,static_cast<unsigned long>(n)) == 0);
		}
		template <typename T>
		static bool binary_equality(const real &r, const T &n, typename std::enable_if<std::is_integral<T>::value &&
			!integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			return binary_equality(r,integer(n));
		}
		template <typename T>
		static bool binary_equality(const real &r, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			if (r.is_nan() || boost::math::isnan(x)) {
				return false;
			}
			return (::mpfr_cmp_d(r.m_value,static_cast<double>(x)) == 0);
		}
		// NOTE: this is the reverse of above.
		template <typename T>
		static bool binary_equality(const T &x, const real &r, typename std::enable_if<
			std::is_arithmetic<T>::value || std::is_same<T,integer>::value ||
			std::is_same<T,rational>::value>::type * = nullptr)
		{
			return binary_equality(r,x);
		}
		// Binary less-than.
		static bool binary_less_than(const real &r1, const real &r2)
		{
			return (::mpfr_less_p(r1.m_value,r2.m_value) != 0);
		}
		static bool binary_less_than(const real &r, const rational &q)
		{
			return (::mpfr_cmp_q(r.m_value,q.m_value) < 0);
		}
		static bool binary_less_than(const real &r, const integer &n)
		{
			return (::mpfr_cmp_z(r.m_value,n.m_value) < 0);
		}
		template <typename T>
		static bool binary_less_than(const real &r, const T &n, typename std::enable_if<std::is_signed<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			return (::mpfr_cmp_si(r.m_value,static_cast<long>(n)) < 0);
		}
		template <typename T>
		static bool binary_less_than(const real &r, const T &n, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			return (::mpfr_cmp_ui(r.m_value,static_cast<unsigned long>(n)) < 0);
		}
		template <typename T>
		static bool binary_less_than(const real &r, const T &n, typename std::enable_if<std::is_integral<T>::value &&
			!integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			return binary_less_than(r,integer(n));
		}
		template <typename T>
		static bool binary_less_than(const real &r, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return (::mpfr_cmp_d(r.m_value,static_cast<double>(x)) < 0);
		}
		// Binary less-than or equal.
		static bool binary_leq(const real &r1, const real &r2)
		{
			return (::mpfr_lessequal_p(r1.m_value,r2.m_value) != 0);
		}
		static bool binary_leq(const real &r, const rational &q)
		{
			return (::mpfr_cmp_q(r.m_value,q.m_value) <= 0);
		}
		static bool binary_leq(const real &r, const integer &n)
		{
			return (::mpfr_cmp_z(r.m_value,n.m_value) <= 0);
		}
		template <typename T>
		static bool binary_leq(const real &r, const T &n, typename std::enable_if<std::is_signed<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			return (::mpfr_cmp_si(r.m_value,static_cast<long>(n)) <= 0);
		}
		template <typename T>
		static bool binary_leq(const real &r, const T &n, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			return (::mpfr_cmp_ui(r.m_value,static_cast<unsigned long>(n)) <= 0);
		}
		template <typename T>
		static bool binary_leq(const real &r, const T &n, typename std::enable_if<std::is_integral<T>::value &&
			!integer::is_gmp_int<T>::value>::type * = nullptr)
		{
			return binary_leq(r,integer(n));
		}
		template <typename T>
		static bool binary_leq(const real &r, const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return (::mpfr_cmp_d(r.m_value,static_cast<double>(x)) <= 0);
		}
		// Inverse forms of less-than and leq.
		template <typename T>
		static bool binary_less_than(const T &x, const real &r, typename std::enable_if<std::is_arithmetic<T>::value ||
			std::is_same<T,integer>::value ||
			std::is_same<T,rational>::value>::type * = nullptr)
		{
			return !binary_leq(r,x);
		}
		template <typename T>
		static bool binary_leq(const T &x, const real &r, typename std::enable_if<std::is_arithmetic<T>::value ||
			std::is_same<T,integer>::value ||
			std::is_same<T,rational>::value>::type * = nullptr)
		{
			return !binary_less_than(r,x);
		}
		// NOTE: we need to handle separately the NaNs as we cannot resort to the inversion of the comparison operators for them.
		static bool check_nan(const real &r)
		{
			return r.is_nan();
		}
		template <typename T>
		static bool check_nan(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return boost::math::isnan(x);
		}
		template <typename T>
		static bool check_nan(const T &, typename std::enable_if<!std::is_floating_point<T>::value>::type * = nullptr)
		{
			return false;
		}
		template <typename T, typename U>
		static bool is_nan_comparison(const T &a, const U &b)
		{
			return (check_nan(a) || check_nan(b));
		}
		// Exponentiation.
		real pow_impl(const real &r) const
		{
			real retval{0,get_prec()};
			::mpfr_pow(retval.m_value,m_value,r.m_value,default_rnd);
			return retval;
		}
		real pow_impl(const integer &n) const
		{
			real retval{0,get_prec()};
			::mpfr_pow_z(retval.m_value,m_value,n.m_value,default_rnd);
			return retval;
		}
		template <typename T>
		real pow_impl(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr) const
		{
			return pow_impl(real{x,get_prec()});
		}
		template <typename T>
		real pow_impl(const T &si, typename std::enable_if<std::is_signed<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr) const
		{
			real retval{0,get_prec()};
			::mpfr_pow_si(retval.m_value,m_value,static_cast<long>(si),default_rnd);
			return retval;
		}
		template <typename T>
		real pow_impl(const T &ui, typename std::enable_if<std::is_unsigned<T>::value &&
			integer::is_gmp_int<T>::value>::type * = nullptr) const
		{
			real retval{0,get_prec()};
			::mpfr_pow_ui(retval.m_value,m_value,static_cast<unsigned long>(ui),default_rnd);
			return retval;
		}
		template <typename T>
		real pow_impl(const T &n, typename std::enable_if<std::is_integral<T>::value &&
			!integer::is_gmp_int<T>::value>::type * = nullptr) const
		{
			return pow_impl(integer(n));
		}
	public:
		/// Default constructor.
		/**
		 * Will initialize the number to zero, using real::default_prec as significand precision.
		 */
		real()
		{
			::mpfr_init2(m_value,default_prec);
			::mpfr_set_zero(m_value,0);
		}
		/// Copy constructor.
		/**
		 * Will deep-copy \p other.
		 * 
		 * @param[in] other real to be copied.
		 */
		real(const real &other)
		{
			// Init with the same precision as other, and then set.
			::mpfr_init2(m_value,other.get_prec());
			::mpfr_set(m_value,other.m_value,default_rnd);
		}
		/// Move constructor.
		/**
		 * @param[in] other real to be moved.
		 */
		real(real &&other) noexcept
		{
			m_value->_mpfr_prec = other.m_value->_mpfr_prec;
			m_value->_mpfr_sign = other.m_value->_mpfr_sign;
			m_value->_mpfr_exp = other.m_value->_mpfr_exp;
			m_value->_mpfr_d = other.m_value->_mpfr_d;
			// Erase other.
			other.m_value->_mpfr_prec = 0;
			other.m_value->_mpfr_sign = 0;
			other.m_value->_mpfr_exp = 0;
			other.m_value->_mpfr_d = nullptr;
		}
		/// Constructor from C string.
		/**
		 * Will use the string \p str and precision \p prec to initialize the number.
		 * The expected string format, assuming representation in base 10, is described in the MPFR documentation.
		 * 
		 * @param[in] str string representation of the real number.
		 * @param[in] prec desired significand precision.
		 * 
		 * @throws std::invalid_argument if the conversion from string fails or if the requested significand precision
		 * is not within the range allowed by the MPFR library.
		 * 
		 * @see http://www.mpfr.org/mpfr-current/mpfr.html#Assignment-Functions
		 */
		explicit real(const char *str, const ::mpfr_prec_t &prec = default_prec)
		{
			construct_from_string(str,prec);
		}
		/// Constructor from C++ string.
		/**
		 * Equivalent to the constructor from C string.
		 * 
		 * @param[in] str string representation of the real number.
		 * @param[in] prec desired significand precision.
		 * 
		 * @throws unspecified any exception thrown by the constructor from C string.
		 */
		explicit real(const std::string &str, const ::mpfr_prec_t &prec = default_prec)
		{
			construct_from_string(str.c_str(),prec);
		}
		/// Copy constructor with different precision.
		/**
		 * \p this will be first initialised with precision \p prec, and then \p other will be assigned to \p this.
		 * 
		 * @param[in] other real to be copied.
		 * @param[in] prec desired significand precision.
		 * 
		 * @throws std::invalid_argument if the requested significand precision
		 * is not within the range allowed by the MPFR library.
		 */
		explicit real(const real &other, const ::mpfr_prec_t &prec)
		{
			prec_check(prec);
			::mpfr_init2(m_value,prec);
			::mpfr_set(m_value,other.m_value,default_rnd);
		}
		/// Generic constructor.
		/**
		 * The supported types for \p T are the \ref interop "interoperable types", piranha::integer and piranha::rational.
		 * Use of other types will result in a compile-time error.
		 * 
		 * @param[in] x object used to construct \p this.
		 * @param[in] prec desired significand precision.
		 * 
		 * @throws std::invalid_argument if the requested significand precision
		 * is not within the range allowed by the MPFR library.
		 */
		template <typename T>
		explicit real(const T &x, const ::mpfr_prec_t &prec = default_prec, typename std::enable_if<
			integer::is_interop_type<T>::value ||
			std::is_same<T,integer>::value ||
			std::is_same<T,rational>::value>::type * = nullptr)
		{
			prec_check(prec);
			::mpfr_init2(m_value,prec);
			construct_from_generic(x);
		}
		/// Destructor.
		/**
		 * Will clear the internal MPFR variable.
		 */
		~real();
		/// Copy assignment operator.
		/**
		 * The assignment operation will deep-copy \p other (i.e., including its precision).
		 * 
		 * @param[in] other real to be copied.
		 * 
		 * @return reference to \p this.
		 */
		real &operator=(const real &other)
		{
			if (this != &other) {
				// Handle assignment to moved-from objects.
				if (m_value->_mpfr_d) {
					// Copy the precision. This will also reset the internal value.
					set_prec(other.get_prec());
				} else {
					piranha_assert(!m_value->_mpfr_prec && !m_value->_mpfr_sign && !m_value->_mpfr_exp);
					// Reinit before setting.
					::mpfr_init2(m_value,other.get_prec());
				}
				::mpfr_set(m_value,other.m_value,default_rnd);
			}
			return *this;
		}
		/// Move assignment operator.
		/**
		 * @param[in] other real to be moved.
		 * 
		 * @return reference to \p this.
		 */
		real &operator=(real &&other) noexcept
		{
			// NOTE: swap() already has the check for this.
			swap(other);
			return *this;
		}
		/// Assignment operator from C++ string.
		/**
		 * The implementation is equivalent to the assignment operator from C string.
		 * 
		 * @param[in] str string representation of the real to be assigned.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the assignment operator from C string.
		 */
		real &operator=(const std::string &str)
		{
			return operator=(str.c_str());
		}
		/// Assignment operator from C string.
		/**
		 * The parsing rules are the same as in the constructor from string. The precision of \p this
		 * will not be changed by the assignment operation, unless \p this was the target of a move operation that
		 * left it in an uninitialised state.
		 * In that case, \p this will be re-initialised with the default precision.
		 * 
		 * In case \p str is malformed, before an exception is thrown the value of \p this will be reset to zero.
		 * 
		 * @param[in] str string representation of the real to be assigned.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws std::invalid_argument if the conversion from string fails.
		 */
		real &operator=(const char *str)
		{
			// Handle moved-from objects.
			if (m_value->_mpfr_d) {
				assign_from_string(str);
			} else {
				piranha_assert(!m_value->_mpfr_prec && !m_value->_mpfr_sign && !m_value->_mpfr_exp);
				construct_from_string(str,default_prec);
			}
			return *this;
		}
		/// Generic assignment operator.
		/**
		 * The supported types for \p T are the \ref interop "interoperable types", piranha::integer and piranha::rational.
		 * Use of other types will result in a compile-time error. The precision of \p this
		 * will not be changed by the assignment operation, unless \p this was the target of a move operation that
		 * left it in an uninitialised state.
		 * In that case, \p this will be re-initialised with the default precision.
		 * 
		 * @param[in] x object that will be assigned to \p this.
		 * 
		 * @return reference to \p this.
		 */
		template <typename T>
		typename std::enable_if<integer::is_interop_type<T>::value ||
			std::is_same<T,integer>::value ||
			std::is_same<T,rational>::value,real &>::type operator=(const T &x)
		{
			if (!m_value->_mpfr_d) {
				piranha_assert(!m_value->_mpfr_prec && !m_value->_mpfr_sign && !m_value->_mpfr_exp);
				// Re-init with default prec if it was moved-from.
				::mpfr_init2(m_value,default_prec);
			}
			// NOTE: all construct_from_generic() methods here are really assignments.
			construct_from_generic(x);
			return *this;
		}
		/// Conversion operator.
		/**
		 * Extract an instance of type \p T from \p this. The supported types for \p T are the \ref interop "interoperable types", piranha::integer
		 * and piranha::rational.
		 * 
		 * Conversion to \p bool is always successful, and returns <tt>sign() != 0</tt>.
		 * Conversion to the other integral types is truncated (i.e., rounded to zero), its success depending on whether or not
		 * the target type can represent the truncated value.
		 * 
		 * Conversion to floating point types is exact if the target type can represent exactly the current value.
		 * If that is not the case, the output value will be the nearest adjacent. If \p this is not finite,
		 * corresponding non-finite values will be produced if the floating-point type supports them, otherwise
		 * an error will be produced.
		 * 
		 * Conversion of finite values to piranha::rational will be exact. Conversion of non-finite values will result in runtime
		 * errors.
		 * 
		 * @return result of the conversion to target type T.
		 * 
		 * @throws std::overflow_error if the conversion fails in one of the ways described above.
		 */
		template <typename T, typename = typename std::enable_if<integer::is_interop_type<T>::value ||
			std::is_same<T,integer>::value ||
			std::is_same<T,rational>::value>::type>
			explicit operator T() const
		{
			return convert_to_impl<T>();
		}
		/// Swap.
		/**
		 * Swap \p this with \p other.
		 * 
		 * @param[in] other swap argument.
		 */
		void swap(real &other) noexcept
		{
			if (this == &other) {
				return;
			}
			std::swap(m_value->_mpfr_d,other.m_value->_mpfr_d);
			std::swap(m_value->_mpfr_prec,other.m_value->_mpfr_prec);
			std::swap(m_value->_mpfr_sign,other.m_value->_mpfr_sign);
			std::swap(m_value->_mpfr_exp,other.m_value->_mpfr_exp);
		}
		/// Sign.
		/**
		 * @return 1 if <tt>this > 0</tt>, 0 if <tt>this == 0</tt> and -1 if <tt>this < 0</tt>. If \p this is NaN, zero will be returned.
		 */
		int sign() const
		{
			if (is_nan()) {
				return 0;
			} else {
				return mpfr_sgn(m_value);
			}
		}
		/// Test for zero.
		/**
		 * @return \p true it \p this is zero, \p false otherwise.
		 */
		bool is_zero() const
		{
			return (mpfr_zero_p(m_value) != 0);
		}
		/// Test for NaN.
		/**
		 * @return \p true if \p this is NaN, \p false otherwise.
		 */
		bool is_nan() const
		{
			return mpfr_nan_p(m_value) != 0;
		}
		/// Test for infinity.
		/**
		 * @return \p true if \p this represents infinity, \p false otherwise.
		 */
		bool is_inf() const
		{
			return mpfr_inf_p(m_value) != 0;
		}
		/// Get precision.
		/**
		 * @return the number of bits used to represent the significand of \p this.
		 */
		::mpfr_prec_t get_prec() const
		{
			return mpfr_get_prec(m_value);
		}
		/// Set precision.
		/**
		 * Will set the significand precision of \p this to exactly \p prec bits, and reset the value of \p this to NaN.
		 * 
		 * @param[in] prec desired significand precision.
		 * 
		 * @throws std::invalid_argument if the requested significand precision
		 * is not within the range allowed by the MPFR library.
		 */
		void set_prec(const ::mpfr_prec_t &prec)
		{
			prec_check(prec);
			::mpfr_set_prec(m_value,prec);
		}
		/// Negate in-place.
		/**
		 * Will set \p this to <tt>-this</tt>.
		 */
		void negate()
		{
			::mpfr_neg(m_value,m_value,default_rnd);
		}
		/// Truncate in-place.
		/**
		 * Set \p this to the next representable integer toward zero. If \p this is infinity or NaN, there will be no effect.
		 */
		void truncate()
		{
			if (is_inf() || is_nan()) {
				return;
			}
			::mpfr_trunc(m_value,m_value);
		}
		/// In-place addition.
		/**
		 * Add \p x to the current value of the real object. This template operator is activated only if
		 * \p T is either real, piranha::rational, piranha::integer or an \ref interop "interoperable type".
		 * 
		 * If \p T is real, \p x is added in-place to \p this. If the precision \p prec of \p x is greater than the precision of \p this,
		 * the precision of \p this is changed to \p prec before the operation takes place.
		 * 
		 * In-place addition of integral values and piranha::rational objects will use the corresponding MPFR routines.
		 * 
		 * If \p T is a floating-point type, the MPFR routine <tt>mpfr_add_d()</tt> is used if the radix of the type is a power
		 * of 2, otherwise \p x will be converted to a real (using the same precision of \p this) before being added to \p this.
		 * 
		 * @param[in] x argument for the addition.
		 * 
		 * @return reference to \p this.
		 * 
		 * @see http://www.mpfr.org/mpfr-current/mpfr.html#Basic-Arithmetic-Functions
		 */
		template <typename T>
		typename std::enable_if<
			integer::is_interop_type<typename std::decay<T>::type>::value ||
			std::is_same<real,typename std::decay<T>::type>::value ||
			std::is_same<rational,typename std::decay<T>::type>::value ||
			std::is_same<integer,typename std::decay<T>::type>::value,real &>::type operator+=(T &&x)
		{
			in_place_add(std::forward<T>(x));
			return *this;
		}
		/// Generic in-place addition with piranha::real.
		/**
		 * Add a piranha::real in-place. This template operator is activated only if \p T is an \ref interop "interoperable type", piranha::integer
		 * or piranha::rational, and \p R is piranha::real.
		 * This method will first compute <tt>r + x</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] r second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception resulting from casting piranha::real to \p T.
		 */
		template <typename T, typename R>
		friend typename std::enable_if<(integer::is_interop_type<T>::value || std::is_same<T,integer>::value ||
			std::is_same<T,rational>::value) &&
			std::is_same<typename std::decay<R>::type,real>::value,T &>::type
			operator+=(T &x, R &&r)
		{
			// NOTE: for the supported types, assignment can never throw.
			x = static_cast<T>(std::forward<R>(r) + x);
			return x;
		}
		/// Generic binary addition involving piranha::real.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::real and \p U is an \ref interop "interoperable type" or piranha::integer or piranha::rational,
		 * - \p U is piranha::real and \p T is an \ref interop "interoperable type" or piranha::integer or piranha::rational,
		 * - both \p T and \p U are piranha::real.
		 * 
		 * The return type is always real.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x + y</tt>.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,real>::type
			operator+(T &&x, U &&y)
		{
			return binary_add(std::forward<T>(x),std::forward<U>(y));
		}
		/// Identity operator.
		/**
		 * @return copy of \p this.
		 */
		real operator+() const
		{
			return *this;
		}
		/// Prefix increment.
		/**
		 * Increment \p this by one.
		 * 
		 * @return reference to \p this after the increment.
		 */
		real &operator++()
		{
			return operator+=(1);
		}
		/// Suffix increment.
		/**
		 * Increment \p this by one and return a copy of \p this as it was before the increment.
		 * 
		 * @return copy of \p this before the increment.
		 */
		real operator++(int)
		{
			const real retval(*this);
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
		typename std::enable_if<
			integer::is_interop_type<typename std::decay<T>::type>::value ||
			std::is_same<real,typename std::decay<T>::type>::value ||
			std::is_same<rational,typename std::decay<T>::type>::value ||
			std::is_same<integer,typename std::decay<T>::type>::value,real &>::type operator-=(T &&x)
		{
			in_place_sub(std::forward<T>(x));
			return *this;
		}
		/// Generic in-place subtraction with piranha::real.
		/**
		 * Subtract a piranha::real in-place. This template operator is activated only if \p T is an \ref interop "interoperable type", piranha::integer
		 * or piranha::rational, and \p R is piranha::real.
		 * This method will first compute <tt>x - r</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] r second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception resulting from casting piranha::real to \p T.
		 */
		template <typename T, typename R>
		friend typename std::enable_if<(integer::is_interop_type<T>::value || std::is_same<T,integer>::value ||
			std::is_same<T,rational>::value) &&
			std::is_same<typename std::decay<R>::type,real>::value,T &>::type
			operator-=(T &x, R &&r)
		{
			x = static_cast<T>(x - std::forward<R>(r));
			return x;
		}
		/// Generic binary subtraction involving piranha::real.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::real and \p U is an \ref interop "interoperable type" or piranha::integer or piranha::rational,
		 * - \p U is piranha::real and \p T is an \ref interop "interoperable type" or piranha::integer or piranha::rational,
		 * - both \p T and \p U are piranha::real.
		 * 
		 * The return type is always real.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x - y</tt>.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,real>::type
			operator-(T &&x, U &&y)
		{
			return binary_sub(std::forward<T>(x),std::forward<U>(y));
		}
		/// Negated copy.
		/**
		 * @return copy of \p -this.
		 */
		real operator-() const
		{
			real retval(*this);
			retval.negate();
			return retval;
		}
		/// Prefix decrement.
		/**
		 * Decrement \p this by one and return.
		 * 
		 * @return reference to \p this.
		 */
		real &operator--()
		{
			return operator-=(1);
		}
		/// Suffix decrement.
		/**
		 * Decrement \p this by one and return a copy of \p this as it was before the decrement.
		 * 
		 * @return copy of \p this before the decrement.
		 */
		real operator--(int)
		{
			const real retval(*this);
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
		typename std::enable_if<
			integer::is_interop_type<typename std::decay<T>::type>::value ||
			std::is_same<real,typename std::decay<T>::type>::value ||
			std::is_same<rational,typename std::decay<T>::type>::value ||
			std::is_same<integer,typename std::decay<T>::type>::value,real &>::type operator*=(T &&x)
		{
			in_place_mul(std::forward<T>(x));
			return *this;
		}
		/// Generic in-place multiplication with piranha::real.
		/**
		 * Multiply by a piranha::real in-place. This template operator is activated only if \p T is an \ref interop "interoperable type", piranha::integer
		 * or piranha::rational, and \p R is piranha::real.
		 * This method will first compute <tt>x * r</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] r second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception resulting from casting piranha::real to \p T.
		 */
		template <typename T, typename R>
		friend typename std::enable_if<(integer::is_interop_type<T>::value || std::is_same<T,integer>::value ||
			std::is_same<T,rational>::value) &&
			std::is_same<typename std::decay<R>::type,real>::value,T &>::type
			operator*=(T &x, R &&r)
		{
			x = static_cast<T>(x * std::forward<R>(r));
			return x;
		}
		/// Generic binary multiplication involving piranha::real.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::real and \p U is an \ref interop "interoperable type" or piranha::integer or piranha::rational,
		 * - \p U is piranha::real and \p T is an \ref interop "interoperable type" or piranha::integer or piranha::rational,
		 * - both \p T and \p U are piranha::real.
		 * 
		 * The return type is always real.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x * y</tt>.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,real>::type
			operator*(T &&x, U &&y)
		{
			return binary_mul(std::forward<T>(x),std::forward<U>(y));
		}
		/// In-place division.
		/**
		 * The same rules described in operator+=() apply.
		 * 
		 * @param[in] x argument for the division.
		 * 
		 * @return reference to \p this.
		 */
		template <typename T>
		typename std::enable_if<
			integer::is_interop_type<typename std::decay<T>::type>::value ||
			std::is_same<real,typename std::decay<T>::type>::value ||
			std::is_same<rational,typename std::decay<T>::type>::value ||
			std::is_same<integer,typename std::decay<T>::type>::value,real &>::type operator/=(T &&x)
		{
			in_place_div(std::forward<T>(x));
			return *this;
		}
		/// Generic in-place division with piranha::real.
		/**
		 * Divide by a piranha::real in-place. This template operator is activated only if \p T is an \ref interop "interoperable type", piranha::integer
		 * or piranha::rational, and \p R is piranha::real.
		 * This method will first compute <tt>x / r</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] r second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception resulting from casting piranha::real to \p T.
		 */
		template <typename T, typename R>
		friend typename std::enable_if<(integer::is_interop_type<T>::value || std::is_same<T,integer>::value ||
			std::is_same<T,rational>::value) &&
			std::is_same<typename std::decay<R>::type,real>::value,T &>::type
			operator/=(T &x, R &&r)
		{
			x = static_cast<T>(x / std::forward<R>(r));
			return x;
		}
		/// Generic binary division involving piranha::real.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::real and \p U is an \ref interop "interoperable type" or piranha::integer or piranha::rational,
		 * - \p U is piranha::real and \p T is an \ref interop "interoperable type" or piranha::integer or piranha::rational,
		 * - both \p T and \p U are piranha::real.
		 * 
		 * The return type is always real.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x / y</tt>.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,real>::type
			operator/(T &&x, U &&y)
		{
			return binary_div(std::forward<T>(x),std::forward<U>(y));
		}
		/// Combined multiply-add.
		/**
		 * Sets \p this to <tt>this + (r1 * r2)</tt>. If the precision of \p this is less than the maximum precision \p max_prec of the two operands
		 * \p r1 and \p r2, the precision of \p this will be set to \p max_prec before performing the operation.
		 * 
		 * @param[in] r1 first argument.
		 * @param[in] r2 second argument.
		 * 
		 * @return reference to \p this.
		 */
		real &multiply_accumulate(const real &r1, const real &r2)
		{
			const auto prec1 = std::max< ::mpfr_prec_t>(r1.get_prec(),r2.get_prec());
			if (prec1 > get_prec()) {
				*this = real{*this,prec1};
			}
			// So the story here is that mpfr_fma has been reported to be slower than the two separate
			// operations. Benchmarks on fateman1 indicate this is indeed the case (3.6 vs 2.7 secs
			// on 4 threads). Hopefully it will be fixed in the future, for now adopt the workaround.
			// http://www.loria.fr/~zimmerma/mpfr-mpc-2014.html
			//::mpfr_fma(m_value,r1.m_value,r2.m_value,m_value,default_rnd);
			// NOTE: the tmp var needs to be thread local.
			static thread_local real tmp;
			// NOTE: set the same precision as this, which is now the max precision of the 3 operands.
			// If we do not do this, then tmp has an undeterminate precision. Use the raw MPFR function
			// in order to avoid the checks in get_prec(), as we know the precision has a sane value.
			::mpfr_set_prec(tmp.m_value,mpfr_get_prec(m_value));
			::mpfr_mul(tmp.m_value,r1.m_value,r2.m_value,MPFR_RNDN);
			::mpfr_add(m_value,m_value,tmp.m_value,MPFR_RNDN);
			return *this;
		}
		/// Generic equality operator involving piranha::real.
		/**
		 * This template operator is activated if either:
		 * 
		 * - \p T is piranha::real and \p U is an \ref interop "interoperable type" or piranha::integer or piranha::rational,
		 * - \p U is piranha::real and \p T is an \ref interop "interoperable type" or piranha::integer or piranha::rational,
		 * - both \p T and \p U are piranha::real.
		 * 
		 * Note that in all comparison operators, apart from operator!=(), if any operand is NaN \p false will be returned.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x == y</tt>, \p false otherwise.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,bool>::type operator==(const T &x, const U &y)
		{
			return binary_equality(x,y);
		}
		/// Generic inequality operator involving piranha::real.
		/**
		 * The implementation is equivalent to the generic equality operator.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x != y</tt>, \p false otherwise.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,bool>::type operator!=(const T &x, const U &y)
		{
			return !binary_equality(x,y);
		}
		/// Generic less-than operator involving piranha::real.
		/**
		 * The implementation is equivalent to the generic equality operator.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x < y</tt>, \p false otherwise.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,bool>::type operator<(const T &x, const U &y)
		{
			if (is_nan_comparison(x,y)) {
				return false;
			}
			return binary_less_than(x,y);
		}
		/// Generic less-than or equal operator involving piranha::real.
		/**
		 * The implementation is equivalent to the generic equality operator.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x <= y</tt>, \p false otherwise.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,bool>::type operator<=(const T &x, const U &y)
		{
			if (is_nan_comparison(x,y)) {
				return false;
			}
			return binary_leq(x,y);
		}
		/// Generic greater-than operator involving piranha::real.
		/**
		 * The implementation is equivalent to the generic equality operator.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x > y</tt>, \p false otherwise.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,bool>::type operator>(const T &x, const U &y)
		{
			if (is_nan_comparison(x,y)) {
				return false;
			}
			return (y < x);
		}
		/// Generic greater-than or equal operator involving piranha::real.
		/**
		 * The implementation is equivalent to the generic equality operator.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x >= y</tt>, \p false otherwise.
		 */
		template <typename T, typename U>
		friend typename std::enable_if<are_binary_op_types<T,U>::value,bool>::type operator>=(const T &x, const U &y)
		{
			if (is_nan_comparison(x,y)) {
				return false;
			}
			return (y <= x);
		}
		/// Exponentiation.
		/**
		 * Return <tt>this ** exp</tt>. This template method is activated only if \p T is real, an
		 * \ref interop "interoperable type" or piranha::integer. Special values are handled as described in the
		 * MPFR documentation.
		 * 
		 * The precision of the result is equal to the precision of \p this. In case \p T is a floating-point type, \p x will
		 * be converted to a real with the same precision as \p this before the exponentiation takes place.
		 * 
		 * @param[in] exp exponent.
		 * 
		 * @return <tt>this ** exp</tt>.
		 * 
		 * @see http://www.mpfr.org/mpfr-current/mpfr.html#Basic-Arithmetic-Functions
		 */
		template <typename T>
		typename std::enable_if<std::is_same<real,T>::value ||
			integer::is_interop_type<T>::value ||
			std::is_same<T,integer>::value,real>::type pow(const T &exp) const
		{
			return pow_impl(exp);
		}
		/// Absolute value.
		/**
		 * @return absolute value of \p this.
		 */
		real abs() const
		{
			real retval(*this);
			::mpfr_abs(retval.m_value,retval.m_value,default_rnd);
			return retval;
		}
		/// Sine.
		/**
		 * @return sine of \p this, computed with the precision of \p this.
		 */
		real sin() const
		{
			real retval(0,get_prec());
			::mpfr_sin(retval.m_value,m_value,default_rnd);
			return retval;
		}
		/// Cosine.
		/**
		 * @return cosine of \p this, computed with the precision of \p this.
		 */
		real cos() const
		{
			real retval(0,get_prec());
			::mpfr_cos(retval.m_value,m_value,default_rnd);
			return retval;
		}
		/// Pi constant.
		/**
		 * @return pi constant calculated to the current precision of \p this.
		 */
		real pi() const
		{
			real retval(0,get_prec());
			::mpfr_const_pi(retval.m_value,default_rnd);
			return retval;
		}
		/// Overload output stream operator for piranha::real.
		/**
		 * The output format for finite numbers is normalised scientific notation, where the exponent is signalled by the letter 'e'
		 * and suppressed if null.
		 * 
		 * For non-finite numbers, the string representation is one of "nan", "inf" or "-inf".
		 * 
		 * @param[in] os output stream.
		 * @param[in] r piranha::real to be directed to stream.
		 * 
		 * @return reference to \p os.
		 * 
		 * @throws std::invalid_argument if the conversion to string via the MPFR API fails.
		 * @throws std::overflow_error if the exponent is smaller than an implementation-defined minimum.
		 * @throws unspecified any exception thrown by memory allocation errors in standard containers.
		 */
		friend std::ostream &operator<<(std::ostream &os, const real &r)
		{
			if (r.is_nan()) {
				os << "nan";
				return os;
			}
			if (r.is_inf()) {
				if (r.sign() > 0) {
					os << "inf";
				} else {
					os << "-inf";
				}
				return os;
			}
			::mpfr_exp_t exp(0);
			real::mpfr_str_manager m(::mpfr_get_str(nullptr,&exp,10,0,r.m_value,default_rnd));
			if (!m.m_str) {
				piranha_throw(std::invalid_argument,"unable to convert real to string");
			}
			// Copy into C++ string.
			std::string cpp_str(m.m_str);
			// Insert the radix point.
			auto it = std::find_if(cpp_str.begin(),cpp_str.end(),[](char c) {return std::isdigit(c);});
			if (it != cpp_str.end()) {
				++it;
				cpp_str.insert(it,'.');
				if (exp == boost::integer_traits< ::mpfr_exp_t>::const_min) {
					piranha_throw(std::overflow_error,"overflow in conversion of real to string");
				}
				--exp;
				if (exp != ::mpfr_exp_t(0) && r.sign() != 0) {
					cpp_str.append(std::string("e") + boost::lexical_cast<std::string>(exp));
				}
			}
			os << cpp_str;
			return os;
		}
		/// Overload input stream operator for piranha::real.
		/**
		 * Equivalent to extracting a line from the stream and then assigning it to \p r.
		 * 
		 * @param[in] is input stream.
		 * @param[in,out] r real to which the contents of the stream will be assigned.
		 * 
		 * @return reference to \p is.
		 * 
		 * @throws unspecified any exception thrown by the assignment operator from string of piranha::real.
		 */
		friend std::istream &operator>>(std::istream &is, real &r)
		{
			std::string tmp_str;
			std::getline(is,tmp_str);
			r = tmp_str;
			return is;
		}
	private:
		::mpfr_t m_value;
};

namespace math
{

/// Specialisation of the piranha::math::negate() functor for piranha::real.
template <typename T>
struct negate_impl<T,typename std::enable_if<std::is_same<T,real>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in,out] x piranha::real to be negated.
	 */
	void operator()(real &x) const
	{
		x.negate();
	}
};

/// Specialisation of the piranha::math::is_zero() functor for piranha::real.
template <typename T>
struct is_zero_impl<T,typename std::enable_if<std::is_same<T,real>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] r piranha::real to be tested.
	 * 
	 * @return \p true if \p r is zero, \p false otherwise.
	 */
	bool operator()(const T &r) const
	{
		return r.is_zero();
	}
};

/// Specialisation of the piranha::math::pow() functor for piranha::real.
/**
 * This specialisation is activated when \p T is piranha::real.
 * The result will be computed via piranha::real::pow().
 */
template <typename T, typename U>
struct pow_impl<T,U,typename std::enable_if<std::is_same<T,real>::value>::type>
{
	/// Call operator.
	/**
	 * The exponentiation will be computed via piranha::real::pow().
	 * 
	 * @param[in] r base.
	 * @param[in] x exponent.
	 * 
	 * @return \p r to the power of \p x.
	 */
	template <typename T2, typename U2>
	auto operator()(const T2 &r, const U2 &x) const -> decltype(r.pow(x))
	{
		return r.pow(x);
	}
};

/// Specialisation of the piranha::math::sin() functor for piranha::real.
template <typename T>
struct sin_impl<T,typename std::enable_if<std::is_same<T,real>::value>::type>
{
	/// Call operator.
	/**
	 * The operation will return the output of piranha::real::sin().
	 * 
	 * @param[in] r argument.
	 * 
	 * @return sine of \p r.
	 */
	real operator()(const T &r) const
	{
		return r.sin();
	}
};

/// Specialisation of the piranha::math::cos() functor for piranha::real.
template <typename T>
struct cos_impl<T,typename std::enable_if<std::is_same<T,real>::value>::type>
{
	/// Call operator.
	/**
	 * The operation will return the output of piranha::real::cos().
	 * 
	 * @param[in] r argument.
	 * 
	 * @return cosine of \p r.
	 */
	real operator()(const T &r) const
	{
		return r.cos();
	}
};

/// Specialisation of the piranha::math::abs() functor for piranha::real.
template <typename T>
struct abs_impl<T,typename std::enable_if<std::is_same<T,real>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] x input parameter.
	 * 
	 * @return absolute value of \p x.
	 */
	T operator()(const T &x) const
	{
		return x.abs();
	}
};

/// Specialisation of the piranha::math::partial() functor for piranha::real.
template <typename T>
struct partial_impl<T,typename std::enable_if<std::is_same<T,real>::value>::type>
{
	/// Call operator.
	/**
	 * @return an instance of piranha::real constructed from zero.
	 */
	real operator()(const real &, const std::string &) const
	{
		return real(0);
	}
};

/// Specialisation of the piranha::math::evaluate() functor for piranha::real.
template <typename T>
struct evaluate_impl<T,typename std::enable_if<std::is_same<T,real>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] x evaluation argument.
	 * 
	 * @return copy of \p x.
	 */
	template <typename U>
	T operator()(const T &x, const std::unordered_map<std::string,U> &) const
	{
		return x;
	}
};

/// Specialisation of the piranha::math::subs() functor for piranha::real.
template <typename T>
struct subs_impl<T,typename std::enable_if<std::is_same<T,real>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] x substitution argument.
	 * 
	 * @return copy of \p x.
	 */
	template <typename U>
	T operator()(const T &x, const std::string &, const U &) const
	{
		return x;
	}
};

/// Specialisation of the piranha::math::integral_cast functor for piranha::real.
template <typename T>
struct integral_cast_impl<T,typename std::enable_if<std::is_same<T,real>::value>::type>
{
	/// Call operator.
	/**
	 * The call will be successful if \p x is finite and if it does not change after truncation.
	 * 
	 * @param[in] x cast argument.
	 * 
	 * @return result of the cast operation.
	 *
	 * @throws std::invalid_argument if the conversion is not successful.
	 */
	integer operator()(const T &x) const
	{
		if (x.is_nan() || x.is_inf()) {
			piranha_throw(std::invalid_argument,"invalid real value");
		}
		auto x_copy(x);
		x_copy.truncate();
		if (x == x_copy) {
			return integer(x);
		}
		piranha_throw(std::invalid_argument,"invalid real value");
	}
};

/// Specialisation of the piranha::math::ipow_subs() functor for piranha::real.
/**
 * This specialisation is activated when \p T is piranha::real.
 * The result will be the input value unchanged.
 */
template <typename T>
struct ipow_subs_impl<T,typename std::enable_if<std::is_same<T,real>::value>::type>
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

/// Specialisation of the piranha::math::binomial() functor for piranha::real.
/**
 * This specialisation is enabled if \p T is piranha::real and \p U is an integral type or piranha::integer.
 */
template <typename T, typename U>
struct binomial_impl<T,U,typename std::enable_if<
	std::is_same<real,T>::value &&
	(std::is_integral<U>::value || std::is_same<integer,U>::value)
	>::type>
{
	/// Call operator.
	/**
	 * @param[in] x top number.
	 * @param[in] k bottom number.
	 * 
	 * @return \p x choose \p k.
	 * 
	 * @throws std::invalid_argument if \p k is negative.
	 * @throws unspecified any exception resulting from arithmetic operations involving piranha::real.
	 */
	real operator()(const real &x, const U &k) const
	{
		return detail::generic_binomial(x,k);
	}
};

/// Specialisation of the implementation of piranha::math::multiply_accumulate() for piranha::real.
template <typename T>
struct multiply_accumulate_impl<T,T,T,typename std::enable_if<std::is_same<T,real>::value>::type>
{
	/// Call operator.
 	/**
	 * This implementation will use piranha::real::multiply_accumulate().
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

}

inline real::~real()
{
	PIRANHA_TT_CHECK(is_cf,real);
	static_assert(default_prec >= MPFR_PREC_MIN && default_prec <= MPFR_PREC_MAX,"Invalid value for default precision.");
	if (m_value->_mpfr_d) {
		::mpfr_clear(m_value);
	} else {
		piranha_assert(!m_value->_mpfr_prec);
		piranha_assert(!m_value->_mpfr_sign);
		piranha_assert(!m_value->_mpfr_exp);
	}
}

}

namespace std
{

/// Specialization of \p std::swap for piranha::real.
/**
 * @param[in] r1 first argument.
 * @param[in] r2 second argument.
 * 
 * @see piranha::real::swap()
 */
template <>
inline void swap(piranha::real &r1, piranha::real &r2) noexcept
{
	r1.swap(r2);
}

}

#endif
