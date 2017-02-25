/* Copyright 2009-2017 Francesco Biscani (bluescarni@gmail.com)

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

#ifndef PYRANHA_UTILS_HPP
#define PYRANHA_UTILS_HPP

#include "python_includes.hpp"

#include <boost/python/extract.hpp>
#include <boost/python/import.hpp>
#include <boost/python/object.hpp>

namespace pyranha
{

namespace bp = boost::python;

// Import and return the builtin module.
inline bp::object builtin()
{
#if PY_MAJOR_VERSION < 3
    return bp::import("__builtin__");
#else
    return bp::import("builtins");
#endif
}

// Check if object has attribute.
inline bool hasattr(const bp::object &o, const char *name)
{
    return bp::extract<bool>(builtin().attr("hasattr")(o, name));
}

// Get string representation.
inline std::string str(const bp::object &o)
{
    return bp::extract<std::string>(builtin().attr("str")(o));
}
}

#endif
