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
 * \todo modify concepts to use declval where applicable, instead of dereferencing nullptr.
 * \todo base_series test: missing merge terms with negative+move (that actually swaps the contents of the series) and negative+move with different series types.
 * \todo concepts: how to deal with generic methods (e.g., coefficient in-place multiply by whatever)? We could add another parameter to the concept, with default void,
 * and use it explictly only when actually using that generic method? ADDENDUM: connected to this, the question of dealing with methods that
 * have additional type requirements wrt those delcared in class. Probably need to review all generic methods and add type requirements there.
 * This should all tie in with the work of starting using type traits more extensively, especially arithmetic ones, an is_exponentiable() type trait, etc.
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
 * only if T + U produces T. When going down that route, use and expand the existing arithmetic type traits (is_addable, etc.). Probably it is a good idea
 * in this piece of work to start reworking the generic series constructors, possibly dropping the converting constructors for keys, and move from there.
 * In general it seems possibly useful to remove interaction with different keys from interoperable operators (but keep it for constructors?).
 * Connected to this, we should specify better the semantics of math operations: a +-*= b means exactly a = a +-* b (use optimising behaviour like
 * implementing in-place add separately only when it is really equivalent), and specify this well in the documentation
 * of series multiplier, term's multiply methods, etc. (they should make clear that the return type deduction and operand orders was already determined for them).
 * \todo series multadd to speed-up series multiplication when coefficients are series?
 * \todo start experimenting with parallel sort in multipliers and truncators (e.g., for the rectangular benchmark).
 * \todo forbid interoperability in +-* between series with different keys?
 * \todo interface to query degree should be the same across power series and truncator: should add missing overload in power series to query partial degree of a single symbol,
 * and maybe replace in the high level interface the std::set of string with initializer list, as it seems sometimes {"a","b"} is not picked up as std::set of string.
 * \todo look into perfect forwarding of members, for use in series, hash set (?) http://stackoverflow.com/questions/8570655/perfect-forwarding-a-member-of-object
 * \todo look into forwarding and delegating constructors when they become available.
 * \todo understand the consequences of not compiling boost libs we link to in c++0x mode. Stuff like this could happen:
 * http://stackoverflow.com/questions/10498887/bug-in-libstdc-regarding-stdlist-assignment -> compile boost in c++11/c++0x mode in gentoo and windows.
 * \todo it seems like default construction of c++ containers might throw :/ We should probably double-check we do not assume any nothrow behaviour in
 * such cases. For instance, if we use an old-style C allocation function and we need to create a container _before_ calling free(), then the behaviour
 * might not be exception-safe.
 * \todo think about the generic binary term constrcutor, especially in conjunction with the generic series interop. Do we want to leave it generic
 * or force it to be strictly from (cf_type,key_type)? In the latter case, we should review its usage.
 * \todo think about replacing the concept system with static_asserts in conjunction with extensive use of type traits. It seems like it
 * would allow a finer control with generic methods (e.g., polynomial is a Poisson series coefficient only if it supports
 * division by int, partial() requires multipliability by int/integer, etc.) and enable meta-programming. But how to implement
 * concept inheriting? Where to put the static asserts? Is it worth it? NOTE: related to this, should we restrict generic and forwarding
 * constructors in series to accept only parameters for which the construction can happen? This way also the type trait is_constructible would
 * work, whereas now the generic constructors gobble up everything.
 * \todo univariate_monomial has been left behind a bit feature-wise.
 * \todo in pyranha, access to static variables should be made thread-safe (first of all in the Python sense,
 * e.g., importing the module from multiple Python threads). In particular, access to the coefficient list (construct on first
 * use with mutex protection instead of at register time? or maybe avoid using static variable and build each time)
 * and python converters (protect the inited flags with mutexes).
 * \todo instead of disabling debug checks at shutdown for series, maybe we should do like in Python and register an
 * atexit() function to clean up custom derivatives before static destruction starts. We could register the atexit
 * at the first invocation of register_custom_derivative() for each series type, set a flag and then query the flag each time.
 * Probably the existing mutex can be resued as well. Probably it makes sense to keep both, as the existing method would
 * work in a more generic fashion.
 * \todo pyranha: enable math for numpy's floating point type, and arrays. Also, think about enabling conversion from the numpy float
 * in the from-python converters?
 * \todo: pyranha tests should test the *exposition* and/or wrapping, not the functionality of the library. For poly/poisson series, add
 * tests for degree/order, plus add in math.py the degree/order methods in order to mirror math.hpp.
 * \todo in the rework of the substitution methods with toolboxes, remember to switch the interface of the key's subs to use string
 * instead of symbol for consistency.
 * \todo initializer_list ctors: should they be explicit or not?
 * \todo binomial coefficient: for double/real types it should really be implemented in terms of gamma functions:
 * http://www.boost.org/doc/libs/release/libs/math/doc/sf_and_dist/html/math_toolkit/special/factorials/sf_binomial.html
 * http://mathworld.wolfram.com/GammaFunction.html
 * http://www.mpfr.org/mpfr-current/mpfr.html (implement in terms of gamma functions as indicated by the Wolfram link)
 * \todo review usage of ::new, we probably want to switch to unqualified new() in order to account for possible overloads
 * to be found via ADL.
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

#include "array_key.hpp"
#include "base_term.hpp"
#include "cache_aligning_allocator.hpp"
#include "concepts.hpp"
#include "config.hpp"
#include "debug_access.hpp"
#include "degree_truncator_settings.hpp"
#include "echelon_size.hpp"
#include "environment.hpp"
#include "exceptions.hpp"
#include "extended_integer_types.hpp"
#include "hash_set.hpp"
#include "integer.hpp"
#include "kronecker_array.hpp"
#include "kronecker_monomial.hpp"
#include "malloc_allocator.hpp"
#include "math.hpp"
#include "monomial.hpp"
#include "poisson_series.hpp"
#include "poisson_series_term.hpp"
#include "polynomial_term.hpp"
#include "polynomial.hpp"
#include "power_series.hpp"
#include "power_series_term.hpp"
#include "power_series_truncator.hpp"
#include "print_coefficient.hpp"
#include "print_tex_coefficient.hpp"
#include "rational.hpp"
#include "real.hpp"
#include "real_trigonometric_kronecker_monomial.hpp"
#include "runtime_info.hpp"
#include "series.hpp"
#include "series_binary_operators.hpp"
#include "series_multiplier.hpp"
#include "settings.hpp"
#include "static_vector.hpp"
#include "symbol.hpp"
#include "symbol_set.hpp"
#include "t_substitutable_series.hpp"
#include "task_group.hpp"
#include "thread_barrier.hpp"
#include "thread_management.hpp"
#include "threading.hpp"
#include "timeit.hpp"
#include "tracing.hpp"
#include "trigonometric_series.hpp"
#include "truncator.hpp"
#include "type_traits.hpp"
#include "univariate_monomial.hpp"

#endif
