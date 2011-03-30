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

#ifndef PIRANHA_UTILS_HPP
#define PIRANHA_UTILS_HPP

/** \file utils.hpp
 * \brief File containing misc utility functions and classes.
 */

#include <cstddef>
#include <tuple>
#include <type_traits>

namespace piranha
{

/// Iterate over tuple.
/**
 * This structure's overloaded <tt>operator()</tt> will apply recursively a \p Functor over all members of the \p Tuple.
 */
template <typename Functor, typename Tuple, std::size_t N = 0, typename Enable = void>
struct tuple_iterate
{
	/// Call operator.
	/**
	 * @param[in] f \p Functor that will be applied to the elements of the tuple.
	 * @param[in] t \p Tuple on whose elements the functor will be applied.
	 * @param[in] params arguments that will be perfectly forwarded to the functor.
	 */
	template <typename... Args>
	void operator()(const Functor &f, Tuple &t, Args && ... params) const
	{
		f(std::get<N>(t),std::forward<Args>(params)...);
		tuple_iterate<Functor,Tuple,N + std::size_t(1),Enable>()(f,t,std::forward<Args>(params)...);
	}
};

template <typename Functor, typename Tuple, std::size_t N>
struct tuple_iterate<Functor,Tuple,N,typename std::enable_if<std::tuple_size<Tuple>::value == N>::type>
{
	template <typename... Args>
	void operator()(const Functor &, Tuple &, Args && ...) const
	{}
};

}

#endif
