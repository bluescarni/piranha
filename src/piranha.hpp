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
/*
 * \todo explain in general section the base assumptions of move semantics and thread safety (e.g., require implicitly that
 * all moved-from objects are assignable and destructable, and everything not thread-safe by default).
 * \todo base_series test: missing merge terms with negative+move (that actually swaps the contents of the series) and negative+move with different series types.
 * \todo check wherever use use std::vector as class member that we implement copy assignment with copy+move. There is no guarantee that copy operator=() on vector
 * (or standard containers) has strong exception safety guarantee.
 * \todo check usage of max_load_factor (especially wrt flukes in * instead of / or viceversa).
 * \todo review use of numeric_cast: in some places we might be using it in such a way we expect errors if converting floating point to int, but this is not the case (from the doc)
 * \todo the tuning parameters should be tested and justified (e.g., when to go into mt mode, etc.).
 * \todo probably we should make overflow-safe all operations on keys that currently are not: multiplication, computation of degree (including in truncators), etc.
 * and then optimize instead in the multipliers (by checking the ranges before performing the multiplication) -> this probably becomes null in case the caching of degree
 * values in the truncators turns out to work ok.
 * \todo series multadd to speed-up series multiplication when coefficients are series?
 * \todo forbid interoperability in +-* between series with different keys?
 * \todo look into perfect forwarding of members, for use in series, hash set (?)
 * http://stackoverflow.com/questions/8570655/perfect-forwarding-a-member-of-object
 * update: tried this on the series insertion methods, it seems like GCC does not implement this correctly (while clang does). Check again in the future.
 * \todo look into forwarding and delegating constructors when they become available.
 * \todo understand the consequences of not compiling boost libs we link to in c++0x mode. Stuff like this could happen:
 * http://stackoverflow.com/questions/10498887/bug-in-libstdc-regarding-stdlist-assignment -> compile boost in c++11/c++0x mode in gentoo and windows.
 * \todo it seems like default construction of c++ containers might throw :/ We should probably double-check we do not assume any nothrow behaviour in
 * such cases. For instance, if we use an old-style C allocation function and we need to create a container _before_ calling free(), then the behaviour
 * might not be exception-safe.
 * \todo think about the generic binary term constrcutor, especially in conjunction with the generic series interop. Do we want to leave it generic
 * or force it to be strictly from (cf_type,key_type)? In the latter case, we should review its usage.
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
 * \todo review usage of new, we probably want to switch to unqualified new() in order to account for possible overloads
 * to be found via ADL -> note that placement new cannot be overloaded:
 * http://stackoverflow.com/questions/3675059/how-could-i-sensibly-overload-placement-operator-new (and 18.6.1.3 in the standard)
 * so we might as well keep ::new in those cases.
 * \todo similarly, review all struct/class is_/has_is_ type traits to prevent ADL by using piranha specifiers as needed.
 * \todo should we always use piranha when calling functions in order to prevent ADL? -> note that these ADL concerns apply
 * only to unqualified function calls, of which there are not many (e.g., the math type traits are all defined outside piranha::math
 * and hence always include the math:: qualifier).
 * \todo consider replacing uses of iterator facade with inheritance from std::iterator.
 * \todo after the switch to 4.8, we can drop in many places the forward ctor macro in favour of just inheriting constructors (in other
 * places, e.g., polynomial, we still need them as we are adding new custom ctors). Probably the assignment macro must stay anyway.
 * update: tried this for a while, it looks like the semantics of inheriting ctors might not be what we need, and the support in compilers
 * is still brittle. Maybe revisit in the future.
 * \todo consider replacing the & operator with std::addressof in positional new forms. It seems there might be a perf. penalty
 * involved in doing that, if that is the case we can either do it only if the type is not POD or maybe even if it does not have
 * the operator overloaded (via decltype SFINAE).
 * \todo in pyranha, we should be able to provide self-descriptive docstrings for the exposed series, based on the
 * template enable_if mechanism.
 * \todo consider bringing back the unroller from the vectorization work into the small_vector class.
 * \todo some versions of mingw want __mingw_aligned_malloc instead of _aligned_malloc, fix this with a check in the build system. Or
 * maybe check whether __mingw_aligned_malloc is available in all mingw versions.
 * \todo pyranha: try to understand what is the best way to have functions which are extensible from the user. E.g., we have math.cos
 * that works on series, mpmath, etc., how can we provide a mechanism for a user to add her own specialisations?
 * http://stackoverflow.com/questions/18957424/proper-way-to-make-functions-extensible-by-the-user
 * \todo check usages of std algorithms against the assumptions on the functors used:
 * http://stackoverflow.com/questions/20119810/parallel-implementations-of-std-algorithms-and-side-effects
 * \todo probably better to remove the thread_management class and use free functions directly for the binding.
 * \todo review the usage of the static keyword for functions: we are header-only now, it's probably not needed (esp. static inline).
 * \todo review all usages of lexical_cast and stringstreams, probably we need either to replace them altogether or at least to make
 * sure they behave consistently wrt locale settings.
 * \todo doxygen: check usage of param[(in,)out], and consider using the tparam command.
 * \todo review the use of return statements with const objects, if any.
 * \todo math::is_zero() is used to determine ignorability of a term in a noexcept method in term. Should we require it to be
 * noexcept as well and put the requirement in the is_cf type trait?
 * \todo floating point stuff: there's a few things we can improve here. The first problem is that, in some places where it could
 * matter (interop mp_integer/rational <--> float) we don't check for math errors, as explained here:
 * http://en.cppreference.com/w/cpp/numeric/math/math_errhandling
 * We should probably check for errors when we need things to be exact and safe. These include ilogb, scalb, trunc, etc. Secondly,
 * we have a bunch of generic fp algorithms that could be easily extended to work with nonstandard fp types such as quadmath
 * and decimal. We need then to abstract fp standard functions in our own wrappers and abstract away in a separate place our
 * generic algos scattered around. Then in the wrappers we could add automatic checks for errno (raise exception) and kill two
 * birds with one stone.
 * \todo review the usage of _fwd headers. It seems it is ok for friend declarations for instance, but wherever we might
 * need the full definition of the object we might want to reorganise the code.
 * \todo the prepare_for_print() should probably become a public print_exponent(), that also takes care of putting brackets
 * e.g. when printing rational exponents with non-unitary denominator.
 * \todo probably we should change the pow() implementation for integer to error out if the power is negative and the base
 * is not unitary.
 * \todo think about providing default implementations of cos/sin/pow/... that use ADL -> we actually already use ADL in expr.
 * sfinae when using operators such as +,-,<<, etc.
 * \todo http://keepachangelog.com/CHANGELOG.md
 * \todo in pyranha, it would be nice to have a reverse lookup from the name of the exposed types to their representation
 * in the type system. Plus, maybe when printing the series they should have a header displaying their name in the type
 * system and maybe the list of arguments. Also, what happens if we expose, say, polynomial<double> *and* polynomial<double,k_monomial>,
 * supposing that they are the same type one day?
 * \todo should the print coefficient operator of real print the precision as well or is the number of digits enough hint?
 * \todo pyranha: in the docstrings probably we should change from relative to absolute imports, for clarity.
 * \todo try to minimise the use of is_instance_of, as it is fragile at the moment.
 * \todo need to review the requirements on all std object we use as members of classes. We often require them to be noexcept
 * but they do not need to be by the standard (e.g., hash, equal_to, vector, ...). Note that in all our classes we mark move
 * operations as noexcept so we don't really need to require std members to be noexcept (if they throw an exception - unlikely
 * - the program will terminate anyway). We should also probably check the uses of std::move in order to make sure we do not use
 * exception guarantees throughout the code.
 * \todo it seems like serialization of kronecker objects could not be platform-agnostic, even in text mode. This is because
 * the width of a type on a platform could be different from the original size in the archive, and the codification/decodification
 * will yield different unpacked values. Needs to be fixed, probably by serialising the unpacked vectors.
 */
