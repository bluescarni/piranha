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
 */

#include <boost/concept/assert.hpp>
#include <boost/integer_traits.hpp>
#include <cstddef>
#include <type_traits>

#include "concepts/term.hpp"
#include "config.hpp"
#include "detail/base_series_fwd.hpp"
#include "detail/base_term_fwd.hpp"

namespace piranha
{

/// Strip reference and top-level cv-qualifiers.
/**
 * This type trait removes top-level cv-qualifiers and, if \p T is a reference, transforms it into the referred-to type.
 */
template <typename T>
struct strip_cv_ref
{
	/// Type-trait type definition.
	typedef typename std::remove_cv<typename std::remove_reference<T>::type>::type type;
};

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
template <typename T>
struct is_nothrow_move_constructible
{
	/// Type-trait value.
	static const bool value = piranha_noexcept_op(T(static_cast<T &&>(*static_cast<T *>(piranha_nullptr))));
};

template <typename T>
const bool is_nothrow_move_constructible<T>::value;

/// Type has non-throwing move assignment operator.
/**
 * Placeholder for <tt>std::is_nothrow_move_assignable</tt>, until it is implemented in GCC.
 * In GCC 4.5, it will be true by default for all types. In GCC >= 4.6, it will use
 * the \p noexcept operator.
 */
template <typename T>
struct is_nothrow_move_assignable
{
	/// Type-trait value.
	static const bool value = piranha_noexcept_op(*static_cast<T *>(piranha_nullptr) = static_cast<T &&>(*static_cast<T *>(piranha_nullptr)));
};

template <typename T>
const bool is_nothrow_move_assignable<T>::value;

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

namespace detail
{

template <typename CfSeries, std::size_t Level = 0, typename Enable = void>
struct echelon_level_impl
{
	static_assert(Level < boost::integer_traits<std::size_t>::const_max,"Overflow error.");
	static const std::size_t value = echelon_level_impl<typename CfSeries::term_type::cf_type,Level + static_cast<std::size_t>(1)>::value;
};

template <typename Cf, std::size_t Level>
struct echelon_level_impl<Cf,Level,typename std::enable_if<!std::is_base_of<base_series_tag,Cf>::value>::type>
{
	static const std::size_t value = Level;
};

}

/// Echelon size.
/**
 * Echelon size of \p Term. The echelon size is defined recursively by the number of times coefficient types are series, in \p Term
 * and its nested types.
 * 
 * For instance, polynomials have numerical coefficients, hence their echelon size is 1. Fourier series are also series with numerical coefficients,
 * hence their echelon size is also 1. Poisson series are Fourier series with polynomial coefficients, hence their echelon size is 2.
 * 
 * \section type_requirements Type requirements
 * 
 * \p Term must be a model of piranha::concept::Term.
 */
template <typename Term>
struct echelon_size
{
	private:
		BOOST_CONCEPT_ASSERT((concept::Term<Term>));
		static_assert(detail::echelon_level_impl<typename Term::cf_type>::value < boost::integer_traits<std::size_t>::const_max,"Overflow error.");
	public:
		/// Value of echelon size.
		static const std::size_t value = detail::echelon_level_impl<typename Term::cf_type>::value + static_cast<std::size_t>(1);
};

template <typename Term>
const std::size_t echelon_size<Term>::value;

namespace detail
{

template <typename Term1, typename Term2, std::size_t CurLevel = 0, typename Enable = void>
struct echelon_position_impl
{
	static_assert(std::is_base_of<base_series_tag,typename Term1::cf_type>::value,"Term type does not appear in echelon hierarchy.");
	static const std::size_t value = echelon_position_impl<typename Term1::cf_type::term_type,Term2>::value + static_cast<std::size_t>(1);
};

template <typename Term1, typename Term2, std::size_t CurLevel>
struct echelon_position_impl<Term1,Term2,CurLevel,typename std::enable_if<std::is_same<Term1,Term2>::value>::type>
{
	static const std::size_t value = CurLevel;
};

}

/// Echelon position.
/**
 * Echelon position of \p Term with respect to \p TopLevelTerm.
 * The echelon position is an index, starting from zero, corresponding to the level in the echelon hierarchy of \p TopLevelTerm
 * in which \p Term appears.
 * 
 * For instance, if \p TopLevelTerm and \p Term are the same type, then the echelon position of \p Term is 0, because \p Term is the
 * first type encountered in the echelon hierarchy of \p TopLevelTerm. If \p TopLevelTerm is a Poisson series term, then the echelon position
 * of the polynomial term type defined by the coefficient of \p TopLevelTerm is 1.
 * 
 * If \p Term does not appear in the echelon hierarchy of \p TopLevelTerm, a compile-time error will be produced.
 * 
 * \section type_requirements Type requirements
 * 
 * \p Term and \p TopLevelTerm must be models of piranha::concept::Term.
 */
template <typename TopLevelTerm, typename Term>
class echelon_position
{
	private:
		BOOST_CONCEPT_ASSERT((concept::Term<Term>));
		BOOST_CONCEPT_ASSERT((concept::Term<TopLevelTerm>));
		static_assert(std::is_same<Term,TopLevelTerm>::value || detail::echelon_position_impl<TopLevelTerm,Term>::value,
			"Assertion error in the calculation of echelon position.");
	public:
		/// Value of echelon position.
		static const std::size_t value = detail::echelon_position_impl<TopLevelTerm,Term>::value;
};

template <typename TopLevelTerm, typename Term>
const std::size_t echelon_position<TopLevelTerm,Term>::value;

/// Return type of arithmetic binary operators.
/**
 * This type-trait will define a boolean flag \p value that is \p true if the return type of binary arithmetic operators
 * \p operator+, \p operator- and \p operator* is the type of the second operand, \p false otherwise.
 * 
 * For instance, a mixed-mode binary arithmetic operation with \p int as first argument type and \p double as second argument
 * type wil return a \p double instance, and hence the value of the type-trait is \p true. If the operands are switched (i.e.,
 * the first operand is a \p double and the second one is a \p int), the type-trait's value will be \p false.
 * 
 * Default implementation will be \p true if the return type of <tt>T1 + T2</tt> is \p T2, false if it is \p T1.
 */
template <typename T1, typename T2, typename Enable = void>
class binary_op_return_type
{
		typedef decltype(*static_cast<T1 * const>(piranha_nullptr) + *static_cast<T2 * const>(piranha_nullptr)) retval_type;
	public:
		/// Type-trait's value.
		static const bool value = std::is_same<retval_type,T2>::value;
	private:
		static_assert(value || std::is_same<retval_type,T1>::value,"Invalid return value type.");
		static_assert(std::is_same<retval_type,decltype(*static_cast<T1 * const>(piranha_nullptr) - *static_cast<T2 * const>(piranha_nullptr))>::value,"Inconsistent return value type.");
		static_assert(std::is_same<retval_type,decltype(*static_cast<T1 * const>(piranha_nullptr) * *static_cast<T2 * const>(piranha_nullptr))>::value,"Inconsistent return value type.");
};

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
