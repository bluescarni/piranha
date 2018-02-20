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

#include <mp++/detail/type_traits.hpp>

#include <piranha/config.hpp>
#include <piranha/detail/init.hpp>

namespace piranha
{

inline namespace impl
{

// Import a bunch of type traits / utils from mp++.
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
}

// Detect C++ FP complex types, in a similar way to std::is_floating_point.
template <typename T>
using is_cpp_complex
    = disjunction<std::is_same<uncv_t<T>, std::complex<float>>, std::is_same<uncv_t<T>, std::complex<double>>,
                  std::is_same<uncv_t<T>, std::complex<long double>>>;

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

template <typename From, typename To>
concept bool Convertible = std::is_convertible<From, To>::value;

template <typename T>
concept bool NonConst = !std::is_const<T>::value;

template <typename T>
concept bool CppComplex = is_cpp_complex<T>::value;

#endif

inline namespace impl
{

// The type resulting from the addition of T and U.
template <typename T, typename U>
using add_t = decltype(std::declval<const T &>() + std::declval<const U &>());
}

// Addable type trait.
template <typename T, typename U = T>
struct is_addable : is_detected<add_t, T, U> {
};

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
}

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
using eq_t = decltype(std::declval<const T &>() == std::declval<const U &>());

template <typename T, typename U>
using ineq_t = decltype(std::declval<const T &>() != std::declval<const U &>());
}

// Equality-comparable type trait.
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
        = conjunction<is_function_object<const T, std::size_t, const addlref_t<U>>, is_container_element<T>>::value;

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
        = conjunction<is_function_object<const T, bool, const addlref_t<U>, const addlref_t<U>>,
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
}

/// Macro to test if class has type definition.
/**
 * This macro will declare a struct template parametrized over one type \p T and called <tt>has_typedef_type_name</tt>,
 * whose static const bool member \p value will be \p true if \p T contains a \p typedef called \p type_name, \p false
 * otherwise.
 *
 * For instance:
 * @code
 * PIRANHA_DECLARE_HAS_TYPEDEF(foo_type);
 * struct foo
 * {
 * 	typedef int foo_type;
 * };
 * struct bar {};
 * @endcode
 * \p has_typedef_foo_type<foo>::value will be true and \p has_typedef_foo_type<bar>::value will be false.
 */
#define PIRANHA_DECLARE_HAS_TYPEDEF(type_name)                                                                         \
    template <typename PIRANHA_DECLARE_HAS_TYPEDEF_ARGUMENT>                                                           \
    class has_typedef_##type_name                                                                                      \
    {                                                                                                                  \
        using Td_ = piranha::uncvref_t<PIRANHA_DECLARE_HAS_TYPEDEF_ARGUMENT>;                                          \
        template <typename U>                                                                                          \
        using type_t = typename U::type_name;                                                                          \
                                                                                                                       \
    public:                                                                                                            \
        static const bool value = piranha::is_detected<type_t, Td_>::value;                                            \
    }

/// Macro for static type trait checks.
/**
 * This macro will check via a \p static_assert that the template type trait \p tt provides a \p true \p value.
 * The variadic arguments are interpreted as the template arguments of \p tt.
 */
