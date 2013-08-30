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

#ifndef PIRANHA_DETAIL_PREPARE_FOR_PRINT_HPP
#define PIRANHA_DETAIL_PREPARE_FOR_PRINT_HPP

#include <type_traits>

namespace piranha
{

namespace detail
{

// Helper to print char types without displaying garbage.
template <typename T>
struct pfp_special
{
	static const bool value = std::is_same<T,char>::value || std::is_same<T,unsigned char>::value || std::is_same<T,signed char>::value;
};

template <typename T>
inline const T &prepare_for_print(const T &x, typename std::enable_if<!pfp_special<T>::value>::type * = nullptr)
{
	return x;
}

template <typename T>
inline int prepare_for_print(const T &n, typename std::enable_if<pfp_special<T>::value && std::is_signed<T>::value>::type * = nullptr)
{
	return static_cast<int>(n);
}

template <typename T>
inline unsigned prepare_for_print(const T &n, typename std::enable_if<pfp_special<T>::value && std::is_unsigned<T>::value>::type * = nullptr)
{
	return static_cast<unsigned>(n);
}

}

}

#endif
