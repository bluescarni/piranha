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

#ifndef PIRANHA_MP_RATIONAL_HPP
#define PIRANHA_MP_RATIONAL_HPP

#include <climits>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>

#include "config.hpp"
#include "detail/mp_rational_fwd.hpp"
#include "exceptions.hpp"
#include "math.hpp"
#include "mp_integer.hpp"

namespace piranha
{

namespace detail
{

// Greatest common divisor using the euclidean algorithm.
template <typename T>
inline T gcd(T a, T b)
{
	while (true) {
		if (math::is_zero(a)) {
			return b;
		}
		b %= a;
		if (math::is_zero(b)) {
			return a;
		}
		a %= b;
	}
}

}

/// Multiple precision rational class.
/**
 * This class encapsulates two instances of piranha::mp_integer to represent an arbitrary-precision rational number
 * in terms of a numerator and a denominator.
 * The meaning of the \p NBits template parameter is the same as in piranha::mp_integer, that is, it represents the
 * bit width of the two limbs stored statically in the numerator and in the denominator.
 * 
 * Unless otherwise specified, rational numbers are always kept in the usual canonical form in which numerator and denominator
 * are coprime, and the denominator is always positive. Zero is uniquely represented by 0/1.
 * 
 * \section interop Interoperability with fundamental types
 * 
 * This class interoperates with the same types as piranha::mp_integer, plus piranha::mp_integer itself.
 * The same caveats with respect to interoperability with floating-point types mentioned in the documentation
 * of piranha::mp_integer apply.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the strong exception safety guarantee for all operations. In case of memory allocation errors by GMP routines,
 * the program will terminate.
 * 
 * \section move_semantics Move semantics
 * 
 * Move construction and move assignment will leave the moved-from object in an unspecified but valid state.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <int NBits = 0>
class mp_rational
{
	public:
		/// The underlying piranha::mp_integer type used to represent numerator and denominator.
		using int_type = mp_integer<NBits>;
	private:
		// Enabler for ctor from num den pair.
		template <typename I0, typename I1>
		using nd_ctor_enabler = typename std::enable_if<(std::is_integral<I0>::value || std::is_same<I0,int_type>::value) &&
			(std::is_integral<I1>::value || std::is_same<I1,int_type>::value)>::type;
		// Enabler for generic ctor.
		template <typename T>
		using generic_ctor_enabler = typename std::enable_if<int_type::template is_interoperable_type<T>::value ||
			std::is_same<T,int_type>::value>::type;
		// Generic constructor implementation.
		template <typename T>
		void construct_from_interoperable(const T &x, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr)
		{
			m_num = int_type(x);
			m_den = 1;
		}
		template <typename T>
		void construct_from_interoperable(const T &x, typename std::enable_if<std::is_same<T,int_type>::value>::type * = nullptr)
		{
			m_num = x;
			m_den = 1;
		}
		template <typename Float>
		void construct_from_interoperable(const Float &x, typename std::enable_if<std::is_floating_point<Float>::value>::type * = nullptr)
		{
			if (unlikely(!std::isfinite(x))) {
				piranha_throw(std::invalid_argument,"cannot construct a rational from a non-finite floating-point number");
			}
			// Denominator is always inited as 1.
			m_den = 1;
			if (x == Float(0)) {
				// m_den is 1 already.
				return;
			}
			Float abs_x = std::abs(x);
			const unsigned radix = static_cast<unsigned>(std::numeric_limits<Float>::radix);
			int_type i_part;
			int exp = std::ilogb(abs_x);
			while (exp >= 0) {
				i_part += int_type(radix).pow(exp);
				const Float tmp = std::scalbn(Float(1),exp);
				if (unlikely(tmp == HUGE_VAL)) {
					piranha_throw(std::invalid_argument,"output of std::scalbn is HUGE_VAL");
				}
				abs_x -= tmp;
				// Break out if x is an exact integer.
				if (abs_x == Float(0)) {
					// m_den is 1 already.
					m_num = i_part;
					return;
				}
				exp = std::ilogb(abs_x);
				if (unlikely(exp == INT_MAX || exp == FP_ILOGBNAN)) {
					piranha_throw(std::invalid_argument,"error calling std::ilogb");
				}
			}
			piranha_assert(abs_x < Float(1));
			// Lift up the decimal part into an integer.
			while (abs_x != Float(0)) {
				abs_x = std::scalbln(abs_x,1);
				if (unlikely(abs_x == HUGE_VAL)) {
					piranha_throw(std::invalid_argument,"output of std::scalbn is HUGE_VAL");
				}
				const auto t_abs_x = std::trunc(abs_x);
				m_den *= radix;
				m_num *= radix;
				// NOTE: here t_abs_x is guaranteed to be in
				// [0,radix - 1], so the cast to unsigned should be ok.
				// Note that floating-point numbers are guaranteed to be able
				// to represent exactly at least a [-exp,+exp] exponent range
				// (see the minimum values for the FLT constants in the C standard).
				m_num += static_cast<unsigned>(t_abs_x);
				abs_x -= t_abs_x;
			}
			math::multiply_accumulate(m_num,i_part,m_den);
			canonicalise();
			if (std::signbit(x)) {
				m_num.negate();
			}
		}
		// Enabler for conversion operator.
		template <typename T>
		using cast_enabler = typename std::enable_if<int_type::template is_interoperable_type<T>::value ||
			std::is_same<T,int_type>::value>::type;
		// Conversion operator implementation.
		template <typename Float>
		Float convert_to_impl(typename std::enable_if<std::is_floating_point<Float>::value>::type * = nullptr) const
		{
			// NOTE: there are better ways of doing this. For instance, here we could generate an inf even
			// if the result is actually representable. It also would be nice if this routine could short-circuit,
			// that is, for every rational generated from a float we get back exactly the same float after the cast.
			// The approach in GMP mpq might work for this, but it's not essential really.
			return static_cast<Float>(m_num) / static_cast<Float>(m_den);
		}
		template <typename Integral>
		Integral convert_to_impl(typename std::enable_if<std::is_integral<Integral>::value>::type * = nullptr) const
		{
			return static_cast<Integral>(static_cast<int_type>(*this));
		}
		template <typename MpInteger>
		MpInteger convert_to_impl(typename std::enable_if<std::is_same<MpInteger,int_type>::value>::type * = nullptr) const
		{
			return m_num / m_den;
		}
	public:
		/// Default constructor.
		/**
		 * This constructor will initialise the rational to zero (that is, the numerator is set to zero, the denominator
		 * to 1).
		 * 
		 * @throws unspecified any exception thrown by the constructor of piranha::mp_integer from \p int.
		 */
		mp_rational(): m_num(),m_den(1) {}
		/// Defaulted copy constructor.
		mp_rational(const mp_rational &) = default;
		/// Defaulted move constructor.
		mp_rational(mp_rational &&) = default;
		/// Constructor from numerator/denominator pair.
		/**
		 * \note
		 * This constructor is enabled only if \p I0 and \p I1 are either integral types or piranha::integer.
		 * 
		 * @param[in] n numerator.
		 * @param[in] d denominator.
		 * 
		 * @throws piranha::zero_division_error if the denominator is zero.
		 * @throws unspecified any exception thrown by the invoked constructor of piranha::mp_integer.
		 */
		template <typename I0, typename I1, typename = nd_ctor_enabler<I0,I1>>
		explicit mp_rational(const I0 &n, const I1 &d):m_num(n),m_den(d)
		{
			if (unlikely(m_den.sign() == 0)) {
				piranha_throw(zero_division_error,"zero denominator in rational");
			}
			canonicalise();
		}
		/// Generic constructor.
		/**
		 * \note
		 * This constructor is enabled only if \p T is an \ref interop "interoperable type".
		 * 
		 * @param[in] x object used to construct \p this.
		 * 
		 * @throws std::invalid_argument if the construction fails (e.g., construction from a non-finite
		 * floating-point value).
		 */
		template <typename T, typename = generic_ctor_enabler<T>>
		explicit mp_rational(const T &x)
		{
			construct_from_interoperable(x);
		}
		/// Destructor.
		~mp_rational()
		{
			// NOTE: no checks no the numerator as we might mess it up
			// with the low-level methods.
			piranha_assert(m_den.sign() > 0);
		}
		/// Defaulted copy assignment operator.
		mp_rational &operator=(const mp_rational &) = default;
		/// Defaulted move assignment operator.
		mp_rational &operator=(mp_rational &&) = default;
		/// Stream operator.
		/**
		 * The printing format is as follows:
		 * - only the numerator is printed if the denominator is 1,
		 * - otherwise, numerator and denominator are printed separated by a '/' sign.
		 * 
		 * @param[in,out] os target stream.
		 * @param[in] q rational to be printed.
		 * 
		 * @return reference to \p os.
		 * 
		 * @throws unspecified any exception thrown by the streaming operator of piranha::mp_integer.
		 */
		friend std::ostream &operator<<(std::ostream &os, const mp_rational &q)
		{
			if (q.m_den == 1) {
				os << q.m_num;
			} else {
				os << q.m_num << '/' << q.m_den;
			}
			return os;
		}
		/// Get const reference to the numerator.
		const int_type &num() const noexcept
		{
			return m_num;
		}
		/// Get const reference to the denominator.
		const int_type &den() const noexcept
		{
			return m_den;
		}
		/// Canonicality check.
		/**
		 * A rational number is in canonical form when numerator and denominator
		 * are coprime. A zero numerator must be paired to a 1 denominator.
		 * 
		 * If low-level methods are not used, this function will always return \p true.
		 * 
		 * @return \p true if \p this is in canonical form, \p false otherwise.
		 */
		bool is_canonical() const noexcept
		{
			// NOTE: here the GCD only involves operations on mp_integers
			// and thus it never throws. The construction from 1 in the comparisons will
			// not throw either.
			const auto gcd = detail::gcd(m_num,m_den);
			return (m_num.sign() != 0 && (gcd == 1 || gcd == -1)) ||
				(m_num.sign() == 0 && m_den == 1);
		}
		/// Canonicalise.
		/**
		 * This method will convert \p this to the canonical form, if needed.
		 * 
		 * @see piranha::mp_rational::is_canonical().
		 */
		void canonicalise() noexcept
		{
			// If the top is null, den must be one.
			if (m_num.sign() == 0) {
				m_den = 1;
				return;
			}
			const int_type gcd = detail::gcd(m_num,m_den);
			// Num and den are coprime already, no need for further divisions.
			if (gcd != 1) {
				m_num /= gcd;
				m_den /= gcd;
			}
			// Fix mismatch in signs.
			if (m_den.sign() == -1) {
				m_num.negate();
				m_den.negate();
			}
		}
		/// Conversion operator.
		/**
		 * \note
		 * This operator is enabled only if \p T is an \ref interop "interoperable type".
		 *
		 * The conversion to piranha::mp_integer is computed by dividing the numerator by the denominator.
		 * The conversion to integral types is computed by casting first to piranha::mp_integer, then to
		 * the target integral type. The conversion to floating-point types might generate non-finite values.
		 *
		 * @return the value of \p this converted to type \p T.
		 *
		 * @throws std::overflow_error if the conversion fails (e.g., the range of the target integral type
		 * is insufficient to represent the value of <tt>this</tt>).
		 */
		template <typename T, typename = cast_enabler<T>>
		explicit operator T() const
		{
			return convert_to_impl<T>();
		}
		/** @name Low-level interface
		 * Low-level methods. These methods allow direct mutable access to numerator and
		 * denominator, and they will not keep the rational in canonical form.
		 */
		//@{
		/// Mutable reference to the numerator.
		/**
		 * @return mutable reference to the numerator.
		 */
		int_type &_num() noexcept
		{
			return m_num;
		}
		/// Set denominator.
	        /**
	         * This method will set the denominator to \p den without canonicalising the rational.
	         * 
	         * @param[in] den desired value for the denominator.
	         * 
	         * @throws std::invalid_argument if \p den is not positive.
	         */
		void _set_den(const int_type &den)
		{
			if (unlikely(den.sign() <= 0)) {
				piranha_throw(std::invalid_argument,"cannot set non-positive denominator in rational");
			}
			m_den = den;
		}
		//@}
	private:
		int_type	m_num;
		int_type	m_den;
};

}

#endif
