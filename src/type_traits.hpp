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
 * This header contains general-purpose type traits classes.
 */

#include <cstdarg>
#include <cstddef>
#include <functional>
#include <iterator>
#include <limits>
#include <ostream>
#include <tuple>
#include <type_traits>
#include <utility>

#include "config.hpp"
#include "detail/base_term_fwd.hpp"
#include "detail/math_tt_fwd.hpp"
#include "detail/sfinae_types.hpp"
#include "detail/symbol_set_fwd.hpp"
#include "print_coefficient.hpp"
#include "print_tex_coefficient.hpp"

namespace piranha
{

/// Type is non-const rvalue reference.
/**
 * This type trait defines a static const boolean \p value flag which is \p true if \p T is a non-const rvalue reference.
 */
template <typename T>
struct is_nonconst_rvalue_ref
{
	/// Type trait value.
	static const bool value = std::is_rvalue_reference<T>::value && !std::is_const<typename std::remove_reference<T>::type>::value;
};

template <typename T>
const bool is_nonconst_rvalue_ref<T>::value;

/// Type is nothrow-destructible.
/**
 * Placeholder for <tt>std::is_nothrow_destructible</tt>, until it is implemented in GCC.
 */
template <typename T, typename = void>
struct is_nothrow_destructible
{
	/// Type trait value.
	static const bool value = noexcept(std::declval<T>().~T());
};

template <typename T>
struct is_nothrow_destructible<T,typename std::enable_if<!std::is_destructible<T>::value>::type>
{
	static const bool value = false;
};

template <typename T>
struct is_nothrow_destructible<T,typename std::enable_if<std::is_reference<T>::value>::type>
{
	static const bool value = true;
};

template <typename T, typename Enable>
const bool is_nothrow_destructible<T,Enable>::value;

template <typename T>
const bool is_nothrow_destructible<T,typename std::enable_if<!std::is_destructible<T>::value>::type>::value;

template <typename T>
const bool is_nothrow_destructible<T,typename std::enable_if<std::is_reference<T>::value>::type>::value;

namespace detail
{

template <typename T, typename U, typename Derived>
class arith_tt_helper
{
		typedef typename std::remove_reference<T>::type Td;
		typedef typename std::remove_reference<U>::type Ud;
	public:
		static const bool value = std::is_same<decltype(Derived::test(*(Td *)nullptr,*(Ud *)nullptr)),
			detail::sfinae_types::yes>::value;
};

}

/// Addable type trait.
/**
 * Will be \p true if objects of type \p T can be added to objects of type \p U using the binary addition operator.
 * 
 * This type trait will strip \p T and \p U of reference qualifiers, and it will test the operator in the form
 @code
 operator+(const Td &, const Ud &)
 @endcode
 * where \p Td and \p Ud are \p T and \p U after the removal of reference qualifiers. E.g.:
 @code
 is_addable<int>::value == true;
 is_addable<int,std::string>::value == false;
 @endcode
 */
template <typename T, typename U = T>
class is_addable: detail::sfinae_types
{
		friend class detail::arith_tt_helper<T,U,is_addable<T,U>>;
		template <typename T1, typename U1>
		static auto test(const T1 &t, const U1 &u) -> decltype(t + u,void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = detail::arith_tt_helper<T,U,is_addable>::value;
};

template <typename T, typename U>
const bool is_addable<T,U>::value;

/// In-place addable type trait.
/**
 * Will be \p true if objects of type \p U can be added in-place to objects of type \p T.
 * 
 * This type trait will strip \p T and \p U of reference qualifiers, and it will test the operator in the form
 @code
 operator+=(Td &, const Ud &)
 @endcode
 * where \p Td and \p Ud are \p T and \p U after the removal of reference qualifiers. E.g.:
 @code
 is_addable_in_place<int>::value == true;
 is_addable_in_place<int,std::string>::value == false;
 @endcode
 */
template <typename T, typename U = T>
class is_addable_in_place: detail::sfinae_types
{
		friend class detail::arith_tt_helper<T,U,is_addable_in_place<T,U>>;
		template <typename T1, typename U1>
		static auto test(T1 &t, const U1 &u) -> decltype(t += u,void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = detail::arith_tt_helper<T,U,is_addable_in_place>::value;
};

template <typename T, typename U>
const bool is_addable_in_place<T,U>::value;

/// Subtractable type trait.
/**
 * @see piranha::is_addable.
 */
template <typename T, typename U = T>
class is_subtractable: detail::sfinae_types
{
		friend class detail::arith_tt_helper<T,U,is_subtractable<T,U>>;
		template <typename T1, typename U1>
		static auto test(const T1 &t, const U1 &u) -> decltype(t - u,void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = detail::arith_tt_helper<T,U,is_subtractable>::value;
};

template <typename T, typename U>
const bool is_subtractable<T,U>::value;

/// In-place subtractable type trait.
/**
 * @see piranha::is_addable_in_place.
 */
template <typename T, typename U = T>
class is_subtractable_in_place: detail::sfinae_types
{
		friend class detail::arith_tt_helper<T,U,is_subtractable_in_place<T,U>>;
		template <typename T1, typename U1>
		static auto test(T1 &t, const U1 &u) -> decltype(t -= u,void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = detail::arith_tt_helper<T,U,is_subtractable_in_place>::value;
};

template <typename T, typename U>
const bool is_subtractable_in_place<T,U>::value;

/// Multipliable type trait.
/**
 * @see piranha::is_addable.
 */
template <typename T, typename U = T>
class is_multipliable: detail::sfinae_types
{
		friend class detail::arith_tt_helper<T,U,is_multipliable<T,U>>;
		template <typename T1, typename U1>
		static auto test(const T1 &t, const U1 &u) -> decltype(t * u,void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = detail::arith_tt_helper<T,U,is_multipliable>::value;
};

template <typename T, typename U>
const bool is_multipliable<T,U>::value;

/// In-place multipliable type trait.
/**
 * @see piranha::is_addable_in_place.
 */
template <typename T, typename U = T>
class is_multipliable_in_place: detail::sfinae_types
{
		friend class detail::arith_tt_helper<T,U,is_multipliable_in_place<T,U>>;
		template <typename T1, typename U1>
		static auto test(T1 &t, const U1 &u) -> decltype(t *= u,void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = detail::arith_tt_helper<T,U,is_multipliable_in_place>::value;
};

template <typename T, typename U>
const bool is_multipliable_in_place<T,U>::value;

/// Divisible type trait.
/**
 * @see piranha::is_addable.
 */
template <typename T, typename U = T>
class is_divisible: detail::sfinae_types
{
		friend class detail::arith_tt_helper<T,U,is_divisible<T,U>>;
		template <typename T1, typename U1>
		static auto test(const T1 &t, const U1 &u) -> decltype(t / u,void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = detail::arith_tt_helper<T,U,is_divisible>::value;
};

template <typename T, typename U>
const bool is_divisible<T,U>::value;

/// In-place divisible type trait.
/**
 * @see piranha::is_addable_in_place.
 */
template <typename T, typename U = T>
class is_divisible_in_place: detail::sfinae_types
{
		friend class detail::arith_tt_helper<T,U,is_divisible_in_place<T,U>>;
		template <typename T1, typename U1>
		static auto test(T1 &t, const U1 &u) -> decltype(t /= u,void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = detail::arith_tt_helper<T,U,is_divisible_in_place>::value;
};

template <typename T, typename U>
const bool is_divisible_in_place<T,U>::value;

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

/// Less-than-comparable type trait.
/**
 * This type trait is \p true if instances of type \p T can be compared to instances of
 * type \p U using the less-than operator. The operator must be non-mutable (i.e., implemented using pass-by-value or const
 * references) and must return a type implicitly convertible to \p bool.
 */
template <typename T, typename U = T>
class is_less_than_comparable: detail::sfinae_types
{
		typedef typename std::decay<T>::type Td;
		typedef typename std::decay<U>::type Ud;
		template <typename T1, typename U1>
		static auto test(const T1 &t, const U1 &u) -> decltype(t < u);
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_convertible<decltype(test(std::declval<Td>(),std::declval<Ud>())),bool>::value;
};

// Static init.
template <typename T, typename U>
const bool is_less_than_comparable<T,U>::value;

/// Greater-than-comparable type trait.
/**
 * This type trait is \p true if instances of type \p T can be compared to instances of
 * type \p U using the greater-than operator. The operator must be non-mutable (i.e., implemented using pass-by-value or const
 * references) and must return a type implicitly convertible to \p bool.
 */
template <typename T, typename U = T>
class is_greater_than_comparable: detail::sfinae_types
{
		typedef typename std::decay<T>::type Td;
		typedef typename std::decay<U>::type Ud;
		template <typename T1, typename U1>
		static auto test(const T1 &t, const U1 &u) -> decltype(t > u);
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_convertible<decltype(test(std::declval<Td>(),std::declval<Ud>())),bool>::value;
};

// Static init.
template <typename T, typename U>
const bool is_greater_than_comparable<T,U>::value;

/// Enable \p noexcept checks.
/**
 * This type trait, to be specialised with the <tt>std::enable_if</tt> mechanism, enables or disables
 * \p noexcept checks in the piranha::is_container_element type trait. This type trait should be used
 * only with legacy pre-C++11 classes that do not support \p noexcept: by specialising this trait to
 * \p false, the piranha::is_container_element trait will disable \p noexcept checks and it will thus be
 * possible to use \p noexcept unaware classes as container elements and series coefficients.
 */
template <typename T, typename = void>
struct enable_noexcept_checks
{
	/// Default value of the type trait.
	static const bool value = true;
};

// Static init.
template <typename T, typename Enable>
const bool enable_noexcept_checks<T,Enable>::value;

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
struct is_container_element
{
	// NOTE: here we do not require copy assignability as in our containers we always implement
	// copy-assign as copy-construct + move for exception safety reasons.
	/// Value of the type trait.
	static const bool value = std::is_default_constructible<T>::value &&
				  std::is_copy_constructible<T>::value &&
				  (!enable_noexcept_checks<T>::value || (
				  is_nothrow_destructible<T>::value
// The Intel compiler has troubles with the noexcept versions of these two type traits.
#if defined(PIRANHA_COMPILER_IS_INTEL)
				  && std::is_move_constructible<T>::value &&
				  std::is_move_assignable<T>::value
#else
				  && std::is_nothrow_move_constructible<T>::value &&
				  std::is_nothrow_move_assignable<T>::value
#endif
				  ));
};

template <typename T>
const bool is_container_element<T>::value;

namespace detail
{

// Here this is quite tricky. Compiler support for variadic template template is shaky
// at best across GCC/Clang/Intel. The only one to get it completely right at this time seems
// GCC 4.9.0. The others:
// - GCC < 4.9.0 compiles this but it has problems when TT has a certain number of template arguments > 0
//   and a final variadic pack. E.g.,
//   class <typename T, typename U, typename ... Args> class TT;
//   will not be detected. See also:
//   http://stackoverflow.com/questions/18724698/variadic-template-deduction-in-variadic-template-template?noredirect=1#comment39290689_18724698
//   The workaround of spelling explicitly N arguments works ok.
// - Clang behaves like GCC 4.8 *except* that the workaround won't work. Clang does not recognize the
//   spelled-out N arguments versions below as specialisations of the general case.
// - The Intel compiler won't work at all unless the workaround is activated.

// This is the base, standard-compliant version.
template <template <typename ...> class TT>
struct iio_converter
{
	template <typename ... Args>
	iio_converter(const TT<Args ...> &);
};

#if (defined(PIRANHA_COMPILER_IS_GCC) && __GNUC__  == 4 && __GNUC_MINOR__ < 9) || \
	defined(PIRANHA_COMPILER_IS_INTEL)

template <template <typename> class TT>
struct iio_converter<TT>
{
	template <typename T0>
	iio_converter(const TT<T0> &);
};

template <template <typename, typename> class TT>
struct iio_converter<TT>
{
	template <typename T0, typename T1>
	iio_converter(const TT<T0,T1> &);
};

template <template <typename, typename, typename> class TT>
struct iio_converter<TT>
{
	template <typename T0, typename T1, typename T2>
	iio_converter(const TT<T0,T1,T2> &);
};

template <template <typename, typename, typename, typename> class TT>
struct iio_converter<TT>
{
	template <typename T0, typename T1, typename T2, typename T3>
	iio_converter(const TT<T0,T1,T2,T3> &);
};

// Let's stop at 5 arguments, add more if needed.
template <template <typename, typename, typename, typename, typename> class TT>
struct iio_converter<TT>
{
	template <typename T0, typename T1, typename T2, typename T3, typename T4>
	iio_converter(const TT<T0,T1,T2,T3,T4> &);
};

#endif

}

/// Type trait for detecting instances of class templates.
/**
 * This type trait will be \p true if the decay type of \p T (or one of its base classes) is an instance
 * of the class template \p TT, \p false otherwise. E.g.,
 @code
 is_instance_of<std::vector<int>,std::vector>::value == true;
 is_instance_of<std::ostream,std::basic_ios>::value == true;
 is_instance_of<int,std::list>::value == false;
 @endcode
 */
template <class T, template <typename ...> class TT>
class is_instance_of
{
		typedef typename std::decay<T>::type Td;
	public:
		/// Value of the type trait.
		static const bool value = std::is_convertible<Td,detail::iio_converter<TT>>::value;
};

template <class T, template <typename ...> class TT>
const bool is_instance_of<T,TT>::value;

/// Type trait for classes that can be output-streamed.
/**
 * This type trait will be \p true if instances of type \p T can be directed to
 * instances of \p std::ostream via the insertion operator. The operator must have a signature
 * compatible with
 @code
 std::ostream &operator<<(std::ostream &, const T &)
 @endcode
 */
template <typename T>
class is_ostreamable: detail::sfinae_types
{
		template <typename T1>
		static auto test(std::ostream &s, const T1 &t) -> decltype(s << t);
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test(*(std::ostream *)nullptr,
			std::declval<T>())),std::ostream &>::value;
};

template <typename T>
const bool is_ostreamable<T>::value;

namespace detail
{

template <typename T>
class is_hashable_impl: detail::sfinae_types
{
		typedef typename std::decay<T>::type Td;
		template <typename T1>
		static auto test(const T1 &t) -> decltype((*(std::hash<T1> const *)nullptr)(t));
		static no test(...);
	public:
		static const bool value = std::is_same<decltype(test(std::declval<Td>())),std::size_t>::value;
};

template <typename Hasher, typename Td>
struct common_hasher_requirements
{
	static const bool value = noexcept((*(Hasher const *)nullptr)(std::declval<Td const &>())) &&
				  std::is_nothrow_default_constructible<Hasher>::value &&
				  // NOTE: a bit of repetition here.
				  is_container_element<Hasher>::value;
};

}

/// Hashable type trait.
/**
 * This type trait will be \p true if the decay type of \p T is hashable, \p false otherwise.
 *
 * A type \p T is hashable when supplied with a specialisation of \p std::hash which satisfies the following
 * prerequisites:
 * - it defines a noexcept const call operator taking as parameter a const instance of \p T and returning
 *   \p std::size_t,
 * - it is nothrow default constructible and it satisfies piranha::is_container_element.
 * 
 * Note that depending on the implementation of the default \p std::hash class, using this type trait with
 * a type which does not provide a specialisation for \p std::hash could result in a compilation error
 * (e.g., if the unspecialised \p std::hash includes a false \p static_assert).
 */
template <typename T, typename = void>
class is_hashable
{
	public:
		/// Value of the type trait.
		static const bool value = false;
};

template <typename T>
class is_hashable<T,typename std::enable_if<detail::is_hashable_impl<T>::value>::type>
{
		typedef typename std::decay<T>::type Td;
		typedef std::hash<Td> hasher;
	public:
		static const bool value = detail::common_hasher_requirements<hasher,Td>::value;
};

template <typename T, typename Enable>
const bool is_hashable<T,Enable>::value;

template <typename T>
const bool is_hashable<T,typename std::enable_if<detail::is_hashable_impl<T>::value>::type>::value;

namespace detail
{

template <typename, typename, typename = void>
struct is_function_object_impl
{
	template <typename ... Args>
	struct tt
	{
		static const bool value = false;
	};
};

template <typename T, typename ReturnType>
struct is_function_object_impl<T,ReturnType,typename std::enable_if<std::is_class<T>::value>::type>
{
	template <typename ... Args>
	struct tt: detail::sfinae_types
	{
		template <typename U>
		static auto test(U &f) -> decltype(f(std::declval<Args>()...));
		static no test(...);
		static const bool value = std::is_same<decltype(test(*(T *)nullptr)),ReturnType>::value;
	};
};

}

/// Function object type trait.
/**
 * This type trait will be true if \p T is a function object returning \p ReturnType and taking
 * \p Args as arguments. That is, the type trait will be \p true if the following conditions are met:
 * - \p T is a class,
 * - \p T is equipped with a call operator returning \p ReturnType and taking \p Args as arguments.
 * 
 * \p T can be const qualified (in which case the call operator must be also const in order for the type trait to be satisfied).
 */
template <typename T, typename ReturnType, typename ... Args>
class is_function_object
{
	public:
		/// Value of the type trait.
		static const bool value = detail::is_function_object_impl<T,ReturnType>::template tt<Args...>::value;
};

template <typename T, typename ReturnType, typename ... Args>
const bool is_function_object<T,ReturnType,Args...>::value;

/// Type trait to detect hash function objects.
/**
 * \p T is a hash function object for \p U if the following requirements are met:
 * - \p T is a function object with const noexcept call operator accepting as input const \p U and returning \p std::size_t,
 * - \p T is nothrow default constructible and it satisfies piranha::is_container_element.
 * 
 * The decay type of \p U is considered in this type trait.
 */
template <typename T, typename U, typename = void>
class is_hash_function_object
{
	public:
		/// Value of the type trait.
		static const bool value = false;
};

template <typename T, typename U>
class is_hash_function_object<T,U,typename std::enable_if<is_function_object<typename std::add_const<T>::type,
	std::size_t,typename std::decay<U>::type const &>::value>::type>
{
		typedef typename std::decay<U>::type Ud;
	public:
		static const bool value = detail::common_hasher_requirements<T,Ud>::value;
};

template <typename T, typename U, typename Enable>
const bool is_hash_function_object<T,U,Enable>::value;

template <typename T, typename U>
const bool is_hash_function_object<T,U,typename std::enable_if<is_function_object<typename std::add_const<T>::type,
	std::size_t,typename std::decay<U>::type const &>::value>::type>::value;

/// Type trait to detect equality function objects.
/**
 * \p T is an equality function object for \p U if the following requirements are met:
 * - \p T is a function object with const noexcept call operator accepting as input two const \p U and returning \p bool,
 * - \p T satisfies piranha::is_container_element.
 * 
 * The decay type of \p U is considered in this type trait.
 */
template <typename T, typename U, typename = void>
class is_equality_function_object
{
	public:
		/// Value of the type trait.
		static const bool value = false;
};

template <typename T, typename U>
class is_equality_function_object<T,U,typename std::enable_if<is_function_object<typename std::add_const<T>::type,
	bool,typename std::decay<U>::type const &,typename std::decay<U>::type const &>::value>::type>
{
	public:
		static const bool value = is_container_element<T>::value;
};

template <typename T, typename U, typename Enable>
const bool is_equality_function_object<T,U,Enable>::value;

template <typename T, typename U>
const bool is_equality_function_object<T,U,typename std::enable_if<is_function_object<typename std::add_const<T>::type,
	bool,typename std::decay<U>::type const &,typename std::decay<U>::type const &>::value>::type>::value;

/// Type trait to detect coefficient types.
/**
 * This type trait will be \p true if \p T can be used as a coefficient type, \p false otherwise.
 * The requisites for \p T are the following:
 * 
 * - it must satisfy piranha::is_container_element,
 * - it must satisfy piranha::has_print_coefficient and piranha::has_print_tex_coefficient,
 * - it must satisfy piranha::has_is_zero and piranha::has_negate,
 * - it must be equality comparable,
 * - it must be addable and subtractable (both binary and in-place forms),
 * - it must be constructible from integer numerals.
 */
template <typename T>
class is_cf
{
	public:
		/// Value of the type trait.
		static const bool value = is_container_element<T>::value &&
					  has_print_coefficient<T>::value && has_print_tex_coefficient<T>::value &&
					  has_is_zero<T>::value && has_negate<T>::value &&
					  is_equality_comparable<T>::value && is_addable<T>::value &&
					  is_addable_in_place<T>::value && is_subtractable_in_place<T>::value &&
					  is_subtractable<T>::value && std::is_constructible<T,int>::value;
};

template <typename T>
const bool is_cf<T>::value;

namespace detail
{

// Test the presence of requested key methods.
template <typename T>
struct is_key_impl: sfinae_types
{
	template <typename U>
	static auto test0(const U &u) -> decltype(u.is_compatible(std::declval<symbol_set const &>()));
	static no test0(...);
	template <typename U>
	static auto test1(const U &u) -> decltype(u.is_ignorable(std::declval<symbol_set const &>()));
	static no test1(...);
	template <typename U>
	static auto test2(const U &u) -> decltype(u.merge_args(std::declval<symbol_set const &>(),std::declval<symbol_set const &>()));
	static no test2(...);
	template <typename U>
	static auto test3(const U &u) -> decltype(u.is_unitary(std::declval<symbol_set const &>()));
	static no test3(...);
	template <typename U>
	static auto test4(const U &u) -> decltype(u.print(std::declval<std::ostream &>(),std::declval<symbol_set const &>()),void(),yes());
	static no test4(...);
	template <typename U>
	static auto test5(const U &u) -> decltype(u.print_tex(std::declval<std::ostream &>(),std::declval<symbol_set const &>()),void(),yes());
	static no test5(...);
	static const bool value = std::is_same<bool,decltype(test0(std::declval<T>()))>::value &&
				  std::is_same<bool,decltype(test1(std::declval<T>()))>::value &&
				  std::is_same<T,decltype(test2(std::declval<T>()))>::value &&
				  std::is_same<bool,decltype(test3(std::declval<T>()))>::value &&
				  std::is_same<yes,decltype(test4(std::declval<T>()))>::value &&
				  std::is_same<yes,decltype(test5(std::declval<T>()))>::value;
};

}

/// Type trait to detect key types.
/**
 * This type trait will be \p true if \p T can be used as a key type, \p false otherwise.
 * The requisites for \p T are the following:
 * 
 * - it must satisfy piranha::is_container_element,
 * - it must be constructible from a const piranha::symbol_set reference,
 * - it must satisfy piranha::is_equality_comparable,
 * - it must satisfy piranha::is_hashable,
 * - it must be provided with a const non-throwing \p is_compatible method accepting a const piranha::symbol_set
 *   reference as input and returning a \p bool,
 * - it must be provided with a const non-throwing \p is_ignorable method accepting a const piranha::symbol_set
 *   reference as input and returning \p bool,
 * - it must be provided with a const \p merge_args method accepting two const piranha::symbol_set
 *   references as inputs and returning \p T,
 * - it must be provided with a const \p is_unitary method accepting a const piranha::symbol_set
 *   reference as input and returning \p bool,
 * - it must be provided with const \p print and \p print_tex methods accepting an \p std::ostream reference as first argument
 *   and a const piranha::symbol_set reference as second argument.
 * 
 * \todo requirements on default-constructed key: must it be unitary? --> this one is probably not needed
 * \todo requirements on vector-of-symbols-constructed key: must it be unitary? (seems like it, look at
 * polynomial ctors from symbol) -> note that both these two checks have to go in the runtime requirements of key
 * when they get documented.
 */
template <typename T, typename = void>
class is_key
{
	public:
		/// Value of the type trait.
		static const bool value = false;
};

template <typename T>
class is_key<T,typename std::enable_if<detail::is_key_impl<T>::value>::type>
{
	public:
		static const bool value = is_container_element<T>::value &&
					  std::is_constructible<T,const symbol_set &>::value &&
					  is_equality_comparable<T>::value &&
					  is_hashable<T>::value &&
					  noexcept(std::declval<T const &>().is_compatible(std::declval<symbol_set const &>())) &&
					  noexcept(std::declval<T const &>().is_ignorable(std::declval<symbol_set const &>()));
};

template <typename T, typename Enable>
const bool is_key<T,Enable>::value;

template <typename T>
const bool is_key<T,typename std::enable_if<detail::is_key_impl<T>::value>::type>::value;

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
class has_typedef_##type_name: piranha::detail::sfinae_types \
{ \
		template <typename T> \
		static auto test(const T &) -> decltype(std::declval<typename T::type_name>(),void(),yes()); \
		static no test(...); \
	public: \
		static const bool value = std::is_same<yes,decltype(test(std::declval<PIRANHA_DECLARE_HAS_TYPEDEF_ARGUMENT>()))>::value; \
}

/// Macro for static type trait checks.
/**
 * This macro will check via a \p static_assert that the template type trait \p tt provides a \p true \p value.
 * The variadic arguments are interpreted as the template arguments of \p tt.
 */
#define PIRANHA_TT_CHECK(tt,...) static_assert(tt<__VA_ARGS__>::value,"type trait check failure -> "#tt"<"#__VA_ARGS__">")

namespace piranha
{

namespace detail
{

// MP to check that all types in a tuple are the same.
template <std::size_t I, typename Tuple, typename = void>
struct tih_impl
{
	static const bool value = std::is_same<typename std::tuple_element<I,Tuple>::type,
				  typename std::tuple_element<I + 1u,Tuple>::type>::value &&
				  tih_impl<I + 1u,Tuple>::value;
};

template <std::size_t I, typename Tuple>
struct tih_impl<I,Tuple,typename std::enable_if<I == std::tuple_size<Tuple>::value - 1u>::type>
{
	static const bool value = true;
};

template <typename Tuple>
struct tuple_is_homogeneous
{
	static const bool value = tih_impl<0u,Tuple>::value;
};

template <>
struct tuple_is_homogeneous<std::tuple<>>
{
	static const bool value = true;
};

}

/// Type trait for multipliable terms.
/**
 * In order for this type trait to evaluate to \p true, a term type must satisfy the following conditions:
 * - it must be provided with a \p typedef \p multiplication_result_type which is either \p Term itself or a tuple of types \p Term of nonzero size,
 *   used to represent the result of the multiplication by another term,
 * - it must be provided with a const member function <tt>multiply()</tt> accepting as first parameter a mutable reference
 *   to an instance of type \p multiplication_result_type, as second parameter a const reference to another term instance and
 *   as third parameter a const reference to a piranha::symbol_set.
 *
 * This type trait requires \p Term to satisfy piranha::is_term.
 * \todo: replace the multiplication_result_type detection with the detection of member function signature, something like this:
 * http://stackoverflow.com/questions/19387339/type-deduction-given-member-function-pointer-with-variadic-templates
 */
template <typename Term>
class term_is_multipliable: detail::sfinae_types
{
		PIRANHA_TT_CHECK(is_term,Term);
		template <typename U>
		static auto test(const U &u) -> decltype(u.multiply(std::declval<typename U::multiplication_result_type &>(),
			std::declval<U const &>(),std::declval<symbol_set const &>()),void(),yes());
		static no test(...);
		PIRANHA_DECLARE_HAS_TYPEDEF(multiplication_result_type);
		template <typename T, typename = void>
		struct check_result_type
		{
			static const bool value = false;
		};
		template <typename T>
		struct check_result_type<T,typename std::enable_if<has_typedef_multiplication_result_type<T>::value>::type>
		{
			template <typename U>
			struct tuple_check
			{
				static const bool value = false;
			};
			template <typename ... Args>
			struct tuple_check<std::tuple<Args...>>
			{
				static const bool value = sizeof...(Args) > 0u && detail::tuple_is_homogeneous<std::tuple<Args...>>::value;
			};
			typedef typename T::multiplication_result_type res_type;
			static const bool value = std::is_same<res_type,T>::value || tuple_check<res_type>::value;
		};
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test(std::declval<Term>())),yes>::value &&
					  check_result_type<Term>::value;
};

template <typename Term>
const bool term_is_multipliable<Term>::value;

namespace detail
{

template <typename T, typename ... Args>
struct min_int_impl
{
	using next = typename min_int_impl<Args...>::type;
	static_assert((std::is_unsigned<T>::value && std::is_unsigned<next>::value && std::is_integral<next>::value) ||
		(std::is_signed<T>::value && std::is_signed<next>::value && std::is_integral<next>::value),"The type trait's arguments must all be (un)signed integers.");
	using type = typename std::conditional<(
		std::numeric_limits<T>::max() < std::numeric_limits<next>::max() &&
		(std::is_unsigned<T>::value || std::numeric_limits<T>::min() > std::numeric_limits<next>::min())),
		T,
		next
		>::type;
};

template <typename T>
struct min_int_impl<T>
{
	static_assert(std::is_integral<T>::value,"The type trait's arguments must all be (un)signed integers.");
	using type = T;
};

template <typename T, typename ... Args>
struct max_int_impl
{
	using next = typename max_int_impl<Args...>::type;
	static_assert((std::is_unsigned<T>::value && std::is_unsigned<next>::value && std::is_integral<next>::value) ||
		(std::is_signed<T>::value && std::is_signed<next>::value && std::is_integral<next>::value),"The type trait's arguments must all be (un)signed integers.");
	using type = typename std::conditional<(
		std::numeric_limits<T>::max() > std::numeric_limits<next>::max() &&
		(std::is_unsigned<T>::value || std::numeric_limits<T>::min() < std::numeric_limits<next>::min())),
		T,
		next
		>::type;
};

template <typename T>
struct max_int_impl<T>
{
	static_assert(std::is_integral<T>::value,"The type trait's arguments must all be (un)signed integers.");
	using type = T;
};

}

/// Detect narrowest integer type
/**
 * This type alias requires \p T and \p Args (if any) to be all signed or unsigned integer types.
 * It will be defined as the input type with the narrowest numerical range.
 */
template <typename T, typename ... Args>
using min_int = typename detail::min_int_impl<T,Args...>::type;

/// Detect widest integer type
/**
 * This type alias requires \p T and \p Args (if any) to be all signed or unsigned integer types.
 * It will be defined as the input type with the widest numerical range.
 */
template <typename T, typename ... Args>
using max_int = typename detail::max_int_impl<T,Args...>::type;

namespace detail
{

// Detect the availability of std::iterator_traits on type It, plus a couple more requisites from the
// iterator concept.
// NOTE: this needs also the is_swappable type trait, but this seems to be difficult to implement in C++11. Mostly because
// it seems that:
// - we cannot detect a specialised std::swap (so if it is not specialised, it will pick the default implementation
//   which could fail in the implementation without giving hints in the prototype),
// - it's tricky to fulfill the requirement that swap has to be called unqualified (cannot use 'using std::swap' within a decltype()
//   SFINAE, might be doable with automatic return type deduction for regular functions in C++14?).
template <typename It>
struct has_iterator_traits
{
	using it_tags = std::tuple<std::input_iterator_tag,std::output_iterator_tag,std::forward_iterator_tag,
		std::bidirectional_iterator_tag,std::random_access_iterator_tag>;
	PIRANHA_DECLARE_HAS_TYPEDEF(difference_type);
	PIRANHA_DECLARE_HAS_TYPEDEF(value_type);
	PIRANHA_DECLARE_HAS_TYPEDEF(pointer);
	PIRANHA_DECLARE_HAS_TYPEDEF(reference);
	PIRANHA_DECLARE_HAS_TYPEDEF(iterator_category);
	using i_traits = std::iterator_traits<It>;
	static const bool value = has_typedef_reference<i_traits>::value && has_typedef_value_type<i_traits>::value &&
		has_typedef_pointer<i_traits>::value && has_typedef_difference_type<i_traits>::value  &&
		has_typedef_iterator_category<i_traits>::value && std::is_copy_constructible<It>::value &&
		std::is_copy_assignable<It>::value && std::is_destructible<It>::value;
};

template <typename It>
const bool has_iterator_traits<It>::value;

// TMP to check if a type is convertible to a type in the tuple.
template <typename T, typename Tuple, std::size_t I = 0u, typename Enable = void>
struct convertible_type_in_tuple
{
	static_assert(I < std::numeric_limits<std::size_t>::max(),"Overflow error.");
	static const bool value = std::is_convertible<T,typename std::tuple_element<I,Tuple>::type>::value ||
		convertible_type_in_tuple<T,Tuple,I + 1u>::value;
};

template <typename T, typename Tuple, std::size_t I>
struct convertible_type_in_tuple<T,Tuple,I,typename std::enable_if<I == std::tuple_size<Tuple>::value>::type>
{
	static const bool value = false;
};

template <typename T, typename = void>
struct is_iterator_impl
{
	static const bool value = false;
};

// NOTE: here the correct condition is the commented one, as opposed to the first one appearing. However, it seems like
// there are inconsistencies between the commented condition and the definition of many output iterators in the standard library:
//
// http://stackoverflow.com/questions/23567244/apparent-inconsistency-in-iterator-requirements
//
// Until this is clarified, it is probably better to keep this workaround.
template <typename T>
struct is_iterator_impl<T,typename std::enable_if</*std::is_same<typename std::iterator_traits<T>::reference,decltype(*std::declval<T &>())>::value &&*/
	// That is the one that would need to be replaced with the one above. Just check that operator*() is defined.
	std::is_same<decltype(*std::declval<T &>()),decltype(*std::declval<T &>())>::value &&
	std::is_same<decltype(++std::declval<T &>()),T &>::value && has_iterator_traits<T>::value &&
	// NOTE: here we used to have type_in_tuple, but it turns out Boost.iterator defines its own set of tags derived from the standard
	// ones. Hence, check that the category can be converted to one of the standard categories. This should not change anything for std iterators,
	// and just enable support for Boost ones.
	convertible_type_in_tuple<typename std::iterator_traits<T>::iterator_category,typename has_iterator_traits<T>::it_tags>::value>::type>
{
	static const bool value = true;
};

}

/// Iterator type trait.
/**
 * This type trait will be \p true if the decayed type of \p T satisfies the compile-time requirements of an iterator (as defined by the C++ standard),
 * \p false otherwise.
 */
template <typename T>
struct is_iterator
{
	/// Value of the type trait.
	static const bool value = detail::is_iterator_impl<typename std::decay<T>::type>::value;
};

template <typename T>
const bool is_iterator<T>::value;

namespace detail
{

template <typename T, typename = void>
struct is_input_iterator_impl
{
	static const bool value = false;
};

template <typename T, typename = void>
struct arrow_operator_type
{};

template <typename T>
struct arrow_operator_type<T,typename std::enable_if<std::is_pointer<T>::value>::type>
{
	using type = T;
};

template <typename T>
struct arrow_operator_type<T,typename std::enable_if<std::is_same<
	typename arrow_operator_type<decltype(std::declval<T &>().operator->())>::type,
	typename arrow_operator_type<decltype(std::declval<T &>().operator->())>::type
	>::value>::type>
{
	using type = typename arrow_operator_type<decltype(std::declval<T &>().operator->())>::type;
};

template <typename T>
struct is_input_iterator_impl<T,typename std::enable_if<is_iterator_impl<T>::value && is_equality_comparable<T>::value &&
	std::is_convertible<decltype(*std::declval<T &>()),typename std::iterator_traits<T>::value_type>::value &&
	std::is_same<decltype(++std::declval<T &>()),T &>::value &&
	std::is_same<decltype((void)std::declval<T &>()++),decltype((void)std::declval<T &>()++)>::value &&
	std::is_convertible<decltype(*std::declval<T &>()++),typename std::iterator_traits<T>::value_type>::value &&
	// NOTE: here we know that the arrow op has to return a pointer, if implemented correctly, and that the syntax
	// it->m must be equivalent to (*it).m. This means that, barring differences in reference qualifications,
	// it-> and *it must return the same thing.
	std::is_same<
		typename std::remove_reference<decltype(*std::declval<typename arrow_operator_type<T>::type>())>::type,
		typename std::remove_reference<decltype(*std::declval<T &>())>::type
	>::value &&
	// NOTE: here the usage of is_convertible guarantees we catch both iterators higher in the type hierarchy and
	// the Boost versions of standard iterators as well.
	std::is_convertible<typename std::iterator_traits<T>::iterator_category,std::input_iterator_tag>::value>::type>
{
	static const bool value = true;
};

}

/// Input iterator type trait.
/**
 * This type trait will be \p true if the decayed type of \p T satisfies the compile-time requirements of an input iterator (as defined by the C++ standard),
 * \p false otherwise.
 */
template <typename T>
struct is_input_iterator
{
	/// Value of the type trait.
	static const bool value = detail::is_input_iterator_impl<typename std::decay<T>::type>::value;
};

template <typename T>
const bool is_input_iterator<T>::value;

namespace detail
{

template <typename T, typename = void>
struct is_forward_iterator_impl
{
	static const bool value = false;
};

template <typename T>
struct is_forward_iterator_impl<T,typename std::enable_if<is_input_iterator_impl<T>::value && std::is_default_constructible<T>::value &&
	(std::is_same<typename std::iterator_traits<T>::value_type &,typename std::iterator_traits<T>::reference>::value ||
	std::is_same<typename std::iterator_traits<T>::value_type const &,typename std::iterator_traits<T>::reference>::value) &&
	std::is_convertible<decltype(std::declval<T &>()++),const T &>::value &&
	std::is_same<decltype(*std::declval<T &>()++),typename std::iterator_traits<T>::reference>::value &&
	std::is_convertible<typename std::iterator_traits<T>::iterator_category,std::forward_iterator_tag>::value
	>::type>
{
	static const bool value = true;
};

}

/// Forward iterator type trait.
/**
 * This type trait will be \p true if the decayed type of \p T satisfies the compile-time requirements of a forward iterator (as defined by the C++ standard),
 * \p false otherwise.
 */
template <typename T>
struct is_forward_iterator
{
	/// Value of the type trait.
	static const bool value = detail::is_forward_iterator_impl<typename std::decay<T>::type>::value;
};

template <typename T>
const bool is_forward_iterator<T>::value;

namespace detail
{

template <typename T, T cur_p = T(1), T cur_n = T(-1), typename = void>
struct safe_abs_sint_impl
{
	static const T value = safe_abs_sint_impl<T,static_cast<T>(cur_p * 2),static_cast<T>(cur_n * 2)>::value;
};

template <typename T, T cur_p, T cur_n>
struct safe_abs_sint_impl<T,cur_p,cur_n,typename std::enable_if<
	(cur_p > std::numeric_limits<T>::max() / T(2) || cur_n < std::numeric_limits<T>::min() / T(2))
	>::type>
{
	static const T value = cur_p;
};

// Determine, for the signed integer T, a value n, power of 2, such that it is safe to take -n.
template <typename T>
struct safe_abs_sint
{
	static_assert(std::is_integral<T>::value && std::is_signed<T>::value,"T must be a signed integral type.");
	static const T value = safe_abs_sint_impl<T>::value;
};

template <typename T>
const T safe_abs_sint<T>::value;

}

}

#endif
