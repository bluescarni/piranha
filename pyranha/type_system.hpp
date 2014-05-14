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

#include <boost/integer_traits.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/list.hpp>
#include <boost/python/object.hpp>
#include <boost/python/scope.hpp>
#include <cstddef>
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

#include "../src/exceptions.hpp"

namespace pyranha
{

// Namespace alias for Boost.Python.
namespace bp = boost::python;

std::string demangled_type_name(const std::type_index &);

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
	static const std::string orig_name; \
}; \
template <typename ... Args> \
const std::string tt_namer<TT<Args...>>::name = tt_name;\
template <typename ... Args> \
const std::string tt_namer<TT<Args...>>::orig_name = #TT;

// Names of Python instances of type generators. We keep track because we do not want multiple instances
// with the same name on the Python side.
extern std::unordered_set<std::string> tg_names;

// Connection between C++ types (type_index) and Python types (encapsulated in bp::object).
extern std::unordered_map<std::type_index,bp::object> et_map;

// Type generator structure. It establishes the connection between a C++ type (the m_t_idx member)
// and its exposed Python counterpart via the call operator, which will query the et_map archive.
struct type_generator
{
	explicit type_generator(const std::type_index &t_idx):m_t_idx(t_idx) {}
	bp::object operator()() const;
	std::string repr() const;
	const std::type_index m_t_idx;
};

// Hasher for vector of type indices.
struct v_idx_hasher
{
	std::size_t operator()(const std::vector<std::type_index> &) const;
};

// Map of generic type generators. Each item in the map is associated to another map, which establishes the
// connection between the concrete set of types used as template parameters for the template template class
// and the final concrete instantiated type.
extern std::unordered_map<std::string,std::unordered_map<std::vector<std::type_index>,std::type_index,v_idx_hasher>> gtg_map;

// Small utility to convert a vector of type indices to a string representation for error reporting purposes.
std::string v_t_idx_to_str(const std::vector<std::type_index> &);

// Like above, but this instead establishes the connection between a template template class instantiated
// with a certain set of params and a type_generator.
struct generic_type_generator
{
	explicit generic_type_generator(const std::string &name, const std::string &orig_name):
		m_name(name),m_orig_name(orig_name)
	{}
	// Get the type generator corresponding to the C++ type defined by name<l[0],l[1],...>,
	// where l is a list of type generators.
	type_generator operator()(bp::list) const;
	std::string repr() const;
	const std::string	m_name;
	const std::string	m_orig_name;
};

template <typename T>
inline void expose_type_generator(const std::string &name)
{
	// We do not want to have duplicate instances on the Python side.
	if (tg_names.find(name) != tg_names.end()) {
		piranha_throw(std::runtime_error,std::string("a type generator called '") + name + "' has already been instantiated");
	}
	tg_names.insert(name);
	type_generator tg(std::type_index(typeid(T)));
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

// Expose generic type generator.
template <template <typename ...> class TT, typename ... Args>
inline void expose_generic_type_generator()
{
	const std::string name = tt_namer<TT<Args...>>::name, orig_name = tt_namer<TT<Args...>>::orig_name;
	// Add a new generic type generator if it does not exist already.
	if (gtg_map.find(name) == gtg_map.end()) {
		generic_type_generator gtg(name,orig_name);
		bp::scope().attr("types").attr(name.c_str()) = gtg;
	}
	// Convert the variadic args to a vector of type indices.
	std::vector<std::type_index> v_t_idx;
	vargs_to_v_t_idx<std::tuple<Args...>>()(v_t_idx);
	// NOTE: the new key in gtg_map, if needed, will be created by the first call
	// to gtg_map[name].
	if (gtg_map[name].find(v_t_idx) != gtg_map[name].end()) {
		piranha_throw(std::runtime_error,std::string("the generic type generator '") + name +
			std::string("' has already been instantiated with the type pack ") + v_t_idx_to_str(v_t_idx));
	}
	gtg_map[name].emplace(std::make_pair(v_t_idx,std::type_index(typeid(TT<Args...>))));
}

}

#endif
