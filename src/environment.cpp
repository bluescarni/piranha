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

#include <cstdlib>
#include <iostream>

#include "detail/mpfr.hpp"
#include "environment.hpp"
#include "threading.hpp"

namespace piranha
{

mutex environment::m_mutex;
bool environment::m_inited = false;
bool environment::m_shutdown = false;

void environment::cleanup_function()
{
	std::cout << "Freeing MPFR caches.\n";
	::mpfr_free_cache();
	std::cout << "Setting shutdown flag.\n";
	environment::m_shutdown = true;
}

/// Environment constructor.
/**
 * Will perform the initialisation of the runtime environment in a thread-safe manner.
 * 
 * @throws unspecified any exception thrown by threading primitives.
 */
environment::environment()
{
	lock_guard<mutex>::type lock(m_mutex);
	if (m_inited) {
		return;
	}
	if (std::atexit(environment::cleanup_function)) {
		std::cout << "Unable to register cleanup function with std::atexit().\n";
		std::abort();
	}
	m_inited = true;
}

}
