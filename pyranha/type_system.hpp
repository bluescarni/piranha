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

#ifndef PYRANHA_TYPE_SYSTEM_HPP
#define PYRANHA_TYPE_SYSTEM_HPP

#include "python_includes.hpp"

#include <boost/functional/hash.hpp>
#include <boost/integer_traits.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/list.hpp>
#include <boost/python/object.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/stl_iterator.hpp>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#if defined(__GNUC__)
#include <cxxabi.h>
#include <cstdlib>
#include <memory>
#endif

#include "../src/config.hpp"
#include "../src/exceptions.hpp"

namespace pyranha
{

// Namespace alias for Boost.Python.
namespace bp = boost::python;

inline std::string demangled_type_name(const std::type_index &t_idx)
{
#if defined(__GNUC__)
	int status = -4;
	// NOTE: abi::__cxa_demangle will return a pointer allocated by std::malloc, which we will delete via std::free.
	std::unique_ptr<char,void(*)(void *)> res{abi::__cxa_demangle(t_idx.name(),nullptr,nullptr,&status),std::free};
	return (status == 0) ? std::string(res.get()) : std::string(t_idx.name());
#else
	// TODO demangling for other platforms.
	return std::string(t_idx.name());
#endif
}

// Meta/macro programming to connect a template template class to a name expressed as a string.
template <typename T>
struct tt_namer
{
	static_assert(!std::is_same<T,T>::value,"No name defined for this template template class.");
};

#define DECLARE_TT_NAMER(TT,tt_name) \
template <typename ... Args> \
struct tt_namer<TT<Args...>> \
{ \
	static const std::string name; \
}; \
template <typename ... Args> \
const std::string tt_namer<TT<Args...>>::name = tt_name;

// Names of Python instances of type getters. We keep track because we do not want multiple instances
// with the same name on the Python side.
extern std::unordered_set<std::string> tg_names;

// Connection between C++ types (type_index) and Python types (encapsulated in bp::object).
// TODO: clean this up on unload? We don't want bp::objects dangling around on shutdown.
extern std::unordered_map<std::type_index,bp::object> et_map;

// Type getter structure. It establishes the connection between a C++ type (the m_t_idx member)
// and its exposed Python counterpart via the call operator, which will query the et_map archive.
struct type_getter
{
	explicit type_getter(const std::type_index &t_idx):m_t_idx(t_idx)
	{}
	bp::object operator()() const
	{
		const auto it = et_map.find(m_t_idx);
		if (it == et_map.end()) {
			::PyErr_SetString(PyExc_TypeError,(std::string("the type '") + demangled_type_name(m_t_idx)
				+ "' has not been exposed").c_str());
			bp::throw_error_already_set();
		}
		return it->second;
	}
	std::string repr() const
	{
		return std::string("Type getter for the C++ type '") + demangled_type_name(m_t_idx) + "'";
	}
	const std::type_index m_t_idx;
};

// Hasher for vector of type indices.
struct v_idx_hasher
{
	std::size_t operator()(const std::vector<std::type_index> &v) const
	{
		std::size_t retval = 0u;
		std::hash<std::type_index> hasher;
		for (const auto &t_idx: v) {
			boost::hash_combine(retval,hasher(t_idx));
		}
		return retval;
	}
};

// Map of generic type getters. Each item in the map is associated to another map, which establishes the
// connection between the concrete set of types used as template parameters for the template template class
// and the final concrete instantiated type.
extern std::unordered_map<std::string,std::unordered_map<std::vector<std::type_index>,std::type_index,v_idx_hasher>> gtg_map;

// Small utility to convert a vector of type indices to a string representation for error reporting purposes.
inline std::string v_t_idx_to_str(const std::vector<std::type_index> &v_t_idx)
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

// Like above, but this instead establishes the connection between a template template class instantiated
// with a certain set of params and a type_getter.
struct generic_type_getter
{
	explicit generic_type_getter(const std::string &name):m_name(name) {}
	// Get the type getter corresponding to the C++ type defined by name<l[0],l[1],...>,
	// where l is a list of type getters.
	type_getter operator()(bp::list l) const
	{
		// We assume that this is created concurrently with the exposition of the gtg
		// (and hence its registration on the C++ and Python sides).
		piranha_assert(gtg_map.find(m_name) != gtg_map.end());
		// Convert the list to a vector of type idx objects.
		std::vector<std::type_index> v_t_idx;
		bp::stl_input_iterator<type_getter> it(l), end;
		for (; it != end; ++it) {
			v_t_idx.push_back((*it).m_t_idx);
		}
		const auto it1 = gtg_map[m_name].find(v_t_idx);
		if (it1 == gtg_map[m_name].end()) {
			::PyErr_SetString(PyExc_TypeError,(std::string("the generic type getter '") + m_name +
				std::string("' has not been instantiated with the type pack ") + v_t_idx_to_str(v_t_idx)).c_str());
			bp::throw_error_already_set();
		}
		return type_getter{it1->second};
	}
	std::string repr() const
	{
		return std::string("Generic type getter for the type '") + m_name + "'";
	}
	const std::string m_name;
};

template <typename T>
inline void expose_type_getter(const std::string &name)
{
	// We do not want to have duplicate instances on the Python side.
	if (tg_names.find(name) != tg_names.end()) {
		piranha_throw(std::runtime_error,std::string("a type getter called '") + name + "' has already been instantiated");
	}
	tg_names.insert(name);
	type_getter tg(std::type_index(typeid(T)));
	bp::scope().attr("types").attr(name.c_str()) = tg;
}

// Convert variadic arguments into a vector of type indices.
template <typename Tuple, std::size_t Idx = 0u, typename = void>
struct vargs_to_v_t_idx
{
	void operator()(std::vector<std::type_index> &v_t_idx) const
	{
		static_assert(Idx < boost::integer_traits<std::size_t>::const_max,"Overflow error.");
		v_t_idx.push_back(std::type_index(typeid(typename std::tuple_element<Idx,Tuple>::type)));
		vargs_to_v_t_idx<Tuple,static_cast<std::size_t>(Idx + 1u)>()(v_t_idx);
	}
};

template <typename Tuple, std::size_t Idx>
struct vargs_to_v_t_idx<Tuple,Idx,typename std::enable_if<Idx == std::tuple_size<Tuple>::value>::type>
{
	void operator()(std::vector<std::type_index> &) const
	{}
};

// Expose generic type getter.
template <template <typename ...> class TT, typename ... Args>
inline void expose_generic_type_getter()
{
	const std::string name = tt_namer<TT<Args...>>::name;
	// Add a new generic type getter if it does not exist already.
	if (gtg_map.find(name) == gtg_map.end()) {
		generic_type_getter gtg(name);
		bp::scope().attr("types").attr(name.c_str()) = gtg;
	}
	// Convert the variadic args to a vector of type indices.
	std::vector<std::type_index> v_t_idx;
	vargs_to_v_t_idx<std::tuple<Args...>>()(v_t_idx);
	// NOTE: the new key in gtg_map, if needed, will be created by the first call
	// to gtg_map[name].
	if (gtg_map[name].find(v_t_idx) != gtg_map[name].end()) {
		piranha_throw(std::runtime_error,std::string("the generic type getter '") + name +
			std::string("' has already been instantiated with the type pack ") + v_t_idx_to_str(v_t_idx));
	}
	gtg_map[name].emplace(std::make_pair(v_t_idx,std::type_index(typeid(TT<Args...>))));
}

}

#endif
