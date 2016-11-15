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

#ifndef PYRANHA_TYPE_SYSTEM_HPP
#define PYRANHA_TYPE_SYSTEM_HPP

#include "python_includes.hpp"

#include <boost/python/class.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/object.hpp>
#include <boost/python/tuple.hpp>
#include <cstddef>
#include <initializer_list>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../src/detail/demangle.hpp"
#include "utils.hpp"

namespace pyranha
{

// Namespace alias for Boost.Python.
namespace bp = boost::python;

// Map C++ types (type_index) to Python types (encapsulated in bp::object) referring to exposed C++ types.
using et_map_t = std::unordered_map<std::type_index, bp::object>;
extern et_map_t et_map;

// Type generator structure. It establishes the connection between a C++ type (the m_t_idx member)
// and its exposed Python counterpart via the call operator, which will query the et_map archive. Type generators
// are called from Python to retrieve the exposed Python type corresponding to the C++ type t_idx.
struct type_generator {
    bp::object operator()() const;
    std::string repr() const;
    const std::type_index m_t_idx;
};

// Register into et_map a C++ type that was exposed to Python, recording the corresponding Python type.
// Will error out if the type has already been registered.
template <typename T, typename... Args>
inline void register_exposed_type(const bp::class_<T, Args...> &c)
{
    std::type_index t_idx(typeid(T));
    if (et_map.find(t_idx) != et_map.end()) {
        ::PyErr_SetString(PyExc_TypeError,
                          ("the C++ type '" + piranha::detail::demangle(t_idx) + "' has already been "
                                                                                 "registered in pyranha's type system")
                              .c_str());
        bp::throw_error_already_set();
    }
    et_map[t_idx] = c;
}

// Instantiate a type generator for type T into the object o (typically a module/submodule, but could be any object in
// principle). If an attribute with the same name already exists, it will error out.
template <typename T>
inline void instantiate_type_generator(const std::string &name, bp::object &o)
{
    // We do not want to have duplicate instances on the Python side.
    if (hasattr(o, name.c_str())) {
        ::PyErr_SetString(PyExc_AttributeError, ("error while trying to instantiate a type generator for the C++ type '"
                                                 + piranha::detail::demangle<T>() + "': an attribute called '" + name
                                                 + "' already exists in the object '" + str(o) + "'")
                                                    .c_str());
        bp::throw_error_already_set();
    }
    o.attr(name.c_str()) = type_generator{std::type_index(typeid(T))};
}

// Meta/macro programming to establish a 1-to-1 connection between a class template and a string. The string will be
// used as a surrogate identifier for the class template, as we don't have objects in the standard to represent at
// runtime the "type" of class templates. This needs to be specialised (via the macro below) in order to be usable,
// otherwise a compile-time error will be generated.
// NOTE: this needs the second template param so that the specialisations below are still templates and we don't
// get funky error messages about name/orig_name being defined in more than 1 translation unit.
template <template <typename...> class TT, typename = void>
struct t_name {
    template <template <typename...> class>
    struct assert_failure {
        static const bool value = false;
    };
    static_assert(assert_failure<TT>::value, "No name defined for this class template.");
};

#define PYRANHA_DECLARE_T_NAME(TT)                                                                                     \
    template <typename T>                                                                                              \
    struct t_name<TT, T> {                                                                                             \
        static const std::string name;                                                                                 \
    };                                                                                                                 \
    template <typename T>                                                                                              \
    const std::string t_name<TT, T>::name = #TT;

// Hasher for vector of type indices.
struct v_idx_hasher {
    std::size_t operator()(const std::vector<std::type_index> &) const;
};

// A dictionary that records template instances. The string is a surrogate for a class template (as we cannot
// extract a type index from a class template), whose various instances are memorized in terms of the types
// defining the instance and a type_index representing the instance itself.
// Example: the instances std::map<int,double> and std::map<std::string,float> would be encoded as following:
// {"map" : {[int,double] : std::map<int,double>,[std::string,float] : std::map<std::string,float>}}
using ti_map_t
    = std::unordered_map<std::string, std::unordered_map<std::vector<std::type_index>, std::type_index, v_idx_hasher>>;
extern ti_map_t ti_map;

// Register a template instance into ti_map. The string identifying the class template is taken from the specialisation
// of t_name for TT.
template <template <typename...> class TT, typename... Args>
void register_template_instance()
{
    const std::string name = t_name<TT>::name;
    // Convert the variadic args to a vector of type indices.
    std::vector<std::type_index> v_t_idx = {std::type_index(typeid(Args))...};
    std::type_index tidx(typeid(TT<Args...>));
    // NOTE: the new key in ti_map, if needed, will be created by the first call
    // to ti_map[name].
    if (ti_map[name].find(v_t_idx) != ti_map[name].end()) {
        ::PyErr_SetString(
            PyExc_TypeError,
            ("the template instance '" + piranha::detail::demangle(tidx) + "' has already been registered").c_str());
        bp::throw_error_already_set();
    }
    ti_map[name].emplace(std::move(v_t_idx), std::move(tidx));
}

// The purpose of this structure is to go look into ti_map for a template instance and,
// if found, return a type generator corresponding to that instance. The template instance
// will be constructed from:
// - the class template connected to the string m_name (via t_name),
// - one or more type generators passed in as arguments to the getitem() method, representing
//   the parameters of the template instance.
struct type_generator_template {
    type_generator getitem_t(bp::tuple) const;
    type_generator getitem_o(bp::object) const;
    std::string repr() const;
    const std::string m_name;
};

// Instantiate a type generator template for the class template TT into the object. If an attribute with the same
// name already exists, it will error out.
template <template <typename...> class TT>
inline void instantiate_type_generator_template(const std::string &name, bp::object &o)
{
    if (hasattr(o, name.c_str())) {
        ::PyErr_SetString(PyExc_AttributeError,
                          ("error while trying to instantiate a type generator for the C++ class template '"
                           + t_name<TT>::name + "': an attribute called '" + name + "' already exists in the object '"
                           + str(o) + "'")
                              .c_str());
        bp::throw_error_already_set();
    }
    o.attr(name.c_str()) = type_generator_template{t_name<TT>::name};
}
}

#endif
