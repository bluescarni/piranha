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

#ifndef PIRANHA_TYPE_TRAITS_HPP
#define PIRANHA_TYPE_TRAITS_HPP

#include <complex>
#include <cstdarg>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <ostream>
#include <tuple>
#include <type_traits>
#include <utility>

#include <mp++/concepts.hpp>
#include <mp++/detail/type_traits.hpp>

#include <piranha/config.hpp>
#include <piranha/detail/init.hpp>

namespace piranha
{

// Import a bunch of public type traits / utils from mp++.
template <typename T>
struct is_string_type : mppp::is_string_type<T> {
};

#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename T>
concept bool StringType = is_string_type<T>::value;
#endif

inline namespace impl
{

// Import a bunch of implementation-detail type traits / utils from mp++.
using mppp::conjunction;
using mppp::detected_t;
using mppp::disjunction;
using mppp::enable_if_t;
using mppp::is_detected;
using mppp::negation;
using mppp::nonesuch;

// This is like disjunction, but instead of providing true/false it provides
// the index of the first boolean class which evaluates to true. If no class
// evaluates to true, then it will return the number of boolean classes
// used in the instantiation.
template <std::size_t CurIdx, class...>
struct disjunction_idx_impl : std::integral_constant<std::size_t, 0u> {
};

template <std::size_t CurIdx, class B1>
struct disjunction_idx_impl<CurIdx, B1>
    : std::integral_constant<std::size_t, (B1::value != false) ? CurIdx : CurIdx + 1u> {
};

template <std::size_t CurIdx, class B1, class... Bn>
struct disjunction_idx_impl<CurIdx, B1, Bn...>
    : std::conditional<B1::value != false, std::integral_constant<std::size_t, CurIdx>,
                       disjunction_idx_impl<CurIdx + 1u, Bn...>>::type {
};

template <class... Bs>
struct disjunction_idx : disjunction_idx_impl<0u, Bs...> {
};

// Deferred conditional. It will check the value of the
// compile-time boolean constant C, and derive from T if
// C is true, from F otherwise.
template <typename C, typename T, typename F>
struct dcond : std::conditional<C::value != false, T, F>::type {
};

#if PIRANHA_CPLUSPLUS >= 201402L

// Handy bits available since C++14, we re-implement them below.
using std::decay_t;
using std::index_sequence;
using std::make_index_sequence;

#else

// std::index_sequence and std::make_index_sequence implementation for C++11. These are available
// in the std library in C++14. Implementation taken from:
// http://stackoverflow.com/questions/17424477/implementation-c14-make-integer-sequence
template <std::size_t... Ints>
struct index_sequence {
    using type = index_sequence;
    using value_type = std::size_t;
    static constexpr std::size_t size() noexcept
    {
        return sizeof...(Ints);
    }
};

template <class Sequence1, class Sequence2>
struct merge_and_renumber;

template <std::size_t... I1, std::size_t... I2>
struct merge_and_renumber<index_sequence<I1...>, index_sequence<I2...>>
    : index_sequence<I1..., (sizeof...(I1) + I2)...> {
};

template <std::size_t N>
struct make_index_sequence
    : merge_and_renumber<typename make_index_sequence<N / 2>::type, typename make_index_sequence<N - N / 2>::type> {
};

template <>
struct make_index_sequence<0> : index_sequence<> {
};

template <>
struct make_index_sequence<1> : index_sequence<0> {
};

template <typename T>
using decay_t = typename std::decay<T>::type;

#endif

// Tuple for_each(). Execute the functor f on each element of the input Tuple.
// https://isocpp.org/blog/2015/01/for-each-arg-eric-niebler
// https://www.reddit.com/r/cpp/comments/2tffv3/for_each_argumentsean_parent/
// https://www.reddit.com/r/cpp/comments/33b06v/for_each_in_tuple/
template <typename T, typename F, std::size_t... Is>
void apply_to_each_item(T &&t, const F &f, index_sequence<Is...>)
{
    (void)std::initializer_list<int>{0, (void(f(std::get<Is>(std::forward<T>(t)))), 0)...};
}

template <class Tuple, class F>
void tuple_for_each(Tuple &&t, const F &f)
{
    apply_to_each_item(std::forward<Tuple>(t), f,
                       make_index_sequence<std::tuple_size<typename std::decay<Tuple>::type>::value>{});
}

// Some handy aliases.
template <typename T>
using uncv_t = typename std::remove_cv<T>::type;

template <typename T>
using unref_t = typename std::remove_reference<T>::type;

template <typename T>
using uncvref_t = uncv_t<unref_t<T>>;

template <typename T>
using addlref_t = typename std::add_lvalue_reference<T>::type;

template <typename T>
using is_nonconst_rvalue_ref = conjunction<std::is_rvalue_reference<T>, negation<std::is_const<unref_t<T>>>>;
} // namespace impl

template <typename T, typename... Args>
using are_same = conjunction<std::is_same<T, Args>...>;

#if defined(PIRANHA_HAVE_CONCEPTS)

// Provide concept versions of a few C++ type traits.
template <typename T>
concept bool CppArithmetic = std::is_arithmetic<T>::value;

template <typename T>
concept bool CppIntegral = std::is_integral<T>::value;

template <typename T>
concept bool CppFloatingPoint = std::is_floating_point<T>::value;

template <typename T, typename... Args>
concept bool Constructible = std::is_constructible<T, Args...>::value;

template <typename T>
concept bool DefaultConstructible = std::is_default_constructible<T>::value;

template <typename From, typename To>
concept bool Convertible = std::is_convertible<From, To>::value;

template <typename T>
concept bool NonConst = !std::is_const<T>::value;

template <typename T, typename... Args>
concept bool Same = are_same<T, Args...>::value;

#endif

template <typename T>
using is_cpp_complex
    = disjunction<std::is_same<uncv_t<T>, std::complex<float>>, std::is_same<uncv_t<T>, std::complex<double>>,
                  std::is_same<uncv_t<T>, std::complex<long double>>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool CppComplex = is_cpp_complex<T>::value;

#endif

// Swappable type-trait/concept.

#if PIRANHA_CPLUSPLUS >= 201703L

// NOTE: this ends up behaving like std::is_swappable_with,
// rather than std::is_swappable, which invokes std::is_swappable_with
// after adding references. Throughout piranha, we adopt the convention
// that the user has to manually add the reference qualification to the
// argument types of a function whose availability is being checked,
// so we prefer the std::is_swappable_with behaviour here.
// http://en.cppreference.com/w/cpp/types/is_swappable
template <typename T, typename U = T>
struct is_swappable : std::is_swappable_with<T, U> {
};

#else

// Swappable implementation, mostly inspired from:
// https://stackoverflow.com/questions/26744589/what-is-a-proper-way-to-implement-is-swappable-to-test-for-the-swappable-concept
// The plan is to check if "using std::swap" + ADL leads to a callable
// swap() function. The complication is that std::swap() will be marked
// as available even for those argument types which do not support swapping because
// they are not move ctible/assignable (C++<17 does not SFINAE on std::swap()).
// Therefore, the strategy is to avoid "using std::swap" if std::swap() is not available,
// and go only for the ADL detection.

// The using std::swap + ADL detection.
namespace using_std_adl_swap
{

using std::swap;

// NOTE: we need to make sure swapping is well defined also with inverted arguments.
template <typename T, typename U>
using swap1_t = decltype(swap(std::declval<T>(), std::declval<U>()));

template <typename T, typename U>
using swap2_t = decltype(swap(std::declval<U>(), std::declval<T>()));

template <typename T, typename U>
using detected = conjunction<is_detected<swap1_t, T, U>, is_detected<swap2_t, T, U>>;
} // namespace using_std_adl_swap

// Pure ADL-based swap detection.
namespace adl_swap
{

template <typename T, typename U>
using swap1_t = decltype(swap(std::declval<T>(), std::declval<U>()));

template <typename T, typename U>
using swap2_t = decltype(swap(std::declval<U>(), std::declval<T>()));

template <typename T, typename U>
using detected = conjunction<is_detected<swap1_t, T, U>, is_detected<swap2_t, T, U>>;
} // namespace adl_swap

inline namespace impl
{

// Detect if std::swap() can be called on types T and U. For std::swap() to be viable we need:
// - to be able to call it, meaning that T and U must be nonconst lvalue refs to the same type,
// - T/U to be move ctible and move assignable.
template <typename T, typename U>
using std_swap_t = decltype(std::swap(std::declval<T>(), std::declval<U>()));

template <typename T, typename U>
using std_swap_viable = conjunction<
    is_detected<std_swap_t, T, U>,
    // NOTE: we need to distinguish is T is an array or not, when checking
    // for move operations.
    dcond<std::is_array<unref_t<T>>, std::is_move_constructible<typename std::remove_extent<unref_t<T>>::type>,
          std::is_move_constructible<unref_t<T>>>,
    dcond<std::is_array<unref_t<T>>, std::is_move_assignable<typename std::remove_extent<unref_t<T>>::type>,
          std::is_move_assignable<unref_t<T>>>>;
} // namespace impl

// Two possibilities:
// - std::swap() is available for the types T and U, check the availability of "using std::swap" + ADL;
// - std::swap() is not available for the types T and U, check the availability of pure ADL-based swapping.
template <typename T, typename U = T>
struct is_swappable : std::conditional<std_swap_viable<T, U>::value, using_std_adl_swap::detected<T, U>,
                                       adl_swap::detected<T, U>>::type {
};

#endif

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U = T>
concept bool Swappable = is_swappable<T, U>::value;

#endif

inline namespace impl
{

// The type resulting from the addition of T and U.
template <typename T, typename U>
using add_t = decltype(std::declval<T>() + std::declval<U>());
} // namespace impl

// Addable type trait.
template <typename T, typename U = T>
struct is_addable : is_detected<add_t, T, U> {
};

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U = T>
concept bool Addable = is_addable<T, U>::value;

#endif

/// In-place addable type trait.
/**
 * This type trait will be \p true if objects of type \p U can be added in-place to objects of type \p T,
 * \p false otherwise. The operator will be tested in the form:
 * @code
 * operator+=(T &, const U &)
 * @endcode
 */
template <typename T, typename U = T>
class is_addable_in_place
{
    template <typename T1, typename U1>
    using ip_add_t = decltype(std::declval<T1 &>() += std::declval<const U1 &>());
    static const bool implementation_defined = is_detected<ip_add_t, T, U>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T, typename U>
const bool is_addable_in_place<T, U>::value;

inline namespace impl
{

#if defined(PIRANHA_CLANG_HAS_WDEPRECATED_INCREMENT_BOOL)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-increment-bool"
#endif

#if defined(PIRANHA_CLANG_HAS_WINCREMENT_BOOL)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincrement-bool"
#endif

template <typename T>
using preinc_t = decltype(++std::declval<T>());

#if defined(PIRANHA_CLANG_HAS_WDEPRECATED_INCREMENT_BOOL) || defined(PIRANHA_CLANG_HAS_WINCREMENT_BOOL)
#pragma clang diagnostic pop
#endif
} // namespace impl

// Pre-incrementable type-trait.
template <typename T>
using is_preincrementable = is_detected<preinc_t, T>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool Preincrementable = is_preincrementable<T>::value;

#endif

inline namespace impl
{

#if defined(PIRANHA_CLANG_HAS_WDEPRECATED_INCREMENT_BOOL)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-increment-bool"
#endif

#if defined(PIRANHA_CLANG_HAS_WINCREMENT_BOOL)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincrement-bool"
#endif

template <typename T>
using postinc_t = decltype(std::declval<T>()++);

#if defined(PIRANHA_CLANG_HAS_WDEPRECATED_INCREMENT_BOOL) || defined(PIRANHA_CLANG_HAS_WINCREMENT_BOOL)
#pragma clang diagnostic pop
#endif
} // namespace impl

// Post-incrementable type-trait.
template <typename T>
using is_postincrementable = is_detected<postinc_t, T>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool Postincrementable = is_postincrementable<T>::value;

#endif

/// Subtractable type trait.
/**
 * This type trait will be \p true if objects of type \p U can be subtracted from objects of type \p T using the binary
 * subtraction operator, \p false otherwise. The operator will be tested in the form:
 * @code
 * operator-(const T &, const U &)
 * @endcode
 */
template <typename T, typename U = T>
class is_subtractable
{
    template <typename T1, typename U1>
    using sub_t = decltype(std::declval<const T1 &>() - std::declval<const U1 &>());
    static const bool implementation_defined = is_detected<sub_t, T, U>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T, typename U>
const bool is_subtractable<T, U>::value;

/// In-place subtractable type trait.
/**
 * This type trait will be \p true if objects of type \p U can be subtracted in-place from objects of type \p T,
 * \p false otherwise. The operator will be tested in the form:
 * @code
 * operator-=(T &, const U &)
 * @endcode
 */
template <typename T, typename U = T>
class is_subtractable_in_place
{
    template <typename T1, typename U1>
    using ip_sub_t = decltype(std::declval<T1 &>() -= std::declval<const U1 &>());
    static const bool implementation_defined = is_detected<ip_sub_t, T, U>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T, typename U>
const bool is_subtractable_in_place<T, U>::value;

inline namespace impl
{

// Type resulting from the multiplication of T and U.
template <typename T, typename U>
using mul_t = decltype(std::declval<const T &>() * std::declval<const U &>());
} // namespace impl

/// Multipliable type trait.
/**
 * This type trait will be \p true if objects of type \p T can be multiplied by objects of type \p U using the binary
 * multiplication operator, \p false otherwise. The operator will be tested in the form:
 * @code
 * operator*(const T &, const U &)
 * @endcode
 */
template <typename T, typename U = T>
class is_multipliable
{
    static const bool implementation_defined = is_detected<mul_t, T, U>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T, typename U>
const bool is_multipliable<T, U>::value;

/// In-place multipliable type trait.
/**
 * This type trait will be \p true if objects of type \p T can be multiplied in-place by objects of type \p U,
 * \p false otherwise. The operator will be tested in the form:
 * @code
 * operator+=(T &, const U &)
 * @endcode
 */
template <typename T, typename U = T>
class is_multipliable_in_place
{
    template <typename T1, typename U1>
    using ip_mul_t = decltype(std::declval<T1 &>() *= std::declval<const U1 &>());
    static const bool implementation_defined = is_detected<ip_mul_t, T, U>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T, typename U>
const bool is_multipliable_in_place<T, U>::value;

/// Divisible type trait.
/**
 * This type trait will be \p true if objects of type \p T can be divided by objects of type \p U using the binary
 * division operator, \p false otherwise. The operator will be tested in the form:
 * @code
 * operator/(const T &, const U &)
 * @endcode
 */
template <typename T, typename U = T>
class is_divisible
{
    template <typename T1, typename U1>
    using div_t = decltype(std::declval<const T1 &>() / std::declval<const U1 &>());
    static const bool implementation_defined = is_detected<div_t, T, U>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T, typename U>
const bool is_divisible<T, U>::value;

/// In-place divisible type trait.
/**
 * This type trait will be \p true if objects of type \p T can be divided in-place by objects of type \p U,
 * \p false otherwise. The operator will be tested in the form:
 * @code
 * operator/=(T &, const U &)
 * @endcode
 */
template <typename T, typename U = T>
class is_divisible_in_place
{
    template <typename T1, typename U1>
    using ip_div_t = decltype(std::declval<T1 &>() /= std::declval<const U1 &>());
    static const bool implementation_defined = is_detected<ip_div_t, T, U>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T, typename U>
const bool is_divisible_in_place<T, U>::value;

/// Left-shift type trait.
/**
 * This type trait will be \p true if objects of type \p T can be left-shifted by objects of type \p U using the binary
 * left-shift operator, \p false otherwise. The operator will be tested in the form:
 * @code
 * operator<<(const T &, const U &)
 * @endcode
 */
template <typename T, typename U = T>
class has_left_shift
{
    template <typename T1, typename U1>
    using ls_t = decltype(std::declval<const T1 &>() << std::declval<const U1 &>());
    static const bool implementation_defined = is_detected<ls_t, T, U>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T, typename U>
const bool has_left_shift<T, U>::value;

/// Right-shift type trait.
/**
 * This type trait will be \p true if objects of type \p T can be right-shifted by objects of type \p U using the binary
 * right-shift operator, \p false otherwise. The operator will be tested in the form:
 * @code
 * operator>>(const T &, const U &)
 * @endcode
 */
template <typename T, typename U = T>
class has_right_shift
{
    template <typename T1, typename U1>
    using rs_t = decltype(std::declval<const T1 &>() >> std::declval<const U1 &>());
    static const bool implementation_defined = is_detected<rs_t, T, U>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T, typename U>
const bool has_right_shift<T, U>::value;

/// In-place left-shift type trait.
/**
 * This type trait will be \p true if objects of type \p T can be left-shifted in-place by objects of type \p U,
 * \p false otherwise. The operator will be tested in the form:
 * @code
 * operator<<=(T &, const U &)
 * @endcode
 */
template <typename T, typename U = T>
class has_left_shift_in_place
{
    template <typename T1, typename U1>
    using ls_t = decltype(std::declval<T1 &>() <<= std::declval<const U1 &>());
    static const bool implementation_defined = is_detected<ls_t, T, U>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T, typename U>
const bool has_left_shift_in_place<T, U>::value;

/// In-place right-shift type trait.
/**
 * This type trait will be \p true if objects of type \p T can be right-shifted in-place by objects of type \p U,
 * \p false otherwise. The operator will be tested in the form:
 * @code
 * operator>>=(T &, const U &)
 * @endcode
 */
template <typename T, typename U = T>
class has_right_shift_in_place
{
    template <typename T1, typename U1>
    using rs_t = decltype(std::declval<T1 &>() >>= std::declval<const U1 &>());
    static const bool implementation_defined = is_detected<rs_t, T, U>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T, typename U>
const bool has_right_shift_in_place<T, U>::value;

inline namespace impl
{

// Return types for the equality and inequality operators.
template <typename T, typename U>
using eq_t = decltype(std::declval<T>() == std::declval<U>());

template <typename T, typename U>
using ineq_t = decltype(std::declval<T>() != std::declval<U>());
} // namespace impl

// Equality-comparable type trait.
// NOTE: if the expressions above for eq/ineq return a type which is not bool,
// the decltype() will also check that the returned type is destructible.
template <typename T, typename U = T>
struct is_equality_comparable : conjunction<std::is_convertible<detected_t<eq_t, T, U>, bool>,
                                            std::is_convertible<detected_t<ineq_t, T, U>, bool>> {
};

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U = T>
concept bool EqualityComparable = is_equality_comparable<T, U>::value;

#endif

/// Less-than-comparable type trait.
/**
 * This type trait is \p true if instances of type \p T can be compared to instances of
 * type \p U using the less-than operator. The operator must be non-mutable (i.e., implemented using pass-by-value or
 * const references) and must return a type implicitly convertible to \p bool.
 */
template <typename T, typename U = T>
class is_less_than_comparable
{
    template <typename T1, typename U1>
    using lt_t = decltype(std::declval<const T1 &>() < std::declval<const U1 &>());
    static const bool implementation_defined = std::is_convertible<detected_t<lt_t, T, U>, bool>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

// Static init.
template <typename T, typename U>
const bool is_less_than_comparable<T, U>::value;

/// Greater-than-comparable type trait.
/**
 * This type trait is \p true if instances of type \p T can be compared to instances of
 * type \p U using the greater-than operator. The operator must be non-mutable (i.e., implemented using pass-by-value or
 * const
 * references) and must return a type implicitly convertible to \p bool.
 */
template <typename T, typename U = T>
class is_greater_than_comparable
{
    template <typename T1, typename U1>
    using gt_t = decltype(std::declval<const T1 &>() > std::declval<const U1 &>());
    static const bool implementation_defined = std::is_convertible<detected_t<gt_t, T, U>, bool>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

// Static init.
template <typename T, typename U>
const bool is_greater_than_comparable<T, U>::value;

/// Enable \p noexcept checks.
/**
 * This type trait, which can be specialised with the <tt>std::enable_if</tt> mechanism, enables or disables
 * \p noexcept checks in the piranha::is_container_element type trait. This type trait should be used
 * only with legacy pre-C++11 classes that do not support \p noexcept: by specialising this trait to
 * \p false, the piranha::is_container_element trait will disable \p noexcept checks and it will thus be
 * possible to use \p noexcept unaware classes as container elements and series coefficients.
 */
template <typename T, typename = void>
struct enable_noexcept_checks {
    /// Default value of the type trait.
    static const bool value = true;
};

// Static init.
template <typename T, typename Enable>
const bool enable_noexcept_checks<T, Enable>::value;

/// Type trait for well-behaved container elements.
/**
 * The type trait will be true if all these conditions hold:
 *
 * - \p T is default-constructible,
 * - \p T is copy-constructible,
 * - \p T has nothrow move semantics,
 * - \p T is nothrow-destructible.
 *
 * If the piranha::enable_noexcept_checks trait is \p false for \p T, then the nothrow checks will be discarded.
 */
template <typename T>
struct is_container_element {
private:
    // NOTE: here we do not require copy assignability as in our containers we always implement
    // copy-assign as copy-construct + move for exception safety reasons.
    static const bool implementation_defined
        = conjunction<std::is_default_constructible<T>, std::is_copy_constructible<T>,
                      disjunction<negation<enable_noexcept_checks<T>>,
                                  conjunction<std::is_nothrow_destructible<T>,
#if defined(PIRANHA_COMPILER_IS_INTEL)
                                              // The Intel compiler has troubles with the noexcept versions of these two
                                              // type traits.
                                              std::is_move_constructible<T>, std::is_move_assignable<T>
#else

