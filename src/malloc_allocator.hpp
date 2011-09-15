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

#ifndef PIRANHA_MALLOC_ALLOCATOR_HPP
#define PIRANHA_MALLOC_ALLOCATOR_HPP

#include <boost/integer_traits.hpp>
#include <cstddef>
#include <cstdlib>
#include <new>
#include <stdexcept>

#if defined(_WIN32)
extern "C"
{
#if defined(__MINGW32__)
// NOTE: for _aligned_malloc, only malloc.h should be needed. However, for MinGW it
// seems like we need to include extra headers and use __mingw_aligned_malloc and friends:
// http://sourceforge.net/apps/trac/mingw-w64/wiki/Missing%20_aligned_malloc
#include <stdlib.h>
// #include <intrin.h> This does not seem to be there in mingw32?
#endif
#include <malloc.h>
#if defined(__MINGW32__)
#include <windows.h>
#endif
}
#endif

#include "config.hpp"
#include "exceptions.hpp"

namespace piranha
{

/// Allocator supporting custom memory alignment.
/**
 * This allocator is based on <tt>malloc()</tt> and other low-level platform-specific memory allocation primitives, and supports
 * memory allocation with custom alignment requirements. The alignment value is selected on construction and it is subject to
 * a series of constraints (explained in the constructors' documentation).
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the strong exception safety guarantee for all operations.
 * 
 * \section move_semantics Move semantics
 * 
 * This class has trivial move semantics.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 *
 * \todo: maybe it makes sense to move the allocation/free methods in a separate .cpp, as we do for runtime_info (this
 * way namespace pollution is prevented and platform-specific parts are isolated).
 * \todo: investigate using std::align for platform-independent memory aligning: allocate n bytes + alignment and use
 * std::align on it.
 */
template <typename T>
class malloc_allocator
{
	public:
		/// Size type.
		typedef std::size_t size_type;
		/// Difference type.
		typedef std::ptrdiff_t difference_type;
		/// Pointer type.
		typedef T * pointer;
		/// Const pointer type.
		typedef const T * const_pointer;
		/// Reference type.
		typedef T & reference;
		/// Const reference type.
		typedef const T & const_reference;
		/// Value type.
		typedef T value_type;
		/// Allocator rebinding.
		template <typename U>
		struct rebind
		{
			/// Rebound allocator type.
			typedef malloc_allocator<U> other;
		};
		/// Trivial default constructor.
		/**
		 * Default construction is always successful and equivalent to construction with an alignment value of zero.
		 * Memory allocation will use the standard <tt>std::malloc()</tt> function.
		 */
		malloc_allocator():m_alignment(0u) {}
		/// Constructor from alignment value.
		/**
		 * The allocated memory will be aligned to the specified \p alignment value.
		 * If \p alignment is not zero, it must be a nonnegative integral power of
		 * two not smaller than the alignment requirement for type \p T. Additionally, \p alignment
		 * might have to conform to other platform-specific requirements. Construction
		 * with nonzero \p alignment will fail if \p have_memalign_primitives is \p false.
		 * 
		 * If \p alignment is zero, construction will always be successful,
		 * and the memory allocated by this allocator is guaranteed to be able
		 * to store any C++ type (thanks to the use of <tt>std::malloc()</tt>).
		 * 
		 * @param[in] alignment desired alignment.
		 * 
		 * @throws std::invalid_argument in case of an invalid nonzero \p alignment.
		 */
		malloc_allocator(const std::size_t &alignment):m_alignment(alignment)
		{
			check_alignment(alignment);
		}
		/// Trivial copy constructor.
		malloc_allocator(const malloc_allocator &other):m_alignment(other.m_alignment) {}
		/// Trivial move constructor.
		malloc_allocator(malloc_allocator &&other) piranha_noexcept_spec(true):m_alignment(other.m_alignment) {}
		/// Trivial destructor.
		~malloc_allocator() piranha_noexcept_spec(true) {}
		/// Deleted copy assignment operator.
		malloc_allocator &operator=(const malloc_allocator &) = delete;
		/// Deleted move assignment operator.
		malloc_allocator &operator=(malloc_allocator &&) = delete;
		/// Memory address of reference.
		/**
		 * @param[in] x reference.
		 * 
		 * @return pointer to input reference.
		 */
		pointer address(reference x) const
		{
			return &x;
		}
		/// Memory address of const reference.
		/**
		 * @param[in] x const reference.
		 * 
		 * @return const pointer to input reference.
		 */
		const_pointer address(const_reference x) const
		{
			return &x;
		}
		/// Allocate memory.
		/**
		 * If the allocator was constructed with a zero \p alignment,
		 * the method will use <tt>std::malloc()</tt>. Otherwise, the address of the allocated memory is guaranteed to be
		 * aligned to \p alignment through the usage of platform-specific routines such as <tt>posix_memalign()</tt> (on POSIX systems)
		 * and <tt>_aligned_malloc()</tt> (on Windows systems).
		 * 
		 * @param[in] size number of instances of \p value_type for which memory will be allocated.
		 * 
		 * @return address to the allocated memory or \p nullptr if \p size is zero.
		 * 
		 * @throws std::bad_alloc if \p size is greater than max_size(), or if the allocation fails.
		 */
		pointer allocate(size_type size, const void * = piranha_nullptr)
		{
			if (!size) {
				return piranha_nullptr;
			}
			if (unlikely(size > max_size())) {
				piranha_throw(std::bad_alloc,0);
			}
			if (m_alignment) {
#if defined(PIRANHA_HAVE_POSIX_MEMALIGN)
				void *ptr;
				const auto retval = ::posix_memalign(&ptr,m_alignment,size * sizeof(T));
				if (unlikely(retval)) {
					piranha_throw(std::bad_alloc,0);
				}
				return static_cast<T *>(ptr);
#elif defined(_WIN32)
#if defined(__MINGW32__)
				void *ptr = ::__mingw_aligned_malloc(size * sizeof(T),m_alignment);
#else
				void *ptr = ::_aligned_malloc(size * sizeof(T),m_alignment);
#endif
				if (unlikely(ptr == NULL)) {
					piranha_throw(std::bad_alloc,0);
				}
				return static_cast<T *>(ptr);
#else
				// We should never get there, as no memalign primitives means the object
				// should have not been constructed in the first place.
				piranha_assert(false);
				return piranha_nullptr;
#endif
			} else {
				pointer ret = static_cast<T *>(std::malloc(size * sizeof(T)));
				if (unlikely(!ret)) {
					piranha_throw(std::bad_alloc,0);
				}
				return ret;
			}
		}
		/// Free memory.
		/**
		 * If \p p is \p nullptr, calling this method will be a no-op. The second input parameter is ignored.
		 * 
		 * @param[in] p pointer to a memory area previously allocated with allocate(), or \p nullptr.
		 */
		void deallocate(pointer p, size_type)
		{
			if (!p) {
				return;
			}
			if (m_alignment) {
#if defined(PIRANHA_HAVE_POSIX_MEMALIGN)
				// Posix memalign can be freed by standard free() function.
				std::free(static_cast<void *>(p));
#elif defined(_WIN32)
#if defined(__MINGW32__)
				::__mingw_aligned_free(static_cast<void *>(p));
#else
				::_aligned_free(static_cast<void *>(p));
#endif
#else
				// We should never get here, as construction would have thrown and we deleted
				// assignment operators.
				piranha_assert(false);
#endif
			} else {
				std::free(static_cast<void *>(p));
			}
		}
		/// Maximum allocatable size.
		/**
		 * @return the maximum value representable by \p size_type divided by <tt>sizeof(value_type)</tt>.
		 */
		size_type max_size() const
		{
			return boost::integer_traits<size_type>::const_max / sizeof(T);
		}
		/// Placement copy construction.
		/**
		 * Equivalent to calling placement \p new on the input pointer.
		 * 
		 * @param[in] p pointer where construction will happen.
		 * @param[in] val value to be copy-constructed in-place.
		 */
		void construct(pointer p, const T &val)
		{
			::new((void *)p) value_type(val);
		}
		/// Placement variadic construction.
		/**
		 * Equivalent to calling placement \p new on the input pointer.
		 * 
		 * @param[in] p pointer where construction will happen.
		 * @param[in] args arguments that will be forwarded for in-place construction.
		 */
		template<typename... Args>
		void construct(pointer p, Args && ... args)
		{
			::new((void *)p) T(std::forward<Args>(args)...);
		}
		/// Placement destruction.
		/**
		 * Equivalent to calling the destructor on the input pointer.
		 * 
		 * @param[in] p pointer from which the destructor will be called.
		 */
		void destroy(pointer p)
		{
			p->~T();
		}
		/// Alignment getter.
		/**
		 * @return the alignment value used during the construction of the allocator.
		 */
		std::size_t get_alignment() const
		{
			return m_alignment;
		}
		/// Type-trait for alignment primitives.
		/**
		 * \p true if the host platform supports the memory alignment primitives needed for nonzero
		 * alignments, \p false otherwise.
		 */
		static const bool have_memalign_primitives =
#if defined(PIRANHA_HAVE_POSIX_MEMALIGN) || defined(_WIN32)
			true;
#else
			false;
#endif
	protected:
		/// Check alignment.
		/**
		 * The checks performed are:
		 * 
		 * - \p have_memalign_primitives is \p true,
		 * - <tt>alignment >= alignof(T)</tt>,
		 * - \p alignment is a power of 2,
		 * - platform-specific checks.
		 * 
		 * If any of these checks fails, an \p std::invalid_argument exception will be thrown. Calling this method
		 * with zero \p alignment never throws.
		 * 
		 * @param[in] alignment alignment value to be checked.
		 * 
		 * @throws std::invalid_argument if the alignment value is not valid.
		 */
		static void check_alignment(const std::size_t &alignment)
		{
			// If alignment is not zero, we must run the checks.
			if (alignment) {
				if (!have_memalign_primitives) {
					piranha_throw(std::invalid_argument,"invalid alignment: nonzero, with no aligning primitives available on the platform");
				}
				// NOTE: here the check is like this because standard says that all alignments larger than that of T
				// include also the alignment of T (so no need to check for mod alignof(T)).
				if (unlikely(alignment < alignof(T))) {
					piranha_throw(std::invalid_argument,"invalid alignment: smaller than alignof(T)");
				}
				if (unlikely(alignment & (alignment - 1u))) {
					piranha_throw(std::invalid_argument,"invalid alignment: not a power of 2");
				}
#if defined(PIRANHA_HAVE_POSIX_MEMALIGN)
				// Extra check for posix_memalign requirements.
				if (unlikely(alignment % sizeof(void *))) {
					piranha_throw(std::invalid_argument,"invalid alignment: not a multiple of sizeof(void *)");
				}
#endif
			}
		}
	private:
		const std::size_t m_alignment;
};

/// Equality operator for piranha::malloc_allocator.
/**
 * @param[in] a1 first allocator.
 * @param[in] a2 second allocator.
 * 
 * @return \p true if the alignments of \p a1 and \p a2 coincide, \p false otherwise.
 */
template <typename T>
inline bool operator==(const malloc_allocator<T> &a1, const malloc_allocator<T> &a2)
{
	return a1.get_alignment() == a2.get_alignment();
}

/// Inequality operator for piranha::malloc_allocator.
/**
 * @param[in] a1 first allocator.
 * @param[in] a2 second allocator.
 * 
 * @return opposite of the equality operator.
 */
template <typename T>
inline bool operator!=(const malloc_allocator<T> &a1, const malloc_allocator<T> &a2)
{
	return !(a1 == a2);
}

}

#endif
