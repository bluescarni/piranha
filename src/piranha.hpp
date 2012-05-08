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
 * (see http://stackoverflow.com/questions/4749863/variadic-templates-and-copy-construction-via-assignment) ->
 * keep in mind in some cases the tags are actually used for discriminating between types, adjust it accordingly (e.g., power series).
 * \todo switch to auto -> decltype declarations of member functions for complicated types (e.g., tuples) when decltype on this
 * becomes available.
 * \todo explain in general section the base assumptions of move semantics and thread safety (e.g., require implicitly that
 * all moved-from objects are assignable and destructable, and everything not thread-safe by default).
 * \todo document classes in detail:: namespace used to implement mathematical functions?
 * \todo fix type traits such as is_nothrow_move_constructible/assignable. They must work also for references,
 * make them use SFINAE, declval and decltype as in the current GCC 4.6 type_traits header (e.g., see is_constructible). Keep in mind
 * reference collapsing rules, etc. etc.
 * \todo modify concepts to use declval where applicable, instead of dereferencing nullptr.
 * \todo base_series test: missing merge terms with negative+move (that actually swaps the contents of the series) and negative+move with different series types.
 * \todo concepts: how to deal with generic methods (e.g., coefficient in-place multiply by whatever)? We could add another parameter to the concept, with default void,
 * and use it explictly only when actually using that generic method? ADDENDUM: connected to this, the question of dealing with methods that
 * have additional type requirements wrt those delcared in class. Probably need to review all generic methods and add type requirements there.
 * \todo hashing of array_key based on kronecker substitution -> use static table of kronecker_array to encode the array key in an integer -> note that this becomes
 * moot in case the homomorphic hashing thing works out.
 * \todo assert that concepts are respected in models.
 * \todo check the series concept: where is it used?
 * \todo check wherever use use std::vector as class member that we implement copy assignment with copy+move. There is no guarantee that copy operator=() on vector
 * (or standard containers) has strong exception safety guarantee.
 * \todo check usage of max_load_factor (especially wrt flukes in * instead of / or viceversa).
 * \todo review use of numeric_cast: in some places we might be using it in such a way we expect errors if converting floating point to int, but this is not the case (from the doc)
 * \todo the tuning parameters should be tested and justified (e.g., when to go into mt mode, etc.).
 * \todo probably we should make overflow-safe all operations on keys that currently are not: multiplication, computation of degree (including in truncators), etc.
 * and then optimize instead in the multipliers (by checking the ranges before performing the multiplication) -> this probably becomes null in case the caching of degree
 * values in the truncators turns out to work ok.
 * \todo think of new way of interoperate between composite types (e.g., complex and series) vs non-composite types. I.e., allow complex<T> + U
 * only if T + U produces T.
 * \todo start experimenting with parallel sort in multipliers and truncators (e.g., for the rectangular benchmark).
 * \todo forbid interoperability in +-* between series with different keys?
 * \todo interface to query degree should be the same across power series and truncator: should add missing overload in power series to query partial degree of a single symbol,
 * and maybe replace in the high level interface the std::set of string with initializer list, as it seems sometimes {"a","b"} is not picked up as std::set of string.
 * \todo look into perfect forwarding of members, for use in series, hash set (?) http://stackoverflow.com/questions/8570655/perfect-forwarding-a-member-of-object
 * \todo look into forwarding and delegating constructors when they become available.
 * \todo understand the consequences of not compiling boost libs we link to in c++0x mode. Stuff like this could happen:
 * http://stackoverflow.com/questions/10498887/bug-in-libstdc-regarding-stdlist-assignment
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
#include "base_term.hpp"
#include "cache_aligning_allocator.hpp"
#include "concepts.hpp"
#include "config.hpp"
#include "debug_access.hpp"
#include "degree_truncator_settings.hpp"
#include "echelon_size.hpp"
#include "exceptions.hpp"
#include "extended_integer_types.hpp"
#include "hash_set.hpp"
#include "integer.hpp"
#include "kronecker_array.hpp"
#include "kronecker_monomial.hpp"
#include "malloc_allocator.hpp"
#include "math.hpp"
#include "monomial.hpp"
#include "polynomial_term.hpp"
#include "polynomial.hpp"
#include "power_series.hpp"
#include "power_series_term.hpp"
#include "power_series_truncator.hpp"
#include "print_coefficient.hpp"
#include "rational.hpp"
#include "runtime_info.hpp"
#include "series.hpp"
#include "series_binary_operators.hpp"
#include "series_multiplier.hpp"
#include "settings.hpp"
#include "static_vector.hpp"
#include "symbol.hpp"
#include "symbol_set.hpp"
#include "task_group.hpp"
#include "thread_barrier.hpp"
#include "thread_management.hpp"
#include "threading.hpp"
#include "timeit.hpp"
#include "tracing.hpp"
#include "truncator.hpp"
#include "type_traits.hpp"
#include "univariate_monomial.hpp"

#endif
