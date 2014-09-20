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
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "config.hpp"
#include "exceptions.hpp"
#include "thread_pool.hpp"
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

/// Parallel value initialisation.
/**
 * \note
 * This function is enabled only if \p T satisfies the piranha::is_container_element type trait.
 * 
 * This function will value-initialise in parallel the array \p ptr
 * of size \p size. The routine will use the first \p n_threads from piranha::thread_pool to perform
 * the operation concurrently. If \p n_threads is 1 or 0, the operation will be performed in the
 * calling thread. If \p ptr is null, this function will be a no-op.
 * 
 * This function provides the strong exception safety guarantee: in case of errors, any constructed
 * instance of \p T will be destroyed before the error is re-thrown.
 * 
 * @param[in] ptr pointer to the array.
 * @param[in] size size of the array.
 * @param[in] n_threads number of threads to use.
 * 
 * @throws std::bad_alloc in case of memory allocation errors in multithreaded mode.
 * @throws unspecified any exception thrown by:
 * - the value initialisation of instances of type \p T,
 * - piranha::thread_pool::enqueue() and piranha::future_list::push_back(), only in multithreaded mode.
 */
template <typename T, typename = typename std::enable_if<is_container_element<T>::value>::type>
inline void parallel_value_init(T *ptr, const std::size_t &size, const unsigned &n_threads)
{
	using ranges_vector = std::vector<std::pair<T *,T *>>;
	using rv_size_type = typename ranges_vector::size_type;
	if (unlikely(ptr == nullptr)) {
		piranha_assert(!size);
		return;
	}
	// Initing functor.
	auto init_function = [](T *start, T *end, const unsigned &thread_idx, ranges_vector *rv) {
		auto orig_start = start;
		try {
			for (; start != end; ++start) {
				::new (static_cast<void *>(start)) T();
			}
		} catch (...) {
			// Cleanup.
			for (; orig_start != start; ++orig_start) {
				orig_start->~T();
			}
			// Re-throw.
			throw;
		}
		// If the init was successful and we are in multithreaded mode, record
		// the range that was inited.
		if (rv != nullptr) {
			(*rv)[static_cast<rv_size_type>(thread_idx)].first = orig_start;
			(*rv)[static_cast<rv_size_type>(thread_idx)].second = end;
		}
	};
	if (n_threads <= 1) {
		init_function(ptr,ptr + size,0u,nullptr);
	} else {
		// Init the ranges vector with (ptr,ptr) pairs, so they are empty ranges.
		ranges_vector inited_ranges(static_cast<rv_size_type>(n_threads),std::make_pair(ptr,ptr));
		if (unlikely(inited_ranges.size() != n_threads)) {
			piranha_throw(std::bad_alloc,);
		}
		// Work per thread.
		const auto wpt = static_cast<std::size_t>(size / n_threads);
		future_list<decltype(thread_pool::enqueue(0u,init_function,ptr,ptr,0u,&inited_ranges))> f_list;
		try {
			for (auto i = 0u; i < n_threads; ++i) {
				auto start = ptr + i * wpt, end = (i == n_threads - 1u) ? ptr + size : ptr + (i + 1u) * wpt;
				f_list.push_back(thread_pool::enqueue(i,init_function,start,end,i,&inited_ranges));
			}
			f_list.wait_all();
			f_list.get_all();
		} catch (...) {
			f_list.wait_all();
			// Rollback the ranges that were inited.
			for (const auto &p: inited_ranges) {
				for (auto start = p.first; start != p.second; ++start) {
					start->~T();
				}
			}
			throw;
		}
	}
}

/// Parallel destruction.
/**
 * \note
 * This function is enabled only if \p T satisfies the piranha::is_container_element type trait.
 * 
 * This function will destroy in parallel the element of an array \p ptr of size \p size. If \p n_threads
 * is 0 or 1, the operation will be performed in the calling thread, otherwise the first \p n_threads in piranha::thread_pool
 * will be used to perform the operation concurrently.
 * 
 * The function is a no-op if \p ptr is null or if \p T has a trivial destructor.
 * 
 * @param[in] ptr pointer to the array.
 * @param[in] size size of the array.
 * @param[in] n_threads number of threads to use.
 */
