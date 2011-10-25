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

#include <cstddef>
#include <tuple>
#include <type_traits>

#include "config.hpp"
#include "detail/base_term_fwd.hpp"

// TODO:
// - check inclusion of this header -> only uses are in concept, the enabler thing and the has_type.

namespace piranha
{

/// Enabler variable.
/**
 * @see http://boost.2283326.n4.nabble.com/New-powerful-way-to-use-enable-if-in-C-0x-td3442723.html.
 */
extern void *enabler;

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
 * In GCC 4.5, it will be true by default for all types. In GCC >= 4.6, it will use
 * the \p noexcept operator.
 */
template <typename T, typename Enable = void>
struct is_nothrow_move_constructible
{
	/// Type-trait value.
	static const bool value = piranha_noexcept_op(T(static_cast<T &&>(*static_cast<T *>(piranha_nullptr))));
};

// TODO: this will have to be removed once GCC support improves. It is used in kronecker_array
// when dealing with vectors of tuples representing the limits.
template <typename T>
struct is_nothrow_move_constructible<std::tuple<T,T,T,T,T,T>,typename std::enable_if<std::is_integral<T>::value>::type>
{
	static const bool value = true;
};

template <typename T, typename Enable>
const bool is_nothrow_move_constructible<T,Enable>::value;

/// Type has non-throwing move assignment operator.
/**
 * Placeholder for <tt>std::is_nothrow_move_assignable</tt>, until it is implemented in GCC.
 * In GCC 4.5, it will be true by default for all types. In GCC >= 4.6, it will use
 * the \p noexcept operator.
 */
template <typename T, typename Enable = void>
struct is_nothrow_move_assignable
{
	/// Type-trait value.
	static const bool value = piranha_noexcept_op(*static_cast<T *>(piranha_nullptr) = static_cast<T &&>(*static_cast<T *>(piranha_nullptr)));
};

// TODO: same as is_nothrow_move_constructible above.
template <typename T>
struct is_nothrow_move_assignable<std::tuple<T,T,T,T,T,T>,typename std::enable_if<std::is_integral<T>::value>::type>
{
	static const bool value = true;
};

template <typename T, typename Enable>
const bool is_nothrow_move_assignable<T,Enable>::value;

/// Type is nothrow-destructible.
/**
 * Placeholder for <tt>std::is_nothrow_destructible</tt>, until it is implemented in GCC.
 * In GCC 4.5, it will be true by default for all types. In GCC >= 4.6, it will use
 * the \p noexcept operator.
 */
template <typename T>
struct is_nothrow_destructible
{
	/// Type-trait value.
	static const bool value = piranha_noexcept_op(static_cast<T *>(piranha_nullptr)->~T());
};

template <typename T>
const bool is_nothrow_destructible<T>::value;

/// Type is trivially destructible.
/**
 * Placeholder for <tt>std::is_trivially_destructible</tt>, until it is implemented in GCC.
 * Will use a GCC-specific type trait for implementation.
 */
template <typename T>
struct is_trivially_destructible : std::has_trivial_destructor<T>
{};

/// Type is trivially copyable.
/**
 * Placeholder for <tt>std::is_trivially_copyable</tt>, until it is implemented in GCC.
 * Will use a GCC-specific type trait for implementation.
 */
template <typename T>
struct is_trivially_copyable : std::has_trivial_copy_constructor<T>
{};

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
