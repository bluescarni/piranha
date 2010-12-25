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

#include "config.hpp"
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
 * with long long and long double, interaction with this types will be somewhat slow due to the extra workload of converting such types
 * to GMP-compatible types. Also, every function interacting with floating-point types will check that the floating-point values are not
 * non-finite: in case of infinities or NaNs, an <tt>std::invalid_argument</tt> exception will be thrown.
 * 
 * @see http://gmplib.org/
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo implementation notes: use of internal GMP implementation details.
 */
class integer
{
		// Metaprogramming to establish the return type of binary arithmetic operations involving integers.
		// Default result type will be integer itself; for consistency with C/C++ when one of the arguments
		// is a floating point type, we will return a value of the same floating point type.
		template <typename T, typename Enable = void>
		struct deduce_result_type
		{
			typedef integer type;
		};
		template <typename T>
		struct deduce_result_type<T,typename boost::enable_if<std::is_floating_point<T>>::type>
		{
			typedef T type;
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
			boost::format f("%.0Lf");
			f % x;
			return f.str();
		}






// 		// Addition with self.
// 		struct self_adder_visitor: public boost::static_visitor<bool>
// 		{
// 			bool operator()(mpz_class &n1, const mpz_class &n2) const
// 			{
// 				n1 += n2;
// 				return true;
// 			}
// 			bool operator()(mpz_class &n1, const max_fast_int &n2) const
// 			{
// 				n1 += to_gmp_type(n2);
// 				return true;
// 			}
// 			bool operator()(max_fast_int &, const mpz_class &) const
// 			{
// 				// NOTE: here probably we can do better than this :)
// 				return false;
// 			}
// 			bool operator()(max_fast_int &n1, const max_fast_int &n2) const
// 			{
// 				// Interval arithmetics: n1 + [n2_m,n2_M] == [m,M], from which:
// 				// - if n1 > 0 -> n2 must be in [m,M - n1]
// 				// - if n1 < 0 -> n2 must be in [m - n1,M]
// 				// OPTIMIZE: here maybe we can reduce the branching by testing for n1 >= 0 and then 'else', in place of a second if.
// 				// OPTIMIZE: likely/unlikely of help here? Returning false will happen just once anyway in the
// 				// lifetime of the integer part of the variant...
// 				if (n1 > 0 && n2 > boost::integer_traits<max_fast_int>::const_max - n1) {
// 					return false;
// 				}
// 				if (n1 < 0 && n2 < boost::integer_traits<max_fast_int>::const_min - n1) {
// 					return false;
// 				}
// 				n1 += n2;
// 				return true;
// 			}
// 		};
// 		// Addition with integral POD types.
// 		template <class T>
// 		struct integral_pod_adder_visitor: public boost::static_visitor<bool>
// 		{
// 			p_static_check(boost::is_integral<T>::value,"");
// 			integral_pod_adder_visitor(const T &value):m_value(value) {}
// 			bool operator()(mpz_class &n) const
// 			{
// 				n += to_gmp_type(m_value);
// 				return true;
// 			}
// 			bool operator()(max_fast_int &n) const
// 			{
// 				try {
// 					const max_fast_int tmp = boost::numeric_cast<max_fast_int>(m_value);
// 					self_adder_visitor a;
// 					return a(n,tmp);
// 				} catch (const boost::numeric::bad_numeric_cast &) {
// 					// If we cannot convert T to max_fast_int, we need to upgrade to mpz_class.
// 					return false;
// 				}
// 			}
// 			const T &m_value;
// 		};
// 		template <class Functor, class Variant>
// 		void generic_binary_applier(const Functor &f, const Variant &other)
// 		{
// 			if (unlikely(!boost::apply_visitor(f,m_value,other))) {
// 				// Make sure we are in max_fast_int mode.
// 				piranha_assert(boost::get<max_fast_int>(&m_value));
// 				upgrade();
// 				const bool new_status = boost::apply_visitor(f,m_value,other);
// 				(void)new_status;
// 				piranha_assert(new_status);
// 			}
// 		}
// 		template <class Functor>
// 		void generic_unary_applier(const Functor &f)
// 		{
// 			if (unlikely(!boost::apply_visitor(f,m_value))) {
// 				// Make sure we are in max_fast_int mode.
// 				piranha_assert(boost::get<max_fast_int>(&m_value));
// 				upgrade();
// 				const bool new_status = boost::apply_visitor(f,m_value);
// 				(void)new_status;
// 				piranha_assert(new_status);
// 			}
// 		}
// 		void dispatch_add(const integer &n)
// 		{
// 			generic_binary_applier(self_adder_visitor(),n.m_value);
// 		}
// 		template <class T>
// 		void dispatch_add(const T &n, typename boost::enable_if<boost::is_integral<T> >::type * = 0)
// 		{
// 			generic_unary_applier(integral_pod_adder_visitor<T>(n));
// 		}
// 		template <class T>
// 		void dispatch_add(const T &x, typename boost::enable_if<boost::is_floating_point<T> >::type * = 0)
// 		{
// 			// NOTE: fp normal checks are performed in the assignment.
// 			*this = operator T() + x;
// 		}
// 		template <class T>
// 		typename deduce_result_type<T>::type dispatch_operator_plus(const T &n, typename boost::enable_if_c<boost::is_integral<T>::value || boost::is_same<integer,T>::value>::type * = 0) const
// 		{
// 			integer retval(*this);
// 			retval += n;
// 			return retval;
// 		}
// 		template <class T>
// 		typename deduce_result_type<T>::type dispatch_operator_plus(const T &x, typename boost::enable_if<boost::is_floating_point<T> >::type * = 0) const
// 		{
// 			// NOTE: No need for fp normal checks, as we do not modify this.
// 			return operator T() + x;
// 		}
// 		// Operator++.
// 		struct self_increment_visitor: public boost::static_visitor<bool>
// 		{
// 			bool operator()(mpz_class &n) const
// 			{
// 				++n;
// 				return true;
// 			}
// 			bool operator()(max_fast_int &n) const
// 			{
// 				// Cannot increase past the upper limit.
// 				if (unlikely(n == boost::integer_traits<max_fast_int>::const_max)) {
// 					return false;
// 				}
// 				++n;
// 				return true;
// 			}
// 		};
// 		// Subtraction with self.
// 		struct self_subtractor_visitor: public boost::static_visitor<bool>
// 		{
// 			bool operator()(mpz_class &n1, const mpz_class &n2) const
// 			{
// 				n1 -= n2;
// 				return true;
// 			}
// 			bool operator()(mpz_class &n1, const max_fast_int &n2) const
// 			{
// 				n1 -= to_gmp_type(n2);
// 				return true;
// 			}
// 			bool operator()(max_fast_int &, const mpz_class &) const
// 			{
// 				return false;
// 			}
// 			bool operator()(max_fast_int &n1, const max_fast_int &n2) const
// 			{
// 				// Check if we can compute -n2 safely and then use the adder visitor.
// 				const max_fast_int offset = boost::integer_traits<max_fast_int>::const_max + boost::integer_traits<max_fast_int>::const_min;
// 				if (offset < 0 && n2 < (-boost::integer_traits<max_fast_int>::const_max)) {
// 					return false;
// 				}
// 				if (offset > 0 && n2 > (-boost::integer_traits<max_fast_int>::const_min)) {
// 					return false;
// 				}
// 				self_adder_visitor a;
// 				return a(n1,-n2);
// 			}
// 		};
// 		// Subtraction with integral POD types.
// 		template <class T>
// 		struct integral_pod_subtractor_visitor: public boost::static_visitor<bool>
// 		{
// 			p_static_check(boost::is_integral<T>::value,"");
// 			integral_pod_subtractor_visitor(const T &value):m_value(value) {}
// 			bool operator()(mpz_class &n) const
// 			{
// 				n -= to_gmp_type(m_value);
// 				return true;
// 			}
// 			bool operator()(max_fast_int &n) const
// 			{
// 				try {
// 					const max_fast_int tmp = boost::numeric_cast<max_fast_int>(m_value);
// 					self_subtractor_visitor a;
// 					return a(n,tmp);
// 				} catch (const boost::numeric::bad_numeric_cast &) {
// 					// If we cannot convert T to max_fast_int, we need to upgrade to mpz_class.
// 					return false;
// 				}
// 			}
// 			const T &m_value;
// 		};
// 		// In-place negation.
// 		struct negate_visitor: public boost::static_visitor<bool>
// 		{
// 			bool operator()(mpz_class &n) const
// 			{
// 				mpz_neg(n.get_mpz_t(),n.get_mpz_t());
// 				return true;
// 			}
// 			bool operator()(max_fast_int &n) const
// 			{
// 				const max_fast_int offset = boost::integer_traits<max_fast_int>::const_max + boost::integer_traits<max_fast_int>::const_min;
// 				if (offset < 0 && n < (-boost::integer_traits<max_fast_int>::const_max)) {
// 					return false;
// 				}
// 				if (offset > 0 && n > (-boost::integer_traits<max_fast_int>::const_min)) {
// 					return false;
// 				}
// 				n = -n;
// 				return true;
// 			}
// 		};
// 		void dispatch_sub(const integer &n)
// 		{
// 			generic_binary_applier(self_subtractor_visitor(),n.m_value);
// 		}
// 		template <class T>
// 		void dispatch_sub(const T &n, typename boost::enable_if<boost::is_integral<T> >::type * = 0)
// 		{
// 			generic_unary_applier(integral_pod_subtractor_visitor<T>(n));
// 		}
// 		template <class T>
// 		void dispatch_sub(const T &x, typename boost::enable_if<boost::is_floating_point<T> >::type * = 0)
// 		{
// 			*this = operator T() - x;
// 		}
// 		template <class T>
// 		typename deduce_result_type<T>::type dispatch_operator_minus(const T &n, typename boost::disable_if<boost::is_floating_point<T> >::type * = 0) const
// 		{
// 			integer retval(*this);
// 			retval -= n;
// 			return retval;
// 		}
// 		template <class T>
// 		typename deduce_result_type<T>::type dispatch_operator_minus(const T &x, typename boost::enable_if<boost::is_floating_point<T> >::type * = 0) const
// 		{
// 			return operator T() - x;
// 		}
// 		// Operator--.
// 		struct self_decrement_visitor: public boost::static_visitor<bool>
// 		{
// 			bool operator()(mpz_class &n) const
// 			{
// 				--n;
// 				return true;
// 			}
// 			bool operator()(max_fast_int &n) const
// 			{
// 				// Cannot decrease below the lower limit.
// 				if (unlikely(n == boost::integer_traits<max_fast_int>::const_min)) {
// 					return false;
// 				}
// 				--n;
// 				return true;
// 			}
// 		};
// 		// Multiplication with self.
// 		struct self_multiplier_visitor: public boost::static_visitor<bool>
// 		{
// 			bool operator()(mpz_class &n1, const mpz_class &n2) const
// 			{
// 				n1 *= n2;
// 				return true;
// 			}
// 			bool operator()(mpz_class &n1, const max_fast_int &n2) const
// 			{
// 				n1 *= to_gmp_type(n2);
// 				return true;
// 			}
// 			bool operator()(max_fast_int &, const mpz_class &) const
// 			{
// 				return false;
// 			}
// 			bool operator()(max_fast_int &n1, const max_fast_int &n2) const
// 			{
// 				if (unlikely(n1 == 0)) {
// 					return true;
// 				}
// 				if (unlikely(n2 == 0)) {
// 					n1 = 0;
// 					return true;
// 				}
// 				const max_fast_int offset = boost::integer_traits<max_fast_int>::const_max + boost::integer_traits<max_fast_int>::const_min;
// 				// If offset is negative, it means we can compute (-max) safely and we just have to check if n1 or n2 are less than (-max). max is the range.
// 				if (unlikely(offset < 0 && (n1 < -boost::integer_traits<max_fast_int>::const_max || n2 < -boost::integer_traits<max_fast_int>::const_max))) {
// 					return false;
// 				}
// 				// If offset is positive, it means we can compute (-min) safely and we just have to check if n1 or n2 are greater than (-min). (-min) is the range.
// 				if (unlikely(offset > 0 && (n1 > -boost::integer_traits<max_fast_int>::const_min || n2 > -boost::integer_traits<max_fast_int>::const_min))) {
// 					return false;
// 				}
// 				// NOTE: if offset is zero, the range is evenly split. No need for special checks.
// 				const std::size_t ulog1 = detail::log2_table::ulog(n1), ulog2 = detail::log2_table::ulog(n2);
// 				// NOTE: it is guaranteed at this point that the result of the multiplication will
// 				// be <= 2 ** (ulog1 + ulog2). Hence we need to compare ulog1 + ulog2 to the last index
// 				// of the table, which corresponds to the log2 of the highest power of two representable
// 				// by the integer type. Anything above that will be unsafe.
// 				if (likely(ulog1 + ulog2 < detail::log2_table::size())) {
// 					n1 *= n2;
// 					return true;
// 				} else {
// 					return false;
// 				}
// 			}
// 		};
// 		// Multiplication with integral POD types.
// 		template <class T>
// 		struct integral_pod_multiplier_visitor: public boost::static_visitor<bool>
// 		{
// 			p_static_check(boost::is_integral<T>::value,"");
// 			integral_pod_multiplier_visitor(const T &value):m_value(value) {}
// 			bool operator()(mpz_class &n) const
// 			{
// 				n *= to_gmp_type(m_value);
// 				return true;
// 			}
// 			bool operator()(max_fast_int &n) const
// 			{
// 				try {
// 					const max_fast_int tmp = boost::numeric_cast<max_fast_int>(m_value);
// 					self_multiplier_visitor a;
// 					return a(n,tmp);
// 				} catch (const boost::numeric::bad_numeric_cast &) {
// 					// If we cannot convert T to max_fast_int, we need to upgrade to mpz_class.
// 					return false;
// 				}
// 			}
// 			const T &m_value;
// 		};
// 		void dispatch_mult(const integer &n)
// 		{
// 			generic_binary_applier(self_multiplier_visitor(),n.m_value);
// 		}
// 		template <class T>
// 		void dispatch_mult(const T &n, typename boost::enable_if<boost::is_integral<T> >::type * = 0)
// 		{
// 			generic_unary_applier(integral_pod_multiplier_visitor<T>(n));
// 		}
// 		template <class T>
// 		void dispatch_mult(const T &x, typename boost::enable_if<boost::is_floating_point<T> >::type * = 0)
// 		{
// 			*this = operator T() * x;
// 		}
// 		template <class T>
// 		typename deduce_result_type<T>::type dispatch_operator_mult(const T &n, typename boost::disable_if<boost::is_floating_point<T> >::type * = 0) const
// 		{
// 			integer retval(*this);
// 			retval *= n;
// 			return retval;
// 		}
// 		template <class T>
// 		typename deduce_result_type<T>::type dispatch_operator_mult(const T &x, typename boost::enable_if<boost::is_floating_point<T> >::type * = 0) const
// 		{
// 			return operator T() * x;
// 		}
// 		// Division with self.
// 		struct self_divider_visitor: public boost::static_visitor<bool>
// 		{
// 			bool operator()(mpz_class &n1, const mpz_class &n2) const
// 			{
// 				n1 /= n2;
// 				return true;
// 			}
// 			bool operator()(mpz_class &n1, const max_fast_int &n2) const
// 			{
// 				n1 /= to_gmp_type(n2);
// 				return true;
// 			}
// 			bool operator()(max_fast_int &n1, const mpz_class &n2) const
// 			{
// 				const max_fast_int offset = boost::integer_traits<max_fast_int>::const_max + boost::integer_traits<max_fast_int>::const_min;
// 				// If the operand is near the bounds, refuse to do anything: a change of sign from the division could lead to overflow.
// 				if (offset < 0 && n1 < -boost::integer_traits<max_fast_int>::const_max) {
// 					return false;
// 				}
// 				if (offset > 0 && n1 > -boost::integer_traits<max_fast_int>::const_min) {
// 					return false;
// 				}
// 				// If abs(n2) > abs(n1), result will be zero. This will let us shortcut any GMP operation even if n2 is larger than max_fast_int.
// 				if (n2 > to_gmp_type(n1) || n2 < to_gmp_type(-n1)) {
// 					n1 = 0;
// 					return true;
// 				}
// 				// Try extracting a max_fast_int from mpz.
// 				if (!n2.fits_slong_p()) {
// 					// Unable to extract long.
// 					return false;
// 				}
// 				// Finally, perform the division. If we fail to convert n2 to max_fast_int, return false.
// 				try {
// 					n1 /= boost::numeric_cast<max_fast_int>(n2.get_si());
// 				} catch (const boost::numeric::bad_numeric_cast &) {
// 					return false;
// 				}
// 				return true;
// 			}
// 			bool operator()(max_fast_int &n1, const max_fast_int &n2) const
// 			{
// 				// Make sure a division with -1 will not produce an overflow.
// 				const max_fast_int offset = boost::integer_traits<max_fast_int>::const_max + boost::integer_traits<max_fast_int>::const_min;
// 				if (offset < 0 && n1 < -boost::integer_traits<max_fast_int>::const_max) {
// 					return false;
// 				}
// 				if (offset > 0 && n1 > -boost::integer_traits<max_fast_int>::const_min) {
// 					return false;
// 				}
// 				n1 /= n2;
// 				return true;
// 			}
// 		};
// 		// Division with integral POD types.
// 		template <class T>
// 		struct integral_pod_divider_visitor: public boost::static_visitor<bool>
// 		{
// 			p_static_check(boost::is_integral<T>::value,"");
// 			integral_pod_divider_visitor(const T &value):m_value(value) {}
// 			bool operator()(mpz_class &n) const
// 			{
// 				n /= to_gmp_type(m_value);
// 				return true;
// 			}
// 			bool operator()(max_fast_int &n) const
// 			{
// 				try {
// 					const max_fast_int tmp = boost::numeric_cast<max_fast_int>(m_value);
// 					self_divider_visitor a;
// 					return a(n,tmp);
// 				} catch (const boost::numeric::bad_numeric_cast &) {
// 					// If we cannot convert T to max_fast_int, we need to upgrade to mpz_class.
// 					return false;
// 				}
// 			}
// 			const T &m_value;
// 		};
// 		void dispatch_divide(const integer &n)
// 		{
// 			if (n == 0) {
// 				piranha_throw(zero_division_error,"cannot divide by zero");
// 			}
// 			generic_binary_applier(self_divider_visitor(),n.m_value);
// 		}
// 		template <class T>
// 		void dispatch_divide(const T &n, typename boost::enable_if<boost::is_integral<T> >::type * = 0)
// 		{
// 			// Prevent division by zero.
// 			if (n == 0) {
// 				piranha_throw(zero_division_error,"cannot divide by zero");
// 			}
// 			generic_unary_applier(integral_pod_divider_visitor<T>(n));
// 		}
// 		template <class T>
// 		void dispatch_divide(const T &x, typename boost::enable_if<boost::is_floating_point<T> >::type * = 0)
// 		{
// 			fp_normal_check(x);
// 			// Prevent division by zero.
// 			if (x == 0) {
// 				piranha_throw(zero_division_error,"cannot divide by zero");
// 			}
// 			*this = operator T() / x;
// 		}
// 		template <class T>
// 		typename deduce_result_type<T>::type dispatch_operator_divide(const T &n, typename boost::disable_if<boost::is_floating_point<T> >::type * = 0) const
// 		{
// 			integer retval(*this);
// 			retval /= n;
// 			return retval;
// 		}
// 		template <class T>
// 		typename deduce_result_type<T>::type dispatch_operator_divide(const T &x, typename boost::enable_if<boost::is_floating_point<T> >::type * = 0) const
// 		{
// 			return operator T() / x;
// 		}
// 		template <class T>
// 		typename deduce_result_type<T>::type dispatch_operator_divide_left(const T &n, typename boost::disable_if<boost::is_floating_point<T> >::type * = 0) const
// 		{
// 			integer retval(n);
// 			retval /= *this;
// 			return retval;
// 		}
// 		template <class T>
// 		typename deduce_result_type<T>::type dispatch_operator_divide_left(const T &x, typename boost::enable_if<boost::is_floating_point<T> >::type * = 0) const
// 		{
// 			return x / operator T();
// 		}
// 		// Modulo with self.
// 		struct self_modulo_visitor: public boost::static_visitor<bool>
// 		{
// 			bool operator()(mpz_class &n1, const mpz_class &n2) const
// 			{
// 				n1 %= n2;
// 				return true;
// 			}
// 			bool operator()(mpz_class &n1, const max_fast_int &n2) const
// 			{
// 				n1 %= to_gmp_type(n2);
// 				return true;
// 			}
// 			bool operator()(max_fast_int &n1, const mpz_class &n2) const
// 			{
// 				if (unlikely(!n2.fits_slong_p())) {
// 					return false;
// 				}
// 				try {
// 					n1 %= boost::numeric_cast<max_fast_int>(n2.get_si());
// 				} catch (const boost::numeric::bad_numeric_cast &) {
// 					return false;
// 				}
// 				return true;
// 			}
// 			bool operator()(max_fast_int &n1, const max_fast_int &n2) const
// 			{
// 				n1 %= n2;
// 				return true;
// 			}
// 		};
// 		// Modulo with integral POD types.
// 		template <class T>
// 		struct integral_pod_modulo_visitor: public boost::static_visitor<bool>
// 		{
// 			p_static_check(boost::is_integral<T>::value,"");
// 			integral_pod_modulo_visitor(const T &value):m_value(value) {}
// 			bool operator()(mpz_class &n) const
// 			{
// 				n %= to_gmp_type(m_value);
// 				return true;
// 			}
// 			bool operator()(max_fast_int &n) const
// 			{
// 				try {
// 					// If we cannot convert m_value to max_fast_int, it means
// 					// that m_value is larger than n: n does not change.
// 					n %= boost::numeric_cast<max_fast_int>(m_value);
// 				} catch (const boost::numeric::bad_numeric_cast &) {}
// 				return true;
// 			}
// 			const T &m_value;
// 		};
// 		void dispatch_modulo(const integer &n)
// 		{
// 			generic_binary_applier(self_modulo_visitor(),n.m_value);
// 		}
// 		template <class T>
// 		void dispatch_modulo(const T &n, typename boost::enable_if<boost::is_integral<T> >::type * = 0)
// 		{
// 			generic_unary_applier(integral_pod_modulo_visitor<T>(n));
// 		}
// 		// Equality with self.
// 		struct self_equality_visitor: public boost::static_visitor<bool>
// 		{
// 			bool operator()(const mpz_class &n1, const mpz_class &n2) const
// 			{
// 				return n1 == n2;
// 			}
// 			bool operator()(const mpz_class &n1, const max_fast_int &n2) const
// 			{
// 				return n1 == to_gmp_type(n2);
// 			}
// 			bool operator()(const max_fast_int &n1, const mpz_class &n2) const
// 			{
// 				return n2 == to_gmp_type(n1);
// 			}
// 			bool operator()(const max_fast_int &n1, const max_fast_int &n2) const
// 			{
// 				return n1 == n2;
// 			}
// 		};
// 		// Equality with integral POD types.
// 		template <class T>
// 		struct integral_pod_equality_visitor: public boost::static_visitor<bool>
// 		{
// 			p_static_check(boost::is_integral<T>::value,"");
// 			integral_pod_equality_visitor(const T &value):m_value(value) {}
// 			bool operator()(const mpz_class &n) const
// 			{
// 				return n == to_gmp_type(m_value);
// 			}
// 			bool operator()(const max_fast_int &n) const
// 			{
// 				try {
// 					return n == boost::numeric_cast<max_fast_int>(m_value);
// 				} catch (const boost::numeric::bad_numeric_cast &) {
// 					// If we cannot convert m_value to max_fast_int, it cannot be equal to n.
// 					return false;
// 				}
// 			}
// 			const T &m_value;
// 		};
// 		bool dispatch_equality(const integer &n) const
// 		{
// 			return boost::apply_visitor(self_equality_visitor(),m_value,n.m_value);
// 		}
// 		template <class T>
// 		bool dispatch_equality(const T &n, typename boost::enable_if<boost::is_integral<T> >::type * = 0) const
// 		{
// 			return boost::apply_visitor(integral_pod_equality_visitor<T>(n),m_value);
// 		}
// 		template <class T>
// 		bool dispatch_equality(const T &x, typename boost::enable_if<boost::is_floating_point<T> >::type * = 0) const
// 		{
// 			return operator T() == x;
// 		}
// 		// Less-than with self.
// 		struct self_less_than_visitor: public boost::static_visitor<bool>
// 		{
// 			bool operator()(const mpz_class &n1, const mpz_class &n2) const
// 			{
// 				return n1 < n2;
// 			}
// 			bool operator()(const mpz_class &n1, const max_fast_int &n2) const
// 			{
// 				return n1 < to_gmp_type(n2);
// 			}
// 			bool operator()(const max_fast_int &n1, const mpz_class &n2) const
// 			{
// 				return n2 > to_gmp_type(n1);
// 			}
// 			bool operator()(const max_fast_int &n1, const max_fast_int &n2) const
// 			{
// 				return n1 < n2;
// 			}
// 		};
// 		// Less-than with integral POD types.
// 		template <class T>
// 		struct integral_pod_less_than_visitor: public boost::static_visitor<bool>
// 		{
// 			p_static_check(boost::is_integral<T>::value,"");
// 			integral_pod_less_than_visitor(const T &value):m_value(value) {}
// 			bool operator()(const mpz_class &n) const
// 			{
// 				return n < to_gmp_type(m_value);
// 			}
// 			bool operator()(const max_fast_int &n) const
// 			{
// 				try {
// 					return n < boost::numeric_cast<max_fast_int>(m_value);
// 				} catch (const boost::numeric::bad_numeric_cast &) {
// 					// If we cannot convert m_value to max_fast_int, depending on its sign
// 					// it will be less-than or greater-than n.
// 					return (m_value >= 0);
// 				}
// 			}
// 			const T &m_value;
// 		};
// 		bool dispatch_less_than(const integer &n) const
// 		{
// 			return boost::apply_visitor(self_less_than_visitor(),m_value,n.m_value);
// 		}
// 		template <class T>
// 		bool dispatch_less_than(const T &n, typename boost::enable_if<boost::is_integral<T> >::type * = 0) const
// 		{
// 			return boost::apply_visitor(integral_pod_less_than_visitor<T>(n),m_value);
// 		}
// 		template <class T>
// 		bool dispatch_less_than(const T &x, typename boost::enable_if<boost::is_floating_point<T> >::type * = 0) const
// 		{
// 			return x > operator T();
// 		}
// 		// Greater-than with self.
// 		struct self_greater_than_visitor: public boost::static_visitor<bool>
// 		{
// 			bool operator()(const mpz_class &n1, const mpz_class &n2) const
// 			{
// 				return n1 > n2;
// 			}
// 			bool operator()(const mpz_class &n1, const max_fast_int &n2) const
// 			{
// 				return n1 > to_gmp_type(n2);
// 			}
// 			bool operator()(const max_fast_int &n1, const mpz_class &n2) const
// 			{
// 				return n2 < to_gmp_type(n1);
// 			}
// 			bool operator()(const max_fast_int &n1, const max_fast_int &n2) const
// 			{
// 				return n1 > n2;
// 			}
// 		};
// 		// Greater-than with integral POD types.
// 		template <class T>
// 		struct integral_pod_greater_than_visitor: public boost::static_visitor<bool>
// 		{
// 			p_static_check(boost::is_integral<T>::value,"");
// 			integral_pod_greater_than_visitor(const T &value):m_value(value) {}
// 			bool operator()(const mpz_class &n) const
// 			{
// 				return n > to_gmp_type(m_value);
// 			}
// 			bool operator()(const max_fast_int &n) const
// 			{
// 				try {
// 					return n > boost::numeric_cast<max_fast_int>(m_value);
// 				} catch (const boost::numeric::bad_numeric_cast &) {
// 					// If we cannot convert m_value to max_fast_int, depending on its sign
// 					// it will be less-than or greater-than n.
// 					return (m_value <= 0);
// 				}
// 			}
// 			const T &m_value;
// 		};
// 		bool dispatch_greater_than(const integer &n) const
// 		{
// 			return boost::apply_visitor(self_greater_than_visitor(),m_value,n.m_value);
// 		}
// 		template <class T>
// 		bool dispatch_greater_than(const T &n, typename boost::enable_if<boost::is_integral<T> >::type * = 0) const
// 		{
// 			return boost::apply_visitor(integral_pod_greater_than_visitor<T>(n),m_value);
// 		}
// 		template <class T>
// 		bool dispatch_greater_than(const T &x, typename boost::enable_if<boost::is_floating_point<T> >::type * = 0) const
// 		{
// 			return x < operator T();
// 		}
// 		// Exponentiation to natural power.
// 		template <class T>
// 		struct natural_power_visitor: public boost::static_visitor<integer>
// 		{
// 			p_static_check((boost::is_integral<T>::value || boost::is_same<T,integer>::value),"");
// 			natural_power_visitor(const T &exp):m_exp(exp)
// 			{
// 				piranha_assert(exp > 0);
// 			}
// 			integer operator()(const mpz_class &n) const
// 			{
// 				// Let's try to use the GMP function.
// 				try {
// 					mpz_class tmp;
// 					mpz_pow_ui(tmp.get_mpz_t(),n.get_mpz_t(),boost::numeric_cast<unsigned long>(m_exp));
// 					return integer(tmp);
// 				} catch (const boost::numeric::bad_numeric_cast &) {}
// 				// NOTE: if exp is an integer, it will throw value_error in a failed numeric_cast.
// 				catch (const value_error &) {}
// 				// If we cannot convert m_exp to unsigned long, resort to exponentiation by squaring.
// 				return integer(math::ebs(n,m_exp));
// 			}
// 			integer operator()(const max_fast_int &n) const
// 			{
// 				return math::ebs(integer(n),m_exp);
// 			}
// 			const T &m_exp;
// 		};
// 		template <class T>
// 		integer dispatch_pow(const T &x, typename boost::enable_if<boost::is_floating_point<T> >::type * = 0) const
// 		{
// 			fp_normal_check(x);
// 			if (!math::is_integral(x)) {
// 				piranha_throw(value_error,"invalid floating-point exponent");
// 			}
// 			return dispatch_pow(integer(x));
// 		}
// 		template <class T>
// 		integer dispatch_pow(const T &exp, typename boost::enable_if_c<boost::is_integral<T>::value || boost::is_same<T,integer>::value>::type * = 0) const
// 		{
// 			// n ** 0 == 1, always.
// 			if (!exp) {
// 				return integer(1);
// 			}
// 			if (exp < 0) {
// 				return 1 / boost::apply_visitor(natural_power_visitor<T>(-exp),m_value);
// 			} else {
// 				return boost::apply_visitor(natural_power_visitor<T>(exp),m_value);
// 			}
// 		}
// 		template <class T>
// 		void construct_from_numerical_pod(const T &x, typename boost::enable_if_c<boost::is_arithmetic<T>::value>::type * = 0)
// 		{
// 			fp_normal_check(x);
// 			try {
// 				// First let's try to convert the POD to a max_fast_int without precision loss.
// 				// Floating point values will be truncated (i.e., rounded to zero).
// 				m_value = boost::numeric_cast<max_fast_int>(x);
// 			} catch (const boost::numeric::bad_numeric_cast &) {
// 				// If the above fails, build a GMP integer instead. Same truncation rules apply for floating point values.
// 				// We must use a wrapper function in order to cope with the fact that the GMP C++ interface does not support long long and long double.
// 				m_value = mpz_class(to_gmp_type(x));
// 			}
// 		}
// 		// Functions and visitor to convert to numerical pod types.
// 		template <class T>
// 		static void mpz_class_convert(T &output, const mpz_class &input, typename boost::enable_if_c<boost::is_arithmetic<T>::value>::type * = 0)
// 		{
// 			if (boost::is_integral<T>::value) {
// 				if (boost::is_signed<T>::value) {
// 					if (input.fits_slong_p()) {
// 						output = boost::numeric_cast<T>(input.get_si());
// 					} else {
// 						throw boost::numeric::bad_numeric_cast();
// 					}
// 				} else {
// 					if (input >= 0 && input.fits_ulong_p()) {
// 						output = boost::numeric_cast<T>(input.get_ui());
// 					} else {
// 						throw boost::numeric::bad_numeric_cast();
// 					}
// 				}
// 			} else {
// 				// Extract always the double-precision value, and cast as needed.
// 				// NOTE: here the GMP docs warn that this operation can fail in horrid ways,
// 				// so far never had problems, but if this becomes an issue we can resort to
// 				// the good old lexical casting.
// 				if (input < boost::numeric::bounds<T>::lowest()) {
// 					output = -std::numeric_limits<T>::infinity();
// 				} else if (input > boost::numeric::bounds<T>::highest()) {
// 					output = std::numeric_limits<T>::infinity();
// 				} else {
// 					output = boost::numeric_cast<T>(input.get_d());
// 				}
// 			}
// 		}
// 		// Handle long double separately, not supported by GMP API.
// 		static void mpz_class_convert(long double &output, const mpz_class &input)
// 		{
// 			try {
// 				output = boost::lexical_cast<long double>(input);
// 			} catch (const boost::bad_lexical_cast &) {
// 				// If the conversion fails, it means we are at +-Inf.
// 				piranha_assert(input != 0);
// 				if (input > 0) {
// 					output = std::numeric_limits<long double>::infinity();
// 				} else {
// 					output = -std::numeric_limits<long double>::infinity();
// 				}
// 			}
// 		}
// #ifdef BOOST_HAS_LONG_LONG
// 		// Same for long long.
// 		static void mpz_class_convert(long long &output, const mpz_class &input)
// 		{
// 			output = boost::lexical_cast<long long>(input);
// 		}
// 		static void mpz_class_convert(unsigned long long &output, const mpz_class &input)
// 		{
// 			output = boost::lexical_cast<unsigned long long>(input);
// 		}
// #endif
// 		template <class T>
// 		struct convert_visitor: public boost::static_visitor<T>
// 		{
// 			p_static_check(boost::is_arithmetic<T>::value,"");
// 			static std::string get_err_msg()
// 			{
// 				return std::string("cannot perform requested conversion to type '") +
// 					std::string(typeid(T).name()) + "\'";
// 			}
// 			T operator()(const mpz_class &n) const
// 			{
// 				T retval;
// 				try {
// 					mpz_class_convert(retval,n);
// 				} catch (const boost::numeric::bad_numeric_cast &) {
// 					piranha_throw(value_error,get_err_msg());
// 				} catch (const boost::bad_lexical_cast &) {
// 					piranha_throw(value_error,get_err_msg());
// 				}
// 				return retval;
// 			}
// 			T operator()(const max_fast_int &n) const
// 			{
// 				try {
// 					return boost::numeric_cast<T>(n);
// 				} catch (const boost::numeric::bad_numeric_cast &) {
// 					piranha_throw(value_error,get_err_msg());
// 				}
// 			}
// 		};


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
			assign_from_string(ld_to_string(x).c_str());
		}
		// Conversion.
		template <typename T>
		typename boost::enable_if_c<std::is_integral<T>::value && std::is_signed<T>::value &&
			!std::is_same<T,long long>::value,T>::type convert_to() const
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
			!std::is_same<T,unsigned long long>::value,T>::type convert_to() const
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
				piranha_assert(mpz_cmp_si(m_value,static_cast<long>(0)) != 0);
				if (mpz_cmp_si(m_value,static_cast<long>(0)) > 0) {
					return std::numeric_limits<long double>::infinity();
				} else {
					return -std::numeric_limits<long double>::infinity();
				}
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
		 * @param[in] x arithmetic type used to construct this.
		 */
		template <typename T>
		explicit integer(const T &x, typename boost::enable_if<std::is_arithmetic<T>>::type * = 0)
		{
			construct_from_arithmetic(x);
		}
		/// Constructor from string.
		/**
		 * The string must be a sequence of decimal digits, preceded by a minus sign for
		 * strictly negative numbers. The first digit of a non-zero number must not be zero. A malformed string will throw a piranha::value_error
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
		 * @return reference to this.
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
		 * @return reference to this.
		 */
		integer &operator=(const integer &other)
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
		 * @return reference to this.
		 * 
		 * @throws std::invalid_argument if the string is malformed.
		 */
		integer &operator=(const std::string &str)
		{
			assign_from_string(str.c_str());
			return *this;
		}
		/// Assignment operator from C string.
		/**
		 * @param[in] str string representation of the integer to be assigned.
		 * 
		 * @see operator=(const std::string &)
		 */
		integer &operator=(const char *str)
		{
			assign_from_string(str);
			return *this;
		}
		/// Generic assignment from arithmetic types.
		/**
		 * The supported types for \p T are all arithmetic types.
		 * Use of other types will result in a compile-time error.
		 * In case a floating-point type is used, \p x will be truncated (i.e., rounded towards zero) before being used to construct
		 * the integer object.
		 * 
		 * @param[in] x arithmetic type that will be assigned to this.
		 * 
		 * @return reference to this.
		 */
		template <typename T>
		typename boost::enable_if_c<std::is_arithmetic<T>::value,integer &>::type operator=(const T &x)
		{
			assign_from_arithmetic(x);
			return *this;
		}
		/// Swap.
		/**
		 * Swap the content of this and n.
		 * 
		 * @param[in] n swap argument.
		 */
		void swap(integer &n) piranha_noexcept(true)
		{
			::mpz_swap(m_value,n.m_value);
		}
		/// Conversion operator to arithmetic type.
		/**
		 * Extract an instance of arithmetic type \p T from this.
		 * 
		 * Conversion to integral types is exact, its success depending on whether or not
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
		 * @throws std::overflow_error if the conversion to an integral type results in (negative) overflow.
		 */
		template <typename T>
		explicit operator T() const
		{
			static_assert(std::is_arithmetic<typename std::remove_cv<T>::type>::value,"Cannot convert to non-arithmetic type.");
			return convert_to<typename std::remove_cv<T>::type>();
		}
