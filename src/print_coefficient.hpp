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

#ifndef PIRANHA_PRINT_COEFFICIENT_HPP
#define PIRANHA_PRINT_COEFFICIENT_HPP

#include <iostream>

namespace piranha
{

/// Default functor for piranha::print_coefficient().
/**
 * This functor should be specialised via the \p std::enable_if mechanism.
 */
template <typename T, typename Enable = void>
struct print_coefficient_impl
{
	/// Call operator.
	/**
	 * The default call operator will print to stream the object.
	 * 
	 * @param[in] os target stream.
	 * @param[in] cf coefficient to be printed.
	 * 
	 * @throws unspecified any exception thrown by printing \p cf to stream \p os.
	 */
	void operator()(std::ostream &os, const T &cf) const
	{
		os << cf;
	}
};

/// Print series coefficient.
/**
 * This function is used in the stream operator overload for piranha::series when
 * printing coefficients.
 * 
 * The implementation uses the call operator of the piranha::print_coefficient_impl functor.
 * Specialisations of piranha::print_coefficient_impl can be defined to customize the behaviour.
 * 
 * @param[in] os target stream.
 * @param[in] cf coefficient object to be printed.
 * 
 * @throws unspecified any exception thrown by the call operator of piranha::print_coefficient_impl.
 */
template <typename T>
inline void print_coefficient(std::ostream &os, const T &cf)
{
	print_coefficient_impl<T>()(os,cf);
}

}

#endif
