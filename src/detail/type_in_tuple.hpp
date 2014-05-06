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

#ifndef PIRANHA_DETAIL_TYPE_IN_TUPLE_HPP
#define PIRANHA_DETAIL_TYPE_IN_TUPLE_HPP

#include <cstddef>
#include <limits>
#include <tuple>
#include <type_traits>

namespace piranha { namespace detail {

// TMP to check if a type is in the tuple.
template <typename T, typename Tuple, std::size_t I = 0u, typename Enable = void>
struct type_in_tuple
{
	static_assert(I < std::numeric_limits<std::size_t>::max(),"Overflow error.");
	static const bool value = std::is_same<T,typename std::tuple_element<I,Tuple>::type>::value ||
		type_in_tuple<T,Tuple,I + 1u>::value;
};
template <typename T, typename Tuple, std::size_t I>
struct type_in_tuple<T,Tuple,I,typename std::enable_if<I == std::tuple_size<Tuple>::value>::type>
{
	static const bool value = false;
};

}}

#endif
