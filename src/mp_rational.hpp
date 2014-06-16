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

#include <boost/math/common_factor_rt.hpp>
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

template <int NBits = 0>
class mp_rational
{
	public:
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
				piranha_throw(std::invalid_argument,"cannot construct integer from non-finite floating-point number");
			}
			if (x == Float(0)) {
				m_den = 1;
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
				if (unlikely(abs_x == Float(0))) {
					break;
				}
				exp = std::ilogb(abs_x);
				if (unlikely(exp == INT_MAX || exp == FP_ILOGBNAN)) {
					piranha_throw(std::invalid_argument,"error calling std::ilogb");
				}
			}
			// Handle the case in which the float is an exact integer.
			if (unlikely(abs_x == Float(0))) {
				m_num = i_part;
				m_den = 1;
				return;
			}
			m_den = 1;
			// Lift up the decimal part into an integer.
			while (std::trunc(abs_x) != abs_x) {
				abs_x = std::scalbln(abs_x,1);
				if (unlikely(abs_x == HUGE_VAL)) {
					piranha_throw(std::invalid_argument,"output of std::scalbn is HUGE_VAL");
				}
				m_den *= radix;
			}
			m_num = int_type(abs_x) + i_part * m_den;
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
			const unsigned radix = static_cast<unsigned>(std::numeric_limits<Float>::radix);
			int_type up_log_r(1);
			int exp = 0;
			while (up_log_r < m_den) {
				up_log_r *= radix;
				// TODO range check.
				--exp;
			}
			auto r = m_den % up_log_r;
			if (math::is_zero(r)) {
				auto q = m_den / up_log_r;
				auto i_part = (m_num * q) / up_log_r;
				auto d_part = m_num * q - up_log_r * i_part;
				// TODO: check output of scalbn.
				return Float(i_part) + std::scalbln(Float(d_part),exp);
			}
			return 0.;
		}
	public:
		mp_rational():m_num(),m_den(1) {}
		mp_rational(const mp_rational &) = default;
		mp_rational(mp_rational &&) = default;
		template <typename I0, typename I1, typename = nd_ctor_enabler<I0,I1>>
		explicit mp_rational(const I0 &n, const I1 &d):m_num(n),m_den(d)
		{
			if (unlikely(m_den.sign() == 0)) {
				piranha_throw(zero_division_error,"zero denominator in rational");
			}
			canonicalise();
		}
		template <typename T, typename = generic_ctor_enabler<T>>
		explicit mp_rational(const T &x)
		{
			construct_from_interoperable(x);
		}
		~mp_rational() noexcept
		{
			piranha_assert(!math::is_zero(m_den) != 0);
			piranha_assert(!math::is_zero(m_num) || (math::is_zero(m_num) && m_den == 1));
			piranha_assert(m_den.sign() > 0);
		}
		mp_rational &operator=(const mp_rational &) = default;
		mp_rational &operator=(mp_rational &&) = default;
		friend std::ostream &operator<<(std::ostream &os, const mp_rational &q)
		{
			if (q.m_num.sign() == 0) {
				os << '0';
			} else if (q.m_den == 1) {
				os << q.m_num;
			} else {
				os << q.m_num << '/' << q.m_den;
			}
			return os;
		}
		const int_type &num() const noexcept
		{
			return m_num;
		}
		const int_type &den() const noexcept
		{
			return m_den;
		}
		bool is_canonical() const noexcept
		{
			return boost::math::gcd(m_num,m_den) == 1;
		}
		void canonicalise()
		{
			// Denominator cannot be negative.
			if (m_den.sign() == -1) {
				m_num.negate();
				m_den.negate();
			}
			// Nothing to do if the denominator is 1.
			if (m_den == 1) {
				return;
			}
			const int_type gcd = boost::math::gcd(m_num,m_den);
			// Num and den are coprime already, no need for further divisions.
			if (gcd != 1) {
				m_num /= gcd;
				m_den /= gcd;
			}
		}
		template <typename T, typename = cast_enabler<T>>
		explicit operator T() const
		{
			return convert_to_impl<T>();
		}
	private:
		int_type	m_num;
		int_type	m_den;
};

}

#endif
