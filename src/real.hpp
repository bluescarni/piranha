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
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "config.hpp"
#include "detail/is_digit.hpp"
#include "detail/mpfr.hpp"
#include "detail/real_fwd.hpp"
#include "exceptions.hpp"
#include "math.hpp"
#include "mp_integer.hpp"
#include "mp_rational.hpp"
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

// Types interoperable with real.
template <typename T>
struct is_real_interoperable_type
{
	static const bool value = detail::is_mp_integer_interoperable_type<T>::value ||
		detail::is_mp_integer<T>::value || detail::is_mp_rational<T>::value;
};

}

/// Arbitrary precision floating-point class.
/**
 * This class represents floating-point ("real") numbers of arbitrary size (i.e., the size is limited only by the available memory).
 * The implementation consists of a C++ wrapper around the \p mpfr_t type from the multiprecision MPFR library. Real numbers
 * are represented in binary format and they consist of an arbitrary-size significand coupled to a fixed-size exponent.
 * 
 * Unless noted otherwise, this implementation always uses the \p MPFR_RNDN (round to nearest) rounding mode for all operations.
 * 
 * \section interop Interoperability with other types
 * 
 * This class interoperates with the same types as piranha::mp_integer and piranha::mp_rational,
 * plus piranha::mp_integer and piranha::mp_rational themselves.
 * The same caveats with respect to interoperability with floating-point types mentioned in the documentation
 * of piranha::mp_integer apply.
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
// NOTES:
// - if we overhaul the tests, put random precision values as well.
// - maybe we should have a setter as well for the global default precision. It would need to be an atomic
//   variable, and we need perf measures to understand the performance impact of this.
// - For series evaluation, we need to be careful performance-wise with the possible conversions that might go
//   on when mixing real with other types. E.g., pow(real,int) when evaluating polynomials. We need to make sure
//   the conversions are as fast as possible.
class real: public detail::real_base<>
{
		// Shortcut for interop type detector.
		template <typename T>
		using is_interoperable_type = detail::is_real_interoperable_type<T>;
		// Enabler for generic ctor.
		template <typename T>
		using generic_ctor_enabler = typename std::enable_if<is_interoperable_type<T>::value,int>::type;
		// Enabler for conversion operator.
		template <typename T>
		using cast_enabler = generic_ctor_enabler<T>;
		// Enabler for in-place arithmetic operations with interop on the left.
		template <typename T>
		using generic_in_place_enabler = typename std::enable_if<is_interoperable_type<T>::value && !std::is_const<T>::value,int>::type;
		// Precision check.
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
		// The idea here is that we use the largest integral and fp types supported by the MPFR api for construction,
		// and down-cast as needed.
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		void construct_from_generic(const T &x)
		{
			::mpfr_set_ld(m_value,static_cast<long double>(x),default_rnd);
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value,int>::type = 0>
		void construct_from_generic(const T &si)
		{
			::mpfr_set_sj(m_value,static_cast<std::intmax_t>(si),default_rnd);
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value,int>::type = 0>
		void construct_from_generic(const T &ui)
		{
			::mpfr_set_uj(m_value,static_cast<std::uintmax_t>(ui),default_rnd);
		}
		template <typename T, typename std::enable_if<detail::is_mp_integer<T>::value,int>::type = 0>
		void construct_from_generic(const T &n)
		{
			auto v = n.get_mpz_view();
			::mpfr_set_z(m_value,v,default_rnd);
		}
		template <typename T, typename std::enable_if<detail::is_mp_rational<T>::value,int>::type = 0>
		void construct_from_generic(const T &q)
		{
			auto v = q.get_mpq_view();
			::mpfr_set_q(m_value,v,default_rnd);
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
		typename std::enable_if<detail::is_mp_integer<T>::value,T>::type convert_to_impl() const
		{
			if (is_nan() || is_inf()) {
				piranha_throw(std::overflow_error,"cannot convert non-finite real to an integral value");
			}
			T retval;
			retval.promote();
			// Explicitly request rounding to zero in this case.
			::mpfr_get_z(&retval.m_int.g_dy(),m_value,MPFR_RNDZ);
			// NOTE: demote candidate.
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
			if (std::is_same<T,long double>::value) {
				return static_cast<T>(::mpfr_get_ld(m_value,default_rnd));
			}
			if (std::is_same<T,double>::value) {
				return static_cast<T>(::mpfr_get_d(m_value,default_rnd));
			}
			return static_cast<T>(::mpfr_get_flt(m_value,default_rnd));
		}
		// Smart pointer to handle the string output from mpfr.
		typedef std::unique_ptr<char,void (*)(char *)> smart_mpfr_str;
		template <typename T>
		typename std::enable_if<detail::is_mp_rational<T>::value,T>::type convert_to_impl() const
		{
			if (is_nan()) {
				piranha_throw(std::overflow_error,"cannot convert NaN to rational");
			}
			if (is_inf()) {
				piranha_throw(std::overflow_error,"cannot convert infinity to rational");
			}
			if (sign() == 0) {
				return T{};
			}
			// Get string representation.
			::mpfr_exp_t exp(0);
			char *cptr = ::mpfr_get_str(nullptr,&exp,10,0,m_value,default_rnd);
			if (unlikely(!cptr)) {
				piranha_throw(std::overflow_error,"error in conversion of real to rational: the call to the MPFR function failed");
			}
			smart_mpfr_str str_ptr(cptr,::mpfr_free_str);
			// Transform into fraction.
			std::size_t digits = 0u;
			for (; *cptr != '\0'; ++cptr) {
				if (detail::is_digit(*cptr)) {
					++digits;
				}
			}
			if (!digits) {
				piranha_throw(std::overflow_error,"error in conversion of real to rational: invalid number of digits");
			}
			// NOTE: here the only exception that can be thrown is when raising to a power
			// that cannot be represented by unsigned long.
			try {
				T retval(str_ptr.get());
				// NOTE: possible optimizations here include going through direct GMP routines.
				retval *= T(1,10).pow(digits);
				retval *= T(10).pow(exp);
				return retval;
			} catch (...) {
				piranha_throw(std::overflow_error,"error in conversion of real to rational: exponent is too large");
			}
		}
		// In-place addition.
		// NOTE: all sorts of optimisations, here and in binary add, are possible (e.g., steal from rvalue ref, 
		// avoid setting precision twice in binary operators, etc.). For the moment we keep it basic.
		real &in_place_add(const real &r)
		{
			if (r.get_prec() > get_prec()) {
				// Re-init this with the prec of r.
				*this = real{*this,r.get_prec()};
			}
			::mpfr_add(m_value,m_value,r.m_value,default_rnd);
			return *this;
		}
		template <typename T, typename std::enable_if<detail::is_mp_rational<T>::value,int>::type = 0>
		real &in_place_add(const T &q)
		{
			auto v = q.get_mpq_view();
			::mpfr_add_q(m_value,m_value,v,default_rnd);
			return *this;
		}
		template <typename T, typename std::enable_if<detail::is_mp_integer<T>::value,int>::type = 0>
		real &in_place_add(const T &n)
		{
			auto v = n.get_mpz_view();
			::mpfr_add_z(m_value,m_value,v,default_rnd);
			return *this;
		}
		// NOTE: possible optimisations here.
		template <typename T, typename std::enable_if<std::is_integral<T>::value,int>::type = 0>
		real &in_place_add(const T &n)
		{
			return in_place_add(integer(n));
		}
		// NOTE: possible optimisations here as well.
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		real &in_place_add(const T &x)
		{
			// Construct real with the same precision as this, then add.
			return in_place_add(real{x,get_prec()});
		}
		// Binary add.
		static real binary_add(const real &a, const real &b)
		{
			real retval{a};
			retval += b;
			return retval;
		}
		// Single implementation for all interoperable types.
		template <typename T, typename std::enable_if<is_interoperable_type<T>::value,int>::type = 0>
		static real binary_add(const real &a, const T &b)
		{
			real retval{a};
			retval += b;
			return retval;
		}
		template <typename T, typename std::enable_if<is_interoperable_type<T>::value,int>::type = 0>
		static real binary_add(const T &a, const real &b)
		{
			return binary_add(b,a);
		}
		// In-place subtraction.
		real &in_place_sub(const real &r)
		{
			if (r.get_prec() > get_prec()) {
				*this = real{*this,r.get_prec()};
			}
			::mpfr_sub(m_value,m_value,r.m_value,default_rnd);
			return *this;
		}
		template <typename T, typename std::enable_if<detail::is_mp_rational<T>::value,int>::type = 0>
		real &in_place_sub(const T &q)
		{
			auto v = q.get_mpq_view();
			::mpfr_sub_q(m_value,m_value,v,default_rnd);
			return *this;
		}
		template <typename T, typename std::enable_if<detail::is_mp_integer<T>::value,int>::type = 0>
		real &in_place_sub(const T &n)
		{
			auto v = n.get_mpz_view();
			::mpfr_sub_z(m_value,m_value,v,default_rnd);
			return *this;
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value,int>::type = 0>
		real &in_place_sub(const T &n)
		{
			return in_place_sub(integer(n));
		}
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		real &in_place_sub(const T &x)
		{
			return in_place_sub(real{x,get_prec()});
		}
		// Binary sub.
		static real binary_sub(const real &a, const real &b)
		{
			real retval{a};
			retval -= b;
			return retval;
		}
		template <typename T, typename std::enable_if<is_interoperable_type<T>::value,int>::type = 0>
		static real binary_sub(const real &a, const T &b)
		{
			real retval{a};
			retval -= b;
			return retval;
		}
		template <typename T, typename std::enable_if<is_interoperable_type<T>::value,int>::type = 0>
		static real binary_sub(const T &a, const real &b)
		{
			auto retval = binary_sub(b,a);
			retval.negate();
			return retval;
		}
		// In-place multiplication.
		real &in_place_mul(const real &r)
		{
			if (r.get_prec() > get_prec()) {
				*this = real{*this,r.get_prec()};
			}
			::mpfr_mul(m_value,m_value,r.m_value,default_rnd);
			return *this;
		}
		template <typename T, typename std::enable_if<detail::is_mp_rational<T>::value,int>::type = 0>
		real &in_place_mul(const T &q)
		{
			auto v = q.get_mpq_view();
			::mpfr_mul_q(m_value,m_value,v,default_rnd);
			return *this;
		}
		template <typename T, typename std::enable_if<detail::is_mp_integer<T>::value,int>::type = 0>
		real &in_place_mul(const T &n)
		{
			auto v = n.get_mpz_view();
			::mpfr_mul_z(m_value,m_value,v,default_rnd);
			return *this;
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value,int>::type = 0>
		real &in_place_mul(const T &n)
		{
			return in_place_mul(integer(n));
		}
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		real &in_place_mul(const T &x)
		{
			return in_place_mul(real{x,get_prec()});
		}
		// Binary mul.
		static real binary_mul(const real &a, const real &b)
		{
			real retval{a};
			retval *= b;
			return retval;
		}
		template <typename T, typename std::enable_if<is_interoperable_type<T>::value,int>::type = 0>
		static real binary_mul(const real &a, const T &b)
		{
			real retval{a};
			retval *= b;
			return retval;
		}
		template <typename T, typename std::enable_if<is_interoperable_type<T>::value,int>::type = 0>
		static real binary_mul(const T &a, const real &b)
		{
			return binary_mul(b,a);
		}
		// In-place division.
		real &in_place_div(const real &r)
		{
			if (r.get_prec() > get_prec()) {
				*this = real{*this,r.get_prec()};
			}
			::mpfr_div(m_value,m_value,r.m_value,default_rnd);
			return *this;
		}
		template <typename T, typename std::enable_if<detail::is_mp_rational<T>::value,int>::type = 0>
		real &in_place_div(const T &q)
		{
			auto v = q.get_mpq_view();
			::mpfr_div_q(m_value,m_value,v,default_rnd);
			return *this;
		}
		template <typename T, typename std::enable_if<detail::is_mp_integer<T>::value,int>::type = 0>
		real &in_place_div(const T &n)
		{
			auto v = n.get_mpz_view();
			::mpfr_div_z(m_value,m_value,v,default_rnd);
			return *this;
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value,int>::type = 0>
		real &in_place_div(const T &n)
		{
			return in_place_div(integer(n));
		}
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		real &in_place_div(const T &x)
		{
			return in_place_div(real{x,get_prec()});
		}
		// Binary div.
		static real binary_div(const real &a, const real &b)
		{
			real retval{a};
			retval /= b;
			return retval;
		}
		template <typename T, typename std::enable_if<is_interoperable_type<T>::value,int>::type = 0>
		static real binary_div(const real &a, const T &b)
		{
			real retval{a};
			retval /= b;
			return retval;
		}
		template <typename T, typename std::enable_if<is_interoperable_type<T>::value,int>::type = 0>
		static real binary_div(const T &a, const real &b)
		{
			// Create with same precision as b.
			real retval{a,b.get_prec()};
			retval /= b;
			return retval;
		}
		// Equality.
		static bool binary_equality(const real &r1, const real &r2)
		{
			return (::mpfr_equal_p(r1.m_value,r2.m_value) != 0);
		}
		template <typename T, typename std::enable_if<detail::is_mp_integer<T>::value,int>::type = 0>
		static bool binary_equality(const real &r, const T &n)
		{
			if (r.is_nan()) {
				return false;
			}
			auto v = n.get_mpz_view();
			return (::mpfr_cmp_z(r.m_value,v) == 0);
		}
		template <typename T, typename std::enable_if<detail::is_mp_rational<T>::value,int>::type = 0>
		static bool binary_equality(const real &r, const T &q)
		{
			if (r.is_nan()) {
				return false;
			}
			auto v = q.get_mpq_view();
			return (::mpfr_cmp_q(r.m_value,v) == 0);
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value,int>::type = 0>
		static bool binary_equality(const real &r, const T &n)
		{
			if (r.is_nan()) {
				return false;
			}
			return r == integer(n);
		}
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		static bool binary_equality(const real &r, const T &x)
		{
			if (r.is_nan() || std::isnan(x)) {
				return false;
			}
			return r == real{x,r.get_prec()};
		}
		// NOTE: this is the reverse of above.
		template <typename T, typename std::enable_if<is_interoperable_type<T>::value,int>::type = 0>
		static bool binary_equality(const T &x, const real &r)
		{
			return binary_equality(r,x);
		}
		// Binary less-than.
		static bool binary_less_than(const real &r1, const real &r2)
		{
			return (::mpfr_less_p(r1.m_value,r2.m_value) != 0);
		}
		template <typename T, typename std::enable_if<detail::is_mp_rational<T>::value,int>::type = 0>
		static bool binary_less_than(const real &r, const T &q)
		{
			auto v = q.get_mpq_view();
			return (::mpfr_cmp_q(r.m_value,v) < 0);
		}
		template <typename T, typename std::enable_if<detail::is_mp_integer<T>::value,int>::type = 0>
		static bool binary_less_than(const real &r, const T &n)
		{
			auto v = n.get_mpz_view();
			return (::mpfr_cmp_z(r.m_value,v) < 0);
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value,int>::type = 0>
		static bool binary_less_than(const real &r, const T &n)
		{
			return r < integer(n);
		}
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		static bool binary_less_than(const real &r, const T &x)
		{
			return r < real{x,r.get_prec()};
		}
		// Binary less-than or equal.
		static bool binary_leq(const real &r1, const real &r2)
		{
			return (::mpfr_lessequal_p(r1.m_value,r2.m_value) != 0);
		}
		template <typename T, typename std::enable_if<detail::is_mp_rational<T>::value,int>::type = 0>
		static bool binary_leq(const real &r, const T &q)
		{
			auto v = q.get_mpq_view();
			return (::mpfr_cmp_q(r.m_value,v) <= 0);
		}
		template <typename T, typename std::enable_if<detail::is_mp_integer<T>::value,int>::type = 0>
		static bool binary_leq(const real &r, const T &n)
		{
			auto v = n.get_mpz_view();
			return (::mpfr_cmp_z(r.m_value,v) <= 0);
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value,int>::type = 0>
		static bool binary_leq(const real &r, const T &n)
		{
			return r <= integer(n);
		}
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		static bool binary_leq(const real &r, const T &x)
		{
			return r <= real{x,r.get_prec()};
		}
		// Inverse forms of less-than and leq.
		template <typename T, typename std::enable_if<is_interoperable_type<T>::value,int>::type = 0>
		static bool binary_less_than(const T &x, const real &r)
		{
			return !binary_leq(r,x);
		}
		template <typename T, typename std::enable_if<is_interoperable_type<T>::value,int>::type = 0>
		static bool binary_leq(const T &x, const real &r)
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
			return std::isnan(x);
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
		 * \note
		 * This constructor is enabled only if \p T is an \ref interop "interoperable type".
		 * 
		 * @param[in] x object used to construct \p this.
		 * @param[in] prec desired significand precision.
		 * 
		 * @throws std::invalid_argument if the requested significand precision
		 * is not within the range allowed by the MPFR library.
		 */
		template <typename T, typename = generic_ctor_enabler<T>>
		explicit real(const T &x, const ::mpfr_prec_t &prec = default_prec)
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
		 * \note
		 * This assignment operator is enabled only if \p T is an \ref interop "interoperable type".
		 * 
		 * The precision of \p this
		 * will not be changed by the assignment operation, unless \p this was the target of a move operation that
		 * left it in an uninitialised state.
		 * In that case, \p this will be re-initialised with the default precision.
		 * 
		 * @param[in] x object that will be assigned to \p this.
		 * 
		 * @return reference to \p this.
		 */
		template <typename T, typename = generic_ctor_enabler<T>>
		real &operator=(const T &x)
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
		 * \note
		 * This operator is enabled only if \p T is an \ref interop "interoperable type".
		 * 
		 * Extract an instance of type \p T from \p this.
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
		 * Conversion of finite values to piranha::mp_rational will be exact. Conversion of non-finite values will result in runtime
		 * errors.
		 * 
		 * @return result of the conversion to target type T.
		 * 
		 * @throws std::overflow_error if the conversion fails in one of the ways described above.
		 */
		template <typename T, typename = cast_enabler<T>>
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
			::mpfr_swap(m_value,other.m_value);
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
		 * Set \p this to the next representable integer towards zero. If \p this is infinity or NaN, there will be no effect.
		 */
		void truncate()
		{
			if (is_inf() || is_nan()) {
				return;
			}
			::mpfr_trunc(m_value,m_value);
		}
		/// Truncated copy.
		/**
		 * @return a truncated copy of \p this.
		 */
		real truncated() const
		{
			real retval{*this};
			retval.truncate();
			return retval;
		}
		/// In-place addition.
		/**
		 * \note
		 * This operator is enabled only if \p T is an \ref interop "interoperable type" or piranha::real.
		 * 
		 * Add \p x to the current value of the real object.
		 * 
		 * If the precision \p prec of \p x is greater than the precision of \p this,
		 * the precision of \p this is changed to \p prec before the operation takes place.
		 * 
		 * @param[in] x argument for the addition.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the contructor of piranha::mp_integer, if invoked.
		 */
		template <typename T>
		auto operator+=(const T &x) -> decltype(this->in_place_add(x))
		{
			return in_place_add(x);
		}
		/// Generic in-place addition with piranha::real.
		/**
		 * \note
		 * This operator is enabled only if \p T is a non-const \ref interop "interoperable type".
		 * 
		 * Add a piranha::real in-place.
		 * This method will first compute <tt>r + x</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] r second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception resulting from the binary operator or by casting piranha::real to \p T.
		 */
		template <typename T, generic_in_place_enabler<T> = 0>
		friend T &operator+=(T &x, const real &r)
		{
			// NOTE: for the supported types, move assignment can never throw.
			return x = static_cast<T>(r + x);
		}
		/// Generic binary addition involving piranha::real.
		/**
		 * \note
		 * This template operator is enabled only if either:
		 * - \p T is piranha::real and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::real and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::real.
		 * 
		 * The return type is always piranha::real.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x + y</tt>.
		 * 
		 * @throws unspecified any exception thrown by the corresponding in-place operator.
		 */
		template <typename T, typename U>
		friend auto operator+(const T &x, const U &y) -> decltype(real::binary_add(x,y))
		{
			return binary_add(x,y);
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
		 * \note
		 * This operator is enabled only if \p T is an \ref interop "interoperable type" or piranha::real.
		 * 
		 * Subtract \p x from the current value of the real object.
		 * 
		 * If the precision \p prec of \p x is greater than the precision of \p this,
		 * the precision of \p this is changed to \p prec before the operation takes place.
		 * 
		 * @param[in] x argument for the subtraction.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the contructor of piranha::mp_integer, if invoked.
		 */
		template <typename T>
		auto operator-=(const T &x) -> decltype(this->in_place_sub(x))
		{
			return in_place_sub(x);
		}
		/// Generic in-place subtraction with piranha::real.
		/**
		 * \note
		 * This operator is enabled only if \p T is a non-const \ref interop "interoperable type".
		 * 
		 * Subtract a piranha::real in-place.
		 * This method will first compute <tt>x - r</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] r second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception resulting from the binary operator or by casting piranha::real to \p T.
		 */
		template <typename T, generic_in_place_enabler<T> = 0>
		friend T &operator-=(T &x, const real &r)
		{
			return x = static_cast<T>(x - r);
		}
		/// Generic binary subtraction involving piranha::real.
		/**
		 * \note
		 * This template operator is enabled only if either:
		 * - \p T is piranha::real and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::real and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::real.
		 * 
		 * The return type is always piranha::real.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x - y</tt>.
		 * 
		 * @throws unspecified any exception thrown by the corresponding in-place operator.
		 */
		template <typename T, typename U>
		friend auto operator-(const T &x, const U &y) -> decltype(real::binary_sub(x,y))
		{
			return binary_sub(x,y);
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
		 * \note
		 * This operator is enabled only if \p T is an \ref interop "interoperable type" or piranha::real.
		 * 
		 * Multiply by \p x the current value of the real object.
		 * 
		 * If the precision \p prec of \p x is greater than the precision of \p this,
		 * the precision of \p this is changed to \p prec before the operation takes place.
		 * 
		 * @param[in] x argument for the multiplication.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the contructor of piranha::mp_integer, if invoked.
		 */
		template <typename T>
		auto operator*=(const T &x) -> decltype(this->in_place_mul(x))
		{
			return in_place_mul(x);
		}
		/// Generic in-place multiplication by piranha::real.
		/**
		 * \note
		 * This operator is enabled only if \p T is a non-const \ref interop "interoperable type".
		 * 
		 * Multiply by a piranha::real in-place.
		 * This method will first compute <tt>x * r</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] r second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception resulting from the binary operator or by casting piranha::real to \p T.
		 */
		template <typename T, generic_in_place_enabler<T> = 0>
		friend T &operator*=(T &x, const real &r)
		{
			return x = static_cast<T>(x * r);
		}
		/// Generic binary multiplication involving piranha::real.
		/**
		 * \note
		 * This template operator is enabled only if either:
		 * - \p T is piranha::real and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::real and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::real.
		 * 
		 * The return type is always piranha::real.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x * y</tt>.
		 * 
		 * @throws unspecified any exception thrown by the corresponding in-place operator.
		 */
		template <typename T, typename U>
		friend auto operator*(const T &x, const U &y) -> decltype(real::binary_mul(x,y))
		{
			return binary_mul(x,y);
		}
		/// In-place division.
		/**
		 * \note
		 * This operator is enabled only if \p T is an \ref interop "interoperable type" or piranha::real.
		 * 
		 * Divide by \p x the current value of the real object.
		 * 
		 * If the precision \p prec of \p x is greater than the precision of \p this,
		 * the precision of \p this is changed to \p prec before the operation takes place.
		 * 
		 * @param[in] x argument for the division.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the contructor of piranha::mp_integer, if invoked.
		 */
		template <typename T>
		auto operator/=(const T &x) -> decltype(this->in_place_div(x))
		{
			return in_place_div(x);
		}
		/// Generic in-place division by piranha::real.
		/**
		 * \note
		 * This operator is enabled only if \p T is a non-const \ref interop "interoperable type".
		 * 
		 * Divide by a piranha::real in-place.
		 * This method will first compute <tt>x / r</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] r second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception resulting from the binary operator or by casting piranha::real to \p T.
		 */
		template <typename T, generic_in_place_enabler<T> = 0>
		friend T &operator/=(T &x, const real &r)
		{
			return x = static_cast<T>(x / r);
		}
		/// Generic binary division involving piranha::real.
		/**
		 * \note
		 * This template operator is enabled only if either:
		 * - \p T is piranha::real and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::real and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::real.
		 * 
		 * The return type is always piranha::real.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x / y</tt>.
		 * 
		 * @throws unspecified any exception thrown by the corresponding in-place operator.
		 */
		template <typename T, typename U>
		friend auto operator/(const T &x, const U &y) -> decltype(real::binary_div(x,y))
		{
			return binary_div(x,y);
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
		 * \note
		 * This template operator is enabled only if either:
		 * - \p T is piranha::real and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::real and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::real.
		 * 
		 * Note that in all comparison operators, apart from piranha::real::operator!=(), if any operand is NaN \p false will be returned.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x == y</tt>, \p false otherwise.
		 */
		template <typename T, typename U>
		friend auto operator==(const T &x, const U &y) -> decltype(real::binary_equality(x,y))
		{
			return binary_equality(x,y);
		}
		/// Generic inequality operator involving piranha::real.
		/**
		 * \note
		 * This template operator is enabled only if either:
		 * - \p T is piranha::real and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::real and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::real.
		 * 
		 * Note that in all comparison operators, apart from piranha::real::operator!=(), if any operand is NaN \p false will be returned.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x != y</tt>, \p false otherwise.
		 */
		template <typename T, typename U>
		friend auto operator!=(const T &x, const U &y) -> decltype(!real::binary_equality(x,y))
		{
			return !binary_equality(x,y);
		}
		/// Generic less-than operator involving piranha::real.
		/**
		 * \note
		 * This template operator is enabled only if either:
		 * - \p T is piranha::real and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::real and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::real.
		 * 
		 * Note that in all comparison operators, apart from piranha::real::operator!=(), if any operand is NaN \p false will be returned.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x < y</tt>, \p false otherwise.
		 */
		template <typename T, typename U>
		friend auto operator<(const T &x, const U &y) -> decltype(real::binary_less_than(x,y))
		{
			if (is_nan_comparison(x,y)) {
				return false;
			}
			return binary_less_than(x,y);
		}
		/// Generic less-than or equal operator involving piranha::real.
		/**
		 * \note
		 * This template operator is enabled only if either:
		 * - \p T is piranha::real and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::real and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::real.
		 * 
		 * Note that in all comparison operators, apart from piranha::real::operator!=(), if any operand is NaN \p false will be returned.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x <= y</tt>, \p false otherwise.
		 */
		template <typename T, typename U>
		friend auto operator<=(const T &x, const U &y) -> decltype(real::binary_leq(x,y))
		{
			if (is_nan_comparison(x,y)) {
				return false;
			}
			return binary_leq(x,y);
		}
		/// Generic greater-than operator involving piranha::real.
		/**
		 * \note
		 * This template operator is enabled only if either:
		 * - \p T is piranha::real and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::real and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::real.
		 * 
		 * Note that in all comparison operators, apart from piranha::real::operator!=(), if any operand is NaN \p false will be returned.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x > y</tt>, \p false otherwise.
		 */
		template <typename T, typename U>
		friend auto operator>(const T &x, const U &y) -> decltype(real::binary_less_than(y,x))
		{
			if (is_nan_comparison(x,y)) {
				return false;
			}
			return y < x;
		}
		/// Generic greater-than or equal operator involving piranha::real.
		/**
		 * \note
		 * This template operator is enabled only if either:
		 * - \p T is piranha::real and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::real and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::real.
		 * 
		 * Note that in all comparison operators, apart from piranha::real::operator!=(), if any operand is NaN \p false will be returned.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return \p true if <tt>x >= y</tt>, \p false otherwise.
		 */
		template <typename T, typename U>
		friend auto operator>=(const T &x, const U &y) -> decltype(real::binary_leq(y,x))
		{
			if (is_nan_comparison(x,y)) {
				return false;
			}
			return y <= x;
		}
		/// Exponentiation.
		/**
		 * The operation is carried out with the maximum precision between \p this and \p exp.
		 * 
		 * @param[in] exp exponent.
		 * 
		 * @return <tt>this ** exp</tt>.
		 */
		real pow(const real &exp) const
		{
			real retval{0,get_prec()};
			if (exp.get_prec() > get_prec()) {
				retval.set_prec(exp.get_prec());
			}
			::mpfr_pow(retval.m_value,m_value,exp.m_value,default_rnd);
			return retval;
		}
		/// Gamma function.
		/**
		 * @return gamma of \p this.
		 */
		real gamma() const
		{
			real retval{0,get_prec()};
			::mpfr_gamma(retval.m_value,m_value,default_rnd);
			return retval;
		}
		/// Logarithm of the gamma function.
		/**
		 * @return logarithm of the absolute value of the gamma of \p this.
		 */
		real lgamma() const
		{
			real retval{0,get_prec()};
			// This is the sign of gamma(*this). We don't use this.
			int sign;
			::mpfr_lgamma(retval.m_value,&sign,m_value,default_rnd);
			return retval;
		}
		/// Exponential.
		/**
		 * @return the exponential of \p this.
		 */
		real exp() const
		{
			real retval{0,get_prec()};
			::mpfr_exp(retval.m_value,m_value,default_rnd);
			return retval;
		}
		real binomial(const real &) const;
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
			char *cptr = ::mpfr_get_str(nullptr,&exp,10,0,r.m_value,default_rnd);
			if (unlikely(!cptr)) {
				piranha_throw(std::overflow_error,"error in conversion of real to rational: the call to the MPFR function failed");
			}
			smart_mpfr_str str(cptr,::mpfr_free_str);
			// Copy into C++ string.
			std::string cpp_str(str.get());
			// Insert the radix point.
			auto it = std::find_if(cpp_str.begin(),cpp_str.end(),[](char c) {return detail::is_digit(c);});
			if (it != cpp_str.end()) {
				++it;
				cpp_str.insert(it,'.');
				if (exp == std::numeric_limits< ::mpfr_exp_t>::min()) {
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

}

namespace detail
{

// Enabler for real pow.
template <typename T, typename U>
using real_pow_enabler = typename std::enable_if<
	(std::is_same<real,T>::value && is_real_interoperable_type<U>::value) ||
	(std::is_same<real,U>::value && is_real_interoperable_type<T>::value) ||
	(std::is_same<real,T>::value && std::is_same<real,U>::value)
>::type;

// Binomial follows the same rules as pow.
template <typename T, typename U>
using real_binomial_enabler = real_pow_enabler<T,U>;

}

namespace math
{

/// Specialisation of the piranha::math::pow() functor for piranha::real.
/**
 * This specialisation is activated when one of the arguments is piranha::real
 * and the other is either piranha::real or an interoperable type for piranha::real.
 *
 * The implementation follows these rules:
 * - if base and exponent are both piranha::real, then piranha::real::pow() is used;
 * - otherwise, the non-real argument is converted to piranha::real and then piranha::real::pow()
 *   is used.
 */
template <typename T, typename U>
struct pow_impl<T,U,detail::real_pow_enabler<T,U>>
{
	/// Call operator, real--real overload.
	/**
	 * @param[in] r base.
	 * @param[in] x exponent.
	 * 
	 * @return \p r to the power of \p x.
	 * 
	 * @throws unspecified any exception thrown by piranha::real::pow().
	 */
	real operator()(const real &r, const real &x) const
	{
		return r.pow(x);
	}
	/// Call operator, real base overload.
	/**
	 * @param[in] r base.
	 * @param[in] x exponent.
	 * 
	 * @return \p r to the power of \p x.
	 *
	 * @throws unspecified any exception thrown by piranha::real::pow() or by
	 * the invoked piranha::real constructor.
	 */
	template <typename T2>
	real operator()(const real &r, const T2 &x) const
	{
		// NOTE: init with the same precision as r in order
		// to maintain the same precision in the result.
		return r.pow(real{x,r.get_prec()});
	}
	/// Call operator, real exponent overload.
	/**
	 * @param[in] r base.
	 * @param[in] x exponent.
	 * 
	 * @return \p r to the power of \p x.
	 * 
	 * @throws unspecified any exception thrown by piranha::real::pow() or by
	 * the invoked piranha::real constructor.
	 */
	template <typename T2>
	real operator()(const T2 &r, const real &x) const
	{
		return real{r,x.get_prec()}.pow(x);
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
 * This specialisation is activated when one of the arguments is piranha::real
 * and the other is either piranha::real or an interoperable type for piranha::real.
 *
 * The implementation follows these rules:
 * - if top and bottom are both piranha::real, then piranha::real::binomial() is used;
 * - otherwise, the non-real argument is converted to piranha::real and then piranha::real::binomial()
 *   is used.
 */
template <typename T, typename U>
struct binomial_impl<T,U,detail::real_binomial_enabler<T,U>>
{
	/// Call operator, real--real overload.
	/**
	 * @param[in] x top.
	 * @param[in] y bottom.
	 *
	 * @return \p x choose \p y.
	 *
	 * @throws unspecified any exception thrown by piranha::real::binomial().
	 */
	real operator()(const real &x, const real &y) const
	{
		return x.binomial(y);
	}
	/// Call operator, real top overload.
	/**
	 * @param[in] x top.
	 * @param[in] y bottom.
	 *
	 * @return \p x choose \p y.
	 *
	 * @throws unspecified any exception thrown by piranha::real::binomial() or by the invoked
	 * piranha::real constructor.
	 */
	template <typename T2>
	real operator()(const real &x, const T2 &y) const
	{
		// NOTE: init with the same precision as r in order
		// to maintain the same precision in the result.
		return x.binomial(real{y,x.get_prec()});
	}
	/// Call operator, real bottom overload.
	/**
	 * @param[in] x top.
	 * @param[in] y bottom.
	 *
	 * @return \p x choose \p y.
	 *
	 * @throws unspecified any exception thrown by piranha::real::binomial() or by the invoked
	 * piranha::real constructor.
	 */
	template <typename T2>
	real operator()(const T2 &x, const real &y) const
	{
		return real{x,y.get_prec()}.binomial(y);
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
// NOTE: the story here is that ICC has a weird behaviour when the thread_local
// storage. Essentially, the thread-local static variable in the fma() function
// upon destruction has the _mpfr_d set to zero for some reason but the other members
// are not zeroed out. This results in the asserts below firing, and probably a memory
// leak as well as the variable is not cleared. We just disable the asserts for now.
#if !defined(PIRANHA_COMPILER_IS_INTEL)
		piranha_assert(!m_value->_mpfr_prec);
		piranha_assert(!m_value->_mpfr_sign);
		piranha_assert(!m_value->_mpfr_exp);
#endif
	}
}

namespace detail
{

// Compute gamma(a)/(gamma(b) * gamma(c)), assuming a, b and c are not negative ints. The logarithm
// of the gamma function is used internally.
inline real real_compute_3_gamma(const real &a, const real &b, const real &c, const ::mpfr_prec_t &prec)
{
	// Here we should never enter with negative ints.
	piranha_assert(a.sign() >= 0 || a.truncated() != a);
	piranha_assert(b.sign() >= 0 || b.truncated() != b);
	piranha_assert(c.sign() >= 0 || c.truncated() != c);
	const real pi = real{0,prec}.pi();
	real tmp0(0), tmp1(1);
	if (a.sign() < 0) {
		tmp0 -= (1 - a).lgamma();
		tmp1 *= pi / (a * pi).sin();
	} else {
		tmp0 += a.lgamma();
	}
	if (b.sign() < 0) {
		tmp0 += (1 - b).lgamma();
		tmp1 *= (b * pi).sin() / pi;
	} else {
		tmp0 -= b.lgamma();
	}
	if (c.sign() < 0) {
		tmp0 += (1 - c).lgamma();
		tmp1 *= (c * pi).sin() / pi;
	} else {
		tmp0 -= c.lgamma();
	}
	return tmp0.exp() * tmp1;
}

}

/// Binomial coefficient.
/**
 * This method will return \p this choose \p y. Any combination of real values is supported.
 * The implementation uses the logarithm of the gamma function, thus the result will not be in
 * general exact (even if \p this and \p y are integral values).
 *
 * The returned value will have the maximum precision between \p this and \p y.
 *
 * @param[in] y bottom value.
 *
 * @return \p this choose \p y.
 *
 * @throws std::invalid_argument if either \p this or \p y is not finite.
 * @throws unspecified any exception resulting from arithmetic operations on piranha::real.
 */
inline real real::binomial(const real &y) const
{
	if (unlikely(is_nan() || is_inf() || y.is_nan() || y.is_inf())) {
		piranha_throw(std::invalid_argument,"cannot compute binomial coefficient with non-finite real argument(s)");
	}
	// Work with the max precision.
	const ::mpfr_prec_t max_prec = std::max< ::mpfr_prec_t>(get_prec(),y.get_prec());
	const bool neg_int_x = truncated() == (*this) && sign() < 0,
		neg_int_y = y.truncated() == y && y.sign() < 0,
		neg_int_x_y = ((*this) - y).truncated() == ((*this) - y) && ((*this) - y).sign() < 0;
	const unsigned mask = unsigned(neg_int_x) + (unsigned(neg_int_y) << 1u) + (unsigned(neg_int_x_y) << 2u);
	switch (mask) {
		case 0u:
			// Case 0 is the non-special one, use the default implementation.
			return detail::real_compute_3_gamma((*this) + 1,y + 1,(*this) - y + 1,max_prec);
		// NOTE: case 1 is not possible: x < 0, y > 0 implies x - y < 0 always.
		case 2u:
		case 4u:
			// These are finite numerators with infinite denominators.
			return real{0,max_prec};
		// NOTE: case 6 is not possible: x > 0, y < 0 implies x - y > 0 always.
		case 3u:
		{
			// 3 and 5 are the cases with 1 inf in num and 1 inf in den. Use the transformation
			// formula to make them finite.
			// NOTE: the phase here is really just a sign, but it seems tricky to compute this exactly
			// due to potential rounding errors. We are attempting to err on the safe side by using pow()
			// here.
			const auto phase = math::pow(-1,(*this) + 1) / math::pow(-1,y + 1);
			return detail::real_compute_3_gamma(-y,-(*this),(*this) - y + 1,max_prec) * phase;
		}
		case 5u:
		{
			const auto phase = math::pow(-1,(*this) - y + 1) / math::pow(-1,(*this) + 1);
			return detail::real_compute_3_gamma(-((*this) - y),y + 1,-(*this),max_prec) * phase;
		}
	}
	// Case 7 returns zero -> from inf / (inf * inf) it becomes a / (b * inf) after the transform.
	// NOTE: put it here so the compiler does not complain about missing return statement in the switch block.
	return real{0,max_prec};
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