template <typename T, typename = typename std::enable_if<is_container_element<T>::value>::type>
inline void parallel_destroy(T *ptr, const std::size_t &size, const unsigned &n_threads)
{
	using ranges_vector = std::vector<std::pair<T *,T *>>;
	using rv_size_type = typename ranges_vector::size_type;
	// Nothing to be done for null pointers.
	if (unlikely(ptr == nullptr)) {
		piranha_assert(!size);
		return;
	}
	// Nothing needs to be done for trivially destructible types:
	// http://en.cppreference.com/w/cpp/language/lifetime
	if (std::is_trivially_destructible<T>::value) {
		return;
	}
	// Destroy functor.
	auto destroy_function = [](T *start, T *end) {
		for (; start != end; ++start) {
			start->~T();
		}
	};
	if (n_threads <= 1u) {
		destroy_function(ptr,ptr + size);
	} else {
		// A vector of ranges representing elements yet to be destroyed in case something goes wrong
		// in the multithreaded part.
		ranges_vector d_ranges;
		future_list<decltype(thread_pool::enqueue(0u,destroy_function,ptr,ptr))> f_list;
		try {
			d_ranges.resize(static_cast<rv_size_type>(n_threads),std::make_pair(ptr,ptr));
			if (unlikely(d_ranges.size() != n_threads)) {
				piranha_throw(std::bad_alloc,);
			}
		} catch (...) {
			// Just perform the single-threaded version, and GTFO.
			destroy_function(ptr,ptr + size);
			return;
		}
		try {
			// Work per thread.
			const auto wpt = static_cast<std::size_t>(size / n_threads);
			// The ranges to destroy in the cleanup phase are, initially, all the ranges.
			for (auto i = 0u; i < n_threads; ++i) {
				auto start = ptr + i * wpt, end = (i == n_threads - 1u) ? ptr + size : ptr + (i + 1u) * wpt;
				d_ranges[static_cast<rv_size_type>(i)] = std::make_pair(start,end);
			}
			// Perform the actual destruction and update the d_ranges vector.
			for (auto i = 0u; i < n_threads; ++i) {
				auto start = d_ranges[static_cast<rv_size_type>(i)].first, end = d_ranges[static_cast<rv_size_type>(i)].second;
				f_list.push_back(thread_pool::enqueue(i,destroy_function,start,end));
				// The range needs not to be destroyed anymore, as the enqueue/push_back was successful. Replace
				// with an empty range.
				d_ranges[static_cast<rv_size_type>(i)].first = ptr;
				d_ranges[static_cast<rv_size_type>(i)].second = ptr;
			}
			f_list.wait_all();
			// NOTE: T is a container_element, no need to get exceptions here.
		} catch (...) {
			f_list.wait_all();
			// If anything failed in the multithreaded part, just destroy in single-thread the ranges
			// that were not destroyed.
			for (const auto &p: d_ranges) {
				destroy_function(p.first,p.second);
			}
		}
	}
}

namespace detail
{

// Deleter functor to be used in std::unique_ptr.
template <typename T>
class parallel_deleter
{
	public:
		explicit parallel_deleter(const std::size_t &size, const unsigned &n_threads):
			m_size(size),m_n_threads(n_threads)
		{}
		void operator()(T *ptr) const
		{
			// Parallel destroy and pfree are no-ops with nullptr. All of this
			// is noexcept.
			parallel_destroy(ptr,m_size,m_n_threads);
			aligned_pfree(0u,static_cast<void *>(ptr));
		}
	private:
		std::size_t	m_size;
		unsigned	m_n_threads;
};

}

/// Create an array in parallel.
/**
 * \note
 * This function is enabled only if \p T satisfies the piranha::is_container_element type trait.
 * 
 * This function will create an array whose values will be initialised in parallel using piranha::parallel_value_init().
 * The pointer to the array is returned wrapped inside an \p std::unique_ptr that will take care of
 * destroying the array elements (also in parallel using piranha::parallel_destroy()) and deallocating the memory when the
 * destructor is called.
 *
 * \note
 * Due to the special semantics of the parallel deleter, the returned smart pointer cannot be reset with another arbitrary pointer
 * without supplying a new deleter. To replace the managed object while supplying a new deleter as well, move semantics may be used.
 *
 * @param[in] size size of the array.
 * @param[in] n_threads number of threads to use.
 * 
 * @return an \p std::unique_ptr wrapping the array.
 * 
 * @throws std::bad_alloc if \p size is greater than an implementation-defined value.
 * @throws unspecified any exception thrown by:
 * - piranha::aligned_palloc() (called with an alignment value of 0),
 * - piranha::parallel_value_init().
 */
template <typename T, typename = typename std::enable_if<is_container_element<T>::value>::type>
inline std::unique_ptr<T[],detail::parallel_deleter<T>> make_parallel_array(const std::size_t &size, const unsigned &n_threads)
{
	if (unlikely(size > std::numeric_limits<std::size_t>::max() / sizeof(T))) {
		piranha_throw(std::bad_alloc,);
	}
	// Allocate space. This could be nullptr if size is zero.
	auto ptr = static_cast<T *>(aligned_palloc(0u,static_cast<std::size_t>(size * sizeof(T))));
	try {
		// No problems here with nullptr, will be a no-op.
		parallel_value_init(ptr,size,n_threads);
	} catch (...) {
		piranha_assert(ptr != nullptr);
		// Free the allocated memory. This is noexcept.
		aligned_pfree(0u,static_cast<void *>(ptr));
		throw;
	}
	return std::unique_ptr<T[],detail::parallel_deleter<T>>(ptr,detail::parallel_deleter<T>{size,n_threads});
}

}

#endif
