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

#include <algorithm>
#include <boost/integer_traits.hpp>
#include <boost/lexical_cast.hpp>
#include <climits>
#include <iostream>
#include <iterator>
#include <type_traits>
#include <vector>

#include "../print_coefficient.hpp"

namespace piranha
{

/// 128-bit signed integer type.
/**
 * This type is available on certain versions of the GCC compiler (typically on 64-bit platforms).
 * Its availability is signalled by the presence of the \p PIRANHA_GCC_INT128_T definition.
 */
typedef PIRANHA_GCC_INT128_T gcc_int128;

/// 128-bit unsigned integer type.
/**
 * This type is available on certain versions of the GCC compiler (typically on 64-bit platforms).
 * Its availability is signalled by the presence of the \p PIRANHA_GCC_INT128_T definition.
 */
typedef PIRANHA_GCC_UINT128_T gcc_uint128;

/// Specialisation of piranha::print_coefficient() for piranha::gcc_int128 and piranha::gcc_uint128.
/**
 * Will print to stream a decimal representation of the coefficient.
 */
template <typename T>
struct print_coefficient_impl<T,typename std::enable_if<
	std::is_same<T,gcc_int128>::value || std::is_same<T,gcc_uint128>::value>::type>
{
	/// Call operator for piranha::gcc_uint128.
	/**
	 * @param[in] os target stream.
	 * @param[in] n coefficient to be printed.
	 * 
	 * @throws unspecified any exception thrown by memory allocation errors in standard containers
	 * or by streaming instances of \p char to \p os.
	 */
	void operator()(std::ostream &os, const gcc_uint128 &n) const
	{
		if (!n) {
			os << '0';
			return;
		}
		std::vector<char> buffer;
		auto n_copy = n;
		while (n_copy) {
			const auto digit = unsigned(n_copy % 10u);
			buffer.push_back(boost::lexical_cast<char>(digit));
			n_copy /= 10u;
		}
		std::ostream_iterator<char> out_it(os);
		std::copy(buffer.rbegin(),buffer.rend(),out_it);
	}
	/// Call operator for piranha::gcc_int128.
	/**
	 * @param[in] os target stream.
	 * @param[in] n coefficient to be printed.
	 * 
	 * @throws unspecified any exception thrown by memory allocation errors in standard containers
	 * or by streaming instances of \p char to \p os.
	 */
	void operator()(std::ostream &os, const gcc_int128 &n) const
	{
		if (n >= 0) {
			operator()(os,gcc_uint128(n));
		} else {
			os << '-';
			operator()(os,-static_cast<gcc_uint128>(n));
		}
	}
};

}

namespace boost
{

#define piranha_glibcxx_signed(T) ((T)(-1) < 0)
#define piranha_glibcxx_digits(T) \
	(sizeof(T) * CHAR_BIT - piranha_glibcxx_signed (T))
#define piranha_glibcxx_min(T) \
	(piranha_glibcxx_signed (T) ? (T)1 << piranha_glibcxx_digits (T) : (T)0)
#define piranha_glibcxx_max(T) \
	(piranha_glibcxx_signed (T) ? \
	(((((T)1 << (piranha_glibcxx_digits (T) - 1)) - 1) << 1) + 1) : ~(T)0)

template <>
struct integer_traits<piranha::gcc_int128>
{
	static const piranha::gcc_int128 const_max = piranha_glibcxx_max(piranha::gcc_int128);
	static const piranha::gcc_int128 const_min = piranha_glibcxx_min(piranha::gcc_int128);
};

#undef piranha_glibcxx_digits
#undef piranha_glibcxx_signed
#undef piranha_glibcxx_min
#undef piranha_glibcxx_max

}

#endif
