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

#include <boost/python/extract.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/list.hpp>
#include <boost/python/object.hpp>
#include <boost/python/scope.hpp>
#include <cstddef>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace pyranha
{

// Namespace alias for Boost.Python.
namespace bp = boost::python;

// Meta/macro programming to connect a class template to a name expressed as a string.
// This needs to be specialised (e.g., via the macro below) in order to be usable, otherwise
// a compile-time error will be generated.
template <template <typename...> class TT>
struct t_name {
    template <template <typename...> class>
    struct assert_failure {
        static const bool value = false;
    };
    static_assert(assert_failure<TT>::value, "No name defined for this class template.");
};

// NOTE: orig_name refers to the original C++ name as used in the template specialisation (e.g., std::vector).
#define PYRANHA_DECLARE_T_NAME(TT, tn)                                                                                 \
    template <>                                                                                                        \
    struct t_name<TT> {                                                                                                \
        static const std::string name;                                                                                 \
        static const std::string orig_name;                                                                            \
    };                                                                                                                 \
    const std::string t_name<TT>::name = tn;                                                                           \
    const std::string t_name<TT>::orig_name = #TT;

// Names of Python instances of type generators. We keep track because we do not want multiple instances
// with the same name on the Python side.
// NOTE: this refers to the names of the type generators as Python variables in the "types" submodule, nothing to
// do with t_idx, t_name, etc.
extern std::unordered_set<std::string> tg_names;

// Connection between C++ types (type_index) and Python types (encapsulated in bp::object).
extern std::unordered_map<std::type_index, bp::object> et_map;

// Type generator structure. It establishes the connection between a C++ type (the m_t_idx member)
// and its exposed Python counterpart via the call operator, which will query the et_map archive. Type generators
// are called from Python to retrieve an exposed Python type from the C++ type t_idx.
struct type_generator {
    explicit type_generator(const std::type_index &t_idx) : m_t_idx(t_idx)
    {
    }
    bp::object operator()() const;
    std::string repr() const;
    const std::type_index m_t_idx;
};

// Hasher for vector of type indices.
struct v_idx_hasher {
    std::size_t operator()(const std::vector<std::type_index> &) const;
};

// Map of class template instances. A class template is identified by a string (given by a specialisation of
// t_name),

// Map of generic type generators. Each item in the map is associated to another map, which establishes the
// connection between the concrete set of types used as template parameters for the class template
// and the final concrete instantiated type.
// NOTE: example: std::map<int,double> and std::map<std::string,float> would be encoded as the following
// record in the dictionary:
// {"map" : {[int,double] : std::map<int,double>,[std::string,float] : std::map<std::string,float>}}
extern std::unordered_map<std::string, std::unordered_map<std::vector<std::type_index>, std::type_index, v_idx_hasher>>
    gtg_map;

// Small utility to convert a vector of type indices to a string representation for error reporting purposes.
std::string v_t_idx_to_str(const std::vector<std::type_index> &);

// Like above, but this instead establishes the connection between a class template instantiated
// with a certain set of params and a type_generator.
struct generic_type_generator {
    explicit generic_type_generator(const std::string &name, const std::string &orig_name)
        : m_name(name), m_orig_name(orig_name)
    {
    }
    // Get the type generator corresponding to the C++ type defined by name<l[0],l[1],...>,
    // where l is a list of type generators.
    type_generator operator()(bp::list) const;
    std::string repr() const;
    const std::string m_name;
    const std::string m_orig_name;
};

template <typename T>
inline void expose_type_generator(const std::string &name)
{
    // We do not want to have duplicate instances on the Python side.
    if (tg_names.find(name) != tg_names.end()) {
        ::PyErr_SetString(PyExc_RuntimeError,
                          ("a type generator called '" + name + "' has already been instantiated").c_str());
        bp::throw_error_already_set();
    }
    tg_names.insert(name);
    type_generator tg(std::type_index(typeid(T)));
    bp::scope().attr("types").attr(name.c_str()) = tg;
}

// Expose generic type generator.
template <template <typename...> class TT, typename... Args>
inline void expose_generic_type_generator()
{
    const std::string name = t_name<TT>::name, orig_name = t_name<TT>::orig_name;
    // Add a new generic type generator if it does not exist already.
    if (gtg_map.find(name) == gtg_map.end()) {
        generic_type_generator gtg(name, orig_name);
        bp::scope().attr("types").attr(name.c_str()) = gtg;
    }
    // Convert the variadic args to a vector of type indices.
    std::vector<std::type_index> v_t_idx = {std::type_index(typeid(Args))...};
    // NOTE: the new key in gtg_map, if needed, will be created by the first call
    // to gtg_map[name].
    if (gtg_map[name].find(v_t_idx) != gtg_map[name].end()) {
        ::PyErr_SetString(PyExc_RuntimeError,
                          ("the generic type generator '" + name + "' has already been instantiated with the type pack "
                           + v_t_idx_to_str(v_t_idx))
                              .c_str());
        bp::throw_error_already_set();
    }
    gtg_map[name].emplace(std::move(v_t_idx), std::type_index(typeid(TT<Args...>)));
}
}

#endif
