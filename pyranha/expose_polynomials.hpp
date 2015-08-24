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

#ifndef PYRANHA_EXPOSE_POLYNOMIALS_HPP
#define PYRANHA_EXPOSE_POLYNOMIALS_HPP

#include <boost/python/class.hpp>
#include <boost/python/list.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/tuple.hpp>
#include <string>
#include <tuple>
#include <type_traits>

#include "../src/polynomial.hpp"
#include "../src/type_traits.hpp"
#include "expose_utils.hpp"

namespace pyranha
{

namespace bp = boost::python;

// Custom hook for polynomials.
template <typename Descriptor>
struct poly_custom_hook
{
	PIRANHA_DECLARE_HAS_TYPEDEF(degree_truncation_types);
	template <typename S>
	static void unset_auto_truncate_degree_wrapper()
	{
		S::unset_auto_truncate_degree();
	}
	template <typename S>
	static bp::tuple get_auto_truncate_degree_wrapper()
	{
		auto retval = S::get_auto_truncate_degree();
		bp::list l;
		for (const auto &s: std::get<2u>(retval)) {
			l.append(s);
		}
		return bp::make_tuple(std::get<0u>(retval),std::get<1u>(retval),l);
	}
	template <typename S, typename std::enable_if<piranha::detail::has_get_auto_truncate_degree<S>::value,int>::type = 0>
	static void expose_degree_auto_truncation_get_unset(bp::class_<S> &series_class)
	{
		series_class.def("unset_auto_truncate_degree",unset_auto_truncate_degree_wrapper<S>).staticmethod("unset_auto_truncate_degree");
		series_class.def("get_auto_truncate_degree",get_auto_truncate_degree_wrapper<S>).staticmethod("get_auto_truncate_degree");
	}
	template <typename S, typename std::enable_if<!piranha::detail::has_get_auto_truncate_degree<S>::value,int>::type = 0>
	static void expose_degree_auto_truncation_get_unset(bp::class_<S> &)
	{}
	template <typename S>
	struct auto_truncate_set_exposer
	{
		auto_truncate_set_exposer(bp::class_<S> &series_class):m_series_class(series_class) {}
		bp::class_<S> &m_series_class;
		template <typename T>
		static void set_auto_truncate_degree_wrapper(const T &max_degree)
		{
			S::set_auto_truncate_degree(max_degree);
		}
		template <typename T>
		static void set_auto_truncate_pdegree_wrapper(const T &max_degree, bp::list l)
		{
			bp::stl_input_iterator<std::string> begin(l), end;
			S::set_auto_truncate_degree(max_degree,std::vector<std::string>(begin,end));
		}
		template <typename T>
		void expose_set_auto_truncate_degree(const T &, typename std::enable_if<piranha::detail::has_set_auto_truncate_degree<S,T>::value>::type * = nullptr) const
		{
			m_series_class.def("set_auto_truncate_degree",set_auto_truncate_degree_wrapper<T>);
			m_series_class.def("set_auto_truncate_degree",set_auto_truncate_pdegree_wrapper<T>);
			set_auto_truncate_degree_exposed() = true;
		}
		template <typename T>
		void expose_set_auto_truncate_degree(const T &, typename std::enable_if<!piranha::detail::has_set_auto_truncate_degree<S,T>::value>::type * = nullptr) const
		{}
		static bool &set_auto_truncate_degree_exposed()
		{
			static bool retval = false;
			return retval;
		}
		template <typename T>
		void operator()(const T &x) const
		{
			expose_set_auto_truncate_degree(x);
		}
	};
	template <typename S, typename T = Descriptor, typename std::enable_if<has_typedef_degree_truncation_types<T>::value,int>::type = 0>
	static void expose_degree_auto_truncation_set(bp::class_<S> &series_class)
	{
		using dt_types = typename Descriptor::degree_truncation_types;
		dt_types dt;
		tuple_for_each(dt,auto_truncate_set_exposer<S>(series_class));
		if (auto_truncate_set_exposer<S>::set_auto_truncate_degree_exposed()) {
			series_class.staticmethod("set_auto_truncate_degree");
		}
	}
	template <typename S, typename T = Descriptor, typename std::enable_if<!has_typedef_degree_truncation_types<T>::value,int>::type = 0>
	static void expose_degree_auto_truncation_set(bp::class_<S> &)
	{}
	template <typename T>
	void operator()(bp::class_<T> &series_class) const
	{
		// We need to separate get and set, as they have different
		// type requirements.
		expose_degree_auto_truncation_get_unset(series_class);
		expose_degree_auto_truncation_set(series_class);
	}
};

void expose_polynomials_0();
void expose_polynomials_1();
void expose_polynomials_2();
void expose_polynomials_3();
void expose_polynomials_4();

}

#endif
