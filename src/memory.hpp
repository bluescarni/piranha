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

#ifndef PIRANHA_MEMORY_HPP
#define PIRANHA_MEMORY_HPP

/** \file memory.hpp
 * \brief Low-level memory management functions.
 */

#include <cstddef>
#include <new>
#include <stdexcept>
#include <type_traits>

#include "config.hpp"
#include "exceptions.hpp"
#include "type_traits.hpp"

#if defined(PIRANHA_HAVE_POSIX_MEMALIGN) // POSIX memalign.
// NOTE: this can go away once we have support for std::align.
#define PIRANHA_HAVE_MEMORY_ALIGNMENT_PRIMITIVES
#include <cstdlib>
#elif defined(_WIN32) // Windows _aligned_malloc.
#define PIRANHA_HAVE_MEMORY_ALIGNMENT_PRIMITIVES
#include <malloc.h>
#else // malloc + std::align.
#include <boost/integer_traits.hpp>
#include <cstdlib>
#include <memory>
#endif

namespace piranha
{

namespace detail
{

// Possible implementation of aligned malloc using std::align (not yet available in GCC).
// NOTE: this needs to be tested and debugged.
#if 0
inline void *cpp_aligned_alloc(const std::size_t &alignment, const std::size_t &size)
{
	if (unlikely(alignment == 0u || size > boost::integer_traits<std::size_t>::const_max - alignment)) {
		piranha_throw(std::bad_alloc,);
	}
	// This is the actual allocated size.
	std::size_t a_size = size + alignment;
	const std::size_t orig_a_size = a_size;
	// Allocate enough space for payload and alignment.
	void *u_ptr = std::malloc(a_size), *orig_u_ptr = u_ptr;
	if (unlikely(u_ptr == nullptr)) {
		piranha_throw(std::bad_alloc,);
	}
	// Try to align.
	void *ptr = std::align(alignment,size,u_ptr,a_size);
	if (unlikely(ptr == nullptr)) {
		std::free(orig_u_ptr);
		piranha_throw(std::bad_alloc,);
	}
	// Some sanity checks.
	piranha_assert(orig_a_size >= a_size);
	// If the size has changed, the pointer must have too.
	piranha_assert(orig_a_size == a_size || orig_u_ptr != ptr);
	// Number of bytes taken by the alignment.
	const std::size_t delta = (orig_a_size == a_size) ? alignment : orig_a_size - a_size;
	piranha_assert(delta <= alignment);
	piranha_assert(delta > 0u);
	// The delta needs to be representable by unsigned char for storage.
	if (unlikely(delta > boost::integer_traits<unsigned char>::const_max)) {
		// NOTE: need to use original pointer, as u_ptr might have been changed by std::align.
		std::free(orig_u_ptr);
		piranha_throw(std::bad_alloc,);
	}
	// If the original pointer was already aligned, we have to move it forward.
	if (orig_u_ptr == ptr) {
		ptr = static_cast<void *>(static_cast<unsigned char *>(ptr) + alignment);
	}
	// Record the delta.
	*(static_cast<unsigned char *>(ptr) - 1) = static_cast<unsigned char>(delta);
	return ptr;
}
#endif

}

/// Allocate memory aligned to a specific value.
/**
 * This function will allocate a block of memory of \p size bytes aligned to \p alignment.
 * If \p size is zero, \p nullptr will be returned. If \p alignment is zero, \p std::malloc()
 * will be used for the allocation. Otherwise, the allocation will be deferred to an implementation-defined
 * and platform-dependent low-level routine (e.g., \p posix_memalign()). If such a low level routine is not
 * available, an exception will be raised.
 *
 * @param[in] alignment desired alignment.
 * @param[in] size number of bytes to allocate.
 *
 * @return a pointer to the allocated memory block, or \p nullptr if \p size is zero.
 *
 * @throws std::bad_alloc if the allocation fails for any reason (e.g., bad alignment value, failure
 * in the low-level allocation routine, etc.).
 * @throws piranha::not_implemented_error if \p alignment and \p size are both nonzero and the
 * low-level allocation function is not available on the platform.
 */
inline void *aligned_palloc(const std::size_t &alignment, const std::size_t &size)
{
	// Platform-independent part: special values for alignment and size.
	if (unlikely(size == 0u)) {
		return nullptr;
	}
	if (alignment == 0u) {
		void *ptr = std::malloc(size);
		if (unlikely(ptr == nullptr)) {
			piranha_throw(std::bad_alloc,);
		}
		return ptr;
	}
#if defined(PIRANHA_HAVE_POSIX_MEMALIGN)
	void *ptr;
	const int retval = ::posix_memalign(&ptr,alignment,size);
	if (unlikely(retval != 0)) {
		piranha_throw(std::bad_alloc,);
	}
	return ptr;
#elif defined(_WIN32)
	void *ptr = ::_aligned_malloc(size,alignment);
	if (unlikely(ptr == nullptr)) {
		piranha_throw(std::bad_alloc,);
	}
	return ptr;
#else
	// return detail::cpp_aligned_alloc(alignment,size);
	piranha_throw(not_implemented_error,"memory alignment primitives are not available");
#endif
}

/// Free memory allocated via piranha::aligned_alloc.
/**
 * This function must be used to deallocate memory obtained via piranha::aligned_alloc(). If \p ptr is
 * \p nullptr, this function will be a no-op. If \p alignment is zero, \p std::free() will be used. Otherwise,
 * the deallocation will be deferred to an implementation-defined
 * and platform-dependent low-level routine (e.g., \p _aligned_free()). If such a low level routine is not
 * available, an exception will be raised.
 *
 * The value of \p alignment must be the same used for the allocation.
 *
 * @param[in] alignment alignment value used during allocation.
 * @param[in] ptr pointer to the memory to be freed.
 *
 * @throws piranha::not_implemented_error if \p ptr is not \p nullptr, \p alignment is not zero and the low-level
 * deallocation routine is not available on the platform.
 */
inline void aligned_pfree(const std::size_t &alignment, void *ptr)
{
	if (unlikely(ptr == nullptr)) {
		return;
	}
	if (alignment == 0u) {
		std::free(ptr);
		return;
	}
#if defined(PIRANHA_HAVE_POSIX_MEMALIGN)
	std::free(ptr);
#elif defined(_WIN32)
	::_aligned_free(ptr);
#else
	piranha_throw(not_implemented_error,"memory alignment primitives are not available");
#endif
}

/// Alignment checks.
/**
 * This function will run a series of checks on an alignment value to be used to allocate storage for objects of the decay
 * type of \p T using piranha::aligned_palloc(),
 * and it will retun \p true if the alignment value passes these checks, \p false otherwise.
 * An alignment of zero will always return \p true.
 *
 * The checks performed are the following:
 * - the alignment must be a power of 2 (3.11/4),
 * - the alignment must not be smaller than the default alignment of \p T, as reported by
 *   \p alignas(),
 * - the alignment must satisfy additional platform-dependent checks (e.g., \p posix_memalign() requires
 *   the alignment to be a multiple of <tt>sizeof(void *)</tt>).
 *
 * Note that piranha::aligned_palloc() will not check the alignment via this function, and that even if this function returns \p true on an alignment value,
 * this will not guarantee that the allocation via piranha::aligned_palloc() will succeed.
 *
 * @param[in] alignment alignment value to be checked.
 *
 * @return \p true if the input value is zero or if it passes the alignment checks, \p false otherwise.
 */
template <typename T>
inline bool alignment_check(const std::size_t &alignment)
{
	// Platform-independent checks.
	if (alignment == 0u) {
		return true;
	}
	// Alignment must be power of 2.
	if (unlikely(static_cast<bool>(alignment & (alignment - 1u)))) {
		return false;
	}
	// Alignment must not be less than the natural alignment of T. We just need the '<' check
	// as we already know that alignment is either a multiple of alignof(T) or a divisor.
	if (unlikely(alignment < alignof(typename std::decay<T>::type))) {
		return false;
	}
#if defined(PIRANHA_HAVE_POSIX_MEMALIGN)
	// Extra check for posix_memalign requirements.
	if (unlikely(static_cast<bool>(alignment % sizeof(void *)))) {
		return false;
	}
#endif
	return true;
}

}

#endif
