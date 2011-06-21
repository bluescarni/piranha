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

#if defined(_WIN32)
#include <malloc.h>
#endif

#include "config.hpp"
#include "exceptions.hpp"

namespace piranha
{

namespace detail
{

template <std::size_t N, std::size_t InitialN = N>
struct is_alignable
{
	static const bool value = (N % 2u || InitialN % sizeof(void *)) ? false : is_alignable<N / 2u,InitialN>::value;
};

template <>
struct is_alignable<0u,0u>
{
	static const bool value = true;
};

template <std::size_t InitialN>
struct is_alignable<1u,InitialN>
{
	// This is valid only when we arrive here from the recursion. If this is the first iteration,
	// alignment is not valid because 1 is not a nonzero power of 2.
	static const bool value = InitialN != 1u;
};

}

/// Allocator supporting memory alignment.
/**
 * This allocator is based on <tt>malloc()</tt> and other low-level platform-specific memory allocation primitives, and supports
 * the allocation of memory aligned to multiples of \p N. The requisites on \p N are the following:
 * 
 * - must be either zero or one of the following:
 * 	- a nonzero power of 2,
 * 	- a multiple of <tt>sizeof(void *)</tt>.
 * 
 * In any case, a nonzero \p N must be a multiple of the alignment requirement for type \p T.
 * 
 * When \p N is zero, memory allocation will use \p std::malloc. Otherwise, the address of the allocated memory is guaranteed to be
 * a multiple of \p N through the usage of platform-specific routines such as <tt>posix_memalign()</tt> (on POSIX systems)
 * and <tt>_aligned_malloc()</tt> (on Windows systems). On unsupported platforms, trying to use this allocator will result in
 * a static assertion failure at compile time.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the strong exception safety guarantee for all operations.
 * 
 * \section move_semantics Move semantics
 * 
 * This class has no data members and hence has trivial move semantics.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo maybe we can allow also N == 1 here.
 */
template <typename T, std::size_t N = 0u>
class malloc_allocator
{
		static_assert(detail::is_alignable<N>::value && !(N % alignof(T)),"Invalid alignment value.");
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
			/**
			 * Alignment is preserved in the rebound allocator type.
			 */
			typedef malloc_allocator<U,N> other;
		};
		/// Trivial default constructor.
		malloc_allocator() {}
		/// Trivial copy constructor.
		malloc_allocator(const malloc_allocator &) {}
		/// Trivial move constructor.
		malloc_allocator(malloc_allocator &&) piranha_noexcept_spec(true) {}
		/// Trivial destructor.
		~malloc_allocator() piranha_noexcept_spec(true) {}
		/// Trivial copy assignment operator.
		/**
		 * @return reference to \p this.
		 */
		malloc_allocator &operator=(const malloc_allocator &)
		{
			return *this;
		}
		/// Trivial move assignment operator.
		/**
		 * @return reference to \p this.
		 */
		malloc_allocator &operator=(malloc_allocator &&) piranha_noexcept_spec(true)
		{
			return *this;
		}
		/// Memory address of reference.
		pointer address(reference x) const
		{
			return &x;
		}
		/// Memory address of const reference.
		const_pointer address(const_reference x) const
		{
			return &x;
		}
		/// Allocate memory.
		/**
		 * @param[in] size number of instances of \p value_type for which memory will be allocated.
		 * 
		 * @return address to the allocated memory or \p nullptr if \p size is zero.
		 * 
		 * @throws std::bad_alloc if \p size is greater than max_size() or if the allocation fails.
		 */
		pointer allocate(size_type size, const void * = piranha_nullptr)
		{
			if (!size) {
				return piranha_nullptr;
			}
			if (unlikely(size > max_size())) {
				throw std::bad_alloc();
			}
			if (N) {
#if _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600
				void *ptr;
				const auto retval = posix_memalign(&ptr,N,size * sizeof(T));
				if (unlikely(retval)) {
					throw std::bad_alloc();
				}
				return static_cast<T *>(ptr);
#elif defined(_WIN32)
				void *ptr = _aligned_malloc(size * sizeof(T),N);
				if (unlikely(ptr == NULL)) {
					throw std::bad_alloc();
				}
				return static_cast<T *>(ptr);
#else
				static_assert(false,"No memory alignmnent primitives available on this platform.");
#endif
			} else {
				pointer ret = static_cast<T *>(std::malloc(size * sizeof(T)));
				if (unlikely(!ret)) {
					throw std::bad_alloc();
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
			if (N) {
#if _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600
				// Posix memalign can be freed by standard free() function.
				std::free(static_cast<void *>(p));
#elif defined(_WIN32)
				_aligned_free(static_cast<void *>(p));
#else
				static_assert(false,"No memory alignmnent primitives available on this platform.");
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
};

/// Equality operator for piranha::malloc_allocator.
/**
 * @return \p true.
 */
template <typename T, std::size_t N>
inline bool operator==(const malloc_allocator<T,N> &, const malloc_allocator<T,N> &)
{
	return true;
}

/// Inequality operator for piranha::malloc_allocator.
/**
 * @return \p false.
 */
template <typename T, std::size_t N>
inline bool operator!=(const malloc_allocator<T,N> &, const malloc_allocator<T,N> &)
{
	return false;
}

}

#endif
