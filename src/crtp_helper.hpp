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

#ifndef PIRANHA_CRTP_HELPER_HPP
#define PIRANHA_CRTP_HELPER_HPP

namespace piranha
{

/// CRTP helper class.
/**
 * Small class that provides convenience methods when using the curiously-recurring
 * template pattern (CRTP). Inheriting from this class will allow for easy access
 * to the derived class in the CRTP.
 * 
 * @see http://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename Derived>
struct crtp_helper
{
	/// Pointer to derived class.
	/**
	 * @return pointer to the derived class.
	 */
	Derived *derived()
	{
		return static_cast<Derived *>(this);
	}
	/// Const pointer to the derived class.
	/**
	 * @return const pointer to the derived class.
	 */
	Derived const *derived() const
	{
		return static_cast<Derived const *>(this);
	}
};

}

#endif
