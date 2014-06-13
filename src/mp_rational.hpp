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
#include <iostream>
#include <type_traits>

#include "config.hpp"
#include "exceptions.hpp"
#include "mp_integer.hpp"

namespace piranha
{

template <int NBits = 0>
class mp_rational
{
	public:
		using int_type = mp_integer<NBits>;
		mp_rational():m_num(),m_den(1) {}
		mp_rational(const mp_rational &) = default;
		mp_rational(mp_rational &&) = default;
	private:
		template <typename I0, typename I1>
		using nd_ctor_enabler = typename std::enable_if<(std::is_integral<I0>::value || std::is_same<I0,int_type>::value) &&
			(std::is_integral<I1>::value || std::is_same<I1,int_type>::value)>::type;
	public:
		template <typename I0, typename I1, typename = nd_ctor_enabler<I0,I1>>
		explicit mp_rational(const I0 &n, const I1 &d):m_num(n),m_den(d)
		{
			if (unlikely(m_den.sign() == 0)) {
				piranha_throw(zero_division_error,"zero denominator in rational");
			}
			canonicalise();
		}
		~mp_rational() = default;
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
	private:
		int_type	m_num;
		int_type	m_den;
};

}

#endif
