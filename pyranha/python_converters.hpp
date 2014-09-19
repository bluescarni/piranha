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

#ifndef PYRANHA_PYTHON_CONVERTERS_HPP
#define PYRANHA_PYTHON_CONVERTERS_HPP

#include "python_includes.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/python/converter/registry.hpp>
#include <boost/python/converter/rvalue_from_python_data.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/import.hpp>
#include <boost/python/object.hpp>
#include <boost/python/type_id.hpp>
#include <stdexcept>
#include <string>

#include "../src/config.hpp"
#include "../src/detail/mpfr.hpp"
#include "../src/exceptions.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/real.hpp"

// NOTE: useful resources for python converters and C API:
// - http://misspent.wordpress.com/2009/09/27/how-to-write-boost-python-converters
// - http://svn.felspar.com/public/fost-py/trunk/fost-py/Cpp/fost-python/pystring.cpp
// - http://svn.felspar.com/public/fost-py/trunk/fost-py/Cpp/fost-python/pyjson.cpp
// - http://stackoverflow.com/questions/937884/how-do-i-import-modules-in-boostpython-embedded-python-code
// - http://docs.python.org/c-api/

namespace pyranha
{

namespace bp = boost::python;

template <typename T>
inline void construct_from_str(::PyObject *obj_ptr, bp::converter::rvalue_from_python_stage1_data *data, const std::string &name)
{
	piranha_assert(obj_ptr);
	::PyObject *str_obj = ::PyObject_Str(obj_ptr);
	if (!str_obj) {
		piranha_throw(std::runtime_error,std::string("unable to extract string representation of ") + name);
	}
	bp::handle<> str_rep(str_obj);
#if PY_MAJOR_VERSION < 3
	const char *s = ::PyString_AsString(str_rep.get());
#else
	::PyObject *unicode_str_obj = ::PyUnicode_AsEncodedString(str_rep.get(),"ascii","strict");
	if (!unicode_str_obj) {
		piranha_throw(std::runtime_error,std::string("unable to extract string representation of ") + name);
	}
	bp::handle<> unicode_str(unicode_str_obj);
	const char *s = ::PyBytes_AsString(unicode_str.get());
	if (!s) {
		::PyErr_Clear();
		piranha_throw(std::runtime_error,std::string("unable to extract string representation of ") + name);
	}
#endif
	void *storage = reinterpret_cast<bp::converter::rvalue_from_python_storage<T> *>(data)->storage.bytes;
	::new (storage) T(s);
	data->convertible = storage;
}

struct integer_converter
{
	integer_converter()
	{
		bp::to_python_converter<piranha::integer,to_python>();
		bp::converter::registry::push_back(&convertible,&construct,bp::type_id<piranha::integer>());
	}
	struct to_python
	{
		static ::PyObject *convert(const piranha::integer &n)
		{
			// NOTE: use PyLong_FromString here instead?
			const std::string str = boost::lexical_cast<std::string>(n);
#if PY_MAJOR_VERSION < 3
			bp::object bi_module = bp::import("__builtin__");
#else
			bp::object bi_module = bp::import("builtins");
#endif
			bp::object int_class = bi_module.attr("int");
			return bp::incref(int_class(str).ptr());
		}
	};
	static void *convertible(::PyObject *obj_ptr)
	{
		if (!obj_ptr || (
#if PY_MAJOR_VERSION < 3
			!PyInt_CheckExact(obj_ptr) &&
#endif
			!PyLong_CheckExact(obj_ptr))) {
			return nullptr;
		}
		return obj_ptr;
	}
	static void construct(::PyObject *obj_ptr, bp::converter::rvalue_from_python_stage1_data *data)
	{
		construct_from_str<piranha::integer>(obj_ptr,data,"integer");
	}
};

struct rational_converter
{
	rational_converter()
	{
		bp::to_python_converter<piranha::rational,to_python>();
		bp::converter::registry::push_back(&convertible,&construct,bp::type_id<piranha::rational>());
	}
	struct to_python
	{
		static ::PyObject *convert(const piranha::rational &q)
		{
			const std::string str = boost::lexical_cast<std::string>(q);
			bp::object frac_module = bp::import("fractions");
			bp::object frac_class = frac_module.attr("Fraction");
			return bp::incref(frac_class(str).ptr());
		}
	};
	static void *convertible(::PyObject *obj_ptr)
	{
		if (!obj_ptr) {
			return nullptr;
		}
		bp::object frac_module = bp::import("fractions");
		bp::object frac_class = frac_module.attr("Fraction");
		if (!::PyObject_IsInstance(obj_ptr,frac_class.ptr())) {
			return nullptr;
		}
		return obj_ptr;
	}
	static void construct(::PyObject *obj_ptr, bp::converter::rvalue_from_python_stage1_data *data)
	{
		construct_from_str<piranha::rational>(obj_ptr,data,"rational");
	}
};

struct real_converter
{
	real_converter()
	{
		bp::to_python_converter<piranha::real,to_python>();
		bp::converter::registry::push_back(&convertible,&construct,bp::type_id<piranha::real>());
	}
	struct to_python
	{
		static PyObject *convert(const piranha::real &r)
		{
			bp::object str(boost::lexical_cast<std::string>(r));
			try {
				bp::object mpmath = bp::import("mpmath");
				bp::object mpf = mpmath.attr("mpf");
				return bp::incref(mpf(str).ptr());
			} catch (...) {
				// NOTE: here it seems like in case of import errors, Boost.Python both throws and
				// sets the Python exception. Clear it and just throw pure C++.
				::PyErr_Clear();
				piranha_throw(std::runtime_error,"could not convert real number to mpf object - please check the installation of mpmath");
			}
		}
	};
	// Check if obj_ptr is an instance of the class str_class in the module str_mod. In case
	// of import or attr errors, false will be returned.
	static bool is_instance_of(::PyObject *obj_ptr, const char *str_mod, const char *str_class)
	{
		try {
			bp::object c_obj = bp::import(str_mod).attr(str_class);
			if (::PyObject_IsInstance(obj_ptr,c_obj.ptr())) {
				return true;
			}
		} catch (...) {
			::PyErr_Clear();
		}
		return false;
	}
	static void *convertible(::PyObject *obj_ptr)
	{
		// Not convertible if nullptr, or obj_ptr is not an instance of any of the supported classes.
		if (!obj_ptr || (!is_instance_of(obj_ptr,"mpmath","mpf") && !is_instance_of(obj_ptr,"sympy","Float"))) {
			return nullptr;
		}
		return obj_ptr;
	}
	static void construct(::PyObject *obj_ptr, bp::converter::rvalue_from_python_stage1_data *data)
	{
		// NOTE: here we cannot construct directly from string, as we need to query the precision.
		piranha_assert(obj_ptr);
		// NOTE: here the handle is from borrowed because we are not responsible for the generation of obj_ptr.
		bp::handle<> obj_handle(bp::borrowed(obj_ptr));
		bp::object obj(obj_handle);
		// NOTE: sympy Float seems to be a wrapper around an instance of mpmath mpf,
		// accessbile via the num attribute.
		if (is_instance_of(obj_ptr,"sympy","Float")) {
			obj = obj.attr("num");
		}
		const ::mpfr_prec_t prec = boost::numeric_cast< ::mpfr_prec_t>(
			static_cast<long>(bp::extract<long>(obj.attr("context").attr("prec"))));
		::PyObject *str_obj = ::PyObject_Repr(obj.ptr());
		if (!str_obj) {
			piranha_throw(std::runtime_error,std::string("unable to extract string representation of real"));
		}
		bp::handle<> str_rep(str_obj);
#if PY_MAJOR_VERSION < 3
		const char *s = ::PyString_AsString(str_rep.get());
#else
		::PyObject *unicode_str_obj = ::PyUnicode_AsEncodedString(str_rep.get(),"ascii","strict");
		if (!unicode_str_obj) {
			piranha_throw(std::runtime_error,std::string("unable to extract string representation of real"));
		}
		bp::handle<> unicode_str(unicode_str_obj);
		const char *s = ::PyBytes_AsString(unicode_str.get());
		if (!s) {
			::PyErr_Clear();
			piranha_throw(std::runtime_error,std::string("unable to extract string representation of real"));
		}
#endif
		// NOTE: the search for "'" is due to the string format of mpmath.mpf objects.
		while (*s != '\0' && *s != '\'') {
			++s;
		}
		if (*s == '\0') {
			piranha_throw(std::runtime_error,std::string("invalid string input converting to real"));
		}
		++s;
		auto start = s;
		while (*s != '\0' && *s != '\'') {
			++s;
		}
		if (*s == '\0') {
			piranha_throw(std::runtime_error,std::string("invalid string input converting to real"));
		}
		void *storage = reinterpret_cast<bp::converter::rvalue_from_python_storage<piranha::real> *>(data)->storage.bytes;
		::new (storage) piranha::real(std::string(start,s),prec);
		data->convertible = storage;
	}
};

}

#endif
