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

#ifndef PIRANHA_TIMEIT_HPP
#define PIRANHA_TIMEIT_HPP

#include <chrono>
#include <iostream>

namespace piranha
{

/// Simple timing function.
/**
 * Will print to \p std::cout the wall clock time (in milliseconds) elapsed during the invocation of function \p f
 * with \p args as its arguments. The output of <tt>f(args)</tt> will be returned.
 * 
 * @param[in] f function that will be invoked.
 * @param[in] args arguments to \p f.
 * 
 * @return the output of invoking \p f with arguments \p args.
 * 
 * @throws unspecified any exception thrown by:
 * - the invocation of \p f,
 * - clock primitives in the standard library.
 */
template <typename F, typename... Args>
inline auto timeit(F &&f, Args && ... args) -> decltype(f(std::forward<Args>(args)...))
{
	struct timer
	{
		timer():m_start(std::chrono::steady_clock::now()) {}
		~timer()
		{
			std::cout << "Elapsed time: " << (std::chrono::steady_clock::now() - m_start).count() / 1000. << '\n';
		}
		const decltype(std::chrono::steady_clock::now()) m_start;
	};
	timer t;
	return f(std::forward<Args>(args)...);
}

}

#endif
