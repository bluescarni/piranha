/* Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)

This file is part of the Piranha library.

The Piranha library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The Piranha library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the Piranha library.  If not,
see https://www.gnu.org/licenses/. */

#ifndef PYRANHA_EXPOSE_RATIONAL_FUNCTIONS_HPP
#define PYRANHA_EXPOSE_RATIONAL_FUNCTIONS_HPP

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/return_arg.hpp>
#include <boost/python/self.hpp>
#include <string>
#include <tuple>

#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/pow.hpp"
#include "../src/rational_function.hpp"
#include "../src/real.hpp"
#include "expose_utils.hpp"
#include "type_system.hpp"

namespace pyranha
{

DECLARE_TT_NAMER(piranha::rational_function,"rational_function")

namespace bp = boost::python;

void expose_rational_functions_0();
void expose_rational_functions_1();

template <typename T>
struct rf_unary_ctor_exposer
{
	rf_unary_ctor_exposer(bp::class_<T> &rf_class):m_rf_class(rf_class) {}
	template <typename U>
	void operator()(const U &) const
	{
		m_rf_class.def(bp::init<const U &>());
	}
	bp::class_<T> &m_rf_class;
};

template <typename T, typename Tuple>
struct rf_binary_ctor_exposer
{
	template <typename U>
	struct inner_functor
	{
		inner_functor(bp::class_<T> &rf_class):m_rf_class(rf_class) {}
		template <typename V>
		void operator()(const V &) const
		{
			m_rf_class.def(bp::init<const U &, const V &>());
		}
		bp::class_<T> &m_rf_class;
	};
	rf_binary_ctor_exposer(bp::class_<T> &rf_class):m_rf_class(rf_class) {}
	template <typename U>
	void operator()(const U &) const
	{
		tuple_for_each(Tuple{},inner_functor<U>(m_rf_class));
	}
	bp::class_<T> &m_rf_class;
};

template <typename T>
struct rf_interop_exposer
{
	rf_interop_exposer(bp::class_<T> &rf_class):m_rf_class(rf_class) {}
	template <typename U>
	void operator()(const U &) const
	{
		U x;
		m_rf_class.def(bp::self += x);
		m_rf_class.def(bp::self + x);
		m_rf_class.def(x + bp::self);
		m_rf_class.def(bp::self -= x);
		m_rf_class.def(bp::self - x);
		m_rf_class.def(x - bp::self);
		m_rf_class.def(bp::self *= x);
		m_rf_class.def(bp::self * x);
		m_rf_class.def(x * bp::self);
#if PY_MAJOR_VERSION < 3
		m_rf_class.def(bp::self /= x);
#else
		m_rf_class.def("__itruediv__",generic_in_place_division_wrapper<T,U>,bp::return_arg<1u>{});
#endif

		m_rf_class.def(bp::self / x);
		m_rf_class.def(x / bp::self);
		m_rf_class.def(bp::self == x);
		m_rf_class.def(x == bp::self);
		m_rf_class.def(bp::self != x);
		m_rf_class.def(x != bp::self);
	}
	bp::class_<T> &m_rf_class;
};

template <typename T>
struct rf_eval_exposer
{
	rf_eval_exposer(bp::class_<T> &rf_class):m_rf_class(rf_class) {}
	template <typename U>
	void operator()(const U &) const
	{
		bp::def("_evaluate",generic_evaluate_wrapper<T,U>);
	}
	bp::class_<T> &m_rf_class;
};

template <typename T>
struct rf_subs_exposer
{
	rf_subs_exposer(bp::class_<T> &rf_class):m_rf_class(rf_class) {}
	template <typename U>
	void operator()(const U &) const
	{
		bp::def("_subs",piranha::math::subs<T,U>);
		bp::def("_ipow_subs",piranha::math::ipow_subs<T,U>);
	}
	bp::class_<T> &m_rf_class;
};

// Num and den wrappers.
template <typename T>
inline typename T::p_type rf_num_wrapper(const T &r)
{
	return r.num();
}

template <typename T>
inline typename T::p_type rf_den_wrapper(const T &r)
{
	return r.den();
}

template <typename Key>
inline void expose_rational_functions_impl()
{
	using piranha::rational_function;
	using r_type = rational_function<Key>;
	using p_type = typename r_type::p_type;
	using q_type = typename r_type::q_type;
	// Register in the generic type generator map.
	expose_generic_type_generator<rational_function,Key>();
	// Initial class exposition, with def ctor.
	auto rf_class = expose_class<r_type>();
	// Add the _is_exposed_type tag.
	rf_class.attr("_is_exposed_type") = true;
	// Unary ctors.
	using unary_ctor_types = std::tuple<std::string,piranha::integer,
		piranha::rational,p_type,q_type>;
	tuple_for_each(unary_ctor_types{},rf_unary_ctor_exposer<r_type>(rf_class));
	// Binary ctors.
	using binary_ctor_types = std::tuple<std::string,piranha::integer,
		piranha::rational,p_type,q_type,r_type>;
	tuple_for_each(binary_ctor_types{},rf_binary_ctor_exposer<r_type,binary_ctor_types>(rf_class));
	// Copy ctor.
	rf_class.def(bp::init<const r_type &>());
	// Shallow and deep copy.
	rf_class.def("__copy__",generic_copy_wrapper<r_type>);
	rf_class.def("__deepcopy__",generic_deepcopy_wrapper<r_type>);
	// NOTE: here repr is found via argument-dependent lookup.
	rf_class.def(repr(bp::self));
	// Interaction with self.
	rf_class.def(bp::self += bp::self);
	rf_class.def(bp::self + bp::self);
	rf_class.def(bp::self -= bp::self);
	rf_class.def(bp::self - bp::self);
	rf_class.def(bp::self *= bp::self);
	rf_class.def(bp::self * bp::self);
#if PY_MAJOR_VERSION < 3
	rf_class.def(bp::self /= bp::self);
#else
	rf_class.def("__itruediv__",generic_in_place_division_wrapper<r_type,r_type>,bp::return_arg<1u>{});
#endif
	rf_class.def(bp::self / bp::self);
	rf_class.def(bp::self == bp::self);
	rf_class.def(bp::self != bp::self);
	rf_class.def(+bp::self);
	rf_class.def(-bp::self);
	// Interoperability with other types.
	using interop_types = std::tuple<piranha::integer,piranha::rational,p_type,q_type>;
	tuple_for_each(interop_types{},rf_interop_exposer<r_type>(rf_class));
	// Pow.
	rf_class.def("__pow__",piranha::math::pow<r_type,piranha::integer>);
	rf_class.def("clear_pow_cache",r_type::clear_pow_cache).staticmethod("clear_pow_cache");
	// Evaluation.
	using eval_types = std::tuple<piranha::integer,piranha::rational,r_type,double,piranha::real>;
	tuple_for_each(eval_types{},rf_eval_exposer<r_type>(rf_class));
	// Subs.
	using subs_types = std::tuple<piranha::integer,piranha::rational,p_type,q_type,r_type>;
	tuple_for_each(subs_types{},rf_subs_exposer<r_type>(rf_class));
	// Integration.
	bp::def("_integrate",piranha::math::integrate<r_type>);
	// Partial.
	bp::def("_partial",piranha::math::partial<r_type>);
	// Poisson bracket.
	bp::def("_pbracket",generic_pbracket_wrapper<r_type>);
	// Canonical transformation.
	bp::def("_transformation_is_canonical",generic_canonical_wrapper<r_type>);
	// Trim.
	rf_class.def("trim",&r_type::trim);
	// Sin and cos.
	bp::def("_sin",piranha::math::sin<r_type>);
	bp::def("_cos",piranha::math::cos<r_type>);
	// Degree.
	bp::def("_degree",generic_degree_wrapper<r_type>);
	bp::def("_degree",generic_partial_degree_wrapper<r_type>);
	// Latex.
	rf_class.def("_latex_",generic_latex_wrapper<r_type>);
	// Pickling.
	rf_class.def_pickle(generic_pickle_suite<r_type>());
	// Invert.
	bp::def("_invert",piranha::math::invert<r_type>);
	// num/den property.
	rf_class.add_property("num",rf_num_wrapper<r_type>);
	rf_class.add_property("den",rf_den_wrapper<r_type>);
}

}

#endif