// 		/// Conversion to bool.
// 		/**
// 		 * This conversion operator returns this != 0, and it can be called implicitly.
// 		 * 
// 		 * @return true if this != 0, false otherwise.
// 		 */
// 		operator bool() const
// 		{
// 			return (*this != 0);
// 		}
// 		/// In-place addition.
// 		/**
// 		 * Add x to the current value of the integer object. This template operator is activated only if
// 		 * T is either integer or an arithmetic type.
// 		 * 
// 		 * If T is integer or an integral type, the result will be exact. If T is a floating-point type the following steps take place:
// 		 * 
// 		 * - this is converted to an instance f of type T via operator T(),
// 		 * - f is added to x,
// 		 * - the result is assigned back to this.
// 		 * 
// 		 * These steps are intended to mimic the behaviour of C++'s integral types.
// 		 * 
// 		 * @param[in] x argument for the addition.
// 		 * 
// 		 * @return reference to this.
// 		 */
// 		template <class T>
// 		typename boost::enable_if_c<boost::is_arithmetic<T>::value || boost::is_same<integer,T>::value,integer &>::type operator+=(const T &x)
// 		{
// 			dispatch_add(x);
// 			return *this;
// 		}
// 		/// Generic in-place addition with integer.
// 		/**
// 		 * Add a piranha::integer in-place. This template operator is activated only if T is an arithmetic type.
// 		 * This method will first compute n + x, then cast the result back to T via static_cast and assign it to x.
// 		 * 
// 		 * @param[in,out] x first argument.
// 		 * @param[in] n second argument.
// 		 * 
// 		 * @return reference to x.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value,T &>::type operator+=(T &x, const integer &n)
// 		{
// 			x = static_cast<T>(n + x);
// 			return x;
// 		}
// 		/// Generic integer addition.
// 		/**
// 		 * This template operator is activated only if T is an arithmetic type or integer.
// 		 * 
// 		 * If T is an integral type, the exact result will be returned as a piranha::integer.
// 		 * 
// 		 * If T is a floating-point type, n will first be converted to T via integer::operator T(), and then
// 		 * added to x. The type of the result will be T.
// 		 * 
// 		 * @param[in] n first argument
// 		 * @param[in] x second argument.
// 		 * 
// 		 * @return n + x.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value || boost::is_same<T,integer>::value,
// 			typename deduce_result_type<T>::type>::type operator+(const integer &n, const T &x)
// 		{
// 			return n.dispatch_operator_plus(x);
// 		}
// 		/// Generic integer addition.
// 		/**
// 		 * Equivalent to n + x. This template operator is activated only if T is an arithmetic type.
// 		 * 
// 		 * @see operator+(const integer &, const T &)
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value,
// 			typename deduce_result_type<T>::type>::type operator+(const T &x, const integer &n)
// 		{
// 			return (n + x);
// 		}
// 		/// Identity operation.
// 		/**
// 		 * @return reference to this.
// 		 */
// 		integer &operator+()
// 		{
// 			return *this;
// 		}
// 		/// Identity operation (const version).
// 		/**
// 		 * @return const reference to this.
// 		 */
// 		const integer &operator+() const
// 		{
// 			return *this;
// 		}
// 		/// Prefix increment.
// 		/**
// 		 * Increment this by one.
// 		 * 
// 		 * @return reference to this after the increment.
// 		 */
// 		integer &operator++()
// 		{
// 			generic_unary_applier(self_increment_visitor());
// 			return *this;
// 		}
// 		/// Suffix increment.
// 		/**
// 		 * Increment this by one and return a copy of this as it was before the increment.
// 		 * 
// 		 * @return copy of this before the increment.
// 		 */
// 		integer operator++(int)
// 		{
// 			const integer retval(*this);
// 			++(*this);
// 			return retval;
// 		}
// 		/// In-place subtraction.
// 		/**
// 		 * The same rules described in operator+=() apply.
// 		 */
// 		template <class T>
// 		typename boost::enable_if_c<boost::is_arithmetic<T>::value || boost::is_same<integer,T>::value,integer &>::type operator-=(const T &x)
// 		{
// 			dispatch_sub(x);
// 			return *this;
// 		}
// 		/// Generic in-place subtraction with integer.
// 		/**
// 		 * The same rules described in operator+=(T &, const integer &) apply.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value,T &>::type operator-=(T &x, const integer &n)
// 		{
// 			x = static_cast<T>(x - n);
// 			return x;
// 		}
// 		/// In-place negation.
// 		/**
// 		 * Set this to -this.
// 		 */
// 		void negate()
// 		{
// 			generic_unary_applier(negate_visitor());
// 		}
// 		/// Negated copy.
// 		/**
// 		 * @return copy of -this.
// 		 */
// 		integer operator-() const
// 		{
// 			integer retval(*this);
// 			retval.negate();
// 			return retval;
// 		}
// 		/// Generic integer subtraction.
// 		/**
// 		 * The same rules described in operator+(const integer &, const T &) apply.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value || boost::is_same<T,integer>::value,
// 			typename deduce_result_type<T>::type>::type operator-(const integer &n, const T &x)
// 		{
// 			return n.dispatch_operator_minus(x);
// 		}
// 		/// Generic integer subtraction.
// 		/**
// 		 * The same rules described in operator+(const T &, const integer &) apply.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value,
// 			typename deduce_result_type<T>::type>::type operator-(const T &x, const integer &n)
// 		{
// 			typename deduce_result_type<T>::type retval(n - x);
// 			// TODO: math namespace.
// 			piranha::negate(retval);
// 			return retval;
// 		}
// 		/// Prefix decrement.
// 		/**
// 		 * Decrement this by one and return.
// 		 * 
// 		 * @return reference to this.
// 		 */
// 		integer &operator--()
// 		{
// 			generic_unary_applier(self_decrement_visitor());
// 			return *this;
// 		}
// 		/// Suffix decrement.
// 		/**
// 		 * Decrement this by one and return a copy of this as it was before the decrement.
// 		 * 
// 		 * @return copy of this before the decrement.
// 		 */
// 		integer operator--(int)
// 		{
// 			const integer retval(*this);
// 			--(*this);
// 			return retval;
// 		}
// 		/// In-place multiplication.
// 		/**
// 		 * The same rules described in operator+=() apply.
// 		 */
// 		template <class T>
// 		typename boost::enable_if_c<boost::is_arithmetic<T>::value || boost::is_same<integer,T>::value,integer &>::type operator*=(const T &x)
// 		{
// 			dispatch_mult(x);
// 			return *this;
// 		}
// 		/// Generic in-place multiplication with integer.
// 		/**
// 		 * The same rules described in operator+=(T &, const integer &) apply.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value,T &>::type operator*=(T &x, const integer &n)
// 		{
// 			x = static_cast<T>(x * n);
// 			return x;
// 		}
// 		/// Generic integer multiplication.
// 		/**
// 		 * The same rules described in operator+(const integer &, const T &) apply.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value || boost::is_same<T,integer>::value,
// 			typename deduce_result_type<T>::type>::type operator*(const integer &n, const T &x)
// 		{
// 			return n.dispatch_operator_mult(x);
// 		}
// 		/// Generic integer multiplication.
// 		/**
// 		 * The same rules described in operator+(const T &, const integer &) apply.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value,
// 			typename deduce_result_type<T>::type>::type operator*(const T &x, const integer &n)
// 		{
// 			return (n * x);
// 		}
// 		/// In-place division.
// 		/**
// 		 * The same rules described in operator+=() apply. Division by integer or by integral type is truncated.
// 		 * Trying to divide by zero will throw a piranha::zero_division_error exception.
// 		 * 
// 		 * @throws piranha::zero_division_error if x is zero.
// 		 */
// 		template <class T>
// 		typename boost::enable_if_c<boost::is_arithmetic<T>::value || boost::is_same<integer,T>::value,integer &>::type operator/=(const T &x)
// 		{
// 			dispatch_divide(x);
// 			return *this;
// 		}
// 		/// Generic in-place division with integer.
// 		/**
// 		 * The same rules described in operator+=(T &, const integer &) apply.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value,T &>::type operator/=(T &x, const integer &n)
// 		{
// 			x = static_cast<T>(x / n);
// 			return x;
// 		}
// 		/// Generic integer division.
// 		/**
// 		 * The same rules described in operator+(const integer &, const T &) apply.
// 		 * Trying to divide by zero will throw a piranha::zero_division_error exception.
// 		 * 
// 		 * @throws piranha::zero_division_error if x is zero.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value || boost::is_same<T,integer>::value,
// 			typename deduce_result_type<T>::type>::type operator/(const integer &n, const T &x)
// 		{
// 			return n.dispatch_operator_divide(x);
// 		}
// 		/// Generic integer division.
// 		/**
// 		 * The same rules described in operator+(const T &, const integer &) apply.
// 		 * Trying to divide by zero will throw a piranha::zero_division_error exception.
// 		 * 
// 		 * @throws piranha::zero_division_error if x is zero.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value,
// 			typename deduce_result_type<T>::type>::type operator/(const T &x, const integer &n)
// 		{
// 			return n.dispatch_operator_divide_left(x);
// 		}
// 		/// In-place modulo operation.
// 		/**
// 		 * Sets this to this % n. This template operator is enabled if T is integer or an integral type.
// 		 * 
// 		 * If n is non-positive or this is negative, a piranha::value_error exception will be thrown.
// 		 * 
// 		 * @param[in] n modulo argument.
// 		 * 
// 		 * @return reference to this.
// 		 * 
// 		 * @throws piranha::value_error if n is non-positive or this is negative.
// 		 */
// 		template <class T>
// 		typename boost::enable_if_c<boost::is_integral<T>::value || boost::is_same<T,integer>::value,integer &>::type operator%=(const T &n)
// 		{
// 			if (n <= 0 || *this < 0) {
// 				piranha_throw(value_error,"invalid argument(s) for modulo operation");
// 			}
// 			dispatch_modulo(n);
// 			return *this;
// 		}
// 		/// Generic in-place modulo operation with integer.
// 		/**
// 		 * This template operator is enabled if T is an integral type.
// 		 * 
// 		 * The same rules described in piranha::integer::operator%=(const T &) apply.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_integral<T>::value,T &>::type
// 			operator%=(T &n1, const integer &n2)
// 		{
// 			n1 = static_cast<T>(n1 % n2);
// 			return n1;
// 		}
// 		/// Generic modulo operation with integer.
// 		/**
// 		 * This template operator is enabled if T is integer or an integral type.
// 		 * 
// 		 * The same rules described in piranha::integer::operator%=(const T &) apply.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_integral<T>::value || boost::is_same<T,integer>::value,integer>::type
// 			operator%(const integer &n1, const T &n2)
// 		{
// 			integer retval(n1);
// 			retval %= n2;
// 			return retval;
// 		}
// 		/// Generic modulo operation with integer.
// 		/**
// 		 * This template operator is enabled if T is an integral type.
// 		 * 
// 		 * The same rules described in piranha::integer::operator%=(const T &) apply.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_integral<T>::value,integer>::type
// 			operator%(const T &n1, const integer &n2)
// 		{
// 			integer retval(n1);
// 			retval %= n2;
// 			return retval;
// 		}
// 		/// Generic integer equality operator.
// 		/**
// 		 * This template operator is activated only if T is an arithmetic type or integer.
// 		 * 
// 		 * If T is an integral type, the comparison will be exact.
// 		 * 
// 		 * If T is a floating-point type, n will be converted to T using piranha::integer::operator T() and then compared to x.
// 		 * 
// 		 * @param[in] n first argument.
// 		 * @param[in] x second argument.
// 		 * 
// 		 * @return true if n == x, false otherwise.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value || boost::is_same<T,integer>::value,
// 			bool>::type operator==(const integer &n, const T &x)
// 		{
// 			return n.dispatch_equality(x);
// 		}
// 		/// Generic integer equality operator.
// 		/**
// 		 * Equivalent to operator==(const integer &, const T &).
// 		 * 
// 		 * This template operator is activated only if T is an arithmetic type.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value,bool>::type operator==(const T &x, const integer &n)
// 		{
// 			return (n == x);
// 		}
// 		/// Generic integer inequality operator.
// 		/**
// 		 * This template operator is activated only if T is an arithmetic type or integer.
// 		 * 
// 		 * Equivalent to the negation of operator==(const integer &, const T &).
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value || boost::is_same<T,integer>::value,
// 			bool>::type operator!=(const integer &n, const T &x)
// 		{
// 			return !(n == x);
// 		}
// 		/// Generic integer inequality operator.
// 		/**
// 		 * This template operator is activated only if T is an arithmetic type.
// 		 * 
// 		 * Equivalent to operator!=(const integer &, const T &).
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value,bool>::type operator!=(const T &x, const integer &n)
// 		{
// 			return (n != x);
// 		}
// 		/// Generic integer less-than operator.
// 		/**
// 		 * This template operator is activated only if T is an arithmetic type or integer.
// 		 * 
// 		 * If T is an integral type, the comparison will be exact.
// 		 * 
// 		 * If T is a floating-point type, n will be converted to T using piranha::integer::operator T() and then compared to x.
// 		 * 
// 		 * @param[in] n first argument.
// 		 * @param[in] x second argument.
// 		 * 
// 		 * @return true if n < x, false otherwise.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value || boost::is_same<T,integer>::value,
// 			bool>::type operator<(const integer &n, const T &x)
// 		{
// 			return n.dispatch_less_than(x);
// 		}
// 		/// Generic integer less-than operator.
// 		/**
// 		 * Equivalent to operator>(const integer &, const T &).
// 		 * 
// 		 * This template operator is activated only if T is an arithmetic type.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value,bool>::type operator<(const T &x, const integer &n)
// 		{
// 			return (n > x);
// 		}
// 		/// Generic integer greater-than operator.
// 		/**
// 		 * The same rules described in operator<(const integer &, const T &) apply.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value || boost::is_same<T,integer>::value,
// 			bool>::type operator>(const integer &n, const T &x)
// 		{
// 			return n.dispatch_greater_than(x);
// 		}
// 		/// Generic integer greater-than operator.
// 		/**
// 		 * Equivalent to operator<(const integer &, const T &).
// 		 * 
// 		 * This template operator is activated only if T is an arithmetic type.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value,bool>::type operator>(const T &x, const integer &n)
// 		{
// 			return (n < x);
// 		}
// 		/// Generic integer greater-than or equal operator.
// 		/**
// 		 * Equivalent to the negation of operator<(const integer &, const T &).
// 		 * 
// 		 * This template operator is activated only if T is an arithmetic type or integer.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value || boost::is_same<T,integer>::value,
// 			bool>::type operator>=(const integer &n, const T &x)
// 		{
// 			return !(n < x);
// 		}
// 		/// Generic integer greater-than or equal operator.
// 		/**
// 		 * Equivalent to operator<=(const integer &, const T &).
// 		 * 
// 		 * This template operator is activated only if T is an arithmetic type.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value,bool>::type operator>=(const T &x, const integer &n)
// 		{
// 			return (n <= x);
// 		}
// 		/// Generic integer less-than or equal operator.
// 		/**
// 		 * Equivalent to the negation of operator>(const integer &, const T &).
// 		 * 
// 		 * This template operator is activated only if T is an arithmetic type or integer.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value || boost::is_same<T,integer>::value,
// 			bool>::type operator<=(const integer &n, const T &x)
// 		{
// 			return !(n > x);
// 		}
// 		/// Generic integer less-than or equal operator.
// 		/**
// 		 * Equivalent to operator>=(const integer &, const T &).
// 		 * 
// 		 * This template operator is activated only if T is an arithmetic type.
// 		 */
// 		template <class T>
// 		friend inline typename boost::enable_if_c<boost::is_arithmetic<T>::value,bool>::type operator<=(const T &x, const integer &n)
// 		{
// 			return (n >= x);
// 		}
// 		/// Combined multiply-add.
// 		/**
// 		 * Sets this to this + (n1 * n2).
// 		 * 
// 		 * @param[in] n1 first argument.
// 		 * @param[in] n2 second argument.
// 		 * 
// 		 * @return reference to this.
// 		 */
// 		integer &multiply_accumulate(const integer &n1, const integer &n2)
// 		{
// 			mpz_class *a = boost::get<mpz_class>(&m_value);
// 			const mpz_class *b = boost::get<mpz_class>(&n1.m_value), *c = boost::get<mpz_class>(&n2.m_value);
// 			// If all the operands are of mpz_class type, then we can use the GMP function.
// 			if (a && b && c) {
// 				mpz_addmul(a->get_mpz_t(),b->get_mpz_t(),c->get_mpz_t());
// 			} else {
// 				*this += n1 * n2;
// 			}
// 			return *this;
// 		}
// 		/// Exponentiation.
// 		/**
// 		 * Return this ** exp. This template method is activated only if T is an arithmetic type or integer.
// 		 * 
// 		 * If T is an integral type or integer, the result will be exact, with negative powers calculated as (1 / this) ** n.
// 		 * 
// 		 * If T is a floating-point type, the result will be exact if exp is an integer, otherwise a piranha::value_error exception will
// 		 * be thrown.
// 		 * 
// 		 * Trying to raise zero to a negative exponent will throw a piranha::zero_division_error exception. this ** 0 will always return 1.
// 		 * 
// 		 * @param[in] exp exponent.
// 		 * 
// 		 * @return this ** exp.
// 		 * 
// 		 * @throws piranha::value_error if T is a floating-point type and exp is not an exact integer.
// 		 * @throws piranha::zero_division_error if this is zero and exp is negative.
// 		 */
// 		template <class T>
// 		typename boost::enable_if_c<boost::is_arithmetic<T>::value || boost::is_same<T,integer>::value,integer>::type pow(const T &exp) const
// 		{
// 			return dispatch_pow(exp);
// 		}






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
 * @see piranha::integer::swap()
 */
template <>
inline void swap(piranha::integer &n1, piranha::integer &n2)
{
	n1.swap(n2);
}

/// Specialization of std::numeric_limits piranha::integer.
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
