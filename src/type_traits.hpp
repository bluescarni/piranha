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

#ifndef PIRANHA_TYPE_TRAITS_HPP
#define PIRANHA_TYPE_TRAITS_HPP

#include <type_traits>

namespace piranha
{

/// Trivially copyable type trait.
/**
 * This type trait is intended to be equivalent to \p std::is_trivially_copyable, which,
 * at the time of this writing, has not been implemented yet in GCC. If the compiler
 * is GCC, then GCC's intrinsic type trait \p __has_trivial_copy will be used. Otherwise,
 * \p std::is_trivially_copyable will be used instead.
 * 
 * @see http://gcc.gnu.org/onlinedocs/gcc/Type-Traits.html
 */
template <typename T>
struct is_trivially_copyable: std::integral_constant<bool,
#if defined(__GNUC__)
	__has_trivial_copy(T)
#else
	std::is_trivially_copyable<T>::value
#endif
	>
{};

}

#endif
