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

#include <algorithm>
#include <stdexcept>

#include "exceptions.hpp"
#include "runtime_info.hpp"
#include "settings.hpp"
#include "threading.hpp"

namespace piranha
{

unsigned settings::m_n_threads = std::max<unsigned>(runtime_info::hardware_concurrency(),static_cast<unsigned>(1));
mutex settings::m_mutex;

/// Get the number of threads available for use by piranha.
/**
 * The initial value upon program startup is set to the maximum between 1 and piranha::runtime_info::hardware_concurrency().
 * 
 * @return the number of threads that will be available for use by piranha.
 * 
 * @throws std::system_error in case of failure(s) by threading primitives.
 */
unsigned settings::get_n_threads()
{
	lock_guard<mutex>::type lock(m_mutex);
	return m_n_threads;
}

/// Set the number of threads available for use by piranha.
/**
 * @param[in] n the desired number of threads.
 * 
 * @throws std::invalid_argument if <tt>n == 0</tt>.
 * @throws std::system_error in case of failure(s) by threading primitives.
 */
void settings::set_n_threads(unsigned n)
{
	lock_guard<mutex>::type lock(m_mutex);
	if (n == 0) {
		piranha_throw(std::invalid_argument,"the number of threads must be strictly positive");
	}
	m_n_threads = n;
}

}
