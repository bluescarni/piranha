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

#include <exception>
#include <iostream>
#include <system_error>
#include <vector>

#include "thread_group.hpp"
#include "threading.hpp"

namespace piranha
{

/// Destructor.
/**
 * Will call join_all() before exiting. If join_all() throws, the program will abort via std::terminate().
 */
thread_group::~thread_group()
{
	try {
		join_all();
	} catch (...) {
		std::terminate();
	}
}

/// Join all threads in the group.
/**
 * Will join iteratively all threads belonging to the group. It is safe to call this method multiple times.
 * Any failure in the <tt>join()</tt> method of piranha::thread will result in the termination of the program via std::abort().
 * 
 * @throws std::system_error in case of failure(s) by threading primitives.
 */
void thread_group::join_all()
{
	lock_guard<mutex>::type lock(m_mutex);
	for (container_type::iterator it = m_threads.begin(); it != m_threads.end(); ++it) {
		if (it->joinable()) {
			try {
				it->join();
			} catch (const std::system_error &se) {
				std::cout << "thread_group::join_all() caused program abortion; error message is:\n";
				std::cout << se.what() << '\n';
				std::terminate();
			}
		}
	}
}

}
