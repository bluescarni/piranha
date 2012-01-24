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

#ifndef PIRANHA_DETERMINE_INTEGER_TYPES_HPP
#define PIRANHA_DETERMINE_INTEGER_TYPES_HPP

#include <algorithm>
#include <boost/integer_traits.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <type_traits>

#include "../extended_integer_types.hpp"
#include "../print_coefficient.hpp" // For converting GCC int128_t.
#include "../integer.hpp"

namespace piranha
{

namespace detail
{

struct int_determiner
{
	typedef boost::mpl::vector<long long,long,int,short,signed char> int_candidates;
	typedef boost::mpl::vector<long long,long,int,short,signed char
#if defined(PIRANHA_GCC_INT128_T)
		,gcc_int128
#endif
		> wide_int_candidates;
	template <typename T>
	static void print_type()
	{
#define piranha_int_determiner_print_case(U) \
if (std::is_same<T,U>::value) { \
	std::cout << #U; \
} else
		piranha_int_determiner_print_case(long long)
		piranha_int_determiner_print_case(long)
		piranha_int_determiner_print_case(int)
		piranha_int_determiner_print_case(short)
		piranha_int_determiner_print_case(signed char)
#if defined(PIRANHA_GCC_INT128_T)
		piranha_int_determiner_print_case(piranha::gcc_int128)
#endif
		{
			std::cout << "Type error, aborting.\n";
			std::exit(1);
		}
#undef piranha_int_determiner_print_case
	}
	template <typename T>
	static integer integer_cast(const T &value)
	{
		return integer(value);
	}
#if defined(PIRANHA_GCC_INT128_T)
	// For GCC 128-bit integer, go through string conversion.
	static integer integer_cast(const gcc_int128 &n)
	{
		std::ostringstream os;
		print_coefficient(os,n);
		return integer(os.str());
	}
#endif
	struct range_checker
	{
		template <typename Int>
		struct runner
		{
			template <typename WideInt>
			void operator()(const WideInt &)
			{
				const integer min = integer_cast(boost::integer_traits<Int>::const_min),
					max = integer_cast(boost::integer_traits<Int>::const_max),
					w_min = integer_cast(boost::integer_traits<WideInt>::const_min),
					w_max = integer_cast(boost::integer_traits<WideInt>::const_max);
				// NOTE: these checks assume that mins are strictly negative and maxs are strictly positive.
				// Addition.
				if (min * 2 < w_min || max * 2 > w_max) {
					return;
				}
				// Subtraction.
				if (min - max < w_min || max - min > w_max) {
					return;
				}
				// Multiplication.
				if (min * max < w_min || std::max<integer>(min * min,max * max) > w_max) {
					return;
				}
				// Division.
				if (-max < w_min || -min > w_max) {
					return;
				}
				// Multiply-accumulate.
				if (min * max + min < w_min || std::max<integer>(min * min,max * max) + max > w_max) {
					return;
				}
				print_type<Int>();
				std::cout << '\n';
				print_type<WideInt>();
				std::cout << '\n';
				std::exit(0);
			}
		};
		template <typename Int>
		void operator()(const Int &)
		{
			boost::mpl::for_each<wide_int_candidates>(runner<Int>());
		}
	};
	int_determiner()
	{
		boost::mpl::for_each<int_candidates>(range_checker());
		std::cout << "No suitable integer types found, aborting.\n";
		std::exit(1);
	}
};

inline void determine_integer_types()
{
	int_determiner d = int_determiner();
}

}

}

#endif
