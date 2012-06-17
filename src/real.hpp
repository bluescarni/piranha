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
#include <cctype>
#include <cstddef>
#include <iostream>
#include <limits>
#include <mpfr.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "config.hpp"
#include "detail/real_fwd.hpp"
#include "exceptions.hpp"
#include "integer.hpp"
#include "rational.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Arbitrary precision floating-point class.
/**
 * This class represents floating-point ("real") numbers of arbitrary size (i.e., the size is limited only by the available memory).
 * The implementation consists of a C++ wrapper around the \p mpfr_t type from the multiprecision MPFR library. Real numbers
 * are represented in binary format and they consist of an arbitrary-size significand coupled to a fixed-size exponent.
 * 
 * This implementation always uses the \p MPFR_RNDN (round to nearest) rounding mode for all operations.
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
class real
{
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
		// Default rounding mode.
		static const ::mpfr_rnd_t default_rnd = MPFR_RNDN;
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
		void construct_from_generic(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = piranha_nullptr)
		{
			::mpfr_set_d(m_value,static_cast<double>(x),default_rnd);
		}
		template <typename T>
		void construct_from_generic(const T &si, typename std::enable_if<std::is_signed<T>::value && integer::is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			::mpfr_set_si(m_value,static_cast<long>(si),default_rnd);
		}
		template <typename T>
		void construct_from_generic(const T &ui, typename std::enable_if<std::is_unsigned<T>::value && integer::is_gmp_int<T>::value>::type * = piranha_nullptr)
		{
			::mpfr_set_ui(m_value,static_cast<unsigned long>(ui),default_rnd);
		}
		template <typename T>
		void construct_from_generic(const T &ll, typename std::enable_if<std::is_integral<T>::value &&
			!integer::is_gmp_int<T>::value>::type * = piranha_nullptr)
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
				if (std::numeric_limits<T>::has_infinity) {
					return (sign() > 0) ? std::numeric_limits<T>::infinity() : -std::numeric_limits<T>::infinity();
				} else {
					piranha_throw(std::overflow_error,"cannot convert infinity to floating-point type");
				}
			}
			if (std::is_same<T,double>::value) {
				return ::mpfr_get_d(m_value,default_rnd);
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
			mpfr_str_manager m(::mpfr_get_str(piranha_nullptr,&exp,10,0,m_value,default_rnd));
			auto str = m.m_str;
			if (!str) {
				piranha_throw(std::overflow_error,"error in conversion of real to rational: the call to the MPFR function failed");
			}
			// Transform into fraction.
			std::size_t digits = 0u;
			for (; *str != '\0'; ++str) {
				if (std::isdigit(*str)) {
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
	public:
		/// Default significand precision.
		/**
		 * The precision is the number of bits used to represent the significand of a floating-point number.
		 * This default value is equivalent to the IEEE 754 quadruple-precision binary floating-point format.
		 */
		static const ::mpfr_prec_t default_prec = 113;
		/// Default constructor.
		/**
		 * Will initialize the number to zero, using \p default_prec as significand precision.
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
		real(real &&other) piranha_noexcept_spec(true)
		{
			m_value->_mpfr_prec = other.m_value->_mpfr_prec;
			m_value->_mpfr_sign = other.m_value->_mpfr_sign;
			m_value->_mpfr_exp = other.m_value->_mpfr_exp;
			m_value->_mpfr_d = other.m_value->_mpfr_d;
			// Erase other.
			other.m_value->_mpfr_prec = 0;
			other.m_value->_mpfr_sign = 0;
			other.m_value->_mpfr_exp = 0;
			other.m_value->_mpfr_d = piranha_nullptr;
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
		 * First \p this will be initialised with precision \p prec, and then \p other will be assigned to \p this.
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
			std::is_same<T,rational>::value>::type * = piranha_nullptr)
		{
			prec_check(prec);
			::mpfr_init2(m_value,prec);
			construct_from_generic(x);
		}
		/// Destructor.
		/**
		 * Will clear the internal MPFR variable.
		 */
		~real() piranha_noexcept_spec(true)
		{
			static_assert(default_prec >= MPFR_PREC_MIN && default_prec <= MPFR_PREC_MAX,"Invalid value for default precision.");
			// TODO restore.
			//BOOST_CONCEPT_ASSERT((concept::Coefficient<integer>));
			if (m_value->_mpfr_d) {
				::mpfr_clear(m_value);
			} else {
				piranha_assert(!m_value->_mpfr_prec);
				piranha_assert(!m_value->_mpfr_sign);
				piranha_assert(!m_value->_mpfr_exp);
			}
		}
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
		real &operator=(real &&other) piranha_noexcept_spec(true)
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
		template <typename T, typename std::enable_if<integer::is_interop_type<T>::value ||
			std::is_same<T,integer>::value ||
			std::is_same<T,rational>::value>::type*& = enabler>
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
		void swap(real &other) piranha_noexcept_spec(true)
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
			real::mpfr_str_manager m(::mpfr_get_str(piranha_nullptr,&exp,10,0,r.m_value,default_rnd));
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
	private:
		::mpfr_t m_value;
};

}

#endif
