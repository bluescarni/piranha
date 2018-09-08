/* Copyright 2009-2017 Francesco Biscani (bluescarni@gmail.com)

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

#ifndef PIRANHA_PIRANHA_HPP
#define PIRANHA_PIRANHA_HPP

/*
 * \todo base_series test: missing merge terms with negative+move (that actually swaps the contents of the series) and
 * negative+move with different series types.
 * \todo check usage of max_load_factor (especially wrt flukes in * instead of / or viceversa).
 * \todo review use of numeric_cast: in some places we might be using it in such a way we expect errors if converting
 * floating point to int, but this is not the case (from the doc)
 * \todo look into perfect forwarding of members, for use in series, hash set (?)
 * http://stackoverflow.com/questions/8570655/perfect-forwarding-a-member-of-object
 * update: tried this on the series insertion methods, it seems like GCC does not implement this correctly (while clang
 * does). Check again in the future.
 * \todo it seems like default construction of c++ containers might throw :/ We should probably double-check we do not
 * assume any nothrow behaviour in
 * such cases. For instance, if we use an old-style C allocation function and we need to create a container _before_
 * calling free(), then the behaviour
 * might not be exception-safe.
 * \todo in pyranha, access to static variables should be made thread-safe (first of all in the Python sense,
 * e.g., importing the module from multiple Python threads).
 * \todo pyranha: enable math for numpy's floating point type, and arrays. Also, think about enabling conversion from
 * the numpy float
 * in the from-python converters? -> if we do this last bit, we must make sure that our custom converter does not
 * override any other
 * converter that might be registered in boost python. We need to query the registry and check at runtime.
 * \todo consider replacing the & operator with std::addressof in positional new forms. It seems there might be a perf.
 * penalty
 * involved in doing that, if that is the case we can either do it only if the type is not POD or maybe even if it does
 * not have
 * the operator overloaded (via decltype SFINAE).
 * \todo in pyranha, we should be able to provide self-descriptive docstrings for the exposed series, based on the
 * template enable_if mechanism.
 * \todo consider bringing back the unroller from the vectorization work into the small_vector class.
 * \todo some versions of mingw want __mingw_aligned_malloc instead of _aligned_malloc, fix this with a check in the
 * build system. Or
 * maybe check whether __mingw_aligned_malloc is available in all mingw versions.
 * \todo check usages of std algorithms against the assumptions on the functors used:
 * http://stackoverflow.com/questions/20119810/parallel-implementations-of-std-algorithms-and-side-effects
 * \todo review all usages of lexical_cast and stringstreams, probably we need either to replace them altogether or at
 * least to make
 * sure they behave consistently wrt locale settings. UPDATE: we can actually switch to std::to_string() in many cases,
 * and keep lexical_cast only for the conversion of piranha's types to string. UPDATE: it looks like to_string is
 * influenced by the locale setting, it's probably better to roll our implementation.
 * \todo review the usage of _fwd headers. It seems it is ok for friend declarations for instance, but wherever we might
 * need the full definition of the object we might want to reorganise the code.
 * \todo the prepare_for_print() should probably become a public print_exponent(), that also takes care of putting
 * brackets
 * e.g. when printing rational exponents with non-unitary denominator.
 * \todo probably we should change the pow() implementation for integer to error out if the power is negative and the
 * base
 * is not unitary.
 * \todo in pyranha, it would be nice to have a reverse lookup from the name of the exposed types to their
 * representation
 * in the type system. Plus, maybe when printing the series they should have a header displaying their name in the type
 * system and maybe the list of arguments. Also, what happens if we expose, say, polynomial<double> *and*
 * polynomial<double,k_monomial>,
 * supposing that they are the same type one day?
 * \todo need to review the requirements on all std object we use as members of classes. We often require them to be
 * noexcept
 * but they do not need to be by the standard (e.g., hash, equal_to, vector, ...). Note that in all our classes we mark
 * move
 * operations as noexcept so we don't really need to require std members to be noexcept (if they throw an exception -
 * unlikely
 * - the program will terminate anyway). We should also probably check the uses of std::move in order to make sure we do
 * not use
 * exception guarantees throughout the code.
 * \todo serialization: it seems like if the text in the archive is complete garbage, the destructor will throw. Check
 * that this behaviour
 * is ok in Python, and that the exception from boost serialization is thrown and translated properly. Maybe test
 * garbage archives
 * also in the existing serialization tests.
 * \todo positional new needs the <new> header.
 * \todo std::move() needs the <utility> header.
 * \todo we probably need a way to handle the excessive growth of ipow caches. Just keep the most recently used entries
 * up to a certain
 * user-configurable limit. Also, it might be useful to give the user the ability to query the cache, see how many items
 * are stored, etc.
 * \todo we should really add some perf tests based on the work by alex perminov. Also, based on this, which operations
 * in his use cases could
 * benefit from parallelisation?
 * \todo the replace_symbol() method for series. Or maybe rename_symbol().
 * \todo consider the use of the upcoming std::shared_lock/mutex for multiple readers/single writer situations (e.g., in
 * the custom derivative
 * machinery). Maybe we can do with the boost counterpart if it does not require extra linking, until C++14.
 * \todo the pattern of sin/cos in poisson series and invert in divisor_series (that is, recurse until a polynomial
 * coefficient is found) should probably
 * be applied in the integration routine for poisson series that integrates by part when coefficient has positive degree
 * in the integration variable
 * and the trig part also depend on the integration variable.
 * \todo related to the above: we should probably generalise the integral_combination() in polynomial to deal also with
 * recursively-represented polys,
 * so that, e.g., we can use them as coefficients in poisson series. Also the polynomial's special pow() and integrate()
 * method should be able to deal
 * with recursive polys in the same fashion. This should probably be a bullet point if we ever decide to support
 * recrusive polynomials as first-class citizens.
 * \todo the tuning:: class should probably be rolled into settings.
 * \todo hash_set needs more testing.
 * \todo maybe we should rename is_container_element to is_regular_type.
 * \todo the following items still remain to be finished up after the truncation rework:
 *   - re-evaluate the heuristic for choosing n_threads in fill_term_pointers, estimate_series_size, and the likes.
 *     Right now we are using the heuristic for series multiplication, but, at least in case of fill_term_pointers,
 *     it seems like we might be running in some overhead.
 *   - the fill_term_pointers parallelisation + deterministic ordering has not been done yet for rational coefficients.
 * \todo in a bunch of generic constructors all over the place, we enable them only if the argument is not the same type
 * as the calling class.
 * This should probably be an is_base_of check, as done in forwarding.hpp, so that if one derives from the class then we
 * are still not mixing
 * up generic ctor and standard copy/move ones in the derived class.
 * \todo the multiplication of a series by single coefficient can probably be handled in the binary_mul_impl() method.
 * \todo it seems like, at least in some cases, it is possible to avoid extra template arguments for enabling purposes
 * if one uses static methods rather than instance methods (something related to the calling class not being a complete
 * type). Keep this in mind in order to simplify signatures when dealing with compelx sfinae stuff.
 * \todo need probably to provide an overload to math::evaluate() taking init list, for ease of use from C++.
 * \todo the series multiplier estimation factor should probably be 1, but let's track performance before changing it.
 */

