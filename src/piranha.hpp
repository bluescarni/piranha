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

#ifndef PIRANHA_PIRANHA_HPP
#define PIRANHA_PIRANHA_HPP

/** \file piranha.hpp
 * \brief Global piranha header file.
 *
 * Include this file to import piranha's entire public interface.
 */

/// Root piranha namespace.
/*
 * \todo explain in general section the base assumptions of move semantics and thread safety (e.g., require implicitly
 * that
 * all moved-from objects are assignable and destructable, and everything not thread-safe by default).
 * \todo base_series test: missing merge terms with negative+move (that actually swaps the contents of the series) and
 * negative+move with different series types.
 * \todo check usage of max_load_factor (especially wrt flukes in * instead of / or viceversa).
 * \todo review use of numeric_cast: in some places we might be using it in such a way we expect errors if converting
 * floating point to int, but this is not the case (from the doc)
 * \todo the tuning parameters should be tested and justified (e.g., when to go into mt mode, etc.).
 * \todo series multadd to speed-up series multiplication when coefficients are series?
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
 * e.g., importing the module from multiple Python threads). In particular, access to the coefficient list (construct on
 * first
 * use with mutex protection instead of at register time? or maybe avoid using static variable and build each time)
 * and python converters (protect the inited flags with mutexes).
 * \todo instead of disabling debug checks at shutdown for series, maybe we should do like in Python and register an
 * atexit() function to clean up custom derivatives before static destruction starts. We could register the atexit
 * at the first invocation of register_custom_derivative() for each series type, set a flag and then query the flag each
 * time.
 * Probably the existing mutex can be resued as well. Probably it makes sense to keep both, as the existing method would
 * work in a more generic fashion.
 * \todo pyranha: enable math for numpy's floating point type, and arrays. Also, think about enabling conversion from
 * the numpy float
 * in the from-python converters? -> if we do this last bit, we must make sure that our custom converter does not
 * override any other
 * converter that might be registered in boost python. We need to query the registry and check at runtime.
 * \todo initializer_list ctors: should they be explicit or not?
 * \todo review usage of new, we probably want to switch to unqualified new() in order to account for possible overloads
 * to be found via ADL -> note that placement new cannot be overloaded:
 * http://stackoverflow.com/questions/3675059/how-could-i-sensibly-overload-placement-operator-new (and 18.6.1.3 in the
 * standard)
 * so we might as well keep ::new in those cases.
 * \todo after the switch to 4.8, we can drop in many places the forward ctor macro in favour of just inheriting
 * constructors (in other
 * places, e.g., polynomial, we still need them as we are adding new custom ctors). Probably the assignment macro must
 * stay anyway.
 * update: tried this for a while, it looks like the semantics of inheriting ctors might not be what we need, and the
 * support in compilers
 * is still brittle. Maybe revisit in the future.
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
 * \todo doxygen: check usage of param[(in,)out], and consider using the tparam command.
 * \todo review the use of return statements with const objects, if any.
 * \todo math::is_zero() is used to determine ignorability of a term in a noexcept method in term. Should we require it
 * to be
 * noexcept as well and put the requirement in the is_cf type trait?
 * \todo floating point stuff: there's a few things we can improve here. The first problem is that, in some places where
 * it could
 * matter (interop mp_integer/rational <--> float) we don't check for math errors, as explained here:
 * http://en.cppreference.com/w/cpp/numeric/math/math_errhandling
 * We should probably check for errors when we need things to be exact and safe. These include ilogb, scalb, trunc, etc.
 * Secondly,
 * we have a bunch of generic fp algorithms that could be easily extended to work with nonstandard fp types such as
 * quadmath
 * and decimal. We need then to abstract fp standard functions in our own wrappers and abstract away in a separate place
 * our
 * generic algos scattered around. Then in the wrappers we could add automatic checks for errno (raise exception) and
 * kill two
 * birds with one stone.
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
 * \todo should the print coefficient operator of real print the precision as well or is the number of digits enough
 * hint?
 * \todo need to review the requirements on all std object we use as members of classes. We often require them to be
 * noexcept
 * but they do not need to be by the standard (e.g., hash, equal_to, vector, ...). Note that in all our classes we mark
 * move
 * operations as noexcept so we don't really need to require std members to be noexcept (if they throw an exception -
 * unlikely
 * - the program will terminate anyway). We should also probably check the uses of std::move in order to make sure we do
 * not use
 * exception guarantees throughout the code.
 * \todo do the noexcept methods in keys really need to be noexcept? Maybe it is better to offer a weaker exception
 * guarantee
 * and be done with them instead.
 * \todo there could be some tension between SFINAE and the hard errors from static asserts in certain type traits such
 * as key_is_*,
 * series_is_*, etc. So far this has resulted in no practical problems, but in the future we might want to look again at
 * this.
 * UPDATE: this came up and was solved in series_is_rebindable by replacing the hard assertion errors with simply
 * setting the value
 * of the type trait to false via a specialisation. Keep this solution in mind if the problem arises elsewhere.
 * \todo serialization: it seems like if the text in the archive is complete garbage, the destructor will throw. Check
 * that this behaviour
 * is ok in Python, and that the exception from boost serialization is thrown and translated properly. Maybe test
 * garbage archives
 * also in the existing serialization tests.
 * \todo positional new needs the <new> header.
 * \todo std::move() needs the <utility> header.
 * \todo as an idea, the series specialisations for the impl functors in the toolboxes might all go in series.hpp, with
 * the following conditions:
 * - the involved object is/are series,
 * - they support the needed methods (e.g., subs(), degree(), etc.).
 * This way if we need, e.g., a custom subs() in a particular series type, we can implement the custom method (i.e.,
 * without using the toolbox) but still ending
 * up with a correct math::subs() specialisation without having to re-code it for the particular series type. We need to
 * check that we always use
 * math::* functors instead of member functions in order to avoid picking the base implementation.
 * \todo related to the above, beautification of the enabling conditions for impl functors - in the same fashion as we
 * do for methods and functions.
 * \todo we probably need a way to handle the excessive growth of ipow caches. Just keep the most recently used entries
 * up to a certain
 * user-configurable limit. Also, it might be useful to give the user the ability to query the cache, see how many items
 * are stored, etc.
 * \todo we should really add some perf tests based on the work by alex perminov. Also, based on this, which operations
 * in his use cases could
 * benefit from parallelisation?
 * \todo the replace_symbol() method for series. Or maybe rename_symbol().
 * \todo get rid of the global state for the symbols, just store strings. This should allow to remove the ugliness of
 * checking the shutdown flag.
 * \todo consider the use of the upcoming std::shared_lock/mutex for multiple readers/single writer situations (e.g., in
 * the custom derivative
 * machinery). Maybe we can do with the boost counterpart if it does not require extra linking, until C++14.
 * \todo it looks like in many cases we can hide excess default template parameters used in TMP by adding an extra layer
 * of indirection. This has only cosmetic
 * value, but might be worth for clarity in the long run.
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
 * \todo think about removing the noexcept requirements for ignorability and compatibility of terms. This makes sense
 * logically as ignorability is anyway
 * gonna call is_zero(), which might throw (see bp_object for instance), we might end up simplifying the logic and we
 * don't lose much (not a big deal
 * if the exception safety is weaker). If we do this, we need to check all usages of is_ignorable()/is_compatible(),
 * re-evaluate the exception handling
 * where they are used and update the docs for exception specifications.
 * \todo hash_set needs more testing.
 * \todo maybe we should rename is_container_element to is_regular_type.
 * \todo the following items still remain to be finished up after the truncation rework:
 *   - re-evaluate the heuristic for choosing n_threads in fill_term_pointers, estimate_series_size, and the likes.
 * Right now we are using
 *     the heuristic for series multiplication, but, at least in case of fill_term_pointers, it seems like we might be
 * running in some overhead.
 *   - the fill_term_pointers parallelisation + deterministic ordering has not been done yet for rational coefficients.
 * \todo in a bunch of generic constructors all over the place, we enable them only if the argument is not the same type
 * as the calling class.
 * This should probably be an is_base_of check, as done in forwarding.hpp, so that if one derives from the class then we
 * are still not mixing
 * up generic ctor and standard copy/move ones in the derived class.
 * \todo in order to circumvent the problem of the lack of thread local storage on osx, we should probably just create a
 * local variable ad-hoc.
 * It will be suboptimal but at least it should work on osx.
 * \todo we need to review the documentation/implementation of type traits were we strip away cv qualifications vs,
 * e.g., implementing the test() method
 * in terms of const references. I think in some cases it should be made more explicit and consistent across the type
 * traits.
 * \todo the multiplication of a series by single coefficient can probably be handled in the binary_mul_impl() method.
 * Once we have this, we could
 * also think about re-implementing multiplication by zero by an actual coefficient multiplication, thus solving the
 * incosistency with double
 * coefficients reported in audi (0 * inf = 0 --> empty polynomial, instead of NaN).
 * \todo in mp_integer probably the ternary operations (and multadd and divexact etc.) should be modified so that the
 * return value is demoted to
 * static if the other operands are static as well. Right now, if one re-uses the same output object multiple times,
 * once it is set to dynamic
 * storage there's no going back. On the other hand, that is what one might want in some cases (e.g., a value that
 * iteratively always increases).
 * Not sure there's a general solution.
 * \todo safe_cast should probably have its own special exception. As it stands, when we do try { safe_cast() } catch {}
 * we are catching other
 * errors as unsafe cast where they might not be (e.g., a memory error). It is important to know when safe_cast fails
 * because of unsafe cast
 * rather than other errors, see e.g. how it is used in the poly linear arg combination.
 * \todo same above applies for linear arg combination
 * \todo the subs methods of the keys should probably use the symbol position map and allow for more than 1 sub at a
 * time.
 * \todo when we rework division/gcd with the ordered poly representation, we need also to solve the issue of the
 * interaction between truncation
 * and division/GCD operations. It seems like we will need to relax some assumptions and assertions (e.g.,
 * multiplication by a non-zero entity
 * could result in zero because of truncation), and add checks so that we can throw cleanly if something goes bad
 * (division by zero, etc.).
 * \todo related to the above, since we use pow() in the GCD algorithms we also have the problem of pow_caches() having
 * the incorrect results.
 * It seems like a good course of action would be to just invalidate the caches every time the truncation limit changes.
 * We need to check with
 * symengine in order to make sure this does not create troubles for them. Even after we do that, we still need to
 * account for potentially strange
 * things happening when we use pow() in GCD (e.g., nonzero poly to positive power returning zero).
 * \todo again related to the above, there are a couple of instances of use of lterm with potentially zero polynomial
 * due to truncation. Keep in mind,
 * probably it is just best to replace poly multiplications with explicitly untruncated counterparts in rational
 * function.
 * \todo implement the GCD benchmarks from the liao paper as a performance test.
 * \todo we need to decide if we want to keep the postifx notation for things like eval, subs etc. or just support the
 * math:: versions. It probably does
 * not make much sense to go back and remove the methods in the toolboxes, but for documentation purposes and in python
 * particularly we should just
 * support the math:: overloads (with partial() being the lone exception).
 * \todo it seems like, at least in some cases, it is possible to avoid extra template arguments for enabling purposes
 * if one uses static methods rather than instance methods (something related to the calling class not being a complete
 * type). Keep this in mind in order to simplify signatures when dealing with compelx sfinae stuff.
 * \todo need probably to provide an overload to math::evaluate() taking init list, for ease of use from C++.
 * \todo is_unitary() should be implemented for real and series as well.
 * \todo we are using std::decay all over the place to, essentially, remove cv and ref qualifiers from types. But decay
 * also turns arrays into pointers and functions into pointers. Maybe we should have a remove_cvr type trait that just
 * removes
 * cv and refs instead. Not sure if we care about plain arrays and function pointers enough though.
 * \todo the evaluate requirements and type trait do not fail when the second type is a reference. this should be fixed
 * in the type-traits rework.
 * \todo in the pyranha doc improvements, we should probably handle better unspecified exceptions and document
 * the return type as well for consistency (see lambdify docs) -> actually start using sphinx napoleon
 * \todo "quick install" should not be the title of the getting started section in sphinx
 * \todo consider poly::udivrem(5*x**2,2*x). This throws an inexact division error triggered by inexact integral cf
 * division, but maybe it should just return (0,5*x**2).
 * \todo might want to put a recursion limit on the recursive algorithms for GCD/division, in order to avoid
 * crashing from Python due to stack overflow.
 * \todo it seems like in C++17 we can finally have an automatically inited global class in which to tuck the init
 * code (and probably the thread pool as well), via inline variables. Probably we will need to define it in a separate
 * header and then make sure to include that header in every piranha public header.
 * \todo safe_cast fixages: remove the dependency on mp_integer, fix the exception usage as explained above,
 * and once this is done check all uses of boost numeric_cast, which should now be replaceable by safe_cast.
 * Check also the fwd declaration usages which work around the current issues.
 * \todo checkout the --enable-fat GMP build option - it looks like this is the way to go for a generic GMP lib
 * for binary windows distributions.
 * \todo the series multiplier estimation factor should probably be 1, but let's track performance before changing it.
 * \todo guidelines for type traits modernization:
 * - beautify enablers,
 * - replace std::decay with uncvref_t,
 * - check returnability,
 * - key_* type traits should probably deal with cvref types (with respect, for instance, to the is_key check),
 *   in the same fashion as the s11n type traits.
 * \todo just replace @param[in]/@param[out] with just @param
 * \todo keep in mind the analysis done for the boost_save() implementation for series. The general issue seems to be
 * that when implementing functionality recursively (e.g., the boost_save() availability depends on the availability
 * of boost_save() for another type) we need to make sure we don't enter a loop in which we instantiate the same check.
 * The general guideline seems to be: in the enabling condition for a foo_impl specialisation for T,
 * avoid checking has_foo for types not depending on T. If this is necessary, do it in 2 stages (as in boost_save).
 */
