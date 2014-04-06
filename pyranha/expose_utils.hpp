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

#ifndef PYRANHA_EXPOSE_UTILS_HPP
#define PYRANHA_EXPOSE_UTILS_HPP

#include "python_includes.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/python/class.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/object.hpp>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <typeinfo>

#include "../src/exceptions.hpp"
#include "type_system.hpp"

namespace pyranha
{

namespace bp = boost::python;

// Counter of exposed types, used for naming said types.
extern std::size_t exposed_types_counter;

// Expose class with a default constructor and map it into the pyranha type system.
template <typename T>
inline bp::class_<T> expose_class()
{
	const auto t_idx = std::type_index(typeid(T));
	if (et_map.find(t_idx) != et_map.end()) {
		piranha_throw(std::runtime_error,std::string("the C++ type '") + demangled_type_name(t_idx) + "' has already been exposed");
	}
	bp::class_<T> class_inst((std::string("_exposed_type_")+boost::lexical_cast<std::string>(exposed_types_counter)).c_str(),bp::init<>());
	++exposed_types_counter;
	auto ptr = ::PyObject_Type(class_inst().ptr());
	if (!ptr) {
		::PyErr_Clear();
		::PyErr_SetString(PyExc_RuntimeError,"cannot extract the Python type of an instantiated class");
		bp::throw_error_already_set();
	}
	// This is always a new reference being returned.
	auto type_object = bp::object(bp::handle<>(ptr));
	// Map the C++ type to the Python type.
	et_map[t_idx] = type_object;
	return class_inst;
}

}

#endif
