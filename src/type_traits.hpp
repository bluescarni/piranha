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

#include <boost/type_traits/has_trivial_copy.hpp>
#include <boost/type_traits/has_trivial_destructor.hpp>
#include <cstdarg>
#include <cstddef>
#include <functional>
#include <ostream>
#include <tuple>
#include <type_traits>
#include <utility>

#include "detail/base_term_fwd.hpp"
#include "detail/sfinae_types.hpp"
#include "detail/symbol_set_fwd.hpp"

namespace piranha
{

/// Type trait to test if type is a tuple.
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
	/// Type trait value.
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
 * pow(), cos(), etc. and the auto-detect the type trait with SFINAE.
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

template <typename T, typename U, typename Derived>
const bool arith_tt_helper<T,U,Derived>::value;

}

/// Addable type trait.
/**
 * Will be \p true if objects of type \p T can be added to objects of type \p U using the binary addition operator.
 * 
 * This type trait will strip \p T and \p U of reference qualifiers, and it will test the operator in the form
 * @code
 * operator+(const Td &, const Ud &)
 * @endcode
 * where \p Td and \p Ud are \p T and \p U after the removal of reference qualifiers.
 * 
 * The value of the type trait is a static const boolean flag inherited by an implementation-defined base class. E.g.,
 * @code
 * is_addable<int>::value == true;
 * is_addable<int,std::string>::value == false;
 * @endcode
 */
template <typename T, typename U = T>
class is_addable: detail::sfinae_types, public detail::arith_tt_helper<T,U,is_addable<T,U>>
{
	friend class detail::arith_tt_helper<T,U,is_addable<T,U>>;
	template <typename T1, typename U1>
	static auto test(const T1 &t, const U1 &u) -> decltype(t + u,void(),yes());
	static no test(...);
};

/// In-place addable type trait.
/**
 * Will be \p true if objects of type \p U can be added in-place to objects of type \p T.
 * 
 * This type trait will strip \p T and \p U of reference qualifiers, and it will test the operator in the form
 * @code
 * operator+=(Td &, const Ud &)
 * @endcode
 * where \p Td and \p Ud are \p T and \p U after the removal of reference qualifiers.
 * 
 * The value of the type trait is a static const boolean flag inherited by an implementation-defined base class. E.g.,
 * @code
 * is_addable_in_place<int>::value == true;
 * is_addable_in_place<int,std::string>::value == false;
 * @endcode
 */
template <typename T, typename U = T>
class is_addable_in_place: detail::sfinae_types, public detail::arith_tt_helper<T,U,is_addable_in_place<T,U>>
{
	friend class detail::arith_tt_helper<T,U,is_addable_in_place<T,U>>;
	template <typename T1, typename U1>
	static auto test(T1 &t, const U1 &u) -> decltype(t += u,void(),yes());
	static no test(...);
};

/// Subtractable type trait.
/**
 * @see piranha::is_addable.
 */
template <typename T, typename U = T>
class is_subtractable: detail::sfinae_types, public detail::arith_tt_helper<T,U,is_subtractable<T,U>>
{
	friend class detail::arith_tt_helper<T,U,is_subtractable<T,U>>;
	template <typename T1, typename U1>
	static auto test(const T1 &t, const U1 &u) -> decltype(t - u,void(),yes());
	static no test(...);
};

/// In-place subtractable type trait.
/**
 * @see piranha::is_addable_in_place.
 */
template <typename T, typename U = T>
class is_subtractable_in_place: detail::sfinae_types, public detail::arith_tt_helper<T,U,is_subtractable_in_place<T,U>>
{
	friend class detail::arith_tt_helper<T,U,is_subtractable_in_place<T,U>>;
	template <typename T1, typename U1>
	static auto test(T1 &t, const U1 &u) -> decltype(t -= u,void(),yes());
	static no test(...);
};

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
 * This type trait is \p true if instances if type \p T can be compared to instances of
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

/// Type trait for well-behaved container elements.
/**
 * The type trait will be true if all these conditions hold:
 * 
 * - \p T is default-constructible,
 * - \p T is copy-constructible,
 * - \p T has nothrow move semantics,
 * - \p T is nothrow-destructible.
 */
template <typename T>
struct is_container_element
{
	/// Value of the type trait.
	static const bool value = std::is_default_constructible<T>::value &&
				  std::is_copy_constructible<T>::value &&
				  is_nothrow_destructible<T>::value &&
				  std::is_nothrow_move_constructible<T>::value &&
				  std::is_nothrow_move_assignable<T>::value;
};

template <typename T>
const bool is_container_element<T>::value;

namespace detail
{

template <template <typename ...> class TT>
struct iio_converter
{
     template <typename ... Args>
     iio_converter(const TT<Args ...> &);
};

}