namespace piranha
{

// Namespace for implementation details.
// Classes and functions defined in this namespace are non-documented implementation details.
// Users should never employ functionality implemented in this namespace.
namespace detail
{
}

// Same as above, new version as inline namespace so we don't have to use detail:: everywhere.
inline namespace impl
{
}

/// Inline namespace for the definition of user-defined literals.
inline namespace literals
{
}
}

#include "array_key.hpp"
#include "base_series_multiplier.hpp"
#include "binomial.hpp"
#include "cache_aligning_allocator.hpp"
#include "config.hpp"
#include "convert_to.hpp"
#include "debug_access.hpp"
#include "divisor.hpp"
#include "divisor_series.hpp"
#include "dynamic_aligning_allocator.hpp"
#include "exceptions.hpp"
#include "hash_set.hpp"
#include "init.hpp"
#include "invert.hpp"
#include "ipow_substitutable_series.hpp"
#include "is_cf.hpp"
#include "is_key.hpp"
#include "key_is_convertible.hpp"
#include "key_is_multipliable.hpp"
#include "kronecker_array.hpp"
#include "kronecker_monomial.hpp"
#include "lambdify.hpp"
#include "math.hpp"
#include "memory.hpp"
#include "monomial.hpp"
#include "mp_integer.hpp"
#include "mp_rational.hpp"
#include "poisson_series.hpp"
#include "polynomial.hpp"
#include "pow.hpp"
#include "power_series.hpp"
#include "print_coefficient.hpp"
#include "print_tex_coefficient.hpp"
#include "rational_function.hpp"
#include "real.hpp"
#include "real_trigonometric_kronecker_monomial.hpp"
#include "runtime_info.hpp"
#include "s11n.hpp"
#include "safe_cast.hpp"
#include "serialization.hpp"
#include "series.hpp"
#include "series_multiplier.hpp"
#include "settings.hpp"
#include "small_vector.hpp"
#include "static_vector.hpp"
#include "substitutable_series.hpp"
#include "symbol.hpp"
#include "symbol_set.hpp"
#include "t_substitutable_series.hpp"
#include "term.hpp"
#include "thread_barrier.hpp"
#include "thread_management.hpp"
#include "thread_pool.hpp"
#include "trigonometric_series.hpp"
#include "tuning.hpp"
#include "type_traits.hpp"

#endif
