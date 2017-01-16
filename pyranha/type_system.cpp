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

#include "python_includes.hpp"

#include <boost/functional/hash.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/object.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/tuple.hpp>
#include <cstddef>
#include <functional>
#include <string>
#include <typeindex>
#include <vector>

#include "../src/config.hpp"
#include "../src/detail/demangle.hpp"
#include "type_system.hpp"

namespace pyranha
{

namespace bp = boost::python;

// Map of exposed types.
et_map_t et_map;

// Map of registered template instances.
ti_map_t ti_map;

// Implementation of the methods of type_generator.
bp::object type_generator::operator()() const
{
    const auto it = et_map.find(m_t_idx);
    if (it == et_map.end()) {
        ::PyErr_SetString(PyExc_TypeError,
                          ("the type '" + piranha::detail::demangle(m_t_idx) + "' has not been registered").c_str());
        bp::throw_error_already_set();
    }
    return it->second;
}

std::string type_generator::repr() const
{
    return "Type generator for the C++ type '" + piranha::detail::demangle(m_t_idx) + "'";
}

// Implementation of the hasher for ti_map_t.
std::size_t v_idx_hasher::operator()(const std::vector<std::type_index> &v) const
{
    std::size_t retval = 0u;
    std::hash<std::type_index> hasher;
    for (const auto &t_idx : v) {
        boost::hash_combine(retval, hasher(t_idx));
    }
    return retval;
}

// Small utility to convert a vector of type indices to a string representation, only for error reporting purposes.
static inline std::string v_t_idx_to_str(const std::vector<std::type_index> &v_t_idx)
{
    std::string tv_name = "[";
    for (decltype(v_t_idx.size()) i = 0u; i < v_t_idx.size(); ++i) {
        tv_name += piranha::detail::demangle(v_t_idx[i]);
        if (i != v_t_idx.size() - 1u) {
            tv_name += ", ";
        }
    }
    tv_name += "]";
    return tv_name;
}

type_generator type_generator_template::getitem_t(bp::tuple t) const
{
    if (ti_map.find(m_name) == ti_map.end()) {
        ::PyErr_SetString(PyExc_TypeError,
                          ("no instance of the C++ class template '" + m_name + "' has been registered").c_str());
        bp::throw_error_already_set();
    }
    // Convert the tuple of generators to a vector of type idx objects.
    std::vector<std::type_index> v_t_idx;
    bp::stl_input_iterator<type_generator> it(t), end;
    for (; it != end; ++it) {
        v_t_idx.push_back((*it).m_t_idx);
    }
    const auto it1 = ti_map[m_name].find(v_t_idx);
    if (it1 == ti_map[m_name].end()) {
        ::PyErr_SetString(PyExc_TypeError, ("no instance of the C++ class template '" + m_name
                                            + "' has been registered with arguments " + v_t_idx_to_str(v_t_idx))
                                               .c_str());
        bp::throw_error_already_set();
    }
    return type_generator{it1->second};
}

type_generator type_generator_template::getitem_o(bp::object o) const
{
    return getitem_t(bp::make_tuple(o));
}

std::string type_generator_template::repr() const
{
    return "Type generator template for the C++ class template '" + m_name + "'";
}
}