/// Type trait for detecting instances of class templates.
/**
 * This type trait will be \p true if the decay type of \p T (or one of its base classes) is an instance
 * of the class template \p TT, \p false otherwise. E.g.,
 * @code
 * is_instance_of<std::vector<int>,std::vector>::value == true;
 * is_instance_of<std::ostream,std::basic_ios>::value == true;
 * is_instance_of<int,std::list>::value == false;
 * @endcode
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
 * @code
 * std::ostream &operator<<(std::ostream &, const T &)
 * @endcode
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
 * This type trait will be true if \p T (disregarding reference qualifiers) is a function object returning \p ReturnType and taking
 * \p Args as arguments. That is, the type trait will be \p true if the following conditions are met:
 * - \p T is a class,
 * - \p T is equipped with a call operator returning \p ReturnType and taking \p Args as arguments.
 * 
 * \p T can be const qualified (in which case the call operator must be also const in order for the type trait to be satisfied).
 */
template <typename T, typename ReturnType, typename ... Args>
class is_function_object
{
		typedef typename std::remove_reference<T>::type Td;
	public:
		/// Value of the type trait.
		static const bool value = detail::is_function_object_impl<Td,ReturnType>::template tt<Args...>::value;
};

template <typename T, typename ReturnType, typename ... Args>
const bool is_function_object<T,ReturnType,Args...>::value;

/// Type trait to detect hash function objects.
/**
 * \p T is a hash function object for \p U if the following requirements are met:
 * - \p T is a function object with const noexcept call operator accepting as input const \p U and returning \p std::size_t,
 * - \p T is nothrow default constructible and it satisfies piranha::is_container_element.
 * 
 * The decay types of \p T and \p U are considered in this type trait.
 */
template <typename T, typename U, typename = void>
class is_hash_function_object
{
	public:
		/// Value of the type trait.
		static const bool value = false;
};

template <typename T, typename U>
class is_hash_function_object<T,U,typename std::enable_if<is_function_object<const typename std::decay<T>::type,
	std::size_t,typename std::decay<U>::type const &>::value>::type>
{
		typedef typename std::decay<T>::type Td;
		typedef typename std::decay<U>::type Ud;
	public:
		static const bool value = detail::common_hasher_requirements<Td,Ud>::value;
};

template <typename T, typename U, typename Enable>
const bool is_hash_function_object<T,U,Enable>::value;

template <typename T, typename U>
const bool is_hash_function_object<T,U,typename std::enable_if<is_function_object<const typename std::decay<T>::type,
	std::size_t,typename std::decay<U>::type const &>::value>::type>::value;

/// Type trait to detect equality function objects.
/**
 * \p T is an equality function object for \p U if the following requirements are met:
 * - \p T is a function object with const noexcept call operator accepting as input two const \p U and returning \p bool,
 * - \p T satisfies piranha::is_container_element.
 * 
 * The decay types of \p T and \p U are considered in this type trait.
 */
template <typename T, typename U, typename = void>
class is_equality_function_object
{
	public:
		/// Value of the type trait.
		static const bool value = false;
};

template <typename T, typename U>
class is_equality_function_object<T,U,typename std::enable_if<is_function_object<const typename std::decay<T>::type,
	bool,typename std::decay<U>::type const &,typename std::decay<U>::type const &>::value>::type>
{
		typedef typename std::decay<T>::type Td;
		typedef typename std::decay<U>::type Ud;
	public:
		static const bool value = is_container_element<Td>::value;
};

template <typename T, typename U, typename Enable>
const bool is_equality_function_object<T,U,Enable>::value;

template <typename T, typename U>
const bool is_equality_function_object<T,U,typename std::enable_if<is_function_object<const typename std::decay<T>::type,
	bool,typename std::decay<U>::type const &,typename std::decay<U>::type const &>::value>::type>::value;

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
 * This type trait will be \p true if the decay type of \p T can be used as a key type, \p false otherwise.
 * The requisites for the decay type of \p T are the following:
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
class is_key<T,typename std::enable_if<detail::is_key_impl<typename std::decay<T>::type>::value>::type>
{
		typedef typename std::decay<T>::type Td;
	public:
		static const bool value = is_container_element<Td>::value &&
					  std::is_constructible<Td,const symbol_set &>::value &&
					  is_equality_comparable<Td>::value &&
					  is_hashable<Td>::value &&
					  noexcept(std::declval<Td const &>().is_compatible(std::declval<symbol_set const &>())) &&
					  noexcept(std::declval<Td const &>().is_ignorable(std::declval<symbol_set const &>()));
};

template <typename T, typename Enable>
const bool is_key<T,Enable>::value;

template <typename T>
const bool is_key<T,typename std::enable_if<detail::is_key_impl<typename std::decay<T>::type>::value>::type>::value;

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

/// Macro for static type trait checks.
/**
 * This macro will check via a \p static_assert that the template type trait \p tt provides a \p true \p value.
 * The variadic arguments are interpreted as the template arguments of \p tt.
 */
#define PIRANHA_TT_CHECK(tt,...) static_assert(tt<__VA_ARGS__>::value,"type trait check failure -> "#tt"<"#__VA_ARGS__">")

#endif
