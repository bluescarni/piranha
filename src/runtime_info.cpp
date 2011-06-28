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

#if defined(__linux__)

extern "C"
{
#include <sys/sysinfo.h>
#include <unistd.h>
}

#elif defined(__FreeBSD__)

extern "C"
{
#include <sys/types.h>
#include <sys/sysctl.h>
}

#elif defined(_WIN32)

extern "C"
{
#include <Windows.h>
}

#endif

#include <boost/numeric/conversion/cast.hpp>

#include "runtime_info.hpp"
#include "threading.hpp"

namespace piranha
{

const thread_id runtime_info::m_main_thread_id = {this_thread::get_id()};
mutex runtime_info::m_mutex;

/// Hardware concurrency.
/**
 * @return number of detected hardware thread contexts (typically equal to the number of logical CPU cores), or 0 if
 * the detection fails.
 * 
 * @throws std::system_error in case of failure(s) by threading primitives.
 */
unsigned runtime_info::hardware_concurrency()
{
	lock_guard<mutex>::type lock(m_mutex);
#if defined(__linux__)
	int candidate = ::get_nprocs();
	return (candidate <= 0) ? 0u : static_cast<unsigned>(candidate);
#elif defined(__FreeBSD__)
	int count;
	std::size_t size = sizeof(count);
	return ::sysctlbyname("hw.ncpu",&count,&size,NULL,0) ? 0 : static_cast<unsigned>(count);
#elif defined(_WIN32)
	SYSTEM_INFO info = SYSTEM_INFO();
	::GetSystemInfo(&info);
	// info.dwNumberOfProcessors is a dword, i.e., an unsigned integer.
	return info.dwNumberOfProcessors;
#else
	return 0u;
#endif
}

/// Size of the data cache line.
/**
 * Will return the data cache line size (in bytes), or 0 if the value cannot be determined.
 * 
 * @return data cache line size in bytes.
 */
unsigned runtime_info::get_cache_line_size()
{
	lock_guard<mutex>::type lock(m_mutex);
#if defined(__linux__)
	const auto ls = ::sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
	return (ls > 0) ? boost::numeric_cast<unsigned>(ls) : 0u;
#else
	return 0u;
#endif
}

}
