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

#include <boost/functional/hash.hpp>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "config.hpp"
#include "detail/mp_rational_fwd.hpp"
#include "exceptions.hpp"
#include "math.hpp"
#include "mp_integer.hpp"
#include "print_tex_coefficient.hpp"

namespace piranha
{

namespace detail
{

// Greatest common divisor using the euclidean algorithm.
// NOTE: this can yield negative values, depending on the signs
// of a and b.
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

// Fwd declaration.
template <typename>
struct is_mp_rational;

// NOTE: this is a bit complicated: the interoperable types for mp_rational are those from mp_integer
// plus mp_integer, but not *any* mp_integer, just the one whose bits value matches that of the rational.
template <typename T, typename Rational, typename = void>
struct is_mp_rational_interoperable_type
{
	static const bool value = is_mp_integer_interoperable_type<T>::value ||
		std::is_same<T,typename Rational::int_type>::value;
};

// The second complication is that we need to cope with the fact that we are using this tt in a context
// in which Rational might not actually be a rational (in the pow_impl specialsiation). In this case we must prevent
// a hard error to be generated.
template <typename T, typename Rational>
struct is_mp_rational_interoperable_type<T,Rational,typename std::enable_if<!is_mp_rational<Rational>::value>::type>
{
	static const bool value = false;
};

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
 * \section interop Interoperability with other types
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
		// Shortcut for interop type detector.
		template <typename T, typename U = mp_rational>
		using is_interoperable_type = detail::is_mp_rational_interoperable_type<T,U>;
		// Enabler for ctor from num den pair.
		template <typename I0, typename I1>
		using nd_ctor_enabler = typename std::enable_if<(std::is_integral<I0>::value || std::is_same<I0,int_type>::value) &&
			(std::is_integral<I1>::value || std::is_same<I1,int_type>::value),int>::type;
		// Enabler for generic ctor.
		template <typename T>
		using generic_ctor_enabler = typename std::enable_if<is_interoperable_type<T>::value,int>::type;
		// Enabler for in-place arithmetic operations with interop on the left.
		template <typename T>
		using generic_in_place_enabler = typename std::enable_if<is_interoperable_type<T>::value && !std::is_const<T>::value,int>::type;
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
		using cast_enabler = generic_ctor_enabler<T>;
		// Conversion operator implementation.
		template <typename Float>
		Float convert_to_impl(typename std::enable_if<std::is_floating_point<Float>::value>::type * = nullptr) const
		{
			// NOTE: there are better ways of doing this. For instance, here we might end up generating an inf even
			// if the result is actually representable. It also would be nice if this routine could short-circuit,
			// that is, for every rational generated from a float we get back exactly the same float after the cast.
			// The approach in GMP mpq might work for this, but it's not essential at the moment.
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
		// In-place add.
		mp_rational &in_place_add(const mp_rational &other)
		{
			// NOTE: all this should never throw because we only operate on mp_integer objects,
			// no conversions involved, etc.
			// Special casing, the first to deal when other and this are the same object,
			// the second when denominators are equal.
			if (unlikely(&other == this) || m_den == other.m_den) {
				m_num += other.m_num;
			} else {
				m_num *= other.m_den;
				math::multiply_accumulate(m_num,m_den,other.m_num);
				m_den *= other.m_den;
			}
			canonicalise();
			return *this;
		}
		mp_rational &in_place_add(const int_type &other)
		{
			math::multiply_accumulate(m_num,m_den,other);
			canonicalise();
			return *this;
		}
		template <typename T>
		mp_rational &in_place_add(const T &n, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr)
		{
			return in_place_add(int_type(n));
		}
		template <typename T>
		mp_rational &in_place_add(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return (*this = static_cast<T>(*this) + x);
		}
		// Binary add.
		template <typename T>
		static mp_rational binary_plus_impl(const mp_rational &q1, const T &x)
		{
			auto retval(q1);
			retval += x;
			return retval;
		}
		static mp_rational binary_plus(const mp_rational &q1, const mp_rational &q2)
		{
			return binary_plus_impl(q1,q2);
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value || std::is_same<T,int_type>::value,int>::type = 0>
		static mp_rational binary_plus(const mp_rational &q1, const T &x)
		{
			return binary_plus_impl(q1,x);
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value || std::is_same<T,int_type>::value,int>::type = 0>
		static mp_rational binary_plus(const T &x, const mp_rational &q2)
		{
			return binary_plus(q2,x);
		}
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		static T binary_plus(const mp_rational &q1, const T &x)
		{
			return x + static_cast<T>(q1);
		}
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		static T binary_plus(const T &x, const mp_rational &q2)
		{
			return binary_plus(q2,x);
		}
		// In-place sub.
		mp_rational &in_place_sub(const mp_rational &other)
		{
			if (unlikely(&other == this) || m_den == other.m_den) {
				m_num -= other.m_num;
			} else {
				m_num *= other.m_den;
				// Negate temporarily in order to use multiply_accumulate.
				// NOTE: candidate for multiply_sub if we ever implement it.
				m_den.negate();
				math::multiply_accumulate(m_num,m_den,other.m_num);
				m_den.negate();
				m_den *= other.m_den;
			}
			canonicalise();
			return *this;
		}
		mp_rational &in_place_sub(const int_type &other)
		{
			m_den.negate();
			math::multiply_accumulate(m_num,m_den,other);
			m_den.negate();
			canonicalise();
			return *this;
		}
		template <typename T>
		mp_rational &in_place_sub(const T &n, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr)
		{
			return in_place_sub(int_type(n));
		}
		template <typename T>
		mp_rational &in_place_sub(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return (*this = static_cast<T>(*this) - x);
		}
		// Binary sub.
		template <typename T>
		static mp_rational binary_minus_impl(const mp_rational &q1, const T &x)
		{
			auto retval(q1);
			retval -= x;
			return retval;
		}
		static mp_rational binary_minus(const mp_rational &q1, const mp_rational &q2)
		{
			return binary_minus_impl(q1,q2);
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value || std::is_same<T,int_type>::value,int>::type = 0>
		static mp_rational binary_minus(const mp_rational &q1, const T &x)
		{
			return binary_minus_impl(q1,x);
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value || std::is_same<T,int_type>::value,int>::type = 0>
		static mp_rational binary_minus(const T &x, const mp_rational &q2)
		{
			auto retval = binary_minus(q2,x);
			retval.negate();
			return retval;
		}
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		static T binary_minus(const mp_rational &q1, const T &x)
		{
			return static_cast<T>(q1) - x;
		}
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		static T binary_minus(const T &x, const mp_rational &q2)
		{
			return -binary_minus(q2,x);
		}
		// In-place mult.
		mp_rational &in_place_mult(const mp_rational &other)
		{
			// NOTE: no issue here if this and other are the same object.
			m_num *= other.m_num;
			m_den *= other.m_den;
			canonicalise();
			return *this;
		}
		mp_rational &in_place_mult(const int_type &other)
		{
			m_num *= other;
			canonicalise();
			return *this;
		}
		template <typename T>
		mp_rational &in_place_mult(const T &n, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr)
		{
			return in_place_mult(int_type(n));
		}
		template <typename T>
		mp_rational &in_place_mult(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return (*this = static_cast<T>(*this) * x);
		}
		// Binary mult.
		template <typename T>
		static mp_rational binary_mult_impl(const mp_rational &q1, const T &x)
		{
			auto retval(q1);
			retval *= x;
			return retval;
		}
		static mp_rational binary_mult(const mp_rational &q1, const mp_rational &q2)
		{
			return binary_mult_impl(q1,q2);
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value || std::is_same<T,int_type>::value,int>::type = 0>
		static mp_rational binary_mult(const mp_rational &q1, const T &x)
		{
			return binary_mult_impl(q1,x);
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value || std::is_same<T,int_type>::value,int>::type = 0>
		static mp_rational binary_mult(const T &x, const mp_rational &q2)
		{
			return binary_mult(q2,x);
		}
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		static T binary_mult(const mp_rational &q1, const T &x)
		{
			return x * static_cast<T>(q1);
		}
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		static T binary_mult(const T &x, const mp_rational &q2)
		{
			return binary_mult(q2,x);
		}
		// In-place div.
		mp_rational &in_place_div(const mp_rational &other)
		{
			// NOTE: need to do this, otherwise the cross num/dem
			// operations below will mess num and den up. This is ok
			// if this == 0, as this is checked in the outer operator.
			if (unlikely(this == &other)) {
				m_num = 1;
				m_den = 1;
			} else {
				m_num *= other.m_den;
				m_den *= other.m_num;
				canonicalise();
			}
			return *this;
		}
		mp_rational &in_place_div(const int_type &other)
		{
			m_den *= other;
			canonicalise();
			return *this;
		}
		template <typename T>
		mp_rational &in_place_div(const T &n, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr)
		{
			return in_place_div(int_type(n));
		}
		template <typename T>
		mp_rational &in_place_div(const T &x, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
		{
			return (*this = static_cast<T>(*this) / x);
		}
		// Binary div.
		template <typename T>
		static mp_rational binary_div_impl(const mp_rational &q1, const T &x)
		{
			auto retval(q1);
			retval /= x;
			return retval;
		}
		static mp_rational binary_div(const mp_rational &q1, const mp_rational &q2)
		{
			return binary_div_impl(q1,q2);
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value || std::is_same<T,int_type>::value,int>::type = 0>
		static mp_rational binary_div(const mp_rational &q1, const T &x)
		{
			return binary_div_impl(q1,x);
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value || std::is_same<T,int_type>::value,int>::type = 0>
		static mp_rational binary_div(const T &x, const mp_rational &q2)
		{
			mp_rational retval(x);
			retval /= q2;
			return retval;
		}
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		static T binary_div(const mp_rational &q1, const T &x)
		{
			return static_cast<T>(q1) / x;
		}
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		static T binary_div(const T &x, const mp_rational &q2)
		{
			return x / static_cast<T>(q2);
		}
		// Equality operator.
		static bool binary_eq(const mp_rational &q1, const mp_rational &q2)
		{
			return q1.num() == q2.num() && q1.den() == q2.den();
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value || std::is_same<T,int_type>::value,int>::type = 0>
		static bool binary_eq(const mp_rational &q, const T &x)
		{
			return q.den() == 1 && q.num() == x;
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value || std::is_same<T,int_type>::value,int>::type = 0>
		static bool binary_eq(const T &x, const mp_rational &q)
		{
			return binary_eq(q,x);
		}
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		static bool binary_eq(const mp_rational &q, const T &x)
		{
			return static_cast<T>(q) == x;
		}
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		static bool binary_eq(const T &x, const mp_rational &q)
		{
			return binary_eq(q,x);
		}
		// Less-than operator.
		static bool binary_less_than(const mp_rational &q1, const mp_rational &q2)
		{
			// NOTE: this is going to be slow in general. The implementation in GMP
			// checks the limbs number before doing any multiplication, and probably
			// other tricks. If this ever becomes a bottleneck, we probably need to do
			// something similar (actually we could just use the view and piggy-back
			// on mpq_cmp()...).
			return q1.num() * q2.den() < q2.num() * q1.den();
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value || std::is_same<T,int_type>::value,int>::type = 0>
		static bool binary_less_than(const mp_rational &q, const T &x)
		{
			return q.num() < q.den() * x;
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value || std::is_same<T,int_type>::value,int>::type = 0>
		static bool binary_less_than(const T &x, const mp_rational &q)
		{
			return q.den() * x < q.num();
		}
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		static bool binary_less_than(const mp_rational &q, const T &x)
		{
			return static_cast<T>(q) < x;
		}
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		static bool binary_less_than(const T &x, const mp_rational &q)
		{
			return x < static_cast<T>(q);
		}
		// Greater-than operator.
		static bool binary_greater_than(const mp_rational &q1, const mp_rational &q2)
		{
			return q1.num() * q2.den() > q2.num() * q1.den();
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value || std::is_same<T,int_type>::value,int>::type = 0>
		static bool binary_greater_than(const mp_rational &q, const T &x)
		{
			return q.num() > q.den() * x;
		}
		template <typename T, typename std::enable_if<std::is_integral<T>::value || std::is_same<T,int_type>::value,int>::type = 0>
		static bool binary_greater_than(const T &x, const mp_rational &q)
		{
			return q.den() * x > q.num();
		}
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		static bool binary_greater_than(const mp_rational &q, const T &x)
		{
			return static_cast<T>(q) > x;
		}
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value,int>::type = 0>
		static bool binary_greater_than(const T &x, const mp_rational &q)
		{
			return x > static_cast<T>(q);
		}
		// mpq view class.
		class mpq_view
		{
				using mpq_struct_t = std::remove_extent< ::mpq_t>::type;
			public:
				explicit mpq_view(const mp_rational &q):m_n_view(q.num().get_mpz_view()),
					m_d_view(q.den().get_mpz_view())
				{
					// Shallow copy over to m_mpq the data from the views.
					auto n_ptr = m_n_view.get();
					auto d_ptr = m_d_view.get();
					mpq_numref(&m_mpq)->_mp_alloc = n_ptr->_mp_alloc;
					mpq_numref(&m_mpq)->_mp_size = n_ptr->_mp_size;
					mpq_numref(&m_mpq)->_mp_d = n_ptr->_mp_d;
					mpq_denref(&m_mpq)->_mp_alloc = d_ptr->_mp_alloc;
					mpq_denref(&m_mpq)->_mp_size = d_ptr->_mp_size;
					mpq_denref(&m_mpq)->_mp_d = d_ptr->_mp_d;
				}
				mpq_view(const mpq_view &) = delete;
				mpq_view(mpq_view &&) = default;
				mpq_view &operator=(const mpq_view &) = delete;
				mpq_view &operator=(mpq_view &&) = delete;
				operator mpq_struct_t const *() const
				{
					return get();
				}
				mpq_struct_t const *get() const
				{
					return &m_mpq;
				}
			private:
				typename int_type::mpz_view	m_n_view;
				typename int_type::mpz_view	m_d_view;
				mpq_struct_t			m_mpq;
		};
		// Pow enabler.
		template <typename T>
		using pow_enabler = typename std::enable_if<std::is_same<decltype(std::declval<const int_type &>().pow(std::declval<const T &>())),
			decltype(std::declval<const int_type &>().pow(std::declval<const T &>()))>::value,int>::type;
	public:
		/// Default constructor.
		/**
		 * This constructor will initialise the rational to zero (that is, the numerator is set to zero, the denominator
		 * to 1).
		 */
		mp_rational(): m_num(),m_den(1) {}
		/// Defaulted copy constructor.
		mp_rational(const mp_rational &) = default;
		/// Move constructor.
		mp_rational(mp_rational &&other) noexcept : m_num(std::move(other.m_num)),m_den(std::move(other.m_den))
		{
			// Fix the denominator of other, as its state depends on the implementation of piranha::mp_integer.
			// Set it to 1, so other will have a valid canonical state regardless of what happens
			// to the numerator.
			other.m_den = 1;
		}
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
		template <typename I0, typename I1, nd_ctor_enabler<I0,I1> = 0>
		explicit mp_rational(const I0 &n, const I1 &d):m_num(n),m_den(d)
		{
			if (unlikely(m_den.sign() == 0)) {
				piranha_throw(zero_division_error,"zero denominator");
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
		template <typename T, generic_ctor_enabler<T> = 0>
		explicit mp_rational(const T &x)
		{
			construct_from_interoperable(x);
		}
		/// Constructor from C string.
		/**
		 * The string must represent either a valid single piranha::mp_integer, or two valid piranha::mp_integer
		 * separated by "/". The rational will be put in canonical form by this constructor.
		 * 
		 * Note that if the string is not null-terminated, undefined behaviour will occur.
		 * 
		 * @param[in] str C string used for construction.
		 * 
		 * @throws std::invalid_argument if the string is not formatted correctly.
		 * @throws piranha::zero_division_error if the denominator, if present, is zero.
		 * @throws unspecified any exception thrown by the constructor from string of piranha::mp_integer
		 * or by memory errors in \p std::string.
		 */
		explicit mp_rational(const char *str):m_num(),m_den(1)
		{
			// String validation.
			auto ptr = str;
			std::size_t num_size = 0u;
			while (*ptr != '\0' && *ptr != '/') {
				++num_size;
				++ptr;
			}
			try {
				int_type::validate_string(str,num_size);
				if (*ptr == '/') {
					int_type::validate_string(ptr + 1u,std::strlen(ptr + 1u));
				}
			} catch (...) {
				piranha_throw(std::invalid_argument,"invalid string input for rational type");
			}
			// String is ok, proceed with construction.
			m_num = int_type(std::string(str,str + num_size));
			if (*ptr == '/') {
				m_den = int_type(std::string(ptr + 1u));
				if (unlikely(math::is_zero(m_den))) {
					piranha_throw(zero_division_error,"zero denominator");
				}
				canonicalise();
			}
		}
		/// Constructor from C++ string.
		/**
		 * Equivalent to the constructor from C string.
		 * 
		 * @param[in] str C string used for construction.
		 * 
		 * @throws unspecified any exception thrown by the constructor from C string.
		 */
		explicit mp_rational(const std::string &str):mp_rational(str.c_str()) {}
		/// Destructor.
		~mp_rational()
		{
			// NOTE: no checks no the numerator as we might mess it up
			// with the low-level methods.
			piranha_assert(m_den.sign() > 0);
		}
		/// Defaulted copy assignment operator.
		mp_rational &operator=(const mp_rational &) = default;
		/// Move assignment operator.
		mp_rational &operator=(mp_rational &&other) noexcept
		{
			if (unlikely(this == &other)) {
				return *this;
			}
			m_num = std::move(other.m_num);
			m_den = std::move(other.m_den);
			// See comments in the move ctor.
			other.m_den = 1;
			return *this;
		}
		/// Generic assignment operator.
		/**
		 * \note
		 * This assignment operator is enabled only if \p T is an \ref interop "interoperable type".
		 *
		 * This operator will construct a temporary piranha::mp_rational from \p x and will then move-assign it
		 * to \p this.
		 *
		 * @param[in] x assignment target.
		 *
		 * @return reference to \p this.
		 *
		 * @throws unspecified any exception thrown by the generic constructor from interoperable type.
		 */
		template <typename T, generic_ctor_enabler<T> = 0>
		mp_rational &operator=(const T &x)
		{
			return (*this = mp_rational(x));
		}
		/// Assignment operator from C string.
		/**
		 * This assignment operator will construct a piranha::mp_rational from the string \p str
		 * and will then move-assign the result to \p this.
		 *
		 * @param[in] str C string.
		 *
		 * @return reference to \p this.
		 *
		 * @throws unspecified any exception thrown by the constructor from string.
		 */
		mp_rational &operator=(const char *str)
		{
			return (*this = mp_rational(str));
		}
		/// Assignment operator from C++ string.
		/**
		 * This assignment operator will construct a piranha::mp_rational from the string \p str
		 * and will then move-assign the result to \p this.
		 *
		 * @param[in] str C++ string.
		 *
		 * @return reference to \p this.
		 *
		 * @throws unspecified any exception thrown by the constructor from string.
		 */
		mp_rational &operator=(const std::string &str)
		{
			return (*this = str.c_str());
		}
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
		/// Overload input stream operator for piranha::mp_rational.
		/**
		 * Equivalent to extracting a line from the stream and then assigning it to \p q.
		 *
		 * @param[in] is input stream.
		 * @param[in,out] q rational to which the contents of the stream will be assigned.
		 *
		 * @return reference to \p is.
		 *
		 * @throws unspecified any exception thrown by the constructor from string of piranha::mp_rational.
		 */
		friend std::istream &operator>>(std::istream &is, mp_rational &q)
		{
			std::string tmp_str;
			std::getline(is,tmp_str);
			q = tmp_str;
			return is;
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
		/// Get an \p mpq view of \p this.
		/**
		 * This method will return an object of an unspecified type \p mpq_view which is implicitly convertible
		 * to a const pointer to an \p mpq struct (and which can thus be used as a <tt>const mpq_t</tt>
		 * parameter in GMP functions). In addition to the implicit conversion operator, the \p mpq struct pointer
		 * can also be retrieved via the <tt>get()</tt> method of the \p mpq_view class.
		 * The pointee will represent a GMP rational whose value is equal to \p this.
		 *
		 * Note that the returned \p mpq_view instance can only be move-constructed (the other constructors and the assignment operators
		 * are disabled). Additionally, the returned object and the pointer might reference internal data belonging to
		 * \p this, and they can thus be used safely only during the lifetime of \p this.
		 * Any modification to \p this will also invalidate the view and the pointer.
		 *
		 * @return an \p mpq view of \p this.
		 */
		mpq_view get_mpq_view() const
		{
			return mpq_view{*this};
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
			// NOTE: there should be no way to set a negative denominator, so no check is performed.
			// The condition is checked in the dtor.
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
			if (math::is_zero(m_num)) {
				m_den = 1;
				return;
			}
			// NOTE: here we can avoid the further division by gcd if it is one or -one.
			// Consider this as a possible optimisation in the future.
			const int_type gcd = detail::gcd(m_num,m_den);
			piranha_assert(!math::is_zero(gcd));
			m_num /= gcd;
			m_den /= gcd;
			// Fix mismatch in signs.
			if (m_den.sign() == -1) {
				m_num.negate();
				m_den.negate();
			}
			// NOTE: this could be a nice place to use the demote() method of mp_integer.
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
		template <typename T, cast_enabler<T> = 0>
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
		/// Identity operator.
		/**
		 * @return a copy of \p this.
		 */
		mp_rational operator+() const
		{
			return mp_rational{*this};
		}
		/// Pre-increment operator.
		/**
		 * @return reference to \p this after the increment.
		 * 
		 * @throws unspecified any exception thrown by in-place addition.
		 */
		mp_rational &operator++()
		{
			return operator+=(1);
		}
		/// Post-increment operator.
		/**
		 * @return copy of \p this before the increment.
		 * 
		 * @throws unspecified any exception thrown by the pre-increment operator.
		 */
		mp_rational operator++(int)
		{
			const mp_rational retval(*this);
			++(*this);
			return retval;
		}
		/// In-place addition.
		/**
		 * \note
		 * This operator is enabled only if \p T is an \ref interop "interoperable type" or piranha::mp_rational.
		 * 
		 * If \p T is not a float, the exact result will be computed. If \p T is a floating-point type, the following
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
		 * @throws unspecified any exception thrown by the conversion operator, the generic constructor of piranha::mp_integer,
		 * or the generic assignment operator, if used.
		 */
		template <typename T>
		auto operator+=(const T &x) -> decltype(this->in_place_add(x))
		{
			return in_place_add(x);
		}
		/// Generic in-place addition with piranha::mp_rational.
		/**
		 * \note
		 * This operator is enabled only if \p T is a non-const \ref interop "interoperable type".
		 * 
		 * Add a piranha::mp_rational in-place. This method will first compute <tt>q + x</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] q second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception thrown by the binary operator or by casting piranha::mp_rational to \p T.
		 */
		template <typename T, generic_in_place_enabler<T> = 0>
		friend T &operator+=(T &x, const mp_rational &q)
		{
			return x = static_cast<T>(q + x);
		}
		/// Generic binary addition involving piranha::mp_rational.
		/**
		 * \note
		 * This template operator is enabled only if either:
		 * - \p T is piranha::mp_rational and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::mp_rational and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::mp_rational.
		 * 
		 * If no floating-point types are involved, the exact result of the operation will be returned as a piranha::mp_rational.
		 * 
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and added to \p f to generate the return value, which will then be of type \p F.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x + y</tt>.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the corresponding in-place operator,
		 * - the invoked constructor or the conversion operator, if used.
		 */
		template <typename T, typename U>
		friend auto operator+(const T &x, const U &y) -> decltype(mp_rational::binary_plus(x,y))
		{
			return mp_rational::binary_plus(x,y);
		}
		/// Negate in-place.
		void negate() noexcept
		{
			m_num.negate();
		}
		/// Negated copy.
		/**
		 * @return a negated copy of \p this.
		 */
		mp_rational operator-() const
		{
			mp_rational retval(*this);
			retval.negate();
			return retval;
		}
		/// Pre-decrement operator.
		/**
		 * @return reference to \p this after the decrement.
		 * 
		 * @throws unspecified any exception thrown by in-place subtraction.
		 */
		mp_rational &operator--()
		{
			return operator-=(1);
		}
		/// Post-decrement operator.
		/**
		 * @return copy of \p this before the decrement.
		 * 
		 * @throws unspecified any exception thrown by the pre-decrement operator.
		 */
		mp_rational operator--(int)
		{
			const mp_rational retval(*this);
			--(*this);
			return retval;
		}
		/// In-place subtraction.
		/**
		 * \note
		 * This operator is enabled only if \p T is an \ref interop "interoperable type" or piranha::mp_rational.
		 * 
		 * If \p T is not a float, the exact result will be computed. If \p T is a floating-point type, the following
		 * sequence of operations takes place:
		 * 
		 * - \p this is converted to an instance \p f of type \p T via the conversion operator,
		 * - \p x is subtracted from \p f,
		 * - the result is assigned back to \p this.
		 * 
		 * @param[in] x argument for the subtraction.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the conversion operator, the generic constructor of piranha::mp_integer,
		 * or the generic assignment operator, if used.
		 */
		template <typename T>
		auto operator-=(const T &x) -> decltype(this->in_place_sub(x))
		{
			return in_place_sub(x);
		}
		/// Generic in-place subtraction with piranha::mp_rational.
		/**
		 * \note
		 * This operator is enabled only if \p T is a non-const \ref interop "interoperable type".
		 * 
		 * Subtract a piranha::mp_rational in-place. This method will first compute <tt>x - q</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] q second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception thrown by the binary operator or by casting piranha::mp_rational to \p T.
		 */
		template <typename T, generic_in_place_enabler<T> = 0>
		friend T &operator-=(T &x, const mp_rational &q)
		{
			return x = static_cast<T>(x - q);
		}
		/// Generic binary subtraction involving piranha::mp_rational.
		/**
		 * \note
		 * This template operator is enabled only if either:
		 * - \p T is piranha::mp_rational and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::mp_rational and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::mp_rational.
		 * 
		 * If no floating-point types are involved, the exact result of the operation will be returned as a piranha::mp_rational.
		 * 
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and subtracted from (or to) \p f to generate the return value, which will then be of type \p F.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x - y</tt>.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the corresponding in-place operator,
		 * - the invoked constructor or the conversion operator, if used.
		 */
		template <typename T, typename U>
		friend auto operator-(const T &x, const U &y) -> decltype(mp_rational::binary_minus(x,y))
		{
			return mp_rational::binary_minus(x,y);
		}
		/// In-place multiplication.
		/**
		 * \note
		 * This operator is enabled only if \p T is an \ref interop "interoperable type" or piranha::mp_rational.
		 * 
		 * If \p T is not a float, the exact result will be computed. If \p T is a floating-point type, the following
		 * sequence of operations takes place:
		 * 
		 * - \p this is converted to an instance \p f of type \p T via the conversion operator,
		 * - \p f is multiplied by \p x,
		 * - the result is assigned back to \p this.
		 * 
		 * @param[in] x argument for the multiplication.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the conversion operator, the generic constructor of piranha::mp_integer,
		 * or the generic assignment operator, if used.
		 */
		template <typename T>
		auto operator*=(const T &x) -> decltype(this->in_place_mult(x))
		{
			return in_place_mult(x);
		}
		/// Generic in-place multiplication with piranha::mp_rational.
		/**
		 * \note
		 * This operator is enabled only if \p T is a non-const \ref interop "interoperable type".
		 * 
		 * Multiply by a piranha::mp_rational in-place. This method will first compute <tt>x * q</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] q second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception thrown by the binary operator or by casting piranha::mp_rational to \p T.
		 */
		template <typename T, generic_in_place_enabler<T> = 0>
		friend T &operator*=(T &x, const mp_rational &q)
		{
			return x = static_cast<T>(x * q);
		}
		/// Generic binary multiplication involving piranha::mp_rational.
		/**
		 * \note
		 * This template operator is enabled only if either:
		 * - \p T is piranha::mp_rational and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::mp_rational and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::mp_rational.
		 * 
		 * If no floating-point types are involved, the exact result of the operation will be returned as a piranha::mp_rational.
		 * 
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and multiplied by \p f to generate the return value, which will then be of type \p F.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x * y</tt>.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the corresponding in-place operator,
		 * - the invoked constructor or the conversion operator, if used.
		 */
		template <typename T, typename U>
		friend auto operator*(const T &x, const U &y) -> decltype(mp_rational::binary_mult(x,y))
		{
			return mp_rational::binary_mult(x,y);
		}
		/// In-place division.
		/**
		 * \note
		 * This operator is enabled only if \p T is an \ref interop "interoperable type" or piranha::mp_rational.
		 * 
		 * If \p T is not a float, the exact result will be computed. If \p T is a floating-point type, the following
		 * sequence of operations takes place:
		 * 
		 * - \p this is converted to an instance \p f of type \p T via the conversion operator,
		 * - \p f is divided by \p x,
		 * - the result is assigned back to \p this.
		 * 
		 * @param[in] x argument for the division.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws piranha::zero_division_error if \p x is zero.
		 * @throws unspecified any exception thrown by the conversion operator, the generic constructor of piranha::mp_integer,
		 * or the generic assignment operator, if used.
		 */
		template <typename T>
		auto operator/=(const T &x) -> decltype(this->in_place_div(x))
		{
			if (unlikely(math::is_zero(x))) {
				piranha_throw(zero_division_error,"division of a rational by zero");
			}
			return in_place_div(x);
		}
		/// Generic in-place division with piranha::mp_rational.
		/**
		 * \note
		 * This operator is enabled only if \p T is a non-const \ref interop "interoperable type".
		 * 
		 * Divide by a piranha::mp_rational in-place. This method will first compute <tt>x / q</tt>, cast it back to \p T via \p static_cast and finally assign the result to \p x.
		 * 
		 * @param[in,out] x first argument.
		 * @param[in] q second argument.
		 * 
		 * @return reference to \p x.
		 * 
		 * @throws unspecified any exception thrown by the binary operator or by casting piranha::mp_rational to \p T.
		 */
		template <typename T, generic_in_place_enabler<T> = 0>
		friend T &operator/=(T &x, const mp_rational &q)
		{
			return x = static_cast<T>(x / q);
		}
		/// Generic binary division involving piranha::mp_rational.
		/**
		 * \note
		 * This template operator is enabled only if either:
		 * - \p T is piranha::mp_rational and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::mp_rational and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::mp_rational.
		 * 
		 * If no floating-point types are involved, the exact result of the operation will be returned as a piranha::mp_rational.
		 * 
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and divided by \p f (or viceversa) to generate the return value, which will then be of type \p F.
		 * 
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 * 
		 * @return <tt>x / y</tt>.
		 * 
		 * @throws piranha::zero_division_error in case of division by zero.
		 * @throws unspecified any exception thrown by:
		 * - the corresponding in-place operator,
		 * - the invoked constructor or the conversion operator, if used.
		 */
		template <typename T, typename U>
		friend auto operator/(const T &x, const U &y) -> decltype(mp_rational::binary_div(x,y))
		{
			return mp_rational::binary_div(x,y);
		}
		/// Generic equality operator involving piranha::mp_rational.
		/**
		 * \note
		 * This template operator is enabled only if either:
		 * - \p T is piranha::mp_rational and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::mp_rational and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::mp_rational.
		 *
		 * If no floating-point types are involved, the exact result of the comparison will be returned.
		 *
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and compared to \p f.
		 *
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 *
		 * @return \p true if <tt>x == y</tt>, \p false otherwise.
		 *
		 * @throws unspecified any exception thrown by:
		 * - the comparison operator of piranha::mp_integer,
		 * - the invoked conversion operator, if used.
		 */
		template <typename T, typename U>
		friend auto operator==(const T &x, const U &y) -> decltype(mp_rational::binary_eq(x,y))
		{
			return mp_rational::binary_eq(x,y);
		}
		/// Generic inequality operator involving piranha::mp_rational.
		/**
		 * \note
		 * This template operator is enabled only if either:
		 * - \p T is piranha::mp_rational and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::mp_rational and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::mp_rational.
		 *
		 * If no floating-point types are involved, the exact result of the comparison will be returned.
		 *
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and compared to \p f.
		 *
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 *
		 * @return \p true if <tt>x != y</tt>, \p false otherwise.
		 *
		 * @throws unspecified any exception thrown by the equality operator.
		 */
		template <typename T, typename U>
		friend auto operator!=(const T &x, const U &y) -> decltype(!mp_rational::binary_eq(x,y))
		{
			return !mp_rational::binary_eq(x,y);
		}
		/// Generic less-than operator involving piranha::mp_rational.
		/**
		 * \note
		 * This template operator is enabled only if either:
		 * - \p T is piranha::mp_rational and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::mp_rational and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::mp_rational.
		 *
		 * If no floating-point types are involved, the exact result of the comparison will be returned.
		 *
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and compared to \p f.
		 *
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 *
		 * @return \p true if <tt>x < y</tt>, \p false otherwise.
		 *
		 * @throws unspecified any exception thrown by:
		 * - the less-than operator of piranha::mp_integer,
		 * - the invoked conversion operator, if used.
		 */
		template <typename T, typename U>
		friend auto operator<(const T &x, const U &y) -> decltype(mp_rational::binary_less_than(x,y))
		{
			return mp_rational::binary_less_than(x,y);
		}
		/// Generic greater-than or equal operator involving piranha::mp_rational.
		/**
		 * \note
		 * This template operator is enabled only if either:
		 * - \p T is piranha::mp_rational and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::mp_rational and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::mp_rational.
		 *
		 * If no floating-point types are involved, the exact result of the comparison will be returned.
		 *
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and compared to \p f.
		 *
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 *
		 * @return \p true if <tt>x >= y</tt>, \p false otherwise.
		 *
		 * @throws unspecified any exception thrown by the less-than operator.
		 */
		template <typename T, typename U>
		friend auto operator>=(const T &x, const U &y) -> decltype(!mp_rational::binary_less_than(x,y))
		{
			return !mp_rational::binary_less_than(x,y);
		}
		/// Generic greater-than operator involving piranha::mp_rational.
		/**
		 * \note
		 * This template operator is enabled only if either:
		 * - \p T is piranha::mp_rational and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::mp_rational and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::mp_rational.
		 *
		 * If no floating-point types are involved, the exact result of the comparison will be returned.
		 *
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and compared to \p f.
		 *
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 *
		 * @return \p true if <tt>x > y</tt>, \p false otherwise.
		 *
		 * @throws unspecified any exception thrown by:
		 * - the greater-than operator of piranha::mp_integer,
		 * - the invoked conversion operator, if used.
		 */
		template <typename T, typename U>
		friend auto operator>(const T &x, const U &y) -> decltype(mp_rational::binary_greater_than(x,y))
		{
			return mp_rational::binary_greater_than(x,y);
		}
		/// Generic less-than or equal operator involving piranha::mp_rational.
		/**
		 * \note
		 * This template operator is enabled only if either:
		 * - \p T is piranha::mp_rational and \p U is an \ref interop "interoperable type",
		 * - \p U is piranha::mp_rational and \p T is an \ref interop "interoperable type",
		 * - both \p T and \p U are piranha::mp_rational.
		 *
		 * If no floating-point types are involved, the exact result of the comparison will be returned.
		 *
		 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
		 * and compared to \p f.
		 *
		 * @param[in] x first argument
		 * @param[in] y second argument.
		 *
		 * @return \p true if <tt>x <= y</tt>, \p false otherwise.
		 *
		 * @throws unspecified any exception thrown by the greater-than operator.
		 */
		template <typename T, typename U>
		friend auto operator<=(const T &x, const U &y) -> decltype(!mp_rational::binary_greater_than(x,y))
		{
			return !mp_rational::binary_greater_than(x,y);
		}
		/// Exponentiation.
		/**
		 * \note
		 * This method is enabled only if piranha::mp_integer::pow() can be called with an argument of type \p T.
		 *
		 * This method computes \p this raised to the integral power \p exp. Internally, the piranha::mp_integer::pow()
		 * method of numerator and denominator is used. Negative powers will raise an error if the numerator of \p this
		 * is zero.
		 *
		 * @param[in] exp exponent.
		 *
		 * @return <tt>this ** exp</tt>.
		 *
		 * @throws piranha::zero_division_error if \p exp is negative and the numerator of \p this is zero.
		 * @throws unspecified any exception thrown by piranha::mp_integer::pow().
		 */
		template <typename T, pow_enabler<T> = 0>
		mp_rational pow(const T &exp) const
		{
			mp_rational retval;
			if (exp >= T(0)) {
				// For non-negative exponents, we can just raw-construct
				// a rational value.
				// NOTE: in case of exceptions here we are good, the worst that can happen
				// is that the numerator has some value and den is still 1 from the initialisation.
				retval.m_num = num().pow(exp);
				retval.m_den = den().pow(exp);
			} else {
				if (unlikely(math::is_zero(num()))) {
					piranha_throw(zero_division_error,"zero denominator in rational exponentiation");
				}
				// For negative exponents, invert.
				const int_type n_exp = -int_type(exp);
				// NOTE: exception safe here as well.
				retval.m_num = den().pow(n_exp);
				retval.m_den = num().pow(n_exp);
				if (retval.m_den.sign() < 0) {
					retval.m_num.negate();
					retval.m_den.negate();
				}
			}
			return retval;
		}
		/// Absolute value.
		/**
		 * @return absolute value of \p this.
		 */
		mp_rational abs() const
		{
			mp_rational retval{*this};
			if (retval.m_num.sign() < 0) {
				retval.m_num.negate();
			}
			return retval;
		}
		/// Hash value.
		/**
		 * The hash value is calculated by combining the hash values of numerator and denominator.
		 *
		 * @return a hash value for this.
		 */
		std::size_t hash() const noexcept
		{
			std::size_t retval = m_num.hash();
			boost::hash_combine(retval,m_den.hash());
			return retval;
		}
		/// Binomial coefficient.
		/**
		 * \note
		 * This method is enabled only if \p T is an integral type or piranha::mp_integer.
		 *
		 * Will return \p this choose \p n.
		 *
		 * @param[in] n bottom argument for the binomial coefficient.
		 *
		 * @return \p this choose \p n.
		 *
		 * @throws unspecified any exception thrown by piranha::mp_integer::binomial()
		 * or by arithmetic operations on piranha::mp_rational.
		 */
		template <typename T, typename std::enable_if<std::is_integral<T>::value ||
			std::is_same<T,int_type>::value,int>::type = 0>
		mp_rational binomial(const T &n) const
		{
			if (m_den == 1) {
				// If this is an integer, offload to mp_integer::binomial().
				return mp_rational{m_num.binomial(n),1};
			}
			if (n < T(0)) {
				// (rational negative-int) will always give zero.
				return mp_rational{};
			}
			// (rational non-negative-int) uses the generic implementation.
			// NOTE: this is going to be really slow, it can be improved by orders
			// of magnitude.
			return detail::generic_binomial(*this,n);
		}
	private:
		int_type	m_num;
		int_type	m_den;
};

/// Alias for piranha::mp_rational with default bit size.
using rational = mp_rational<>;

inline namespace literals
{

/// Literal for arbitrary-precision rationals.
/**
 * @param[in] s literal string.
 * 
 * @return a piranha::mp_rational constructed from \p s.
 * 
 * @throws unspecified any exception thrown by the constructor of
 * piranha::mp_rational from string.
 */
inline rational operator "" _q(const char *s)
{
	return rational(s);
}

}

namespace detail
{

// Temporary TMP structure to detect mp_rational types.
// Should be replaced by is_instance_of once (or if) we move
// from NBits to an integral_constant for selecting limb
// size in mp_integer.
template <typename T>
struct is_mp_rational: std::false_type {};

template <int NBits>
struct is_mp_rational<mp_rational<NBits>>: std::true_type {};

template <typename T, typename U>
using rational_pow_enabler = typename std::enable_if<
	(is_mp_rational<T>::value && is_mp_rational_interoperable_type<U,T>::value) ||
	(is_mp_rational<U>::value && is_mp_rational_interoperable_type<T,U>::value) ||
	// NOTE: here we are catching two rational arguments with potentially different
	// bits. BUT this case is not caught in the pow_impl, so we should be ok as long
	// as we don't allow interoperablity with different bits.
	(is_mp_rational<T>::value && is_mp_rational<U>::value)
>::type;

// Binomial follows the same rules as pow.
template <typename T, typename U>
using rational_binomial_enabler = rational_pow_enabler<T,U>;

}

/// Specialisation of the piranha::print_tex_coefficient() functor for piranha::mp_rational.
template <typename T>
struct print_tex_coefficient_impl<T,typename std::enable_if<detail::is_mp_rational<T>::value>::type>
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
		if (math::is_zero(cf.num())) {
			os << "0";
			return;
		}
		if (cf.den() == 1) {
			os << cf.num();
			return;
		}
		auto num = cf.num();
		if (num.sign() < 0) {
			os << "-";
			num.negate();
		}
		os << "\\frac{" << num << "}{" << cf.den() << "}";
	}
};

namespace math
{

/// Specialisation of the piranha::math::is_zero() functor for piranha::mp_rational.
/**
 * This specialisation is enabled when \p T is an instance of piranha::mp_rational.
 */
template <typename T>
struct is_zero_impl<T,typename std::enable_if<detail::is_mp_rational<T>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] q piranha::mp_rational to be tested.
	 * 
	 * @return \p true if \p q is zero, \p false otherwise.
	 */
	bool operator()(const T &q) const noexcept
	{
		return is_zero(q.num());
	}
};

/// Specialisation of the piranha::math::negate() functor for piranha::mp_rational.
template <typename T>
struct negate_impl<T,typename std::enable_if<detail::is_mp_rational<T>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in,out] q piranha::mp_rational to be negated.
	 */
	void operator()(T &q) const
	{
		q.negate();
	}
};

///// Specialisation of the piranha::math::pow() functor for piranha::mp_rational.
/**
 * This specialisation is activated when one of the arguments is piranha::mp_rational
 * and the other is either piranha::mp_rational or an interoperable type for piranha::mp_rational.
 *
 * The implementation follows these rules:
 * - if the base is rational and the exponent an integral type or piranha::mp_integer, then
 *   piranha::mp_rational::pow() is used;
 * - if the non-rational argument is a floating-point type, then the rational argument is converted
 *   to that floating-point type and piranha::math::pow() is used;
 * - if both arguments are rational, they are both converted to \p double and then piranha::math::pow()
 *   is used;
 * - if the base is an integral type or piranha::mp_integer and the exponent a rational, then both
 *   arguments are converted to \p double and piranha::math::pow() is used.
 */
template <typename T, typename U>
struct pow_impl<T,U,detail::rational_pow_enabler<T,U>>
{
	/// Call operator, rational--integral overload.
	/**
	 * @param[in] b base.
	 * @param[in] e exponent.
	 *
	 * @returns <tt>b**e</tt>.
	 *
	 * @throws unspecified any exception thrown by piranha::mp_rational::pow().
	 */
	template <int NBits, typename T2>
	auto operator()(const mp_rational<NBits> &b, const T2 &e) const -> decltype(b.pow(e))
	{
		return b.pow(e);
	}
	/// Call operator, rational--floating-point overload.
	/**
	 * @param[in] b base.
	 * @param[in] e exponent.
	 *
	 * @returns <tt>b**e</tt>.
	 *
	 * @throws unspecified any exception thrown by converting piranha::mp_rational
	 * to a floating-point type.
	 */
	template <int NBits, typename T2, typename std::enable_if<std::is_floating_point<T2>::value,int>::type = 0>
	T2 operator()(const mp_rational<NBits> &b, const T2 &e) const
	{
		return math::pow(static_cast<T2>(b),e);
	}
	/// Call operator, floating-point--rational overload.
	/**
	 * @param[in] b base.
	 * @param[in] e exponent.
	 *
	 * @returns <tt>b**e</tt>.
	 *
	 * @throws unspecified any exception thrown by converting piranha::mp_rational
	 * to a floating-point type.
	 */
	template <int NBits, typename T2, typename std::enable_if<std::is_floating_point<T2>::value,int>::type = 0>
	T2 operator()(const T2 &e, const mp_rational<NBits> &b) const
	{
		return math::pow(e,static_cast<T2>(b));
	}
	/// Call operator, rational--rational overload.
	/**
	 * @param[in] b base.
	 * @param[in] e exponent.
	 *
	 * @returns <tt>b**e</tt>.
	 *
	 * @throws unspecified any exception thrown by converting piranha::mp_rational
	 * to \p double.
	 */
	template <int NBits>
	double operator()(const mp_rational<NBits> &b, const mp_rational<NBits> &e) const
	{
		return math::pow(static_cast<double>(e),static_cast<double>(b));
	}
	/// Call operator, integral--rational overload.
	/**
	 * @param[in] b base.
	 * @param[in] e exponent.
	 *
	 * @returns <tt>b**e</tt>.
	 *
	 * @throws unspecified any exception thrown by converting piranha::mp_rational
	 * or piranha::mp_integer to \p double.
	 */
	template <int NBits, typename T2, typename std::enable_if<std::is_integral<T2>::value || detail::is_mp_integer<T2>::value,int>::type = 0>
	double operator()(const T2 &b, const mp_rational<NBits> &e) const
	{
		return math::pow(static_cast<double>(b),static_cast<double>(e));
	}
};

/// Specialisation of the piranha::math::sin() functor for piranha::mp_rational.
template <typename T>
struct sin_impl<T,typename std::enable_if<detail::is_mp_rational<T>::value>::type>
{
	/// Call operator.
	/**
	 * The argument will be converted to \p double and piranha::math::sin()
	 * will then be used.
	 *
	 * @param[in] q argument.
	 *
	 * @return sine of \p q.
	 *
	 * @throws unspecified any exception thrown by converting piranha::mp_rational to \p double.
	 */
	double operator()(const T &q) const
	{
		return math::sin(static_cast<double>(q));
	}
};

/// Specialisation of the piranha::math::cos() functor for piranha::mp_rational.
template <typename T>
struct cos_impl<T,typename std::enable_if<detail::is_mp_rational<T>::value>::type>
{
	/// Call operator.
	/**
	 * The argument will be converted to \p double and piranha::math::cos()
	 * will then be used.
	 *
	 * @param[in] q argument.
	 *
	 * @return cosine of \p q.
	 *
	 * @throws unspecified any exception thrown by converting piranha::mp_rational to \p double.
	 */
	double operator()(const T &q) const
	{
		return math::cos(static_cast<double>(q));
	}
};

/// Specialisation of the piranha::math::abs() functor for piranha::mp_rational.
template <typename T>
struct abs_impl<T,typename std::enable_if<detail::is_mp_rational<T>::value>::type>
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

/// Specialisation of the piranha::math::partial() functor for piranha::mp_rational.
template <typename T>
struct partial_impl<T,typename std::enable_if<detail::is_mp_rational<T>::value>::type>
{
	/// Call operator.
	/**
	 * @return an instance of piranha::mp_rational constructed from zero.
	 */
	T operator()(const T &, const std::string &) const
	{
		return T{0};
	}
};

/// Specialisation of the piranha::math::evaluate() functor for piranha::mp_rational.
template <typename T>
struct evaluate_impl<T,typename std::enable_if<detail::is_mp_rational<T>::value>::type>
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

/// Specialisation of the piranha::math::subs() functor for piranha::mp_rational.
template <typename T>
struct subs_impl<T,typename std::enable_if<detail::is_mp_rational<T>::value>::type>
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

/// Specialisation of the piranha::math::integral_cast functor for piranha::mp_rational.
template <typename T>
struct integral_cast_impl<T,typename std::enable_if<std::is_same<T,rational>::value>::type>
{
	/// Call operator.
	/**
	 * The call will be successful if the denominator of \p x is unitary.
	 *
	 * @param[in] q cast argument.
	 *
	 * @return result of the cast operation.
	 *
	 * @throws std::invalid_argument if the call is unsuccessful.
	 */
	integer operator()(const T &q) const
	{
		if (q.den() == 1) {
			return q.num();
		}
		piranha_throw(std::invalid_argument,"invalid rational");
	}
};

/// Specialisation of the piranha::math::ipow_subs() functor for piranha::mp_rational.
/**
 * This specialisation is activated when \p T is piranha::mp_rational.
 * The result will be the input value unchanged.
 */
template <typename T>
struct ipow_subs_impl<T,typename std::enable_if<detail::is_mp_rational<T>::value>::type>
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

/// Specialisation of the piranha::math::binomial() functor for piranha::mp_rational.
/**
 * This specialisation is activated when one of the arguments is piranha::mp_rational and the other is either
 * piranha::mp_rational or an interoperable type for piranha::mp_rational.
 *
 * The implementation follows these rules:
 * - if the top is rational and the bottom an integral type or piranha::mp_integer, then
 *   piranha::mp_rational::binomial() is used;
 * - if the non-rational argument is a floating-point type, then the rational argument is converted
 *   to that floating-point type and piranha::math::binomial() is used;
 * - if both arguments are rational, they are both converted to \p double and then piranha::math::binomial()
 *   is used;
 * - if the top is an integral type or piranha::mp_integer and the bottom a rational, then both
 *   arguments are converted to \p double and piranha::math::binomial() is used.
 */
template <typename T, typename U>
struct binomial_impl<T,U,detail::rational_binomial_enabler<T,U>>
{
	/// Call operator, rational--integral overload.
	/**
	 * @param[in] x top argument.
	 * @param[in] y bottom argument.
	 *
	 * @returns \f$ x \choose y \f$.
	 *
	 * @throws unspecified any exception thrown by piranha::mp_rational::binomial().
	 */
	template <int NBits, typename T2>
	auto operator()(const mp_rational<NBits> &x, const T2 &y) const -> decltype(x.binomial(y))
	{
		return x.binomial(y);
	}
	/// Call operator, rational--floating-point overload.
	/**
	 * @param[in] x top argument.
	 * @param[in] y bottom argument.
	 *
	 * @returns \f$ x \choose y \f$.
	 *
	 * @throws unspecified any exception thrown by converting piranha::mp_rational
	 * to a floating-point type.
	 */
	template <int NBits, typename T2, typename std::enable_if<std::is_floating_point<T2>::value,int>::type = 0>
	T2 operator()(const mp_rational<NBits> &x, const T2 &y) const
	{
		return math::binomial(static_cast<T2>(x),y);
	}
	/// Call operator, floating-point--rational overload.
	/**
	 * @param[in] x top argument.
	 * @param[in] y bottom argument.
	 *
	 * @returns \f$ x \choose y \f$.
	 *
	 * @throws unspecified any exception thrown by converting piranha::mp_rational
	 * to a floating-point type.
	 */
	template <int NBits, typename T2, typename std::enable_if<std::is_floating_point<T2>::value,int>::type = 0>
	T2 operator()(const T2 &x, const mp_rational<NBits> &y) const
	{
		return math::binomial(x,static_cast<T2>(y));
	}
	/// Call operator, rational--rational overload.
	/**
	 * @param[in] x top argument.
	 * @param[in] y bottom argument.
	 *
	 * @returns \f$ x \choose y \f$.
	 *
	 * @throws unspecified any exception thrown by converting piranha::mp_rational
	 * to \p double.
	 */
	template <int NBits>
	double operator()(const mp_rational<NBits> &x, const mp_rational<NBits> &y) const
	{
		return math::binomial(static_cast<double>(x),static_cast<double>(y));
	}
	/// Call operator, integral--rational overload.
	/**
	 * @param[in] x top argument.
	 * @param[in] y bottom argument.
	 *
	 * @returns \f$ x \choose y \f$.
	 *
	 * @throws unspecified any exception thrown by converting piranha::mp_rational
	 * or piranha::mp_integer to \p double.
	 */
	template <int NBits, typename T2, typename std::enable_if<std::is_integral<T2>::value || detail::is_mp_integer<T2>::value,int>::type = 0>
	double operator()(const T2 &x, const mp_rational<NBits> &y) const
	{
		return math::binomial(static_cast<double>(x),static_cast<double>(y));
	}
};

}

}

namespace std
{

/// Specialisation of \p std::hash for piranha::mp_rational.
template <int NBits>
struct hash<piranha::mp_rational<NBits>>
{
	/// Result type.
	typedef size_t result_type;
	/// Argument type.
	typedef piranha::mp_rational<NBits> argument_type;
	/// Hash operator.
	/**
	 * @param[in] q piranha::mp_rational whose hash value will be returned.
	 *
	 * @return <tt>q.hash()</tt>.
	 *
	 * @see piranha::mp_rational::hash()
	 */
	result_type operator()(const argument_type &q) const noexcept
	{
		return q.hash();
	}
};

}

#endif