namespace piranha
{

// Namespace for implementation details.
// Classes and functions defined in this namespace are non-documented implementation details.
// Users should never employ functionality implemented in this namespace.
namespace detail {}

/// Inline namespace for the definition of user-defined literals.
inline namespace literals {}

}

#include "array_key.hpp"
#include "cache_aligning_allocator.hpp"
#include "config.hpp"
#include "convert_to.hpp"
#include "debug_access.hpp"
#include "dynamic_aligning_allocator.hpp"
#include "environment.hpp"
#include "exceptions.hpp"
#include "hash_set.hpp"
#include "is_cf.hpp"
#include "is_key.hpp"
#include "key_is_multipliable.hpp"
#include "kronecker_array.hpp"
#include "kronecker_monomial.hpp"
#include "math.hpp"
#include "memory.hpp"
#include "monomial.hpp"
#include "mp_integer.hpp"
#include "mp_rational.hpp"
#include "poisson_series.hpp"
#include "polynomial.hpp"
#include "power_series.hpp"
#include "print_coefficient.hpp"
#include "print_tex_coefficient.hpp"
#include "real.hpp"
#include "real_trigonometric_kronecker_monomial.hpp"
#include "runtime_info.hpp"
#include "safe_cast.hpp"
#include "serialization.hpp"
#include "series.hpp"
#include "series_multiplier.hpp"
#include "settings.hpp"
#include "small_vector.hpp"
#include "static_vector.hpp"
#include "symbol.hpp"
#include "symbol_set.hpp"
#include "t_substitutable_series.hpp"
#include "term.hpp"
#include "thread_barrier.hpp"
#include "thread_management.hpp"
#include "thread_pool.hpp"
#include "tracing.hpp"
#include "trigonometric_series.hpp"
#include "tuning.hpp"
#include "type_traits.hpp"

#endif
