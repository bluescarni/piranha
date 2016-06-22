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

#include "python_includes.hpp"

#include <boost/functional/hash.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/object.hpp>
#include <boost/python/stl_iterator.hpp>
#include <cstddef>
#include <functional>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#if defined(__GNUC__)
#include <cstdlib>
#include <cxxabi.h>
#include <memory>
#endif

#include "../src/config.hpp"
#include "type_system.hpp"

namespace pyranha
{

namespace bp = boost::python;

std::unordered_set<std::string> tg_names;
std::unordered_map<std::type_index, bp::object> et_map;
std::unordered_map<std::string, std::unordered_map<std::vector<std::type_index>, std::type_index, v_idx_hasher>>
    gtg_map;

std::string demangled_type_name(const std::type_index &t_idx)
{
#if defined(__GNUC__)
    int status = -4;
    // NOTE: abi::__cxa_demangle will return a pointer allocated by std::malloc, which we will delete via std::free.
    std::unique_ptr<char, void (*)(void *)> res{::abi::__cxa_demangle(t_idx.name(), nullptr, nullptr, &status),
                                                std::free};
    return (status == 0) ? std::string(res.get()) : std::string(t_idx.name());
#else
    // TODO demangling for other platforms. E.g.,
    // http://stackoverflow.com/questions/13777681/demangling-in-msvc
    return std::string(t_idx.name());
#endif
}

bp::object type_generator::operator()() const
{
    const auto it = et_map.find(m_t_idx);
    if (it == et_map.end()) {
        ::PyErr_SetString(
            PyExc_TypeError,
            (std::string("the type '") + demangled_type_name(m_t_idx) + "' has not been exposed").c_str());
        bp::throw_error_already_set();
    }
    return it->second;
}

std::string type_generator::repr() const
{
    return std::string("Type generator for the C++ type '") + demangled_type_name(m_t_idx) + "'";
}

std::size_t v_idx_hasher::operator()(const std::vector<std::type_index> &v) const
{
    std::size_t retval = 0u;
    std::hash<std::type_index> hasher;
    for (const auto &t_idx : v) {
        boost::hash_combine(retval, hasher(t_idx));
    }
    return retval;
}

type_generator generic_type_generator::operator()(bp::list l) const
{
    // We assume that this is created concurrently with the exposition of the gtg
    // (and hence its registration on the C++ and Python sides).
    piranha_assert(gtg_map.find(m_name) != gtg_map.end());
    // Convert the list to a vector of type idx objects.
    std::vector<std::type_index> v_t_idx;
    bp::stl_input_iterator<type_generator> it(l), end;
    for (; it != end; ++it) {
        v_t_idx.push_back((*it).m_t_idx);
    }
    const auto it1 = gtg_map[m_name].find(v_t_idx);
    if (it1 == gtg_map[m_name].end()) {
        ::PyErr_SetString(PyExc_TypeError,
                          (std::string("the generic type generator '") + m_name
                           + std::string("' has not been instantiated with the type pack ") + v_t_idx_to_str(v_t_idx))
                              .c_str());
        bp::throw_error_already_set();
    }
    return type_generator{it1->second};
}

std::string generic_type_generator::repr() const
{
    return std::string("Type generator for the generic C++ type '") + m_orig_name + "'";
}

std::string v_t_idx_to_str(const std::vector<std::type_index> &v_t_idx)
{
    std::string tv_name = "[";
    for (decltype(v_t_idx.size()) i = 0u; i < v_t_idx.size(); ++i) {
        tv_name += demangled_type_name(v_t_idx[i]);
        if (i != v_t_idx.size() - 1u) {
            tv_name += ",";
        }
    }
    tv_name += "]";
    return tv_name;
}
}
