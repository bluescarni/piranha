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

#ifndef PIRANHA_PIRANHA_HPP
#define PIRANHA_PIRANHA_HPP

/** \file piranha.hpp
 * \brief Global piranha header file.
 * 
 * Include this file to import piranha's entire public interface.
 */

/// Root piranha namespace.
/**
 * \todo implement is_instance_of when GCC support for variadic templates improves, and remove the tag structs
 * (see http://stackoverflow.com/questions/4749863/variadic-templates-and-copy-construction-via-assignment)
 * \todo better piranha_throw macro with support of boost_throw when using boost thread instead of native c++0x threads.
 * \todo switch to auto -> decltype declarations of member functions for complicated types (e.g., tuples) when decltype on this
 * becomes available.
 * \todo explain in general section the base assumptions of move semantics and thread safety (e.g., require implicitly that
 * all moved-from objects are assignable and destructable, and everything not thread-safe by default).
 * \todo document classes in detail:: namespace used to implement mathematical functions?
 * \todo fix type traits such as is_nothrow_move_constructible/assignable. They must work also for references,
 * make them use SFINAE, declval and decltype as in the current GCC 4.6 type_traits header (e.g., see is_constructible). Keep in mind
 * reference collapsing rules, etc. etc.
 * \todo modify concepts to use declval where applicable, instead of dereferencing nullptr.
 * \todo inline friend functions in class scope: is inline redundant? yes: http://stackoverflow.com/questions/3980627/is-a-friend-function-defined-in-class-automatically-inline
 * \todo base_series test: missing merge terms with negative+move (that actually swaps the contents of the series) and negative+move with different series types.
 * \todo concepts: how to deal with generic methods (e.g., coefficient in-place multiply by whatever)? We could add another parameter to the concept, with default void,
 * and use it explictly only when actually using that generic method? ADDENDUM: connected to this, the question of dealing with methods that
 * have additional type requirements wrt those delcared in class. Probably need to review all generic methods and add type requirements there.
 * \todo strip_cv_ref should be std::decay probably.
 * \todo try to get away with the friendship to binary series operators by base series.
 * \todo hashing of array_key based on kronecker substitution -> use static table of kronecker_array to encode the array key in an integer.
 * \todo cache aligned memory allocation: to be completely sure that we are not sharing cache lines among threads,
 * we must 1) figure a way to determine cache line size
 * (like http://strupat.ca/2010/10/cross-platform-function-to-get-the-line-size-of-your-cache/) 2) use the aligning
 * allocator _everywhere_ there might be potential cache line sharing -> investigate the use of mt_alloc with custom
 * alignment to solve the problem of frequently-used cache values during multiplication (e.g., the m_tmp members in
 * functors).
 * \todo use cache aligning allocator by default in GMP memory allocation functions if in mt mode? investigate performance impact.
 * \todo assert that concepts are respected in models.
 * \todo do we need an is_assignable_concept? it is used in monomial, for instance, or when doing std::copy
 * \todo replace exact-width types with least/fast, which are guaranteed to exist (exact width are not guaranteed) -> fix also in tests.
 * \todo check the include of pure C stuff (GMP is already fine, maybe the platform-specific stuff needs to be reviewed - aligning alloc, cache size, number of
 * threads, etc.). http://www.parashift.com/c++-faq-lite/mixing-c-and-cpp.html
 * \todo type-trait and report on startup if thread management primitives are available?
 */
namespace piranha
{

/// Namespace for implementation details.
/**
 * Classes and functions defined in this namespace are non-documented implementation details.
 * Users should never employ functionality implemented in this namespace.
 */
namespace detail {}

}

// NOTES FOR DOCUMENTATION:
// - thread safety: assume none unless specified
// - bad_cast due to boost numeric cast: say that it might be thrown in many places, too cumbersome
//   to document every occurrence.
// - c++0x features not implemented yet in GCC latest version: what impact they have and piranha_* macros used
//   to signal/emulate them.

#include "array_key.hpp"
#include "base_series.hpp"
#include "base_term.hpp"
#include "cache_aligning_allocator.hpp"
#include "concepts.hpp"
#include "config.hpp"
#include "cvector.hpp"
#include "debug_access.hpp"
#include "echelon_descriptor.hpp"
#include "echelon_position.hpp"
#include "echelon_size.hpp"
#include "exceptions.hpp"
#include "hash_set.hpp"
#include "integer.hpp"
#include "kronecker_array.hpp"
#include "kronecker_monomial.hpp"
#include "math.hpp"
#include "monomial.hpp"
#include "numerical_coefficient.hpp"
#include "polynomial_term.hpp"
#include "polynomial.hpp"
#include "runtime_info.hpp"
#include "series_binary_operators.hpp"
#include "series_multiplier.hpp"
#include "settings.hpp"
#include "static_vector.hpp"
#include "symbol.hpp"
#include "thread_barrier.hpp"
#include "thread_group.hpp"
#include "thread_management.hpp"
#include "threading.hpp"
#include "top_level_series.hpp"
#include "tracing.hpp"
#include "type_traits.hpp"
#include "univariate_monomial.hpp"
#include "utils.hpp"

#endif
