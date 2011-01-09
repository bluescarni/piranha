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

#ifndef PIRANHA_MF_INT_HPP
#define PIRANHA_MF_INT_HPP

#include <array>
#include <boost/integer_traits.hpp>
#include <cstdint>
#include <type_traits>

#include "config.hpp"

namespace piranha
{

/// Traits for the maximum-width "fast" integer type.
/**
 * This class defines, describes and operates on the "fast" integer type of maximum width
 * available on the host platform.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
class mf_int_traits
{
	public:
		/// Maximum-width fast signed integer type.
#if defined(PIRANHA_64BIT_MODE)
		typedef std::int64_t type;
#else
		typedef std::int32_t type;
#endif
		/// Maximum-width fast unsigned integer.
		typedef std::make_unsigned<type>::type utype;
		/// Number of bits of the maximum-width fast integer.
		/**
		 * Typically this number will be either 64 or 32, depending on the platform.
		 */
#if defined(PIRANHA_64BIT_MODE)
		static const utype nbits = 64;
#else
		static const utype nbits = 32;
#endif
		/// Most significant bit.
		/**
		 * @param n unsigned integer whose most significant bit will be computed.
		 * 
		 * @return the position, starting from 0, of the most significant bit (i.e., truncated base-2 logarithm) of input \p n.
		 * If \p n is zero, -1 will be returned.
		 * 
		 * @see http://www-graphics.stanford.edu/~seander/bithacks.html
		 */
		static int msb(const utype &n)
		{
			int retval;
			msb_impl<nbits / static_cast<utype>(2)>::run(retval,n);
			return retval;
		}
	private:
		template <utype NextShift, utype TotalShift = 0>
		struct msb_impl
		{
			static void run(int &retval, const utype &n)
			{
				static_assert(NextShift != 0,"Invalid value for next shift.");
				static_assert(TotalShift < boost::integer_traits<utype>::const_max - NextShift,"Overflow error.");
				const utype new_n = n >> NextShift;
				if (new_n) {
					msb_impl<NextShift / static_cast<utype>(2), TotalShift + NextShift>::run(retval,new_n);
				} else {
					msb_impl<NextShift / static_cast<utype>(2), TotalShift>::run(retval,n);
				}
			}
		};
		template <utype TotalShift>
		struct msb_impl<static_cast<utype>(8),TotalShift>
		{
			static void run(int &retval, const utype &n)
			{
				static_assert(TotalShift < static_cast<unsigned>(boost::integer_traits<int>::const_max),"Overflow error.");
				static_assert(static_cast<int>(TotalShift) < boost::integer_traits<int>::const_max - 15,"Overflow error.");
				const utype new_n = n >> static_cast<utype>(8);
				if (new_n) {
					piranha_assert(new_n < log_table_256.size());
					retval = static_cast<int>(TotalShift) + 8 + log_table_256[new_n];
				} else {
					piranha_assert(n < log_table_256.size());
					retval = static_cast<int>(TotalShift) + log_table_256[n];
				}
			}
		};
		// NOTE: try with char here instead?
		typedef std::array<int,256> table_type;
		static table_type init_log_table_256();
		static const table_type log_table_256;
};

/// Maximum-width "fast" signed integer.
typedef mf_int_traits::type mf_int;

/// Maximum-width "fast" unsigned integer.
typedef mf_int_traits::utype mf_uint;

}

#endif
