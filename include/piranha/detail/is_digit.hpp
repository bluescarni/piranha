/* Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)

This file is part of the Piranha library.

The Piranha library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The Piranha library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the Piranha library.  If not,
see https://www.gnu.org/licenses/. */

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
    return std::find(digits, digits + 10, c) != (digits + 10);
}
}
}

#endif