                                              std::is_nothrow_move_constructible<T>, std::is_nothrow_move_assignable<T>
#endif
                                              >>>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T>
const bool is_container_element<T>::value;

/// Type trait for classes that can be output-streamed.
/**
 * This type trait will be \p true if instances of type \p T can be directed to
 * instances of \p std::ostream via the insertion operator. The operator must have a signature
 * compatible with
 * @code
 * std::ostream &operator<<(std::ostream &, const T &)
 * @endcode
 */
template <typename T>
class is_ostreamable
{
    template <typename T1>
    using ostreamable_t = decltype(std::declval<std::ostream &>() << std::declval<const T1 &>());
    static const bool implementation_defined = std::is_same<detected_t<ostreamable_t, T>, std::ostream &>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T>
const bool is_ostreamable<T>::value;

/// Function object type trait.
/**
 * This type trait will be true if \p T is a function object returning \p ReturnType and taking
 * \p Args as arguments. That is, the type trait will be \p true if the following conditions are met:
 * - \p T is a class,
 * - \p T is equipped with a call operator returning \p ReturnType and taking \p Args as arguments.
 *
 * \p T can be const qualified (in which case the call operator must be also const in order for the type trait to be
 * satisfied).
 */
// NOTE: the standard includes among function objects also function pointers. It would be nice in the future
// to exactly map the standard definition of callable into this type trait. See, e.g.:
// http://stackoverflow.com/questions/19278424/what-is-a-callable-object-in-c
template <typename T, typename ReturnType, typename... Args>
class is_function_object
{
    template <typename T1, typename... Args1>
    using ret_t = decltype(std::declval<T1 &>()(std::declval<Args1>()...));
    static const bool implementation_defined
        = conjunction<std::is_class<T>, std::is_same<detected_t<ret_t, T, Args...>, ReturnType>>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T, typename ReturnType, typename... Args>
const bool is_function_object<T, ReturnType, Args...>::value;

/// Type trait to detect hash function objects.
/**
 * \p T is a hash function object for \p U if the following requirements are met:
 * - \p T is a function object with const call operator accepting as input const \p U and returning \p std::size_t,
 * - \p T satisfies piranha::is_container_element.
 */
template <typename T, typename U>
class is_hash_function_object
{
    // NOTE: use addlref_t to avoid forming a ref to void.
    static const bool implementation_defined
        = conjunction<is_function_object<const T, std::size_t, addlref_t<const U>>, is_container_element<T>>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T, typename U>
const bool is_hash_function_object<T, U>::value;

/// Type trait to detect equality function objects.
/**
 * \p T is an equality function object for \p U if the following requirements are met:
 * - \p T is a function object with const call operator accepting as input two const \p U and returning \p bool,
 * - \p T satisfies piranha::is_container_element.
 */
template <typename T, typename U>
class is_equality_function_object
{
    static const bool implementation_defined
        = conjunction<is_function_object<const T, bool, addlref_t<const U>, addlref_t<const U>>,
                      is_container_element<T>>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T, typename U>
const bool is_equality_function_object<T, U>::value;

/// Hashable type trait.
/**
 * This type trait will be \p true if \p T is hashable (after the removal of cv/ref qualifiers), \p false otherwise.
 *
 * A type \p T is hashable when supplied with a specialisation of \p std::hash which satisfies
 * piranha::is_hash_function_object.
 *
 * Note that depending on the implementation of the default \p std::hash class, using this type trait with
 * a type which does not provide a specialisation for \p std::hash could result in a compilation error
 * (e.g., if the unspecialised \p std::hash includes a \p false \p static_assert).
 */
// NOTE: when we remove the is_container_element check we might need to make sure that std::hash is def ctible,
// depending on how we use it. Check.
template <typename T>
class is_hashable
{
    static const bool implementation_defined = is_hash_function_object<std::hash<uncvref_t<T>>, T>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T>
const bool is_hashable<T>::value;
} // namespace piranha

/// Macro for static type trait checks.
/**
 * This macro will check via a \p static_assert that the template type trait \p tt provides a \p true \p value.
 * The variadic arguments are interpreted as the template arguments of \p tt.
 */
#define PIRANHA_TT_CHECK(tt, ...)                                                                                      \
    static_assert(tt<__VA_ARGS__>::value, "type trait check failure -> " #tt "<" #__VA_ARGS__ ">")

namespace piranha
{

inline namespace impl
{

template <typename T, typename... Args>
struct min_int_impl {
    using next = typename min_int_impl<Args...>::type;
    static_assert((std::is_unsigned<T>::value && std::is_unsigned<next>::value && std::is_integral<next>::value)
                      || (std::is_signed<T>::value && std::is_signed<next>::value && std::is_integral<next>::value),
                  "The type trait's arguments must all be (un)signed integers.");
    using type = typename std::conditional<(std::numeric_limits<T>::max() < std::numeric_limits<next>::max()
                                            && (std::is_unsigned<T>::value
                                                || std::numeric_limits<T>::min() > std::numeric_limits<next>::min())),
                                           T, next>::type;
};

template <typename T>
struct min_int_impl<T> {
    static_assert(std::is_integral<T>::value, "The type trait's arguments must all be (un)signed integers.");
    using type = T;
};

template <typename T, typename... Args>
struct max_int_impl {
    using next = typename max_int_impl<Args...>::type;
    static_assert((std::is_unsigned<T>::value && std::is_unsigned<next>::value && std::is_integral<next>::value)
                      || (std::is_signed<T>::value && std::is_signed<next>::value && std::is_integral<next>::value),
                  "The type trait's arguments must all be (un)signed integers.");
    using type = typename std::conditional<(std::numeric_limits<T>::max() > std::numeric_limits<next>::max()
                                            && (std::is_unsigned<T>::value
                                                || std::numeric_limits<T>::min() < std::numeric_limits<next>::min())),
                                           T, next>::type;
};

template <typename T>
struct max_int_impl<T> {
    static_assert(std::is_integral<T>::value, "The type trait's arguments must all be (un)signed integers.");
    using type = T;
};
} // namespace impl

/// Detect narrowest integer type
/**
 * This type alias requires \p T and \p Args (if any) to be all signed or unsigned integer types.
 * It will be defined as the input type with the narrowest numerical range.
 */
template <typename T, typename... Args>
using min_int = typename min_int_impl<T, Args...>::type;

/// Detect widest integer type
/**
 * This type alias requires \p T and \p Args (if any) to be all signed or unsigned integer types.
 * It will be defined as the input type with the widest numerical range.
 */
template <typename T, typename... Args>
using max_int = typename max_int_impl<T, Args...>::type;

inline namespace impl
{

// Helpers for the detection of the typedefs in std::iterator_traits.
// Use a macro (yuck) to reduce typing.
#define PIRANHA_DECLARE_IT_TRAITS_TYPE(type)                                                                           \
    template <typename T>                                                                                              \
    using it_traits_##type = typename std::iterator_traits<T>::type;

PIRANHA_DECLARE_IT_TRAITS_TYPE(difference_type)
PIRANHA_DECLARE_IT_TRAITS_TYPE(value_type)
PIRANHA_DECLARE_IT_TRAITS_TYPE(pointer)
PIRANHA_DECLARE_IT_TRAITS_TYPE(reference)
PIRANHA_DECLARE_IT_TRAITS_TYPE(iterator_category)

#undef PIRANHA_DECLARE_IT_TRAITS_TYPE

// All standard iterator tags packed in a tuple.
using it_tags_tuple = std::tuple<std::input_iterator_tag, std::output_iterator_tag, std::forward_iterator_tag,
                                 std::bidirectional_iterator_tag, std::random_access_iterator_tag>;

// Detect the availability of std::iterator_traits on type It.
template <typename It>
using has_iterator_traits = conjunction<is_detected<it_traits_reference, It>, is_detected<it_traits_value_type, It>,
                                        is_detected<it_traits_pointer, It>, is_detected<it_traits_difference_type, It>,
                                        is_detected<it_traits_iterator_category, It>>;

// Type resulting from the dereferencing operation.
template <typename T>
using deref_t = decltype(*std::declval<T>());

// TMP to check if a type T derives from at least one type in the tuple.
// NOTE: default empty for hard error.
template <typename, typename>
struct base_type_in_tuple {
};

template <typename T, typename... Args>
struct base_type_in_tuple<T, std::tuple<Args...>> : disjunction<std::is_base_of<Args, T>...> {
    static_assert(sizeof...(Args) > 0u, "Invalid parameter pack.");
};
} // namespace impl

// Detect iterator types.
template <typename T>
using is_iterator = conjunction<
    // Copy constr/ass, destructible.
    std::is_copy_constructible<T>, std::is_copy_assignable<T>, std::is_destructible<T>,
    // Lvalue swappable.
    is_swappable<addlref_t<T>>,
    // Valid std::iterator_traits.
    has_iterator_traits<T>,
    // Lvalue dereferenceable.
    is_detected<deref_t, addlref_t<T>>,
    // Lvalue preincrementable, returning T &.
    std::is_same<detected_t<preinc_t, addlref_t<T>>, addlref_t<T>>,
    // Add a check that the iterator category is one of the standard ones
    // or at least derives from it. This allows Boost.iterator iterators
    // (which have their own tags) to satisfy this type trait.
    base_type_in_tuple<detected_t<it_traits_iterator_category, T>, it_tags_tuple>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool Iterator = is_iterator<T>::value;

#endif

inline namespace impl
{

// The purpose of these bits is to check whether U correctly implements the arrow operator.
// A correct implementation will return a pointer, after potentially calling
// the operator recursively as many times as needed. See:
// http://stackoverflow.com/questions/10677804/how-arrow-operator-overloading-works-internally-in-c

// The expression x->m is either:
// - equivalent to (*x).m, if x is a pointer, or
// - equivalent to (x.operator->())->m otherwise. That is, if operator->()
//   returns a pointer, then the member "m" of the pointee is returned,
//   otherwise there's a recursion to call again operator->() on the returned
//   value.
// This type trait will extract the final pointer type whose pointee type
// contains the "m" member.
template <typename T, typename = void>
struct arrow_operator_type {
};

// Handy alias.
template <typename T>
using arrow_operator_t = typename arrow_operator_type<T>::type;

// If T is a pointer (after ref removal), we don't need to do anything: the final pointer type
// will be T itself (unreffed).
template <typename T>
struct arrow_operator_type<T, enable_if_t<std::is_pointer<unref_t<T>>::value>> {
    using type = unref_t<T>;
};

// Type resulting from the invocation of the member function operator->().
template <typename T>
using mem_arrow_op_t = decltype(std::declval<T>().operator->());

// T is not a pointer, it is a class whose operator->() returns some type U.
// We call again arrow_operator_type on that U: if that leads eventually to a pointer
// (possibly by calling this specialisation recursively) then we define that pointer
// as the internal "type" member, otherwise we will SFINAE out.
template <typename T>
struct arrow_operator_type<T, enable_if_t<is_detected<arrow_operator_t, mem_arrow_op_t<T>>::value>> {
    using type = arrow_operator_t<mem_arrow_op_t<T>>;
};

// *it++ expression, used below.
template <typename T>
using it_inc_deref_t = decltype(*std::declval<T>()++);

// The type resulting from dereferencing an lvalue of T,
// or nonesuch. Shortcut useful below.
template <typename T>
using det_deref_t = detected_t<deref_t, addlref_t<T>>;
} // namespace impl

// Input iterator type trait.
template <typename T>
using is_input_iterator = conjunction<
    // Must be a class or pointer.
    disjunction<std::is_class<T>, std::is_pointer<T>>,
    // Base iterator requirements.
    is_iterator<T>,
    // Lvalue equality-comparable (just test the const-const variant).
    is_equality_comparable<addlref_t<const T>>,
    // Quoting the standard:
    // """
    // For every iterator type X for which equality is defined, there is a corresponding signed integer type called the
    // difference type of the iterator.
    // """
    // http://eel.is/c++draft/iterator.requirements#general-1
    // The equality comparable requirement appears in the input iterator requirements,
    // and it is then inherited by all the other iterator types (i.e., forward iterator,
    // bidir iterator, etc.).
    std::is_integral<detected_t<it_traits_difference_type, T>>,
    std::is_signed<detected_t<it_traits_difference_type, T>>,
    // *it returns it_traits::reference_type, both in mutable and const forms.
    // NOTE: it_traits::reference_type is never nonesuch, we tested its availability
    // in is_iterator.
    std::is_same<det_deref_t<T>, detected_t<it_traits_reference, T>>,
    std::is_same<det_deref_t<const T>, detected_t<it_traits_reference, T>>,
    // *it is convertible to it_traits::value_type.
    // NOTE: as above, it_traits::value_type does exist.
    std::is_convertible<det_deref_t<T>, detected_t<it_traits_value_type, T>>,
    std::is_convertible<det_deref_t<const T>, detected_t<it_traits_value_type, T>>,
    // it->m must be the same as (*it).m. What we test here is that the pointee type of the pointer type
    // yielded eventually by the arrow operator is the same as *it, but minus references: the arrow operator
    // always returns a pointer, but *it could return a new object (e.g., a transform iterator).
    // NOTE: we already verified earlier that T is dereferenceable, so deref_t will not be nonesuch.
    // NOTE: make this check conditional on whether the ref type is a class or not. If it's not a class,
    // no expression such as (*it).m is possible, and apparently some input iterators which are not
    // expected to point to classes do *not* implement the arrow operator as a consequence (e.g.,
    // see std::istreambuf_iterator).
    dcond<std::is_class<unref_t<det_deref_t<T>>>,
          conjunction<
              std::is_same<unref_t<det_deref_t<detected_t<arrow_operator_t, addlref_t<T>>>>, unref_t<det_deref_t<T>>>,
              std::is_same<unref_t<det_deref_t<detected_t<arrow_operator_t, addlref_t<const T>>>>,
                           unref_t<det_deref_t<const T>>>>,
          std::true_type>,
    // ++it returns &it. Only non-const needed.
    std::is_same<detected_t<preinc_t, addlref_t<T>>, addlref_t<T>>,
    // it is post-incrementable. Only non-const needed.
    is_postincrementable<addlref_t<T>>,
    // *it++ is convertible to the value type. Only non-const needed.
    std::is_convertible<detected_t<it_inc_deref_t, addlref_t<T>>, detected_t<it_traits_value_type, T>>,
    // Check that the iterator category of T derives from the standard
    // input iterator tag. This accommodates the Boost iterators as well, who have
    // custom categories derived from the standard ones.
    std::is_base_of<std::input_iterator_tag, detected_t<it_traits_iterator_category, T>>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool InputIterator = is_input_iterator<T>::value;

#endif

inline namespace impl
{

template <typename T, typename U>
using out_iter_assign_t = decltype(*std::declval<T>() = std::declval<U>());

template <typename T, typename U>
using out_iter_pia_t = decltype(*std::declval<T>()++ = std::declval<U>());
} // namespace impl

// Output iterator type trait.
template <typename T, typename U>
using is_output_iterator = conjunction<
    // Must be a class or pointer.
    disjunction<std::is_class<T>, std::is_pointer<T>>,
    // Must be an iterator.
    is_iterator<T>,
    // *r = o must be valid (r is an lvalue T, o is an U).
    is_detected<out_iter_assign_t, addlref_t<T>, U>,
    // Lvalue pre-incremetable and returning lref to T.
    std::is_same<detected_t<preinc_t, addlref_t<T>>, addlref_t<T>>,
    // Lvalue post-incrementable and returning convertible to const T &.
    std::is_convertible<detected_t<postinc_t, addlref_t<T>>, addlref_t<const T>>,
    // Can post-increment-assign on lvalue.
    is_detected<out_iter_pia_t, addlref_t<T>, U>,
    // NOTE: if T is an input iterator, its category tag must *not* derive from std::output_iterator_tag
    // (the fact that it is an input iterator takes the precedence in the category tagging).
    // If T is a pure output iterator, its category tag must derive from std::output_iterator_tag.
    dcond<is_input_iterator<T>,
          negation<std::is_base_of<std::output_iterator_tag, detected_t<it_traits_iterator_category, T>>>,
          std::is_base_of<std::output_iterator_tag, detected_t<it_traits_iterator_category, T>>>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U>
concept bool OutputIterator = is_output_iterator<T, U>::value;

#endif

template <typename T>
using is_forward_iterator = conjunction<
    // Must be an input iterator.
    // NOTE: the pointer or class requirement is already in the input iterator.
    is_input_iterator<T>,
    // Must be def-ctible.
    std::is_default_constructible<T>,
    // If it is a mutable (i.e., output) iterator, it_traits::reference
    // must be a reference to the value type. Otherwise, it_traits::reference
    // must be a reference to const value type.
    // NOTE: we do not do the is_output_iterator check here, as we don't really know
    // what to put as a second template parameter.
    // NOTE: if the ref type is a mutable reference, then a forward iterator satisfies
    // also all the reqs of an output iterator.
    disjunction<std::is_same<detected_t<it_traits_reference, T>, addlref_t<detected_t<it_traits_value_type, T>>>,
                std::is_same<detected_t<it_traits_reference, T>, addlref_t<const detected_t<it_traits_value_type, T>>>>,
    // Post-incrementable lvalue returns convertible to const T &.
    std::is_convertible<detected_t<postinc_t, addlref_t<T>>, addlref_t<const T>>,
    // *r++ returns it_traits::reference.
    std::is_same<detected_t<it_inc_deref_t, addlref_t<T>>, detected_t<it_traits_reference, T>>,
    // Category check.
    std::is_base_of<std::forward_iterator_tag, detected_t<it_traits_iterator_category, T>>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool ForwardIterator = is_forward_iterator<T>::value;

#endif

template <typename T>
using is_mutable_forward_iterator
    = conjunction<is_forward_iterator<T>,
                  std::is_same<detected_t<it_traits_reference, T>, addlref_t<detected_t<it_traits_value_type, T>>>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool MutableForwardIterator = is_mutable_forward_iterator<T>::value;

#endif

inline namespace impl
{

template <typename T>
constexpr T safe_abs_sint_impl(T cur_p = T(1), T cur_n = T(-1))
{
    return (cur_p > std::numeric_limits<T>::max() / T(2) || cur_n < std::numeric_limits<T>::min() / T(2))
               ? cur_p
               : safe_abs_sint_impl(static_cast<T>(cur_p * 2), static_cast<T>(cur_n * 2));
}

// Determine, for the signed integer T, a value n, power of 2, such that it is safe to take -n.
template <typename T>
struct safe_abs_sint {
    static_assert(std::is_integral<T>::value && std::is_signed<T>::value, "T must be a signed integral type.");
    static const T value = safe_abs_sint_impl<T>();
};

template <typename T>
const T safe_abs_sint<T>::value;

// A simple true type-trait that can be used inside enable_if with T a decltype() expression
// subject to SFINAE. It is similar to the proposed void_t type (in C++14, maybe?).
template <typename T>
struct true_tt {
    static const bool value = true;
};

template <typename T>
const bool true_tt<T>::value;
} // namespace impl

// Ranges.
namespace begin_adl
{

using std::begin;

template <typename T>
using type = decltype(begin(std::declval<T>()));
} // namespace begin_adl

namespace end_adl
{

using std::end;

template <typename T>
using type = decltype(end(std::declval<T>()));
} // namespace end_adl

// Input range.
template <typename T>
using is_input_range = conjunction<is_input_iterator<detected_t<begin_adl::type, T>>,
                                   std::is_same<detected_t<begin_adl::type, T>, detected_t<end_adl::type, T>>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool InputRange = is_input_range<T>::value;

#endif

// Forward range.
template <typename T>
using is_forward_range = conjunction<is_forward_iterator<detected_t<begin_adl::type, T>>,
                                     std::is_same<detected_t<begin_adl::type, T>, detected_t<end_adl::type, T>>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool ForwardRange = is_forward_range<T>::value;

#endif

// Mutable forward range.
template <typename T>
using is_mutable_forward_range
    = conjunction<is_mutable_forward_iterator<detected_t<begin_adl::type, T>>,
                  std::is_same<detected_t<begin_adl::type, T>, detected_t<end_adl::type, T>>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool MutableForwardRange = is_mutable_forward_range<T>::value;

#endif

// Detect if type can be returned from a function.
// NOTE: constructability implies destructability:
// https://cplusplus.github.io/LWG/issue2116
// NOTE: checking for void should be enough, cv void
// as return type of a function is same as void.
template <typename T>
using is_returnable = disjunction<std::is_same<T, void>, std::is_copy_constructible<T>, std::is_move_constructible<T>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
concept bool Returnable = is_returnable<T>::value;

#endif

/// Detect if zero is a multiplicative absorber.
/**
 * This type trait, defaulting to \p true, establishes if the zero element of the type \p T is a multiplicative
 * absorber. That is, if for any value \f$ x \f$ of type \p T the relation
 * \f[
 * x \times 0 = 0
 * \f]
 * holds, then this type trait should be \p true.
 *
 * This type trait requires \p T to satsify piranha::is_multipliable, after removal of cv/reference qualifiers.
 * This type trait can be specialised via \p std::enable_if.
 */
template <typename T, typename = void>
class zero_is_absorbing
{
    PIRANHA_TT_CHECK(is_multipliable, uncvref_t<T>);

public:
    /// Value of the type trait.
    static const bool value = true;
};

template <typename T, typename Enable>
const bool zero_is_absorbing<T, Enable>::value;

inline namespace impl
{

// NOTE: no conj/disj as we are not using ::value members from numeric_limits.
template <typename T>
using fp_zero_is_absorbing_enabler = enable_if_t<std::is_floating_point<uncvref_t<T>>::value
                                                 && (std::numeric_limits<uncvref_t<T>>::has_quiet_NaN
                                                     || std::numeric_limits<uncvref_t<T>>::has_signaling_NaN)>;
} // namespace impl

/// Specialisation of piranha::zero_is_absorbing for floating-point types.
/**
 * \note
 * This specialisation is enabled if \p T, after the removal of cv/reference qualifiers, is a C++ floating-point type
 * supporting NaN.
 *
 * In the presence of NaN, a floating-point zero is not the multiplicative absorbing element, and thus this
 * specialisation will be unconditionally \p false if activated.
 */
template <typename T>
class zero_is_absorbing<T, fp_zero_is_absorbing_enabler<T>>
{
    PIRANHA_TT_CHECK(is_multipliable, uncvref_t<T>);

public:
    /// Value of the type trait.
    static const bool value = false;
};

template <typename T>
const bool zero_is_absorbing<T, fp_zero_is_absorbing_enabler<T>>::value;
} // namespace piranha

#endif
