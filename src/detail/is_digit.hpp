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

#ifndef PIRANHA_DETAIL_IS_DIGIT_HPP
#define PIRANHA_DETAIL_IS_DIGIT_HPP

#include <algorithm>

namespace piranha
{

namespace detail
{

// NOTE: check this answer:
// http://stackoverflow.com/questions/13827180/char-ascii-relation
// """
// The mapping of integer values for characters does have one guarantee given
// by the Standard: the values of the decimal digits are continguous.
// (i.e., '1' - '0' == 1, ... '9' - '0' == 9)
// """
// It should be possible to implement this with a binary search.
inline bool is_digit(char c)
{
	const char digits[] = "0123456789";
	return std::find(digits,digits + 10,c) != (digits + 10);
}

}

}

#endif
