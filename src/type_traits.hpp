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

#ifndef PIRANHA_TYPE_TRAITS_HPP
#define PIRANHA_TYPE_TRAITS_HPP

/** \file type_traits.hpp
 * \brief Type traits.
 * 
 * This header contains general-purpose type-traits classes.
 */

#include <boost/type_traits/has_trivial_copy.hpp>
#include <boost/type_traits/has_trivial_destructor.hpp>
#include <cstdarg>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include "detail/base_term_fwd.hpp"
#include "detail/sfinae_types.hpp"

namespace piranha
{

/// Type-trait to test if type is a tuple.
/**
 * The \p value member will be \p true if \p T is an \p std::tuple, \p false otherwise.
 */
template <typename T>
struct is_tuple: std::false_type {};

template <typename... Args>
struct is_tuple<std::tuple<Args...>>: std::true_type {};

/// Type is reference or is cv-qualified.
/**
 * This type trait defines a static const boolean \p value flag which is \p true if either \p T is a reference or it is cv-qualified, \p false otherwise.
 */
template <typename T>
struct is_cv_or_ref
{
	/// Type-trait value.
	static const bool value = (std::is_reference<T>::value || std::is_const<T>::value || std::is_volatile<T>::value);
};

template <typename T>
const bool is_cv_or_ref<T>::value;

/// Type is non-const rvalue reference.
/**
 * This type trait defines a static const boolean \p value flag which is \p true if \p T is a non-const rvalue reference.
 */
template <typename T>
struct is_nonconst_rvalue_ref
{
	/// Type-trait value.
	static const bool value = std::is_rvalue_reference<T>::value && !std::is_const<typename std::remove_reference<T>::type>::value;
};

template <typename T>
const bool is_nonconst_rvalue_ref<T>::value;

/// Type has non-throwing move constructor.
/**
 * Placeholder for <tt>std::is_nothrow_move_constructible</tt>, until it is implemented in GCC.
 */
template <typename T, typename Enable = void>
struct is_nothrow_move_constructible
{
	/// Type-trait value.
	static const bool value = noexcept(T(static_cast<T &&>(*static_cast<T *>(nullptr))));
};

template <typename T, typename Enable>
const bool is_nothrow_move_constructible<T,Enable>::value;

/// Type has non-throwing move assignment operator.
/**
 * Placeholder for <tt>std::is_nothrow_move_assignable</tt>, until it is implemented in GCC.
 */
template <typename T, typename Enable = void>
struct is_nothrow_move_assignable
{
	/// Type-trait value.
	static const bool value = noexcept(*static_cast<T *>(nullptr) = static_cast<T &&>(*static_cast<T *>(nullptr)));
};

template <typename T, typename Enable>
const bool is_nothrow_move_assignable<T,Enable>::value;

/// Type is nothrow-destructible.
/**
 * Placeholder for <tt>std::is_nothrow_destructible</tt>, until it is implemented in GCC.
 */
template <typename T>
struct is_nothrow_destructible
{
	/// Type-trait value.
	static const bool value = noexcept(static_cast<T *>(nullptr)->~T());
};

template <typename T>
const bool is_nothrow_destructible<T>::value;

/// Type is trivially destructible.
/**
 * Equivalent to <tt>std::is_trivially_destructible</tt>.
 * Will use an implementation-defined type trait internally.
 */
template <typename T>
struct is_trivially_destructible :
#if defined(__clang__)
std::is_trivially_destructible<T>
#elif defined(__GNUC__)
#if __GNUC__ == 4 && __GNUC_MINOR__ < 8
std::has_trivial_destructor<T>
#else
std::is_trivially_destructible<T>
#endif
#else
boost::has_trivial_destructor<T>
#endif
{};

/// Type is trivially copyable.
/**
 * Equivalent to <tt>std::is_trivially_copyable</tt>.
 * Will use an implementation-defined type trait internally.
 */
template <typename T>
struct is_trivially_copyable :
#if defined(__clang__)
std::is_trivially_copyable<T>
#elif defined(__GNUC__)
std::has_trivial_copy_constructor<T>
#else
boost::has_trivial_copy_constructor<T>
#endif
{};

/// Type has degree.
/**
 * This type trait establishes if a type is provided with a "degree" and "low degree" property (in the mathematical sense,
 * as in polynomial degree),
 * and how such property can be queried. The default implementation's value is \p false.
 * 
 * Any specialisation which sets the value to \p true shall also provide a set of static methods
 * to query the total or partial (low) degree of an instance of type \p T as follows:
 * 
 * - a <tt>get(const T &x)</tt> static method, returning the total degree of \p x,
 * - a <tt>get(const T &x, const std::set<std::string> &s)</tt> static method, returning
 *   the partial degree of \p x when only the literal variables with names in \p s are considered
 *   in the computation of the degree,
 * - a <tt>lget(const T &x)</tt> static method, returning the total low degree of \p x (i.e.,
 *   the smallest degree appearing in the collection of items composing \p x - this is essentially
 *   analogous to the notion of order in formal power series),
 * - a <tt>lget(const T &x, const std::set<std::string> &s)</tt> static method, returning the
 *   the partial low degree.
 * 
 * \todo we could probably do away with this, and just introduce math::degree() functions similar to
 * pow(), cos(), etc. and the auto-detect the type-trait with SFINAE.
 */
template <typename T, typename Enable = void>
class has_degree
{
	public:
		/// Value of the type trait.
		static const bool value = false;
};

template <typename T, typename Enable>
const bool has_degree<T,Enable>::value;

/// Addable type trait.
/**
 * Will be \p true if adding an instance of \p T to an instance of \p U is a valid expression, \p false otherwise.
 */
template <typename T, typename U>
class is_addable: detail::sfinae_types
{
		template <typename V>
		static auto test(const V *t) -> decltype(*t + std::declval<U>(),void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test((T const *)nullptr)),yes>::value;
};

template <typename T, typename U>
const bool is_addable<T,U>::value;

/// In-place addable type trait.
/**
 * Will be \p true if adding in-place an instance of \p U to an instance of \p T is a valid expression, \p false otherwise.
 */
template <typename T, typename U>
class is_addable_in_place: detail::sfinae_types
{
		template <typename V>
		static auto test(V *t) -> decltype(*t += std::declval<U>(),void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test((T *)nullptr)),yes>::value;
};

template <typename T, typename U>
const bool is_addable_in_place<T,U>::value;

/// Subtractable type trait.
/**
 * Will be \p true if subtracting an instance of \p U from an instance of \p T is a valid expression, \p false otherwise.
 */
template <typename T, typename U>
class is_subtractable: detail::sfinae_types
{
		template <typename V>
		static auto test(const V *t) -> decltype(*t - std::declval<U>(),void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test((T const *)nullptr)),yes>::value;
};

template <typename T, typename U>
const bool is_subtractable<T,U>::value;

/// In-place subtractable type trait.
/**
 * Will be \p true if subtracting in-place an instance of \p U from an instance of \p T is a valid expression, \p false otherwise.
 */
template <typename T, typename U>
class is_subtractable_in_place: detail::sfinae_types
{
		template <typename V>
		static auto test(V *t) -> decltype(*t -= std::declval<U>(),void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test((T *)nullptr)),yes>::value;
};

template <typename T, typename U>
const bool is_subtractable_in_place<T,U>::value;

/// Assignable type trait.
/**
 * This type trait is analogous to \p std::is_assignable.
 */
template <typename T, typename U>
class is_assignable: detail::sfinae_types
{
		template <typename T1, typename U1>
		static decltype(std::declval<T1>() = std::declval<U1>(),void(),yes()) test (int);
		template <typename, typename>
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test<T,U>(0)),yes>::value;
};

