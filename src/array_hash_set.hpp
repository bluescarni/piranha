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

#ifndef PIRANHA_ARRAY_HASH_SET_HPP
#define PIRANHA_ARRAY_HASH_SET_HPP

#include <cstddef>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>

#include "small_vector.hpp"

namespace piranha
{

template <typename T, typename Hash = std::hash<T>, typename Pred = std::equal_to<T>>
class array_hash_set
{
		using bucket_type = small_vector<T,std::integral_constant<std::size_t,1u>>;
		using container_type = bucket_type *;
		using allocator_type = std::allocator<bucket_type>;
	public:
		/// Size type.
		using size_type = std::size_t;
		/// Functor type for the calculation of hash values.
		using hasher = Hash;
		/// Functor type for comparing the items in the set.
		using key_equal = Pred;
		/// Key type.
		using key_type = T;
	private:
		using pack_type = std::tuple<container_type,
			hasher,key_equal,allocator_type>;
		pack_type	m_pack;
		size_type	m_log2_size;
		size_type	m_n_elements;
};

}

#endif
