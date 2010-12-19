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

#include <boost/numeric/conversion/cast.hpp>
#include <mutex>
#include <stdexcept>
#include <tuple>

#include "config.hpp"
#include "exceptions.hpp"
#include "runtime_info.hpp"
#include "thread_management.hpp"

#ifdef PIRANHA_THREAD_MODEL_PTHREADS
extern "C"
{
#include <pthread.h>
}
#endif

#ifdef _GNU_SOURCE
extern "C"
{
#include <sched.h>
}
#endif

namespace piranha
{

std::mutex thread_management::m_mutex;

/// Bind thread to specific processor.
/**
 * Upon successful completion of this method, the calling thread will be confined on the specified processor.
 * This method requires platform-specific functions and thus might not be available on all configurations.
 * 
 * @param[in] n index of the processor to which the thread will be bound (starting from index 0).
 * 
 * @throws piranha::value_error if one of these conditions arises:
 * \li n is greater than an implementation-defined maximum value,
 * \li piranha::runtime_info::hardware_concurrency() returns a nonzero value m and n >= m.
 * @throws piranha::not_implemented_error if the method is not available on the current platform.
 * @throws std::runtime_error if the operation fails in an unspecified way.
 */
void thread_management::bind_to_proc(unsigned n)
{
	std::lock_guard<std::mutex> lock(m_mutex);
#if defined(PIRANHA_THREAD_MODEL_PTHREADS) && defined(_GNU_SOURCE)
	unsigned cpu_setsize;
	int n_int;
	try {
		cpu_setsize = boost::numeric_cast<unsigned>(CPU_SETSIZE);
		n_int = boost::numeric_cast<int>(n);
	} catch (const boost::numeric::bad_numeric_cast &) {
		piranha_throw(std::runtime_error,"numeric conversion error");
	}
	if (n >= cpu_setsize) {
		piranha_throw(piranha::value_error,"processor index is larger than the maximum allowed value");
	}
	if (runtime_info::hardware_concurrency() != 0 && n >= runtime_info::hardware_concurrency()) {
		piranha_throw(piranha::value_error,"processor index is larger than the detected hardware concurrency");
	}
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(n_int,&cpuset);
	const int errno_ = ::pthread_setaffinity_np(::pthread_self(),sizeof(cpuset),&cpuset);
	if (errno_ != 0) {
		piranha_throw(std::runtime_error,"the call to pthread_setaffinity_np() failed");
	}
#else
#warning No bind_to_proc implementation available.
	(void)n;
	piranha_throw(not_implemented_error,"bind_to_proc is not available on this platform")
#endif
}

/// Query if current thread is bound to a processor.
/**
 * The complexity of the operation is at most linear in the number of processors available on the system.
 * This method requires platform-specific functions and thus might not be available on all configurations.
 * 
 * @return the pair (true,n) if the calling thread is bound to a single processor with index n, (false,0)
 * otherwise.
 * 
 * @throws piranha::not_implemented_error if the method is not available on the current platform.
 * @throws std::runtime_error if the operation fails in an unspecified way.
 */
std::tuple<bool,unsigned> thread_management::bound_proc()
{
	std::lock_guard<std::mutex> lock(m_mutex);
#if defined(PIRANHA_THREAD_MODEL_PTHREADS) && defined(_GNU_SOURCE)
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	const int errno_ = ::pthread_getaffinity_np(::pthread_self(),sizeof(cpuset),&cpuset);
	if (errno_ != 0) {
		piranha_throw(std::runtime_error,"the call to pthread_getaffinity_np() failed");
	}
	const int cpu_count = CPU_COUNT(&cpuset);
	if (cpu_count == 0 || cpu_count > 1) {
		return std::make_tuple(false,static_cast<unsigned>(0));
	}
	int cpu_setsize;
	try {
		cpu_setsize = boost::numeric_cast<int>(CPU_SETSIZE);
	} catch (const boost::numeric::bad_numeric_cast &) {
		piranha_throw(std::runtime_error,"numeric conversion error");
	}
	for (int i = 0; i < cpu_setsize; ++i) {
		if (CPU_ISSET(i,&cpuset)) {
			// Cast is safe here (verified above that cpu_setsize is representable in int,
			// and, by extension, in unsigned).
			return std::make_tuple(true,static_cast<unsigned>(i));
		}
	}
	piranha_throw(std::runtime_error,"operation failed");
#else
#warning No bound_proc implementation available.
	piranha_throw(not_implemented_error,"bound_proc is not available on this platform")
#endif
}

}

