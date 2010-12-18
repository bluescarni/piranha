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

#ifndef PIRANHA_THREAD_MANAGEMENT_HPP
#define PIRANHA_THREAD_MANAGEMENT_HPP

#include <mutex>
#include <tuple>

namespace piranha
{

/// Thread management.
/**
 * This class contains static methods to manage thread behaviour. The methods of this class, unless otherwise
 * specified, are thread-safe.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
class thread_management
{
	public:
		static void bind_to_proc(unsigned);
		static std::tuple<bool,unsigned> bound_proc();
	private:
		static std::mutex m_mutex;
};

}

#endif