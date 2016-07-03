/* Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)

This file is part of the Piranha library.

The Piranha library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The Piranha library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the Piranha library.  If not,
see https://www.gnu.org/licenses/. */

#ifndef PIRANHA_THREAD_MANAGEMENT_HPP
#define PIRANHA_THREAD_MANAGEMENT_HPP

#include <boost/numeric/conversion/cast.hpp>
#include <stdexcept>
#include <utility>

#include "config.hpp"
#include "exceptions.hpp"
#include "runtime_info.hpp"
#include "thread_management.hpp"

#if defined(PIRANHA_HAVE_PTHREAD_AFFINITY)

extern "C"
{
#include <pthread.h>
#include <sched.h>
}

#elif defined(__FreeBSD__)

extern "C"
{
#include <sys/param.h>
#include <sys/cpuset.h>
}

#elif defined(_WIN32)

extern "C"
{
#include <Windows.h>
}
#include <limits>

#endif

namespace piranha
{

/// Bind thread to specific processor.
/**
 * Upon successful completion of this method, the calling thread will be confined on the specified processor.
 * This method requires platform-specific functions and thus might not be available on all configurations.
 *
 * In case of exceptions, the thread will not have been bound to any processor.
 *
 * @param[in] n index of the processor to which the thread will be bound (starting from index 0).
 *
 * @throws std::invalid_argument if one of these conditions arises:
 * - n is greater than an implementation-defined maximum value,
 * - piranha::runtime_info::get_hardware_concurrency() returns a nonzero value \p m and <tt>n >= m</tt>.
 * @throws piranha::not_implemented_error if the method is not available on the current platform.
 * @throws std::system_error in case of failure(s) by threading primitives.
 * @throws std::runtime_error if the operation fails in an unspecified way.
 */
inline void bind_to_proc(unsigned n)
{
	const auto hc = runtime_info::get_hardware_concurrency();
	if (hc != 0u && n >= hc) {
		piranha_throw(std::invalid_argument,"processor index is larger than the detected hardware concurrency");
	}
#if defined(PIRANHA_HAVE_PTHREAD_AFFINITY)
	unsigned cpu_setsize;
	int n_int;
	try {
		cpu_setsize = boost::numeric_cast<unsigned>(CPU_SETSIZE);
		n_int = boost::numeric_cast<int>(n);
	} catch (const boost::numeric::bad_numeric_cast &) {
		piranha_throw(std::runtime_error,"numeric conversion error");
	}
	if (n >= cpu_setsize) {
		piranha_throw(std::invalid_argument,"processor index is larger than the maximum allowed value");
	}
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(n_int,&cpuset);
	const int errno_ = ::pthread_setaffinity_np(::pthread_self(),sizeof(cpuset),&cpuset);
	if (errno_ != 0) {
		piranha_throw(std::runtime_error,"the call to pthread_setaffinity_np() failed");
	}
#elif defined(__FreeBSD__)
	unsigned cpu_setsize;
	int n_int;
	try {
		cpu_setsize = boost::numeric_cast<unsigned>(CPU_SETSIZE);
		n_int = boost::numeric_cast<int>(n);
	} catch (const boost::numeric::bad_numeric_cast &) {
		piranha_throw(std::runtime_error,"numeric conversion error");
	}
	if (n >= cpu_setsize) {
		piranha_throw(std::invalid_argument,"processor index is larger than the maximum allowed value");
	}
	cpuset_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(n,&cpuset);
	if (::cpuset_setaffinity(CPU_LEVEL_WHICH,CPU_WHICH_TID,-1,sizeof(cpuset),&cpuset) == -1) {
		piranha_throw(std::runtime_error,"the call to cpuset_setaffinity() failed");
	}
#elif defined(_WIN32)
	// Check we are not going to bit shift too much.
	if (n >= static_cast<unsigned>(std::numeric_limits<DWORD_PTR>::digits)) {
		piranha_throw(std::invalid_argument,"processor index is larger than the maximum allowed value");
	}
	if (::SetThreadAffinityMask(::GetCurrentThread(),(DWORD_PTR)1 << n) == 0) {
		piranha_throw(std::runtime_error,"the call to SetThreadAffinityMask() failed");
	}
#else
	piranha_throw(not_implemented_error,"bind_to_proc() is not available on this platform");
#endif
}

/// Query if current thread is bound to a processor.
/**
 * The complexity of the operation is at most linear in the maximum number of processors that can be
 * represented on the system (e.g., in Unix-like system this number will typically correspond to the
 * \p CPU_SETSIZE macro).
 * This method requires platform-specific functions and thus might not be available on all configurations.
 * Note that if only a single core/cpu is available, the returned value will always be <tt>(true,0)</tt>.
 *
 * @return the pair <tt>(true,n)</tt> if the calling thread is bound to a single processor with index \p n, <tt>(false,0)</tt>
 * otherwise.
 *
 * @throws piranha::not_implemented_error if the method is not available on the current platform.
 * @throws std::runtime_error if the operation fails in an unspecified way.
 */
inline std::pair<bool,unsigned> bound_proc()
{
#if defined(PIRANHA_HAVE_PTHREAD_AFFINITY)
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	const int errno_ = ::pthread_getaffinity_np(::pthread_self(),sizeof(cpuset),&cpuset);
	if (errno_ != 0) {
		piranha_throw(std::runtime_error,"the call to pthread_getaffinity_np() failed");
	}
	const int cpu_count = CPU_COUNT(&cpuset);
	if (cpu_count == 0 || cpu_count > 1) {
		return std::make_pair(false,static_cast<unsigned>(0));
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
			return std::make_pair(true,static_cast<unsigned>(i));
		}
	}
	piranha_throw(std::runtime_error,"operation failed");
#elif defined(__FreeBSD__)
	cpuset_t cpuset;
	CPU_ZERO(&cpuset);
	const int errno_ = ::cpuset_getaffinity(CPU_LEVEL_WHICH,CPU_WHICH_TID,-1,sizeof(cpuset),&cpuset);
	if (errno_ != 0) {
		piranha_throw(std::runtime_error,"the call to ::cpuset_getaffinity() failed");
	}
	int cpu_setsize;
	try {
		cpu_setsize = boost::numeric_cast<int>(CPU_SETSIZE);
	} catch (const boost::numeric::bad_numeric_cast &) {
		piranha_throw(std::runtime_error,"numeric conversion error");
	}
	unsigned bound_cpus = 0;
	int candidate = 0;
	for (int i = 0; i < cpu_setsize; ++i) {
		if (CPU_ISSET(i,&cpuset)) {
			++bound_cpus;
			candidate = i;
		}
	}
	if (bound_cpus == 0 || bound_cpus > 1) {
		return std::make_pair(false,static_cast<unsigned>(0));
	} else {
		return std::make_pair(true,static_cast<unsigned>(candidate));
	}
#elif defined(_WIN32)
	// Store the original affinity mask.
	const DWORD_PTR original_affinity_mask = ::SetThreadAffinityMask(::GetCurrentThread(),(DWORD_PTR)1);
	if (original_affinity_mask == 0) {
		piranha_throw(std::runtime_error,"the call to SetThreadAffinityMask() failed");
	}
	const unsigned cpu_setsize = static_cast<unsigned>(std::numeric_limits<DWORD_PTR>::digits);
	unsigned bound_cpus = 0, candidate = 0;
	for (unsigned i = 0; i < cpu_setsize; ++i) {
		if (((DWORD_PTR)1 << i) & original_affinity_mask) {
			++bound_cpus;
			candidate = i;
		}
	}
	// Restore the original affinity mask.
	if (::SetThreadAffinityMask(::GetCurrentThread(),original_affinity_mask) == 0) {
		piranha_throw(std::runtime_error,"the call to SetThreadAffinityMask() failed");
	}
	if (bound_cpus == 0 || bound_cpus > 1) {
		return std::make_pair(false,static_cast<unsigned>(0));
	} else {
		return std::make_pair(true,candidate);
	}
#else
	piranha_throw(not_implemented_error,"bound_proc() is not available on this platform");
#endif
}

}

#endif
