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

#ifndef PIRANHA_CONFIG_GCC_HPP
#define PIRANHA_CONFIG_GCC_HPP

#if __GNUC__  < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 5)
	#error Minimum GCC version supported is 4.5.0.
#endif

// c++0x features depending on the GCC version.
// NOTE: when removing support for GCC < 4.6 remember that
// we can default move assignment in derived classes.
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 6
	#define piranha_nullptr nullptr
	// noexcept
	#define piranha_noexcept_spec(expr) noexcept(expr)
	#define piranha_noexcept_op(expr) noexcept(expr)
	#define PIRANHA_HAVE_NOEXCEPT
#else
	#define piranha_nullptr (NULL)
	#define piranha_noexcept_spec(expr)
	#define piranha_noexcept_op(expr) (true)
#endif

// Explicit override available from 4.7.
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 7
	#define piranha_override override
#else
	#define piranha_override
#endif

#include <chrono>

namespace piranha
{
namespace detail
{
// Steady vs monotonic clock.
typedef
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 7
	std::chrono::steady_clock
#else
	std::chrono::monotonic_clock
#endif
steady_clock;
}
}

#define likely(x) __builtin_expect((x),1)
#define unlikely(x) __builtin_expect((x),0)

// Visibility support.
#if defined(_WIN32)
	#if defined(PIRANHA_BUILDING_DLL)
		#define PIRANHA_PUBLIC __declspec(dllexport)
	#elif defined(PIRANHA_USING_DLL)
		#define PIRANHA_PUBLIC __declspec(dllimport)
	#else
		#define PIRANHA_PUBLIC
	#endif
#else
	#define PIRANHA_PUBLIC __attribute__ ((visibility ("default")))
#endif

#endif