#define PIRANHA_TT_CHECK(tt, ...)                                                                                      \
    static_assert(tt<__VA_ARGS__>::value, "type trait check failure -> " #tt "<" #__VA_ARGS__ ">")

namespace piranha
{

namespace detail
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
}

/// Detect narrowest integer type
/**
 * This type alias requires \p T and \p Args (if any) to be all signed or unsigned integer types.
 * It will be defined as the input type with the narrowest numerical range.
 */
template <typename T, typename... Args>
using min_int = typename detail::min_int_impl<T, Args...>::type;

/// Detect widest integer type
/**
 * This type alias requires \p T and \p Args (if any) to be all signed or unsigned integer types.
 * It will be defined as the input type with the widest numerical range.
 */
template <typename T, typename... Args>
using max_int = typename detail::max_int_impl<T, Args...>::type;

inline namespace impl
{

// Detect the availability of std::iterator_traits on type It, plus a couple more requisites from the
// iterator concept.
// NOTE: this needs also the is_swappable type trait, but this seems to be difficult to implement in C++11. Mostly
// because it seems that:
// - we cannot detect a specialised std::swap (so if it is not specialised, it will pick the default implementation
//   which could fail in the implementation without giving hints in the prototype),
// - it's tricky to fulfill the requirement that swap has to be called unqualified (cannot use 'using std::swap' within
//   a decltype() SFINAE, might be doable with automatic return type deduction for regular functions in C++14?).
template <typename It>
struct has_iterator_traits {
    using it_tags = std::tuple<std::input_iterator_tag, std::output_iterator_tag, std::forward_iterator_tag,
                               std::bidirectional_iterator_tag, std::random_access_iterator_tag>;
    PIRANHA_DECLARE_HAS_TYPEDEF(difference_type);
    PIRANHA_DECLARE_HAS_TYPEDEF(value_type);
    PIRANHA_DECLARE_HAS_TYPEDEF(pointer);
    PIRANHA_DECLARE_HAS_TYPEDEF(reference);
    PIRANHA_DECLARE_HAS_TYPEDEF(iterator_category);
    using i_traits = std::iterator_traits<It>;
    static const bool value
        = conjunction<has_typedef_reference<i_traits>, has_typedef_value_type<i_traits>, has_typedef_pointer<i_traits>,
                      has_typedef_difference_type<i_traits>, has_typedef_iterator_category<i_traits>,
                      std::is_copy_constructible<It>, std::is_copy_assignable<It>, std::is_destructible<It>>::value;
};

// TMP to check if a type is convertible to a type in the tuple.
template <typename, typename>
struct convertible_type_in_tuple {
};

template <typename T, typename... Args>
struct convertible_type_in_tuple<T, std::tuple<Args...>> : disjunction<std::is_convertible<T, Args>...> {
};
}

// NOTE: this and the other iterator type traits seem to work ok in practice, but probably
// there are some corner cases which are not handled fully according to the standard. After spending
// some time on these, it seems like a nontrivial amount of work would be needed to refine them
// further, so let's just leave them like this for now.

/// Iterator type trait.
/**
 * This type trait will be \p true if \p T, after the removal of cv/ref qualifiers, satisfies the compile-time
 * requirements of an iterator (as defined by the C++ standard), \p false otherwise.
 */
template <typename T>
class is_iterator
{
    template <typename U>
    using deref_t = decltype(*std::declval<U &>());
    template <typename U>
    using inc_t = decltype(++std::declval<U &>());
    template <typename U>
    using it_cat = typename std::iterator_traits<U>::iterator_category;
    using uT = uncvref_t<T>;
    static const bool implementation_defined = conjunction<
        // NOTE: here the correct condition is the commented one, as opposed to the first one appearing. However, it
        // seems like there are inconsistencies between the commented condition and the definition of many output
        // iterators in the standard library:
        //
        // http://stackoverflow.com/questions/23567244/apparent-inconsistency-in-iterator-requirements
        //
        // Until this is clarified, it is probably better to keep this workaround.
        /* std::is_same<typename std::iterator_traits<T>::reference,decltype(*std::declval<T &>())>::value */
        is_detected<deref_t, uT>, std::is_same<detected_t<inc_t, uT>, addlref_t<uT>>, has_iterator_traits<uT>,
        // NOTE: here we used to have type_in_tuple, but it turns out Boost.iterator defines its own set of tags derived
        // from the standard ones. Hence, check that the category can be converted to one of the standard categories.
        // This should not change anything for std iterators, and just enable support for Boost ones.
        convertible_type_in_tuple<detected_t<it_cat, uT>, typename has_iterator_traits<uT>::it_tags>>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T>
const bool is_iterator<T>::value;

inline namespace impl
{

// The purpose of these bits is to check whether U correctly implements the arrow operator.
// A correct implementation will return a pointer, after potentially calling
// the operator recursively as many times as needed. See:
// http://stackoverflow.com/questions/10677804/how-arrow-operator-overloading-works-internally-in-c
template <typename U, typename = void>
struct arrow_operator_type {
};

// This represents the terminator of the recursion.
template <typename U>
struct arrow_operator_type<U *> {
    using type = U *;
};

// Handy alias.
template <typename U>
using arrow_operator_t = typename arrow_operator_type<U>::type;

// These bits invoke arrow_operator_type recursively, until a pointer is found.
template <typename U>
using rec_arrow_op_t = arrow_operator_t<decltype(std::declval<U &>().operator->())>;

template <typename U>
struct arrow_operator_type<U, enable_if_t<is_detected<rec_arrow_op_t, U>::value>> {
    using type = rec_arrow_op_t<U>;
};

// NOTE: need the SFINAE wrapper here and below because we need to soft-error
// out in case the types in std::iterator_traits are not defined.
template <typename T, typename = void>
struct is_input_iterator_impl : std::false_type {
};

template <typename T>
struct is_input_iterator_impl<
    T,
    enable_if_t<conjunction<
        is_iterator<T>, is_equality_comparable<T>,
        std::is_convertible<decltype(*std::declval<T &>()), typename std::iterator_traits<T>::value_type>,
        std::is_same<decltype(++std::declval<T &>()), T &>,
        std::is_same<decltype((void)std::declval<T &>()++), decltype((void)++std::declval<T &>())>,
        std::is_convertible<decltype(*std::declval<T &>()++), typename std::iterator_traits<T>::value_type>,
        // NOTE: here we know that the arrow op has to return a pointer, if
        // implemented correctly, and that the syntax it->m must be
        // equivalent to (*it).m. This means that, barring differences in
        // reference qualifications, it-> and *it must return the same thing.
        std::is_same<unref_t<decltype(*std::declval<arrow_operator_t<T>>())>, unref_t<decltype(*std::declval<T &>())>>,
        // NOTE: here the usage of is_convertible guarantees we catch both
        // iterators higher in the type hierarchy and the Boost versions of
        // standard iterators as well.
        std::is_convertible<typename std::iterator_traits<T>::iterator_category, std::input_iterator_tag>>::value>>
    : std::true_type {
};
}

/// Input iterator type trait.
/**
 * This type trait will be \p true if \p T, after the removal of cv/reference qualifiers, satisfies the compile-time
 * requirements of an input iterator (as defined by the C++ standard), \p false otherwise.
 */
template <typename T>
class is_input_iterator
{
    static const bool implementation_defined = is_input_iterator_impl<uncvref_t<T>>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T>
const bool is_input_iterator<T>::value;

inline namespace impl
{

template <typename T, typename = void>
struct is_forward_iterator_impl : std::false_type {
};

template <typename T>
struct is_forward_iterator_impl<
    T, enable_if_t<conjunction<
           is_input_iterator<T>, std::is_default_constructible<T>,
           disjunction<std::is_same<typename std::iterator_traits<T>::value_type &,
                                    typename std::iterator_traits<T>::reference>,
                       std::is_same<typename std::iterator_traits<T>::value_type const &,
                                    typename std::iterator_traits<T>::reference>>,
           std::is_convertible<decltype(std::declval<T &>()++), const T &>,
           std::is_same<decltype(*std::declval<T &>()++), typename std::iterator_traits<T>::reference>,
           std::is_convertible<typename std::iterator_traits<T>::iterator_category, std::forward_iterator_tag>>::value>>
    : std::true_type {
};
}

/// Forward iterator type trait.
/**
 * This type trait will be \p true if \p T, after the removal of cv/reference qualifiers, satisfies the compile-time
 * requirements of a forward iterator (as defined by the C++ standard), \p false otherwise.
 */
template <typename T>
class is_forward_iterator
{
    static const bool implementation_defined = is_forward_iterator_impl<uncvref_t<T>>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T>
const bool is_forward_iterator<T>::value;

namespace detail
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
}

/// Detect the availability of <tt>std::begin()</tt> and <tt>std::end()</tt>.
/**
 * This type trait will be \p true if all the following conditions are fulfilled:
 *
 * - <tt>std::begin()</tt> and <tt>std::end()</tt> can be called on instances of \p T, yielding the type \p It,
 * - \p It is an input iterator.
 */
template <typename T>
class has_input_begin_end
{
    template <typename U>
    using begin_t = decltype(std::begin(std::declval<U &>()));
    template <typename U>
    using end_t = decltype(std::end(std::declval<U &>()));
    static const bool implementation_defined
        = conjunction<is_input_iterator<detected_t<begin_t, T>>, is_input_iterator<detected_t<end_t, T>>,
                      std::is_same<detected_t<begin_t, T>, detected_t<end_t, T>>>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T>
const bool has_input_begin_end<T>::value;

// Detect if type can be returned from a function.
template <typename T>
using is_returnable = disjunction<
    std::is_same<T, void>,
    conjunction<std::is_destructible<T>, disjunction<std::is_copy_constructible<T>, std::is_move_constructible<T>>>>;

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
}

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
}

#endif
