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

#ifndef PYRANHA_EXPOSE_UTILS_HPP
#define PYRANHA_EXPOSE_UTILS_HPP

#include "python_includes.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/import.hpp>
#include <boost/python/init.hpp>
#include <boost/python/list.hpp>
#include <boost/python/object.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/tuple.hpp>
#include <cstddef>
#include <limits>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../src/detail/sfinae_types.hpp"
#include "../src/detail/type_in_tuple.hpp"
#include "../src/invert.hpp"
#include "../src/math.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/pow.hpp"
#include "../src/power_series.hpp"
#include "../src/real.hpp"
#include "../src/serialization.hpp"
#include "../src/series.hpp"
#include "../src/type_traits.hpp"
#include "type_system.hpp"

namespace pyranha
{

namespace bp = boost::python;

// Pickle support for series.
template <typename Series>
struct series_pickle_suite : bp::pickle_suite
{
	static bp::tuple getinitargs(const Series &)
	{
		return bp::make_tuple();
	}
	static bp::tuple getstate(const Series &s)
	{
		std::stringstream ss;
		boost::archive::text_oarchive oa(ss);
		oa << s;
		return bp::make_tuple(ss.str());
	}
	static void setstate(Series &s, bp::tuple state)
	{
		if (bp::len(state) != 1) {
			::PyErr_SetString(PyExc_ValueError,"the 'state' tuple must have exactly one element");
			bp::throw_error_already_set();
		}
		std::string st = bp::extract<std::string>(state[0]);
		std::stringstream ss;
		ss.str(st);
		boost::archive::text_iarchive ia(ss);
		ia >> s;
	}
};

// Counter of exposed types, used for naming said types.
extern std::size_t exposed_types_counter;

// Expose class with a default constructor and map it into the pyranha type system.
template <typename T>
inline bp::class_<T> expose_class()
{
	const auto t_idx = std::type_index(typeid(T));
	if (et_map.find(t_idx) != et_map.end()) {
		// NOTE: it is ok here and elsewhere to use c_str(), as PyErr_SetString will convert the second argument to a Python
		// string object.
		::PyErr_SetString(PyExc_RuntimeError,(std::string("the C++ type '") + demangled_type_name(t_idx) + "' has already been exposed").c_str());
		bp::throw_error_already_set();
	}
	bp::class_<T> class_inst((std::string("_exposed_type_")+boost::lexical_cast<std::string>(exposed_types_counter)).c_str(),bp::init<>());
	++exposed_types_counter;
	// NOTE: class_ inherits from bp::object, here the "call operator" of a class type will construct an instance
	// of that object. We then get the Python type out of that. It seems like another possible way of achieving
	// this is directly through the class' attributes:
	// http://stackoverflow.com/questions/17968091/boostpythonclass-programatically-obtaining-the-class-name
	auto ptr = ::PyObject_Type(class_inst().ptr());
	if (!ptr) {
		::PyErr_SetString(PyExc_RuntimeError,"cannot extract the Python type of an instantiated class");
		bp::throw_error_already_set();
	}
	// This is always a new reference being returned.
	auto type_object = bp::object(bp::handle<>(ptr));
	// Map the C++ type to the Python type.
	et_map[t_idx] = type_object;
	return class_inst;
}

// for_each tuple algorithm.
template <typename Tuple, typename Op, std::size_t B = 0u, std::size_t E = std::tuple_size<Tuple>::value, typename = void>
static void tuple_for_each(const Tuple &t, const Op &op, typename std::enable_if<B != E>::type * = nullptr)
{
	op(std::get<B>(t));
	static_assert(B != std::numeric_limits<std::size_t>::max(),"Overflow error.");
	tuple_for_each<Tuple,Op,static_cast<std::size_t>(B + std::size_t(1)),E>(t,op);
}

template <typename Tuple, typename Op, std::size_t B = 0u, std::size_t E = std::tuple_size<Tuple>::value, typename = void>
static void tuple_for_each(const Tuple &, const Op &, typename std::enable_if<B == E>::type * = nullptr)
{}

struct NullHook
{
	template <typename T>
	void operator()(bp::class_<T> &) const
	{}
};

// Generic series exposer.
template <template <typename ...> class Series, typename Descriptor, std::size_t Begin = 0u,
	std::size_t End = std::tuple_size<typename Descriptor::params>::value, typename CustomHook = NullHook>
class series_exposer
{
		using params = typename Descriptor::params;
		// Detect the presence of interoperable types.
		PIRANHA_DECLARE_HAS_TYPEDEF(interop_types);
		// Detect the presence of pow types.
		PIRANHA_DECLARE_HAS_TYPEDEF(pow_types);
		// Detect the presence of evaluation types.
		PIRANHA_DECLARE_HAS_TYPEDEF(eval_types);
		// Detect the presence of substitution types.
		PIRANHA_DECLARE_HAS_TYPEDEF(subs_types);
		// Detect the presence of degree truncation types.
		PIRANHA_DECLARE_HAS_TYPEDEF(degree_truncation_types);
		// Expose constructor conditionally.
		template <typename U, typename T>
		static void expose_ctor(bp::class_<T> &cl,
			typename std::enable_if<std::is_constructible<T,U>::value>::type * = nullptr)
		{
			cl.def(bp::init<U>());
		}
		template <typename U, typename T>
		static void expose_ctor(bp::class_<T> &,
			typename std::enable_if<!std::is_constructible<T,U>::value>::type * = nullptr)
		{}
		// Copy operations.
		template <typename S>
		static S copy_wrapper(const S &s)
		{
			return s;
		}
		template <typename S>
		static S deepcopy_wrapper(const S &s, bp::dict)
		{
			return copy_wrapper(s);
		}
		// Sparsity wrapper.
		template <typename S>
		static bp::dict table_sparsity_wrapper(const S &s)
		{
			const auto tmp = s.table_sparsity();
			bp::dict retval;
			for (const auto &p: tmp) {
				retval[p.first] = p.second;
			}
			return retval;
		}
		// Wrapper to list.
		template <typename S>
		static bp::list to_list_wrapper(const S &s)
		{
			bp::list retval;
			for (const auto &t: s) {
				retval.append(bp::make_tuple(t.first,t.second));
			}
			return retval;
		}
		// Expose arithmetics operations with another type, if supported.
		template <typename S, typename T>
		using common_ops_ic = std::integral_constant<bool,
			piranha::is_addable_in_place<S,T>::value &&
			piranha::is_addable<S,T>::value &&
			piranha::is_addable<T,S>::value &&
			piranha::is_subtractable_in_place<S,T>::value &&
			piranha::is_subtractable<S,T>::value &&
			piranha::is_subtractable<T,S>::value &&
			piranha::is_multipliable_in_place<S,T>::value &&
			piranha::is_multipliable<S,T>::value &&
			piranha::is_multipliable<T,S>::value &&
			piranha::is_equality_comparable<T,S>::value &&
			piranha::is_equality_comparable<S,T>::value>;
		template <typename S, typename T, typename std::enable_if<common_ops_ic<S,T>::value,int>::type = 0>
		static void expose_common_ops(bp::class_<S> &series_class, const T &in)
		{
			namespace sn = boost::python::self_ns;
			series_class.def(sn::operator+=(bp::self,in));
			series_class.def(sn::operator+(bp::self,in));
			series_class.def(sn::operator+(in,bp::self));
			series_class.def(sn::operator-=(bp::self,in));
			series_class.def(sn::operator-(bp::self,in));
			series_class.def(sn::operator-(in,bp::self));
			series_class.def(sn::operator*=(bp::self,in));
			series_class.def(sn::operator*(bp::self,in));
			series_class.def(sn::operator*(in,bp::self));
			series_class.def(sn::operator==(bp::self,in));
			series_class.def(sn::operator==(in,bp::self));
			series_class.def(sn::operator!=(bp::self,in));
			series_class.def(sn::operator!=(in,bp::self));
		}
		template <typename S, typename T, typename std::enable_if<!common_ops_ic<S,T>::value,int>::type = 0>
		static void expose_common_ops(bp::class_<S> &, const T &)
		{}
		// Handle division separately.
		template <typename S, typename T>
		using division_ops_ic = std::integral_constant<bool,
			piranha::is_divisible_in_place<S,T>::value &&
			piranha::is_divisible<S,T>::value>;
		template <typename S, typename T, typename std::enable_if<division_ops_ic<S,T>::value,int>::type = 0>
		static void expose_division(bp::class_<S> &series_class, const T &in)
		{
			namespace sn = boost::python::self_ns;
			series_class.def(sn::operator/=(bp::self,in));
			series_class.def(sn::operator/(bp::self,in));
		}
		template <typename S, typename T, typename std::enable_if<!division_ops_ic<S,T>::value,int>::type = 0>
		static void expose_division(bp::class_<S> &, const T &)
		{}
		// Main wrapper.
		template <typename T, typename S>
		static void expose_arithmetics(bp::class_<S> &series_class)
		{
			T in;
			expose_common_ops(series_class,in);
			expose_division(series_class,in);
		}
		// Exponentiation support.
		template <typename S>
		struct pow_exposer
		{
			pow_exposer(bp::class_<S> &series_class):m_series_class(series_class) {}
			bp::class_<S> &m_series_class;
			template <typename T, typename U>
			static auto pow_wrapper(const T &s, const U &x) -> decltype(piranha::math::pow(s,x))
			{
				return piranha::math::pow(s,x);
			}
			template <typename T>
			void operator()(const T &, typename std::enable_if<piranha::is_exponentiable<S,T>::value>::type * = nullptr) const
			{
				m_series_class.def("__pow__",pow_wrapper<S,T>);
			}
			template <typename T>
			void operator()(const T &, typename std::enable_if<!piranha::is_exponentiable<S,T>::value>::type * = nullptr) const
			{}
		};
		template <typename S, typename T = Descriptor>
		static void expose_pow(bp::class_<S> &series_class, typename std::enable_if<has_typedef_pow_types<T>::value>::type * = nullptr)
		{
			using pow_types = typename Descriptor::pow_types;
			pow_types pt;
			tuple_for_each(pt,pow_exposer<S>(series_class));
		}
		template <typename S, typename T = Descriptor>
		static void expose_pow(bp::class_<S> &, typename std::enable_if<!has_typedef_pow_types<T>::value>::type * = nullptr)
		{}
		// Evaluation.
		template <typename S>
		struct eval_exposer
		{
			eval_exposer(bp::class_<S> &series_class):m_series_class(series_class) {}
			bp::class_<S> &m_series_class;
			template <typename T>
			void operator()(const T &, typename std::enable_if<piranha::is_evaluable<S,T>::value>::type * = nullptr) const
			{
				m_series_class.def("_evaluate",evaluate_wrapper<S,T>);
				bp::def("_evaluate",evaluate_wrapper<S,T>);
			}
			template <typename T>
			void operator()(const T &, typename std::enable_if<!piranha::is_evaluable<S,T>::value>::type * = nullptr) const
			{}
		};
		// NOTE: math::evaluate for series is always the evaluate() member function, so we just need one wrapper.
		// This is true at the moment for other functions, such as the subs ones, integrate, etc.
		template <typename S, typename T>
		static auto evaluate_wrapper(const S &s, bp::dict dict, const T &)
			-> decltype(s.evaluate(std::declval<std::unordered_map<std::string,T>>()))
		{
			std::unordered_map<std::string,T> cpp_dict;
			bp::stl_input_iterator<std::string> it(dict), end;
			for (; it != end; ++it) {
				cpp_dict[*it] = bp::extract<T>(dict[*it])();
			}
			return s.evaluate(cpp_dict);
		}
		template <typename S, typename T = Descriptor>
		static void expose_eval(bp::class_<S> &series_class, typename std::enable_if<has_typedef_eval_types<T>::value>::type * = nullptr)
		{
			using eval_types = typename Descriptor::eval_types;
			eval_types et;
			tuple_for_each(et,eval_exposer<S>(series_class));
		}
		template <typename S, typename T = Descriptor>
		static void expose_eval(bp::class_<S> &, typename std::enable_if<!has_typedef_eval_types<T>::value>::type * = nullptr)
		{}
		// Substitution.
		template <typename S>
		struct subs_exposer
		{
			subs_exposer(bp::class_<S> &series_class):m_series_class(series_class) {}
			bp::class_<S> &m_series_class;
			template <typename T>
			void operator()(const T &x) const
			{
				impl_subs(x);
				impl_ipow_subs(x);
				impl_t_subs(x);
			}
			template <typename T, typename std::enable_if<piranha::has_subs<S,T>::value,int>::type = 0>
			void impl_subs(const T &) const
			{
				m_series_class.def("subs",subs_wrapper<T>);
				bp::def("_subs",subs_wrapper<T>);
			}
			template <typename T, typename std::enable_if<!piranha::has_subs<S,T>::value,int>::type = 0>
			void impl_subs(const T &) const {}
			template <typename T, typename std::enable_if<piranha::has_ipow_subs<S,T>::value,int>::type = 0>
			void impl_ipow_subs(const T &) const
			{
				m_series_class.def("ipow_subs",ipow_subs_wrapper<T>);
				bp::def("_ipow_subs",ipow_subs_wrapper<T>);
			}
			template <typename T, typename std::enable_if<!piranha::has_ipow_subs<S,T>::value,int>::type = 0>
			void impl_ipow_subs(const T &) const {}
			template <typename T, typename std::enable_if<piranha::has_t_subs<S,T,T>::value,int>::type = 0>
			void impl_t_subs(const T &) const
			{
				m_series_class.def("t_subs",t_subs_wrapper<T>);
				bp::def("_t_subs",t_subs_wrapper<T>);
			}
			template <typename T, typename std::enable_if<!piranha::has_t_subs<S,T,T>::value,int>::type = 0>
			void impl_t_subs(const T &) const {}
			// The actual wrappers.
			template <typename T>
			static auto subs_wrapper(const S &s, const std::string &name, const T &x) -> decltype(s.subs(name,x))
			{
				return s.subs(name,x);
			}
			template <typename T>
			static auto ipow_subs_wrapper(const S &s, const std::string &name, const piranha::integer &n, const T &x) -> decltype(s.ipow_subs(name,n,x))
			{
				return s.ipow_subs(name,n,x);
			}
			template <typename T>
			static auto t_subs_wrapper(const S &s, const std::string &name, const T &x, const T &y) -> decltype(s.t_subs(name,x,y))
			{
				return s.t_subs(name,x,y);
			}
		};
		template <typename S, typename T = Descriptor>
		static void expose_subs(bp::class_<S> &series_class, typename std::enable_if<has_typedef_subs_types<T>::value>::type * = nullptr)
		{
			using subs_types = typename Descriptor::subs_types;
			subs_types st;
			tuple_for_each(st,subs_exposer<S>(series_class));
			// Implement subs with self.
			S tmp;
			subs_exposer<S> se(series_class);
			se(tmp);
		}
		template <typename S, typename T = Descriptor>
		static void expose_subs(bp::class_<S> &, typename std::enable_if<!has_typedef_subs_types<T>::value>::type * = nullptr)
		{}
		// Interaction with interoperable types.
		template <typename S>
		struct interop_exposer
		{
			interop_exposer(bp::class_<S> &series_class):m_series_class(series_class) {}
			bp::class_<S> &m_series_class;
			template <typename T>
			void operator()(const T &) const
			{
				expose_ctor<const T &>(m_series_class);
				expose_arithmetics<T>(m_series_class);
			}
		};
		// Here we have three implementations of this function. The final objective is to enable construction and arithmetics
		// with respect to the coefficient type, and, recursively, the coefficient type of the coefficient type.
		// 1. Series2 is a series type whose cf is not among interoperable types.
		// In this case, we expose the interoperability with the cf, and we try to expose also the interoperability with the coefficient
		// type of cf, if it is a series type.
		template <typename InteropTypes, typename Series2, typename S>
		static void expose_cf_interop(bp::class_<S> &series_class,
			typename std::enable_if<!piranha::detail::type_in_tuple<typename Series2::term_type::cf_type,InteropTypes>::value>::type * = nullptr)
		{
			using cf_type = typename Series2::term_type::cf_type;
			cf_type cf;
			interop_exposer<S> ie(series_class);
			ie(cf);
			expose_cf_interop<InteropTypes,cf_type>(series_class);
		}
		// 2. Series2 is a series type whose cf is among the interoperable types. Try to go deeper in the hierarchy.
		template <typename InteropTypes, typename Series2, typename S>
		static void expose_cf_interop(bp::class_<S> &series_class,
			typename std::enable_if<piranha::detail::type_in_tuple<typename Series2::term_type::cf_type,InteropTypes>::value>::type * = nullptr)
		{
			expose_cf_interop<InteropTypes,typename Series2::term_type::cf_type>(series_class);
		}
		// 3. Series2 is not a series type. This signals the end of the recursion.
		template <typename InteropTypes, typename Series2, typename S>
		static void expose_cf_interop(bp::class_<S> &,
			typename std::enable_if<!piranha::is_series<Series2>::value>::type * = nullptr)
		{}
		template <typename S, typename T = Descriptor>
		static void expose_interoperable(bp::class_<S> &series_class, typename std::enable_if<has_typedef_interop_types<T>::value>::type * = nullptr)
		{
			using interop_types = typename Descriptor::interop_types;
			interop_types it;
			tuple_for_each(it,interop_exposer<S>(series_class));
			// Interoperate conditionally with coefficient type, if it is not already in the
			// list of interoperable types.
			expose_cf_interop<interop_types,S>(series_class);
		}
		template <typename S, typename T = Descriptor>
		static void expose_interoperable(bp::class_<S> &, typename std::enable_if<!has_typedef_interop_types<T>::value>::type * = nullptr)
		{}
		// Expose integration conditionally.
		template <typename S>
		static auto integrate_wrapper(const S &s, const std::string &name) -> decltype(piranha::math::integrate(s,name))
		{
			return piranha::math::integrate(s,name);
		}
		template <typename S>
		static void expose_integrate(bp::class_<S> &series_class,
			typename std::enable_if<piranha::is_integrable<S>::value>::type * = nullptr)
		{
			series_class.def("integrate",integrate_wrapper<S>);
			bp::def("_integrate",integrate_wrapper<S>);
		}
		template <typename S>
		static void expose_integrate(bp::class_<S> &,
			typename std::enable_if<!piranha::is_integrable<S>::value>::type * = nullptr)
		{}
		// Differentiation.
		// NOTE: here it is important to keep the member/free distinction because of the special semantics of partial.
		template <typename S>
		static auto partial_wrapper(const S &s, const std::string &name) -> decltype(piranha::math::partial(s,name))
		{
			return piranha::math::partial(s,name);
		}
		template <typename S>
		static auto partial_member_wrapper(const S &s, const std::string &name) -> decltype(s.partial(name))
		{
			return s.partial(name);
		}
		// Custom partial derivatives registration wrapper.
		// NOTE: here we need to take care of multithreading in the future, most likely by adding
		// the Python threading bits inside the lambda and also outside when checking func.
		template <typename S>
		static void register_custom_derivative(const std::string &name, bp::object func)
		{
			using partial_type = decltype(std::declval<const S &>().partial(std::string()));
			check_callable(func);
			S::register_custom_derivative(name,[func](const S &s) -> partial_type {
				return bp::extract<partial_type>(func(s));
			});
		}
		template <typename S>
		static void expose_partial(bp::class_<S> &series_class,
			typename std::enable_if<piranha::is_differentiable<S>::value>::type * = nullptr)
		{
			// NOTE: we need this below in order to specifiy exactly the address of the templated
			// static method for unregistering the custom derivatives.
			using partial_type = decltype(piranha::math::partial(std::declval<const S &>(),std::string{}));
			series_class.def("partial",partial_member_wrapper<S>);
			bp::def("_partial",partial_wrapper<S>);
			// Custom derivatives support.
			series_class.def("register_custom_derivative",register_custom_derivative<S>).staticmethod("register_custom_derivative");
			series_class.def("unregister_custom_derivative",
				S::template unregister_custom_derivative<S,partial_type>).staticmethod("unregister_custom_derivative");
			series_class.def("unregister_all_custom_derivatives",
				S::template unregister_all_custom_derivatives<S,partial_type>).staticmethod("unregister_all_custom_derivatives");
		}
		template <typename S>
		static void expose_partial(bp::class_<S> &,
			typename std::enable_if<!piranha::is_differentiable<S>::value>::type * = nullptr)
		{}
		// Poisson bracket.
		template <typename S>
		static auto pbracket_wrapper(const S &s1, const S &s2, bp::list p_list, bp::list q_list) ->
			decltype(piranha::math::pbracket(s1,s2,{},{}))
		{
			bp::stl_input_iterator<std::string> begin_p(p_list), end_p;
			bp::stl_input_iterator<std::string> begin_q(q_list), end_q;
			return piranha::math::pbracket(s1,s2,std::vector<std::string>(begin_p,end_p),
				std::vector<std::string>(begin_q,end_q));
		}
		template <typename S>
		static void expose_pbracket(bp::class_<S> &,
			typename std::enable_if<piranha::has_pbracket<S>::value>::type * = nullptr)
		{
			bp::def("_pbracket",pbracket_wrapper<S>);
		}
		template <typename S>
		static void expose_pbracket(bp::class_<S> &,
			typename std::enable_if<!piranha::has_pbracket<S>::value>::type * = nullptr)
		{}
		// Canonical transformation.
		// NOTE: last param is dummy to let the Boost.Python type system to pick the correct type.
		template <typename S>
		static bool canonical_wrapper(bp::list new_p, bp::list new_q, bp::list p_list, bp::list q_list, const S &)
		{
			bp::stl_input_iterator<S> begin_new_p(new_p), end_new_p;
			bp::stl_input_iterator<S> begin_new_q(new_q), end_new_q;
			bp::stl_input_iterator<std::string> begin_p(p_list), end_p;
			bp::stl_input_iterator<std::string> begin_q(q_list), end_q;
			return piranha::math::transformation_is_canonical(std::vector<S>(begin_new_p,end_new_p),std::vector<S>(begin_new_q,end_new_q),
				std::vector<std::string>(begin_p,end_p),std::vector<std::string>(begin_q,end_q));
		}
		template <typename S>
		static void expose_canonical(bp::class_<S> &,
			typename std::enable_if<piranha::has_transformation_is_canonical<S>::value>::type * = nullptr)
		{
			bp::def("_transformation_is_canonical",canonical_wrapper<S>);
		}
		template <typename S>
		static void expose_canonical(bp::class_<S> &,
			typename std::enable_if<!piranha::has_transformation_is_canonical<S>::value>::type * = nullptr)
		{}
		// Utility function to check if object is callable. Will throw TypeError if not.
		static void check_callable(bp::object func)
		{
#if PY_MAJOR_VERSION < 3
			bp::object builtin_module = bp::import("__builtin__");
			if (!builtin_module.attr("callable")(func)) {
				::PyErr_SetString(PyExc_TypeError,"object is not callable");
				bp::throw_error_already_set();
			}
#else
			// This will throw on failure.
			try {
				bp::object call_method = func.attr("__call__");
				(void)call_method;
			} catch (...) {
				// NOTE: it seems like it is ok to overwrite the global error status of Python here,
				// after it has already been set by Boost.Python via the exception thrown above.
				::PyErr_SetString(PyExc_TypeError,"object is not callable");
				bp::throw_error_already_set();
			}
#endif
		}
		// filter() wrap.
		template <typename S>
		static S wrap_filter(const S &s, bp::object func)
		{
			typedef typename S::term_type::cf_type cf_type;
			check_callable(func);
			auto cpp_func = [func](const std::pair<cf_type,S> &p) -> bool {
				return bp::extract<bool>(func(bp::make_tuple(p.first,p.second)));
			};
			return s.filter(cpp_func);
		}
		// Check if type is tuple with two elements (for use in wrap_transform).
		static void check_tuple_2(bp::object obj)
		{
			bp::object builtin_module = bp::import(
#if PY_MAJOR_VERSION < 3
			"__builtin__"
#else
			"builtins"
#endif
			);
			bp::object isinstance = builtin_module.attr("isinstance");
			bp::object tuple_type = builtin_module.attr("tuple");
			if (!isinstance(obj,tuple_type)) {
				::PyErr_SetString(PyExc_TypeError,"object is not a tuple");
				bp::throw_error_already_set();
			}
			const std::size_t len = bp::extract<std::size_t>(obj.attr("__len__")());
			if (len != 2u) {
				::PyErr_SetString(PyExc_ValueError,"the tuple to be returned in series transformation must have 2 elements");
				bp::throw_error_already_set();
			}
		}
		// transform() wrap.
		template <typename S>
		static S wrap_transform(const S &s, bp::object func)
		{
			typedef typename S::term_type::cf_type cf_type;
			check_callable(func);
			auto cpp_func = [func](const std::pair<cf_type,S> &p) -> std::pair<cf_type,S> {
				bp::object tmp = func(bp::make_tuple(p.first,p.second));
				check_tuple_2(tmp);
				cf_type tmp_cf = bp::extract<cf_type>(tmp[0]);
				S tmp_key = bp::extract<S>(tmp[1]);
				return std::make_pair(std::move(tmp_cf),std::move(tmp_key));
			};
			return s.transform(cpp_func);
		}
		// Sin and cos.
		template <typename S>
		static auto sin_wrapper(const S &s) -> decltype(piranha::math::sin(s))
		{
			return piranha::math::sin(s);
		}
		template <typename S>
		static auto cos_wrapper(const S &s) -> decltype(piranha::math::cos(s))
		{
			return piranha::math::cos(s);
		}
		template <typename S>
		static void expose_sin_cos(typename std::enable_if<piranha::has_sine<S>::value && piranha::has_cosine<S>::value>::type * = nullptr)
		{
			bp::def("_sin",sin_wrapper<S>);
			bp::def("_cos",cos_wrapper<S>);
		}
		template <typename S>
		static void expose_sin_cos(typename std::enable_if<!piranha::has_sine<S>::value || !piranha::has_cosine<S>::value>::type * = nullptr)
		{}
		// Power series exposer.
		template <typename S>
		static void expose_power_series(bp::class_<S> &series_class)
		{
			expose_degree(series_class);
			expose_degree_truncation(series_class);
		}
		template <typename T>
		static void expose_degree(bp::class_<T> &series_class,
			typename std::enable_if<piranha::has_degree<T>::value && piranha::has_ldegree<T>::value>::type * = nullptr)
		{
			// NOTE: probably we should make these piranha::math:: wrappers. Same for the trig ones.
			series_class.def("degree",wrap_degree<T>);
			series_class.def("degree",wrap_partial_degree_set<T>);
			series_class.def("ldegree",wrap_ldegree<T>);
			series_class.def("ldegree",wrap_partial_ldegree_set<T>);
		}
		template <typename T>
		static void expose_degree(bp::class_<T> &,
			typename std::enable_if<!piranha::has_degree<T>::value || !piranha::has_ldegree<T>::value>::type * = nullptr)
		{}
		// degree() wrappers.
		template <typename S>
		static auto wrap_degree(const S &s) -> decltype(s.degree())
		{
			return s.degree();
		}
		template <typename S>
		static auto wrap_partial_degree_set(const S &s, bp::list l) -> decltype(s.degree(std::vector<std::string>{}))
		{
			bp::stl_input_iterator<std::string> begin(l), end;
			return s.degree(std::vector<std::string>(begin,end));
		}
		template <typename S>
		static auto wrap_ldegree(const S &s) -> decltype(s.ldegree())
		{
			return s.ldegree();
		}
		template <typename S>
		static auto wrap_partial_ldegree_set(const S &s, bp::list l) -> decltype(s.ldegree(std::vector<std::string>{}))
		{
			bp::stl_input_iterator<std::string> begin(l), end;
			return s.ldegree(std::vector<std::string>(begin,end));
		}
		// Truncation.
		template <typename S>
		struct truncate_degree_exposer
		{
			truncate_degree_exposer(bp::class_<S> &series_class):m_series_class(series_class) {}
			bp::class_<S> &m_series_class;
			template <typename T>
			static S truncate_degree_wrapper(const S &s, const T &x)
			{
				return s.truncate_degree(x);
			}
			template <typename T>
			static S truncate_pdegree_wrapper(const S &s, const T &x, bp::list l)
			{
				bp::stl_input_iterator<std::string> begin(l), end;
				return s.truncate_degree(x,std::vector<std::string>(begin,end));
			}
			template <typename T>
			void expose_truncate_degree(const T &, typename std::enable_if<piranha::has_truncate_degree<S,T>::value>::type * = nullptr) const
			{
				// Expose both as member function and free function.
				m_series_class.def("truncate_degree",truncate_degree_wrapper<T>);
				bp::def("_truncate_degree",truncate_degree_wrapper<T>);
				// The partial version.
				m_series_class.def("truncate_degree",truncate_pdegree_wrapper<T>);
				bp::def("_truncate_degree",truncate_pdegree_wrapper<T>);
			}
			template <typename T>
			void expose_truncate_degree(const T &, typename std::enable_if<!piranha::has_truncate_degree<S,T>::value>::type * = nullptr) const
			{}
			template <typename T>
			void operator()(const T &x) const
			{
				expose_truncate_degree(x);
			}
		};
		template <typename S, typename T = Descriptor, typename std::enable_if<has_typedef_degree_truncation_types<T>::value,int>::type = 0>
		static void expose_degree_truncation(bp::class_<S> &series_class)
		{
			using dt_types = typename Descriptor::degree_truncation_types;
			dt_types dt;
			tuple_for_each(dt,truncate_degree_exposer<S>(series_class));
		}
		template <typename S, typename T = Descriptor, typename std::enable_if<!has_typedef_degree_truncation_types<T>::value,int>::type = 0>
		static void expose_degree_truncation(bp::class_<S> &)
		{}
		// Trigonometric exposer.
		template <typename S>
		static void expose_trigonometric_series(bp::class_<S> &series_class, typename std::enable_if<
			piranha::has_t_degree<S>::value && piranha::has_t_ldegree<S>::value && piranha::has_t_order<S>::value && piranha::has_t_lorder<S>::value>::type * = nullptr)
		{
			series_class.def("t_degree",wrap_t_degree<S>);
			series_class.def("t_degree",wrap_partial_t_degree<S>);
			series_class.def("t_ldegree",wrap_t_ldegree<S>);
			series_class.def("t_ldegree",wrap_partial_t_ldegree<S>);
			series_class.def("t_order",wrap_t_order<S>);
			series_class.def("t_order",wrap_partial_t_order<S>);
			series_class.def("t_lorder",wrap_t_lorder<S>);
			series_class.def("t_lorder",wrap_partial_t_lorder<S>);
		}
		template <typename S>
		static void expose_trigonometric_series(bp::class_<S> &, typename std::enable_if<
			!piranha::has_t_degree<S>::value || !piranha::has_t_ldegree<S>::value || !piranha::has_t_order<S>::value || !piranha::has_t_lorder<S>::value>::type * = nullptr)
		{}
		template <typename S>
		static auto wrap_t_degree(const S &s) -> decltype(s.t_degree())
		{
			return s.t_degree();
		}
		template <typename S>
		static auto wrap_partial_t_degree(const S &s, bp::list l) -> decltype(s.t_degree(std::vector<std::string>{}))
		{
			bp::stl_input_iterator<std::string> begin(l), end;
			return s.t_degree(std::vector<std::string>(begin,end));
		}
		template <typename S>
		static auto wrap_t_ldegree(const S &s) -> decltype(s.t_ldegree())
		{
			return s.t_ldegree();
		}
		template <typename S>
		static auto wrap_partial_t_ldegree(const S &s, bp::list l) -> decltype(s.t_ldegree(std::vector<std::string>{}))
		{
			bp::stl_input_iterator<std::string> begin(l), end;
			return s.t_ldegree(std::vector<std::string>(begin,end));
		}
		template <typename S>
		static auto wrap_t_order(const S &s) -> decltype(s.t_order())
		{
			return s.t_order();
		}
		template <typename S>
		static auto wrap_partial_t_order(const S &s, bp::list l) -> decltype(s.t_order(std::vector<std::string>{}))
		{
			bp::stl_input_iterator<std::string> begin(l), end;
			return s.t_order(std::vector<std::string>(begin,end));
		}
		template <typename S>
		static auto wrap_t_lorder(const S &s) -> decltype(s.t_lorder())
		{
			return s.t_lorder();
		}
		template <typename S>
		static auto wrap_partial_t_lorder(const S &s, bp::list l) -> decltype(s.t_lorder(std::vector<std::string>{}))
		{
			bp::stl_input_iterator<std::string> begin(l), end;
			return s.t_lorder(std::vector<std::string>(begin,end));
		}
		// Latex representation.
		template <typename S>
		static std::string wrap_latex(const S &s)
		{
			std::ostringstream oss;
			s.print_tex(oss);
			return oss.str();
		}
		// Symbol set wrapper.
		template <typename S>
		static bp::list symbol_set_wrapper(const S &s)
		{
			bp::list retval;
			for (auto it = s.get_symbol_set().begin(); it != s.get_symbol_set().end(); ++it) {
				retval.append(it->get_name());
			}
			return retval;
		}
		// File load/save.
		template <typename S>
		static void expose_save_load(bp::class_<S> &series_class)
		{
			// Save.
			typedef void (*s1)(const S &, const std::string &, piranha::file_format, piranha::file_compression);
			typedef void (*s2)(const S &, const std::string &);
			typedef void (*s3)(const S &, const std::string &, piranha::file_compression);
			typedef void (*s4)(const S &, const std::string &, piranha::file_format);
			series_class.def("save",static_cast<s1>(&S::save));
			series_class.def("save",static_cast<s2>(&S::save));
			series_class.def("save",static_cast<s3>(&S::save));
			series_class.def("save",static_cast<s4>(&S::save));
			series_class.staticmethod("save");
			// Load.
			typedef S (*l1)(const std::string &, piranha::file_format, piranha::file_compression);
			typedef S (*l2)(const std::string &);
			typedef S (*l3)(const std::string &, piranha::file_compression);
			typedef S (*l4)(const std::string &, piranha::file_format);
			series_class.def("load",static_cast<l1>(&S::load));
			series_class.def("load",static_cast<l2>(&S::load));
			series_class.def("load",static_cast<l3>(&S::load));
			series_class.def("load",static_cast<l4>(&S::load));
			series_class.staticmethod("load");
		}
		// invert().
		template <typename S>
		static auto invert_wrapper(const S &s) -> decltype(piranha::math::invert(s))
		{
			return piranha::math::invert(s);
		}
		template <typename S, typename std::enable_if<piranha::is_invertible<S>::value,int>::type = 0>
		static void expose_invert(bp::class_<S> &)
		{
			bp::def("_invert",invert_wrapper<S>);
		}
		template <typename S, typename std::enable_if<!piranha::is_invertible<S>::value,int>::type = 0>
		static void expose_invert(bp::class_<S> &)
		{}
		// Main exposer.
		struct exposer_op
		{
			explicit exposer_op() = default;
			template <typename ... Args>
			void operator()(const std::tuple<Args...> &) const
			{
				using s_type = Series<Args...>;
				// Register in the generic type generator map.
				expose_generic_type_generator<Series,Args...>();
				// Start exposing.
				auto series_class = expose_class<s_type>();
				// Add the _is_series tag.
				series_class.attr("_is_series") = true;
				// Constructor from string, if available.
				expose_ctor<const std::string &>(series_class);
				// Copy constructor.
				series_class.def(bp::init<const s_type &>());
				// Shallow and deep copy.
				series_class.def("__copy__",copy_wrapper<s_type>);
				series_class.def("__deepcopy__",deepcopy_wrapper<s_type>);
				// NOTE: here repr is found via argument-dependent lookup.
				series_class.def(repr(bp::self));
				// Length.
				series_class.def("__len__",&s_type::size);
				// Table properties.
				series_class.def("table_load_factor",&s_type::table_load_factor);
				series_class.def("table_bucket_count",&s_type::table_bucket_count);
				series_class.def("table_sparsity",table_sparsity_wrapper<s_type>);
				// Conversion to list.
				series_class.add_property("list",to_list_wrapper<s_type>);
				// Interaction with self.
				series_class.def(bp::self += bp::self);
				series_class.def(bp::self + bp::self);
				series_class.def(bp::self -= bp::self);
				series_class.def(bp::self - bp::self);
				series_class.def(bp::self *= bp::self);
				series_class.def(bp::self * bp::self);
				series_class.def(bp::self == bp::self);
				series_class.def(bp::self != bp::self);
				series_class.def(+bp::self);
				series_class.def(-bp::self);
				// NOTE: here this method is available if is_identical() is (that is, if the series are comparable), so
				// put it here - even if logically it belongs to exponentiation. We assume the series are comparable anyway.
				series_class.def("clear_pow_cache",s_type::template clear_pow_cache<s_type,0>).staticmethod("clear_pow_cache");
				// Expose interoperable types.
				expose_interoperable(series_class);
				// Expose pow.
				expose_pow(series_class);
				// Evaluate.
				expose_eval(series_class);
				// Subs.
				expose_subs(series_class);
				// Integration.
				expose_integrate(series_class);
				// Partial differentiation.
				expose_partial(series_class);
				// Poisson bracket.
				expose_pbracket(series_class);
				// Canonical test.
				expose_canonical(series_class);
				// Filter and transform.
				series_class.def("filter",wrap_filter<s_type>);
				series_class.def("transform",wrap_transform<s_type>);
				// Trimming.
				series_class.def("trim",&s_type::trim);
				// Sin and cos.
				expose_sin_cos<s_type>();
				// Power series.
				expose_power_series(series_class);
				// Trigonometric series.
				expose_trigonometric_series(series_class);
				// Latex.
				series_class.def("_latex_",wrap_latex<s_type>);
				// Arguments set.
				series_class.add_property("symbol_set",symbol_set_wrapper<s_type>);
				// Pickle support.
				series_class.def_pickle(series_pickle_suite<s_type>());
				// Save and load.
				expose_save_load(series_class);
				// Expose invert(), if present.
				expose_invert(series_class);
				// Run the custom hook.
				CustomHook{}(series_class);
			}
		};
	public:
		series_exposer()
		{
			// NOTE: probably we can avoid instantiating p here, look at the tuple for_each
			// in vargs_to_v_t_idx.
			params p;
			tuple_for_each<params,exposer_op,Begin,End>(p,exposer_op{});
		}
		series_exposer(const series_exposer &) = delete;
		series_exposer(series_exposer &&) = delete;
		series_exposer &operator=(const series_exposer &) = delete;
		series_exposer &operator=(series_exposer &&) = delete;
		~series_exposer() = default;
};

inline bp::list get_series_list()
{
	bp::list retval;
	for (const auto &p: et_map) {
		if (::PyObject_HasAttrString(p.second.ptr(),"_is_series")) {
			retval.append(p.second);
		}
	}
	return retval;
}

}

#endif
