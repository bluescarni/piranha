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

#include "python_includes.hpp"

#include <boost/python/object.hpp>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#if defined(__GNUC__)
#include <cxxabi.h>
#include <cstdlib>
#include <memory>
#endif

#include "type_system.hpp"

namespace pyranha
{

namespace bp = boost::python;

std::unordered_set<std::string> tg_names;
std::unordered_map<std::type_index,bp::object> et_map;
std::unordered_map<std::string,std::unordered_map<std::vector<std::type_index>,std::type_index,v_idx_hasher>> gtg_map;

std::string demangled_type_name(const std::type_index &t_idx)
{
#if defined(__GNUC__)
	int status = -4;
	// NOTE: abi::__cxa_demangle will return a pointer allocated by std::malloc, which we will delete via std::free.
	std::unique_ptr<char,void(*)(void *)> res{::abi::__cxa_demangle(t_idx.name(),nullptr,nullptr,&status),std::free};
	return (status == 0) ? std::string(res.get()) : std::string(t_idx.name());
#else
	// TODO demangling for other platforms. E.g.,
	// http://stackoverflow.com/questions/13777681/demangling-in-msvc
	return std::string(t_idx.name());
#endif
}

}