template <typename T, typename U>
const bool is_assignable<T,U>::value;

/// Equality-comparable type trait.
/**
 * This type trait is \p true if instances if type \p T can be compared for equality and inequality to instances of
 * type \p U. The operators must be non-mutable (i.e., implemented using pass-by-value or const
 * references) and must return a type implicitly convertible to \p bool.
 */
template <typename T, typename U = T>
class is_equality_comparable: detail::sfinae_types
{
		typedef typename std::decay<T>::type Td;
		typedef typename std::decay<U>::type Ud;
		template <typename T1, typename U1>
		static auto test1(const T1 &t, const U1 &u) -> decltype(t == u);
		static no test1(...);
		template <typename T1, typename U1>
		static auto test2(const T1 &t, const U1 &u) -> decltype(t != u);
		static no test2(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_convertible<decltype(test1(std::declval<Td>(),std::declval<Ud>())),bool>::value &&
			std::is_convertible<decltype(test2(std::declval<Td>(),std::declval<Ud>())),bool>::value;
};

// Static init.
template <typename T, typename U>
const bool is_equality_comparable<T,U>::value;

}

/// Macro to test if class has type definition.
/**
 * This macro will declare a template struct parametrized over one type \p T and called <tt>has_typedef_type_name</tt>,
 * whose static const bool member \p value will be \p true if \p T contains a \p typedef called \p type_name, false otherwise.
 * 
 * For instance:
 * \code
 * PIRANHA_DECLARE_HAS_TYPEDEF(foo_type);
 * struct foo
 * {
 * 	typedef int foo_type;
 * };
 * struct bar {};
 * \endcode
 * \p has_typedef_foo_type<foo>::value will be true and \p has_typedef_foo_type<bar>::value will be false.
 */
#define PIRANHA_DECLARE_HAS_TYPEDEF(type_name) \
template <typename PIRANHA_DECLARE_HAS_TYPEDEF_ARGUMENT> \
struct has_typedef_##type_name \
{ \
	typedef char yes[1]; \
	typedef char no[2]; \
	template <typename C> \
	static yes &test(typename C::type_name *); \
	template <typename> \
	static no &test(...); \
	static const bool value = sizeof(test<PIRANHA_DECLARE_HAS_TYPEDEF_ARGUMENT>(0)) == sizeof(yes); \
}

#endif
