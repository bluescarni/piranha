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

// NOTE: the order of inclusion in the first two items here is forced by these two issues:
// http://mail.python.org/pipermail/python-list/2004-March/907592.html
// http://mail.python.org/pipermail/new-bugs-announce/2011-March/010395.html
#if defined(_WIN32)
#include <cmath>
#include <Python.h>
#else
#include <Python.h>
#include <cmath>
#endif

#include <boost/python.hpp>
#include <stdexcept>
#include <string>

#include "../src/config.hpp"
#include "../src/integer.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/rational.hpp"
#include "../src/polynomial.hpp"

using namespace boost::python;
using namespace piranha;

template <typename T>
inline void construct_from_str(PyObject *obj_ptr, converter::rvalue_from_python_stage1_data *data, const std::string &name)
{
	PyObject *str_obj = PyObject_Str(obj_ptr);
	if (!str_obj) {
		piranha_throw(std::runtime_error,std::string("unable to extract string representation of ") + name);
	}
	handle<> str_rep(str_obj);
	const char *s = PyString_AsString(str_rep.get());
	void *storage = reinterpret_cast<converter::rvalue_from_python_storage<T> *>(data)->storage.bytes;
	::new (storage) T(s);
	data->convertible = storage;
}

struct integer_from_python_int
{
	integer_from_python_int()
	{
		converter::registry::push_back(&convertible,&construct,type_id<integer>());
	}
	static void *convertible(PyObject *obj_ptr)
	{
		if (!obj_ptr || (!PyInt_CheckExact(obj_ptr) && !PyLong_CheckExact(obj_ptr))) {
			return piranha_nullptr;
		}
		return obj_ptr;
	}
	static void construct(PyObject *obj_ptr, converter::rvalue_from_python_stage1_data *data)
	{
		construct_from_str<integer>(obj_ptr,data,"integer");
	}
};

struct rational_from_python_fraction
{
	rational_from_python_fraction()
	{
		converter::registry::push_back(&convertible,&construct,type_id<rational>());
	}
	static void *convertible(PyObject *obj_ptr)
	{
		if (!obj_ptr) {
			return piranha_nullptr;
		}
		object frac_module = import("fractions");
		object frac_class = frac_module.attr("Fraction");
		if (!PyObject_IsInstance(obj_ptr,frac_class.ptr())) {
			return piranha_nullptr;
		}
		return obj_ptr;
	}
	static void construct(PyObject *obj_ptr, converter::rvalue_from_python_stage1_data *data)
	{
		construct_from_str<rational>(obj_ptr,data,"rational");
	}
};

BOOST_PYTHON_MODULE(_core)
{
	docstring_options doc_options(true,true,false);

	integer_from_python_int integer_converter;
	rational_from_python_fraction rational_converter;

	class_<polynomial<integer,kronecker_monomial<>>>("poly",init<std::string>())
		.def(init<const double &>())
		.def(init<const integer &>())
		.def(init<const rational &>())
		.def(repr(self));
}
