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

#ifndef PYRANHA_EXPOSE_DIVISOR_SERIES_HPP
#define PYRANHA_EXPOSE_DIVISOR_SERIES_HPP

#include <boost/python/class.hpp>
#include <tuple>
#include <type_traits>
#include <utility>

#include "../src/detail/sfinae_types.hpp"
#include "../src/divisor_series.hpp"
#include "../src/polynomial.hpp"
#include "expose_utils.hpp"

namespace pyranha
{

namespace bp = boost::python;

void expose_divisor_series_0();
void expose_divisor_series_1();
void expose_divisor_series_2();

// Detect the presence of the from_polynomial() method in divisor_series types.
template <typename DSeries, typename Poly>
struct has_from_polynomial: piranha::detail::sfinae_types
{
	template <typename T1>
	static auto test(const T1 &t) -> decltype(T1::from_polynomial(std::declval<const Poly &>()),void(),yes());
	static no test(...);
	static const bool value = std::is_same<decltype(test(std::declval<DSeries>())),yes>::value;
};

// Machinery to expose conditionally the from_polynomial() static method in divisor series.
template <typename PolyDesc>
struct from_polynomial_exposer
{
	using p_params = typename PolyDesc::params;
	template <typename T>
	struct exposer_op
	{
		explicit exposer_op(bp::class_<T> &sc):m_sc(sc),m_flag(false) {}
		~exposer_op()
		{
			// Mark the method as static, but only if we actually exposed it.
			if (m_flag) {
				m_sc.staticmethod("from_polynomial");
			}
		}
		template <typename ... Args>
		void operator()(const std::tuple<Args...> &) const
		{
			using ptype = piranha::polynomial<Args...>;
			expose_impl<ptype>();
		}
		template <typename Poly>
		static auto from_polynomial(const Poly &p) -> decltype(T::from_polynomial(p))
		{
			return T::from_polynomial(p);
		}
		template <typename Poly, typename U = Poly, typename std::enable_if<has_from_polynomial<T,U>::value,int>::type = 0>
		void expose_impl() const
		{
			m_sc.def("from_polynomial",from_polynomial<Poly>);
			m_flag = true;
		}
		template <typename Poly, typename U = Poly, typename std::enable_if<!has_from_polynomial<T,U>::value,int>::type = 0>
		void expose_impl() const {}
		bp::class_<T>	&m_sc;
		mutable bool	m_flag;
	};
	template <typename T>
	void operator()(bp::class_<T> &sc) const
	{
		p_params p;
		tuple_for_each<p_params>(p,exposer_op<T>{sc});
	}
};

}

#endif
