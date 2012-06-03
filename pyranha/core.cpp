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

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/python.hpp>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>

#include "../src/config.hpp"
#include "../src/integer.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/rational.hpp"
#include "../src/polynomial.hpp"

using namespace boost::python;
using namespace piranha;

// NOTE: useful resources for python converters and C API:
// - http://misspent.wordpress.com/2009/09/27/how-to-write-boost-python-converters
// - http://svn.felspar.com/public/fost-py/trunk/fost-py/Cpp/fost-python/pystring.cpp
// - http://svn.felspar.com/public/fost-py/trunk/fost-py/Cpp/fost-python/pyjson.cpp
// - http://stackoverflow.com/questions/937884/how-do-i-import-modules-in-boostpython-embedded-python-code
// - http://docs.python.org/c-api/

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

typedef boost::mpl::vector<double,integer,rational> cf_types;

struct polynomial_exposer
{
	template <typename Class>
	struct ctor_exposer
	{
		ctor_exposer(Class &cl):m_cl(cl) {}
		template <typename Cf>
		void operator()(const Cf &) const
		{
			m_cl.def(init<const Cf &>());
		}
		Class &m_cl;
	};
	template <typename Cf>
	struct arithmetic_exposer
	{
		typedef class_<polynomial<Cf,kronecker_monomial<>>> c_type;
		arithmetic_exposer(c_type &cl):m_cl(cl) {}
		template <typename Cf2>
		void operator()(const Cf2 &cf2) const
		{
			m_cl.def(self += cf2);
			m_cl.def(self + cf2);
			m_cl.def(cf2 + self);
			m_cl.def(self -= cf2);
			m_cl.def(self - cf2);
			m_cl.def(cf2 - self);
			m_cl.def(self *= cf2);
			m_cl.def(self * cf2);
			m_cl.def(cf2 * self);
		}
		c_type &m_cl;
	};
	template <typename Cf>
	void operator()(const Cf &cf) const
	{
		typedef polynomial<Cf,kronecker_monomial<>> p_type;
		class_<p_type> p_class((std::string("polynomial_")+typeid(cf).name()).c_str(),init<std::string>());
		p_class.def(repr(self));
		boost::mpl::for_each<cf_types>(ctor_exposer<decltype(p_class)>(p_class));
		// Arithmetic with self.
		p_class.def(self += self);
		p_class.def(self + self);
		p_class.def(self -= self);
		p_class.def(self - self);
		p_class.def(self *= self);
		p_class.def(self * self);
		// Arithmetic with numeric types.
		boost::mpl::for_each<cf_types>(arithmetic_exposer<Cf>(p_class));
	}
};

BOOST_PYTHON_MODULE(_core)
{
	// Arithmetic converters.
	integer_from_python_int integer_converter;
	rational_from_python_fraction rational_converter;

	// Docstring options setup.
	docstring_options doc_options(true,true,false);

	boost::mpl::for_each<cf_types>(polynomial_exposer());
}