#include <mp++/config.hpp>

#include <piranha/array_key.hpp>
#include <piranha/base_series_multiplier.hpp>
#include <piranha/cache_aligning_allocator.hpp>
#include <piranha/config.hpp>
#include <piranha/convert_to.hpp>
#include <piranha/divisor.hpp>
#include <piranha/divisor_series.hpp>
#include <piranha/dynamic_aligning_allocator.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/hash_set.hpp>
#include <piranha/integer.hpp>
#include <piranha/invert.hpp>
#include <piranha/ipow_substitutable_series.hpp>
#include <piranha/is_cf.hpp>
#include <piranha/is_key.hpp>
#include <piranha/key/key_degree.hpp>
#include <piranha/key/key_is_one.hpp>
#include <piranha/key/key_is_zero.hpp>
#include <piranha/key/key_ldegree.hpp>
#include <piranha/key_is_convertible.hpp>
#include <piranha/key_is_multipliable.hpp>
#include <piranha/kronecker_monomial.hpp>
#include <piranha/lambdify.hpp>
#include <piranha/math.hpp>
#include <piranha/math/binomial.hpp>
#include <piranha/math/cos.hpp>
#include <piranha/math/degree.hpp>
#include <piranha/math/gcd.hpp>
#include <piranha/math/gcd3.hpp>
#include <piranha/math/is_one.hpp>
#include <piranha/math/is_zero.hpp>
#include <piranha/math/ldegree.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/math/sin.hpp>
#include <piranha/memory.hpp>
#include <piranha/monomial.hpp>
#include <piranha/poisson_series.hpp>
#include <piranha/polynomial.hpp>
#include <piranha/power_series.hpp>
#include <piranha/print_coefficient.hpp>
#include <piranha/print_tex_coefficient.hpp>
#include <piranha/rational.hpp>
#include <piranha/utils/kronecker_encdec.hpp>
#if defined(MPPP_WITH_MPFR)
#include <piranha/real.hpp>
#endif
#include <piranha/real_trigonometric_kronecker_monomial.hpp>
#include <piranha/runtime_info.hpp>
#include <piranha/s11n.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/safe_convert.hpp>
#include <piranha/series.hpp>
#include <piranha/series_multiplier.hpp>
#include <piranha/settings.hpp>
#include <piranha/small_vector.hpp>
#include <piranha/static_vector.hpp>
#include <piranha/substitutable_series.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/t_substitutable_series.hpp>
#include <piranha/term.hpp>
#include <piranha/thread_barrier.hpp>
#include <piranha/thread_management.hpp>
#include <piranha/thread_pool.hpp>
#include <piranha/trigonometric_series.hpp>
#include <piranha/tuning.hpp>
#include <piranha/type_traits.hpp>

#endif
