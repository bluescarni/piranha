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

#include <list>
#include <memory>
#include <mutex>
#include <thread>

#include "thread_group.hpp"

namespace piranha
{

/// Destructor.
/**
 * Will call join_all() before exiting.
 */
thread_group::~thread_group()
{
	join_all();
}

/// Join all threads in the group.
/**
 * Will join iteratively all threads belonging to the group. It is safe to call this method multiple times.
 */
void thread_group::join_all()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	for (std::list<std::unique_ptr<std::thread>>::iterator it = m_threads.begin(); it != m_threads.end(); ++it) {
		if ((*it)->joinable()) {
			(*it)->join();
		}
	}
}

}
