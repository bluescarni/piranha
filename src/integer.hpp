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
 * \todo improve interaction with long long via decomposition of operations in long operands
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
