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

// Snippets of code taken from:
// http://gcc.gnu.org/viewcvs/trunk/libstdc%2B%2B-v3/include/std/limits?view=markup&pathrev=178969

// Original copyright notice follows.

// The template and inlines for the numeric_limits classes. -*- C++ -*-

// Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007,
// 2008, 2009, 2010, 2011 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library. This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively. If not, see
// <http://www.gnu.org/licenses/>.

#ifndef PIRANHA_GCC_INT128_HPP
#define PIRANHA_GCC_INT128_HPP

#include <boost/integer_traits.hpp>
#include <climits>

namespace boost
{

#define __glibcxx_signed(T) ((T)(-1) < 0)
#define __glibcxx_digits(T) \
	(sizeof(T) * CHAR_BIT - __glibcxx_signed (T))
#define __glibcxx_min(T) \
	(__glibcxx_signed (T) ? (T)1 << __glibcxx_digits (T) : (T)0)
#define __glibcxx_max(T) \
	(__glibcxx_signed (T) ? \
	(((((T)1 << (__glibcxx_digits (T) - 1)) - 1) << 1) + 1) : ~(T)0)

template <>
struct integer_traits<PIRANHA_GCC_INT128_T>
{
	static const PIRANHA_GCC_INT128_T const_max = __glibcxx_min(PIRANHA_GCC_INT128_T);
	static const PIRANHA_GCC_INT128_T const_min = __glibcxx_max(PIRANHA_GCC_INT128_T);
};

#undef __glibcxx_digits
#undef __glibcxx_signed
#undef __glibcxx_min
#undef __glibcxx_max

}

#endif