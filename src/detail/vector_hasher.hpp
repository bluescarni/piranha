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

#ifndef PIRANHA_DETAIL_VECTOR_HASHER_HPP
#define PIRANHA_DETAIL_VECTOR_HASHER_HPP

#include <boost/functional/hash.hpp>
#include <cstddef>
#include <functional>

namespace piranha { namespace detail {

template <typename Vector>
inline std::size_t vector_hasher(const Vector &v)
{
	using value_type = typename Vector::value_type;
	const auto size = v.size();
	switch (size)
	{
		case 0u:
			return 0u;
		case 1u:
		{
			std::hash<value_type> hasher;
			return hasher(v[0u]);
		}
	}
	std::hash<value_type> hasher;
	std::size_t retval = hasher(v[0u]);
	for (decltype(v.size()) i = 1u; i < size; ++i) {
		boost::hash_combine(retval,hasher(v[i]));
	}
	return retval;
}

}}

#endif
