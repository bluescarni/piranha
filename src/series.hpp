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

#ifndef PIRANHA_SERIES_HPP
#define PIRANHA_SERIES_HPP

#include <algorithm>
#include <boost/any.hpp>
#include <boost/iterator/indirect_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <iostream>
#include <iterator>
#include <limits>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base_term.hpp"
#include "config.hpp"
#include "debug_access.hpp"
#include "detail/sfinae_types.hpp"
#include "detail/series_fwd.hpp"
#include "environment.hpp"
#include "hash_set.hpp"
#include "math.hpp" // For negate() and math specialisations.
#include "mp_integer.hpp"
#include "print_coefficient.hpp"
#include "print_tex_coefficient.hpp"
#include "safe_cast.hpp"
#include "serialization.hpp"
#include "series_multiplier.hpp"
#include "settings.hpp"
#include "symbol_set.hpp"
#include "symbol.hpp"
#include "tracing.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

// Hash functor for term type in series.
template <typename Term>
struct term_hasher
{
	std::size_t operator()(const Term &term) const noexcept
	{
		return term.hash();
	}
};

// NOTE: this needs to go here, instead of in the series class as private method,
// because of a bug in GCC 4.7:
// http://gcc.gnu.org/bugzilla/show_bug.cgi?id=53137
template <typename Term, typename Derived>
inline std::pair<typename Term::cf_type,Derived> pair_from_term(const symbol_set &s, const Term &t)
{
	typedef typename Term::cf_type cf_type;
	Derived retval;
	retval.m_symbol_set = s;
	retval.insert(Term(cf_type(1),t.m_key));
	return std::make_pair(t.m_cf,std::move(retval));
}

}

/// Type trait to detect series types.
/**
 * This type trait will be true if \p T is an instance of piranha::series and it satisfies piranha::is_container_element.
 */
template <typename T>
class is_series
{
	public:
		/// Value of the type trait.
		static const bool value = std::is_base_of<detail::series_tag,T>::value;
};

template <typename T>
const bool is_series<T>::value;

/// Check if a series can be rebound.
/**
 * \note
 * This class requires \p T to satisfy piranha::is_series and \p Cf to satisfy piranha::is_cf.
 *
 * Rebinding a series type \p T means to change its coefficient type to \p Cf. The ability of a series
 * to be rebound is signalled by the presence of a <tt>rebind</tt> template alias defined within the series class.
 *
 * A series is considered rebindable if all the following conditions hold:
 * - there exists a template alias <tt>rebind</tt> in the series type \p T,
 * - <tt>T::rebind<Cf></tt> is a series types,
 * - the coefficient type of <tt>T::rebind<Cf></tt> is \p Cf.
 *
 * The decay types of \p T and \p Cf are considered in this type trait.
 */
template <typename T, typename Cf>
class series_is_rebindable: detail::sfinae_types
{
#if !defined(PIRANHA_DOXYGEN_INVOKED)
		using Td = typename std::decay<T>::type;
		using Cfd = typename std::decay<Cf>::type;
		static_assert(is_series<Td>::value,"This type trait can only be used on series types.");
		static_assert(is_cf<Cfd>::value,"This type trait requires Cf to be a coefficient type.");
		template <typename T1>
		using rebound_type = typename T1::template rebind<Cfd>;
		template <typename T1, typename std::enable_if<is_series<rebound_type<T1>>::value &&
			std::is_same<typename rebound_type<T1>::term_type::cf_type,Cfd>::value,int>::type = 0>
		static rebound_type<T1> test(const T1 &);
		static no test(...);
#endif
	public:
		/// Value of the type trait.
		static const bool value = !std::is_same<no,decltype(test(std::declval<Td>()))>::value;
};

template <typename T, typename Cf>
const bool series_is_rebindable<T,Cf>::value;

namespace detail
{

// Implementation of the series_rebind alias, with SFINAE to soft-disable it in case
// the series is not rebindable.
template <typename T, typename Cf>
using series_rebind_ = typename std::enable_if<
	series_is_rebindable<T,Cf>::value,
	typename std::decay<T>::type::template rebind<typename std::decay<Cf>::type>
>::type;

}

/// Rebind series.
/**
 * Utility alias to rebind series \p T to coefficient \p Cf. See piranha::series_is_rebindable for an explanation.
 * In addition to being a shortcut to the \p rebind alias in \p T (if present), the implementation will
 * also check that \p T and \p Cf satisfy the piranha::series_is_rebindable type traits.
 */
template <typename T, typename Cf>
using series_rebind = detail::series_rebind_<T,Cf>;

/// Series recursion index.
/**
 * The recursion index measures how many times the coefficient of a series types is itself a series type.
 * The algorithm works as follows:
 * - if \p T is not a series type, the recursion index is 0,
 * - else, the recursion index of \p T is the recursion index of its coefficient type plus one.
 *
 * The decay type of \p T is considered in this type trait.
 */
template <typename T, typename = void>
class series_recursion_index
{
	public:
		/// Value of the recursion index.
		static const std::size_t value = 0u;
};

template <typename T, typename Enable>
const std::size_t series_recursion_index<T,Enable>::value;

template <typename T>
class series_recursion_index<T,typename std::enable_if<is_series<typename std::decay<T>::type>::value>::type>
{
		using cf_type = typename std::decay<T>::type::term_type::cf_type;
		static_assert(series_recursion_index<cf_type>::value < std::numeric_limits<std::size_t>::max(),
			"Overflow error.");
	public:
		static const std::size_t value = is_series<cf_type>::value ? series_recursion_index<cf_type>::value + 1u : 1u;
};

template <typename T>
const std::size_t series_recursion_index<T,typename std::enable_if<is_series<typename std::decay<T>::type>::value>::type>::value;

namespace detail
{

// Some notes on this machinery:
// - this is only for determining the type of the result, but it does not guarantee that we can actually compute it.
//   In general we should separate the algorithmic requirements from the determination of the type. Note that we still use
//   the operators on the coefficients to determine the return type, but that's inevitable.

// Alias for getting the cf type from a series. Will generate a type error if S is not a series.
// NOTE: the recursion index != 0 and is_series checks are an extra safe guard to really assert we are
// operating on series. Without the checks, in theory classes with internal term_type and similar typedefs
// could trigger no errors.
template <typename S>
using bso_cf_t = typename std::enable_if<is_series<S>::value,typename S::term_type::cf_type>::type;

// Result of a generic arithmetic binary operation:
// - 0 for addition,
// - 1 for subtraction,
// - 2 for multiplication,
// - 3 for division.
// No type is defined if the operation is not supported by the involved types.
template <typename, typename, int, typename = void>
struct op_result
{};

template <typename T, typename U>
struct op_result<T,U,0,typename std::enable_if<is_addable<T,U>::value>::type>
{
	using type = decltype(std::declval<const T &>() + std::declval<const U &>());
};

template <typename T, typename U>
struct op_result<T,U,1,typename std::enable_if<is_subtractable<T,U>::value>::type>
{
	using type = decltype(std::declval<const T &>() - std::declval<const U &>());
};

template <typename T, typename U>
struct op_result<T,U,2,typename std::enable_if<is_multipliable<T,U>::value>::type>
{
	using type = decltype(std::declval<const T &>() * std::declval<const U &>());
};

template <typename T, typename U>
struct op_result<T,U,3,typename std::enable_if<is_divisible<T,U>::value>::type>
{
	using type = decltype(std::declval<const T &>() / std::declval<const U &>());
};

// Coefficient type in a binary arithmetic operation between two series with the same recursion index.
// Will generate a type error if S1 or S2 are not series with same recursion index or if their coefficients
// do not support the operation.
template <typename S1, typename S2, int N>
using bso_cf_op_t = typename std::enable_if<
	series_recursion_index<S1>::value == series_recursion_index<S2>::value && series_recursion_index<S1>::value != 0u,
	typename op_result<bso_cf_t<S1>,bso_cf_t<S2>,N>::type
>::type;

// Coefficient type in a mixed binary arithmetic operation in which the first operand has recursion index
// greater than the second. Will generate a type error if S does not have a rec. index > T, or if the coefficient
// of S cannot be opped to T.
template <typename S, typename T, int N>
using bsom_cf_op_t = typename std::enable_if<
	(series_recursion_index<S>::value > series_recursion_index<T>::value),
	typename op_result<bso_cf_t<S>,T,N>::type
>::type;

// Default case is empty for SFINAE.
template <typename, typename, int, typename = void>
struct binary_series_op_return_type
{};

// Case 0:
// a. both are series with same recursion index,
// b. the coefficients are the same and the op results in the same coefficient,
// c. the two series types are equal.
template <typename S1, typename S2, int N>
struct binary_series_op_return_type<S1,S2,N,typename std::enable_if<
	// NOTE: in all these checks, every access to S1 and S2 should not incur in hard errors:
	// - the series recursion index works on all types safely,
	// - the various _t aliases are SFINAE friendly - if S1 or S2 are not series with the appropriate characteristics,
	//   or the coefficients are not oppable, etc., there should be a soft error.
	/*a*/ series_recursion_index<S1>::value == series_recursion_index<S2>::value && series_recursion_index<S1>::value != 0u &&
	/*b*/ std::is_same<bso_cf_op_t<S1,S2,N>,bso_cf_t<S1>>::value && std::is_same<bso_cf_op_t<S1,S2,N>,bso_cf_t<S2>>::value &&
	/*c*/ std::is_same<S1,S2>::value
	>::type>
{
	using type = S1;
	static const unsigned value = 0u;
};

// Case 1:
// a. both are series with same recursion index,
// b. the coefficients are not the same and the op results in the first coefficient.
template <typename S1, typename S2, int N>
struct binary_series_op_return_type<S1,S2,N,typename std::enable_if<
	/*a*/ series_recursion_index<S1>::value == series_recursion_index<S2>::value && series_recursion_index<S1>::value != 0u &&
	/*b*/ std::is_same<bso_cf_op_t<S1,S2,N>,bso_cf_t<S1>>::value && !std::is_same<bso_cf_t<S1>,bso_cf_t<S2>>::value
	>::type>
{
	using type = S1;
	static const unsigned value = 1u;
};

// Case 2:
// a. both are series with same recursion index,
// b. the coefficients are not the same and the op results in the second coefficient.
template <typename S1, typename S2, int N>
struct binary_series_op_return_type<S1,S2,N,typename std::enable_if<
	/*a*/ series_recursion_index<S1>::value == series_recursion_index<S2>::value && series_recursion_index<S1>::value != 0u &&
	/*b*/ std::is_same<bso_cf_op_t<S1,S2,N>,bso_cf_t<S2>>::value && !std::is_same<bso_cf_t<S1>,bso_cf_t<S2>>::value
	>::type>
{
	using type = S2;
	static const unsigned value = 2u;
};

// Case 3:
// a. both are series with same recursion index,
// b. the coefficient op results in something else than first or second cf,
// c. both series are rebindable to the new coefficient, yielding the same series.
template <typename S1, typename S2, int N>
struct binary_series_op_return_type<S1,S2,N,typename std::enable_if<
	/*a*/ series_recursion_index<S1>::value == series_recursion_index<S2>::value && series_recursion_index<S1>::value != 0u &&
	/*b*/ !std::is_same<bso_cf_op_t<S1,S2,N>,bso_cf_t<S1>>::value && !std::is_same<bso_cf_op_t<S1,S2,N>,bso_cf_t<S2>>::value &&
	/*c*/ std::is_same<series_rebind<S1,bso_cf_op_t<S1,S2,N>>,series_rebind<S2,bso_cf_op_t<S1,S2,N>>>::value
	>::type>
{
	using type = series_rebind<S1,bso_cf_op_t<S1,S2,N>>;
	static const unsigned value = 3u;
};

// Case 4:
// a. the first operand has recursion index greater than the second operand,
// b. the op of the coefficient of S1 by S2 results in the coefficient of S1.
template <typename S1, typename S2, int N>
struct binary_series_op_return_type<S1,S2,N,typename std::enable_if<
	/*a*/ (series_recursion_index<S1>::value > series_recursion_index<S2>::value) &&
	/*b*/ std::is_same<bsom_cf_op_t<S1,S2,N>,bso_cf_t<S1>>::value
	>::type>
{
	using type = S1;
	static const unsigned value = 4u;
};

// Case 5:
// a. the first operand has recursion index greater than the second operand,
// b. the op of the coefficient of S1 by S2 results in a type T different from the coefficient of S1,
// c. S1 is rebindable to T.
template <typename S1, typename S2, int N>
struct binary_series_op_return_type<S1,S2,N,typename std::enable_if<
	/*a*/ (series_recursion_index<S1>::value > series_recursion_index<S2>::value) &&
	/*b*/ !std::is_same<bsom_cf_op_t<S1,S2,N>,bso_cf_t<S1>>::value &&
	/*c*/ series_is_rebindable<S1,bsom_cf_op_t<S1,S2,N>>::value
	>::type>
{
	using type = series_rebind<S1,bsom_cf_op_t<S1,S2,N>>;
	static const unsigned value = 5u;
};

// Case 6 (symmetric of 4):
// a. the second operand has recursion index greater than the first operand,
// b. the op of the coefficient of S2 by S1 results in the coefficient of S2.
template <typename S1, typename S2, int N>
struct binary_series_op_return_type<S1,S2,N,typename std::enable_if<
	/*a*/ (series_recursion_index<S2>::value > series_recursion_index<S1>::value) &&
	/*b*/ std::is_same<bsom_cf_op_t<S2,S1,N>,bso_cf_t<S2>>::value
	>::type>
{
	using type = S2;
	static const unsigned value = 6u;
};

// Case 7 (symmetric of 5):
// a. the second operand has recursion index greater than the first operand,
// b. the op of the coefficient of S2 by S1 results in a type T different from the coefficient of S2,
// c. S2 is rebindable to T.
template <typename S1, typename S2, int N>
struct binary_series_op_return_type<S1,S2,N,typename std::enable_if<
	/*a*/ (series_recursion_index<S2>::value > series_recursion_index<S1>::value) &&
	/*b*/ !std::is_same<bsom_cf_op_t<S2,S1,N>,bso_cf_t<S2>>::value &&
	/*c*/ series_is_rebindable<S2,bsom_cf_op_t<S2,S1,N>>::value
	>::type>
{
	using type = series_rebind<S2,bsom_cf_op_t<S2,S1,N>>;
	static const unsigned value = 7u;
};

}

class series_operators
{
		// A couple of handy aliases.
		// NOTE: we need both bso_type and common_type because bso gives access to the ::value
		// member to decide on the implementation, common_type is handy because it's shorter
		// than using ::type on the bso.
		template <typename T, typename U, int N>
		using bso_type = detail::binary_series_op_return_type<typename std::decay<T>::type,
			typename std::decay<U>::type,N>;
		template <typename T, typename U, int N>
		using series_common_type = typename bso_type<T,U,N>::type;
		// Main implementation function for add/sub: all the possible cases end up here eventually, after
		// any necessary conversion.
		template <bool Sign, typename T, typename U>
		static series_common_type<T,U,0> binary_add_impl(T &&x, U &&y)
		{
			// This is the same as T and U.
			using ret_type = series_common_type<T,U,0>;
			static_assert(std::is_same<typename std::decay<T>::type,ret_type>::value,"Invalid type.");
			static_assert(std::is_same<typename std::decay<U>::type,ret_type>::value,"Invalid type.");
			// Init the return value from the first operand.
			// NOTE: this is always possible to do, a series is always copy/move-constructible.
			ret_type retval(std::forward<T>(x));
			// NOTE: here we cover the corner case in which x and y are the same object.
			// This could be problematic in the algorithm below if retval was move-cted from
			// x, since in such a case y will also be erased. This will happen if one does
			// std::move(a) + std::move(a)
			if (unlikely(&x == &y)) {
				retval.template merge_terms<Sign>(retval);
				return retval;
			}
			// NOTE: we do not use x any more, as it might have been moved.
			if (likely(retval.m_symbol_set == y.m_symbol_set)) {
				retval.template merge_terms<Sign>(std::forward<U>(y));
			} else {
				// Let's fix the args of the first series, if needed.
				auto merge = retval.m_symbol_set.merge(y.m_symbol_set);
				if (merge != retval.m_symbol_set) {
					// This is a move assignment, always possible.
					retval = retval.merge_arguments(merge);
				}
				// Fix the args of the second series.
				if (merge != y.m_symbol_set) {
					retval.template merge_terms<Sign>(y.merge_arguments(merge));
				} else {
					retval.template merge_terms<Sign>(std::forward<U>(y));
				}
			}
			return retval;
		}
		// NOTE: this case has no special algorithmic requirements, the base requirements for cf and series types
		// already cover everything (including the needeed series constructors and cf arithmetic operators).
		template <typename T, typename U, typename std::enable_if<bso_type<T,U,0>::value == 0u,int>::type = 0>
		static series_common_type<T,U,0> dispatch_binary_add(T &&x, U &&y)
		{
			// Two-layers implementation, to optimise the case in which one series is much bigger than the other.
			if (x.size() >= y.size()) {
				return binary_add_impl<true>(std::forward<T>(x),std::forward<U>(y));
			}
			return binary_add_impl<true>(std::forward<U>(y),std::forward<T>(x));
		}
		// NOTE: this covers two cases:
		// - equal recursion, first series wins,
		// - first higher recursion, coefficient of first series wins,
		// In both cases we need to construct a T from y.
		template <typename T, typename U, typename std::enable_if<(bso_type<T,U,0>::value == 1u || bso_type<T,U,0>::value == 4u) &&
			// In order for the implementation to work, we need to be able to build T from U.
			std::is_constructible<typename std::decay<T>::type,const typename std::decay<U>::type &>::value,
			int>::type = 0>
		static series_common_type<T,U,0> dispatch_binary_add(T &&x, U &&y)
		{
			typename std::decay<T>::type y1(std::forward<U>(y));
			return dispatch_binary_add(std::forward<T>(x),std::move(y1));
		}
		// Symmetric of 1.
		template <typename T, typename U, typename std::enable_if<bso_type<T,U,0>::value == 2u,int>::type = 0>
		static auto dispatch_binary_add(T &&x, U &&y) -> decltype(dispatch_binary_add(std::forward<U>(y),std::forward<T>(x)))
		{
			return dispatch_binary_add(std::forward<U>(y),std::forward<T>(x));
		}
		// Two cases:
		// - equal series recursion, 3rd coefficient type generated,
		// - first higher recursion, coefficient result is different from first series.
		// In both cases we need to promote both operands to a 3rd type.
		template <typename T, typename U, typename std::enable_if<(bso_type<T,U,0>::value == 3u || bso_type<T,U,0>::value == 5u) &&
			// In order for the implementation to work, we need to be able to build the common type from T and U.
			std::is_constructible<series_common_type<T,U,0>,const typename std::decay<T>::type &>::value &&
			std::is_constructible<series_common_type<T,U,0>,const typename std::decay<U>::type &>::value,
			int>::type = 0>
		static series_common_type<T,U,0> dispatch_binary_add(T &&x, U &&y)
		{
			series_common_type<T,U,0> x1(std::forward<T>(x));
			series_common_type<T,U,0> y1(std::forward<U>(y));
			return dispatch_binary_add(std::move(x1),std::move(y1));
		}
		// 6 and 7 are symmetric cases of 4 and 5. Just reverse the operands.
		template <typename T, typename U, typename std::enable_if<bso_type<T,U,0>::value == 6u || bso_type<T,U,0>::value == 7u,int>::type = 0>
		static auto dispatch_binary_add(T &&x, U &&y) -> decltype(dispatch_binary_add(std::forward<U>(y),std::forward<T>(x)))
		{
			return dispatch_binary_add(std::forward<U>(y),std::forward<T>(x));
		}
		// NOTE: here we use the version of binary_add with const references. The idea
		// here is that any overload other than the const references one is an optimisation detail
		// and that for the operator to be enabled the "canonical" form of addition operator must be available.
		// The consequence is that if, for instance, only a move constructor is available, the operator
		// will anyway be disabled even if technically we could still perform the computation.
		template <typename T, typename U>
		using binary_add_type = decltype(dispatch_binary_add(std::declval<const typename std::decay<T>::type &>(),
			std::declval<const typename std::decay<U>::type &>()));
		// In-place add. Default implementation is to do simply x = x + y, if possible.
		template <typename T, typename U, typename std::enable_if<
			!std::is_const<T>::value && std::is_assignable<T &,binary_add_type<T,U>>::value,
			int>::type = 0>
		static T &dispatch_in_place_add(T &x, U &&y)
		{
			// NOTE: we can move x here, as it is going to be re-assigned anyway.
			x = dispatch_binary_add(std::move(x),std::forward<U>(y));
			return x;
		}
		template <typename T, typename U>
		using in_place_add_type = decltype(dispatch_in_place_add(std::declval<typename std::decay<T>::type &>(),
			std::declval<const typename std::decay<U>::type &>()));
		// Subtraction.
		template <typename T, typename U, typename std::enable_if<bso_type<T,U,1>::value == 0u,int>::type = 0>
		static series_common_type<T,U,1> dispatch_binary_sub(T &&x, U &&y)
		{
			if (x.size() > y.size()) {
				return binary_add_impl<false>(std::forward<T>(x),std::forward<U>(y));
			} else {
				auto retval = binary_add_impl<false>(std::forward<U>(y),std::forward<T>(x));
				retval.negate();
				return retval;
			}
		}
		template <typename T, typename U, typename std::enable_if<(bso_type<T,U,1>::value == 1u || bso_type<T,U,1>::value == 4u) &&
			std::is_constructible<typename std::decay<T>::type,const typename std::decay<U>::type &>::value,
			int>::type = 0>
		static series_common_type<T,U,1> dispatch_binary_sub(T &&x, U &&y)
		{
			typename std::decay<T>::type y1(std::forward<U>(y));
			return dispatch_binary_sub(std::forward<T>(x),std::move(y1));
		}
		template <typename T, typename U, typename std::enable_if<bso_type<T,U,1>::value == 2u,int>::type = 0>
		static auto dispatch_binary_sub(T &&x, U &&y) -> decltype(dispatch_binary_sub(std::forward<U>(y),std::forward<T>(x)))
		{
			auto retval = dispatch_binary_sub(std::forward<U>(y),std::forward<T>(x));
			retval.negate();
			return retval;
		}
		template <typename T, typename U, typename std::enable_if<(bso_type<T,U,1>::value == 3u || bso_type<T,U,1>::value == 5u) &&
			std::is_constructible<series_common_type<T,U,1>,const typename std::decay<T>::type &>::value &&
			std::is_constructible<series_common_type<T,U,1>,const typename std::decay<U>::type &>::value,
			int>::type = 0>
		static series_common_type<T,U,1> dispatch_binary_sub(T &&x, U &&y)
		{
			series_common_type<T,U,1> x1(std::forward<T>(x));
			series_common_type<T,U,1> y1(std::forward<U>(y));
			return dispatch_binary_sub(std::move(x1),std::move(y1));
		}
		template <typename T, typename U, typename std::enable_if<bso_type<T,U,1>::value == 6u || bso_type<T,U,1>::value == 7u,int>::type = 0>
		static auto dispatch_binary_sub(T &&x, U &&y) -> decltype(dispatch_binary_sub(std::forward<U>(y),std::forward<T>(x)))
		{
			auto retval = dispatch_binary_sub(std::forward<U>(y),std::forward<T>(x));
			retval.negate();
			return retval;
		}
		template <typename T, typename U>
		using binary_sub_type = decltype(dispatch_binary_sub(std::declval<const typename std::decay<T>::type &>(),
			std::declval<const typename std::decay<U>::type &>()));
		template <typename T, typename U, typename std::enable_if<
			!std::is_const<T>::value && std::is_assignable<T &,binary_sub_type<T,U>>::value,
			int>::type = 0>
		static T &dispatch_in_place_sub(T &x, U &&y)
		{
			x = dispatch_binary_sub(std::move(x),std::forward<U>(y));
			return x;
		}
		template <typename T, typename U>
		using in_place_sub_type = decltype(dispatch_in_place_sub(std::declval<typename std::decay<T>::type &>(),
			std::declval<const typename std::decay<U>::type &>()));
		// Multiplication.
		template <typename T, typename U>
		static series_common_type<T,U,2> binary_mul_impl(T &&x, U &&y)
		{
			return series_multiplier<series_common_type<T,U,2>>(std::forward<T>(x),std::forward<U>(y))();
		}
		template <typename T, typename U, typename std::enable_if<bso_type<T,U,2>::value == 0u,int>::type = 0>
		static series_common_type<T,U,2> dispatch_binary_mul(T &&x, U &&y)
		{
			using ret_type = series_common_type<T,U,2>;
			static_assert(std::is_same<typename std::decay<T>::type,ret_type>::value,"Invalid type.");
			static_assert(std::is_same<typename std::decay<U>::type,ret_type>::value,"Invalid type.");
			if (likely(x.m_symbol_set == y.m_symbol_set)) {
				return binary_mul_impl(std::forward<T>(x),std::forward<U>(y));
			} else {
				auto merge = x.m_symbol_set.merge(y.m_symbol_set);
				// Couple of paranoia checks.
				piranha_assert(merge == y.m_symbol_set.merge(x.m_symbol_set));
				piranha_assert(merge == y.m_symbol_set.merge(merge));
				const bool need_copy_x = (merge != x.m_symbol_set), need_copy_y = (merge != y.m_symbol_set);
				piranha_assert(need_copy_x || need_copy_y);
				if (need_copy_x) {
					ret_type x_copy(x.merge_arguments(merge));
					if (need_copy_y) {
						ret_type y_copy(y.merge_arguments(merge));
						return binary_mul_impl(std::move(x_copy),std::move(y_copy));
					}
					return binary_mul_impl(std::move(x_copy),std::forward<U>(y));
				} else {
					piranha_assert(need_copy_y);
					ret_type y_copy(y.merge_arguments(merge));
					return binary_mul_impl(std::forward<T>(x),std::move(y_copy));
				}
			}
		}
		template <typename T, typename U, typename std::enable_if<(bso_type<T,U,2>::value == 1u || bso_type<T,U,2>::value == 4u) &&
			std::is_constructible<typename std::decay<T>::type,const typename std::decay<U>::type &>::value,
			int>::type = 0>
		static series_common_type<T,U,2> dispatch_binary_mul(T &&x, U &&y)
		{
			typename std::decay<T>::type y1(std::forward<U>(y));
			return dispatch_binary_mul(std::forward<T>(x),std::move(y1));
		}
		template <typename T, typename U, typename std::enable_if<bso_type<T,U,2>::value == 2u,int>::type = 0>
		static auto dispatch_binary_mul(T &&x, U &&y) -> decltype(dispatch_binary_mul(std::forward<U>(y),std::forward<T>(x)))
		{
			return dispatch_binary_mul(std::forward<U>(y),std::forward<T>(x));
		}
		template <typename T, typename U, typename std::enable_if<(bso_type<T,U,2>::value == 3u || bso_type<T,U,2>::value == 5u) &&
			std::is_constructible<series_common_type<T,U,2>,const typename std::decay<T>::type &>::value &&
			std::is_constructible<series_common_type<T,U,2>,const typename std::decay<U>::type &>::value,
			int>::type = 0>
		static series_common_type<T,U,2> dispatch_binary_mul(T &&x, U &&y)
		{
			series_common_type<T,U,2> x1(std::forward<T>(x));
			series_common_type<T,U,2> y1(std::forward<U>(y));
			return dispatch_binary_mul(std::move(x1),std::move(y1));
		}
		template <typename T, typename U, typename std::enable_if<bso_type<T,U,2>::value == 6u || bso_type<T,U,2>::value == 7u,int>::type = 0>
		static auto dispatch_binary_mul(T &&x, U &&y) -> decltype(dispatch_binary_mul(std::forward<U>(y),std::forward<T>(x)))
		{
			return dispatch_binary_mul(std::forward<U>(y),std::forward<T>(x));
		}
		// TODO: in the definitions of the mul types, we need to check that a series_multiplier is defined for the operands.
		// Need to investigate where it is best to put this check.
		template <typename T, typename U>
		using binary_mul_type = decltype(dispatch_binary_mul(std::declval<const typename std::decay<T>::type &>(),
			std::declval<const typename std::decay<U>::type &>()));
		template <typename T, typename U, typename std::enable_if<
			!std::is_const<T>::value && std::is_assignable<T &,binary_mul_type<T,U>>::value,
			int>::type = 0>
		static T &dispatch_in_place_mul(T &x, U &&y)
		{
			x = dispatch_binary_mul(std::move(x),std::forward<U>(y));
			return x;
		}
		template <typename T, typename U>
		using in_place_mul_type = decltype(dispatch_in_place_mul(std::declval<typename std::decay<T>::type &>(),
			std::declval<const typename std::decay<U>::type &>()));
		// Division.
		// NOTE: only two cases are possible at the moment, when we divide a series by an object with lower recursion index.
		// The implementation of these two cases 4 and 5 is different from the other operations, as we cannot promote to a common
		// type (true series division is not implemented).
		template <typename T, typename U, typename std::enable_if<bso_type<T,U,3>::value == 4u,int>::type = 0>
		static series_common_type<T,U,3> dispatch_binary_div(T &&x, U &&y)
		{
			using ret_type = series_common_type<T,U,3>;
			static_assert(std::is_same<typename std::decay<T>::type,ret_type>::value,"Invalid type.");
			// Create a copy of x and work on it. This is always possible.
			ret_type retval(std::forward<T>(x));
			// NOTE: x is not used any more.
			const auto it_f = retval.m_container.end();
			try {
				for (auto it = retval.m_container.begin(); it != it_f;) {
					// NOTE: here the original requirement is that cf / y is defined, but we know
					// that cf / y results in another cf, and we assume always that cf /= y is exactly equivalent
					// to cf = cf / y. And cf must be move-assignable. So this should be possible.
					it->m_cf /= y;
					// NOTE: no need to check for compatibility, as it depends only on the key type and here
					// we are only acting on the coefficient.
					if (unlikely(it->is_ignorable(retval.m_symbol_set))) {
						// Erase will return the next iterator.
						it = retval.m_container.erase(it);
					} else {
						++it;
					}
				}
			} catch (...) {
				// In case of errors clear out the series.
				retval.m_container.clear();
				throw;
			}
			return retval;
		}
		// NOTE: the trailing decltype() syntax is used here to make sure we can actually call the other overload of the function
		// in the body. It *should* always be the case.
		template <typename T, typename U, typename std::enable_if<bso_type<T,U,3>::value == 5u &&
			std::is_constructible<series_common_type<T,U,3>,const typename std::decay<T>::type &>::value,int>::type = 0>
		static auto dispatch_binary_div(T &&x, U &&y) ->
			decltype(dispatch_binary_div(std::declval<const series_common_type<T,U,3> &>(),std::forward<U>(y)))
		{
			series_common_type<T,U,3> x1(std::forward<T>(x));
			return dispatch_binary_div(std::move(x1),std::forward<U>(y));
		}
		template <typename T, typename U>
		using binary_div_type = decltype(dispatch_binary_div(std::declval<const typename std::decay<T>::type &>(),
			std::declval<const typename std::decay<U>::type &>()));
		template <typename T, typename U, typename std::enable_if<
			!std::is_const<T>::value && std::is_assignable<T &,binary_div_type<T,U>>::value,
			int>::type = 0>
		static T &dispatch_in_place_div(T &x, U &&y)
		{
			x = dispatch_binary_div(std::move(x),std::forward<U>(y));
			return x;
		}
		template <typename T, typename U>
		using in_place_div_type = decltype(dispatch_in_place_div(std::declval<typename std::decay<T>::type &>(),
			std::declval<const typename std::decay<U>::type &>()));
		// Equality.
		// Low-level implementation of equality.
		template <typename T>
		static bool equality_impl(const T &x, const T &y)
		{
			if (x.size() != y.size()) {
				return false;
			}
			piranha_assert(x.m_symbol_set == y.m_symbol_set);
			const auto it_f_x = x.m_container.end(), it_f_y = y.m_container.end();
			for (auto it = x.m_container.begin(); it != it_f_x; ++it) {
				const auto tmp_it = y.m_container.find(*it);
				if (tmp_it == it_f_y || tmp_it->m_cf != it->m_cf) {
					return false;
				}
			}
			return true;
		}
		// NOTE: we are using operator+() on the coefficients to determine type conversions.
		template <typename T, typename U, typename std::enable_if<bso_type<T,U,0>::value == 0u,int>::type = 0>
		static bool dispatch_equality(const T &x, const U &y)
		{
			static_assert(std::is_same<T,U>::value,"Invalid types for the equality operator.");
			// Arguments merging.
			if (likely(x.m_symbol_set == y.m_symbol_set)) {
				return equality_impl(x,y);
			} else {
				auto merge = x.m_symbol_set.merge(y.m_symbol_set);
				const bool x_needs_copy = (merge != x.m_symbol_set);
				const bool y_needs_copy = (merge != y.m_symbol_set);
				piranha_assert(x_needs_copy || y_needs_copy);
				return equality_impl(
					x_needs_copy ? x.merge_arguments(merge) : x,
					y_needs_copy ? y.merge_arguments(merge) : y
				);
			}
		}
		template <typename T, typename U, typename std::enable_if<(bso_type<T,U,0>::value == 1u || bso_type<T,U,0>::value == 4u) &&
			std::is_constructible<T,const U &>::value,
			int>::type = 0>
		static bool dispatch_equality(const T &x, const U &y)
		{
			return dispatch_equality(x,T(y));
		}
		template <typename T, typename U, typename std::enable_if<bso_type<T,U,0>::value == 2u,int>::type = 0>
		static auto dispatch_equality(const T &x, const U &y) -> decltype(dispatch_equality(y,x))
		{
			return dispatch_equality(y,x);
		}
		template <typename T, typename U, typename std::enable_if<(bso_type<T,U,0>::value == 3u || bso_type<T,U,0>::value == 5u) &&
			std::is_constructible<series_common_type<T,U,0>,const T &>::value &&
			std::is_constructible<series_common_type<T,U,0>,const U &>::value,
			int>::type = 0>
		static bool dispatch_equality(const T &x, const U &y)
		{
			series_common_type<T,U,0> x1(x);
			series_common_type<T,U,0> y1(y);
			return dispatch_equality(x1,y1);
		}
		template <typename T, typename U, typename std::enable_if<bso_type<T,U,0>::value == 6u || bso_type<T,U,0>::value == 7u,int>::type = 0>
		static auto dispatch_equality(const T &x, const U &y) -> decltype(dispatch_equality(y,x))
		{
			return dispatch_equality(y,x);
		}
		template <typename T, typename U>
		using eq_type = decltype(dispatch_equality(std::declval<const typename std::decay<T>::type &>(),
			std::declval<const typename std::decay<U>::type &>()));
	public:
		template <typename T, typename U>
		friend binary_add_type<T,U> operator+(T &&x, U &&y)
		{
			return dispatch_binary_add(std::forward<T>(x),std::forward<U>(y));
		}
		template <typename T, typename U>
		friend in_place_add_type<T,U> operator+=(T &x, U &&y)
		{
			return dispatch_in_place_add(x,std::forward<U>(y));
		}
		template <typename T, typename U>
		friend binary_sub_type<T,U> operator-(T &&x, U &&y)
		{
			return dispatch_binary_sub(std::forward<T>(x),std::forward<U>(y));
		}
		template <typename T, typename U>
		friend in_place_sub_type<T,U> operator-=(T &x, U &&y)
		{
			return dispatch_in_place_sub(x,std::forward<U>(y));
		}
		template <typename T, typename U>
		friend binary_mul_type<T,U> operator*(T &&x, U &&y)
		{
			return dispatch_binary_mul(std::forward<T>(x),std::forward<U>(y));
		}
		template <typename T, typename U>
		friend in_place_mul_type<T,U> operator*=(T &x, U &&y)
		{
			return dispatch_in_place_mul(x,std::forward<U>(y));
		}
		template <typename T, typename U>
		friend binary_div_type<T,U> operator/(T &&x, U &&y)
		{
			return dispatch_binary_div(std::forward<T>(x),std::forward<U>(y));
		}
		template <typename T, typename U>
		friend in_place_div_type<T,U> operator/=(T &x, U &&y)
		{
			return dispatch_in_place_div(x,std::forward<U>(y));
		}
		template <typename T, typename U>
		friend eq_type<T,U> operator==(const T &x, const U &y)
		{
			return dispatch_equality(x,y);
		}
		template <typename T, typename U>
		friend eq_type<T,U> operator!=(const T &x, const U &y)
		{
			return !dispatch_equality(x,y);
		}
};

/// Series class.
/**
 * This class provides arithmetic and relational operators overloads for interaction with other series and non-series (scalar) types.
 * Addition and subtraction are implemented directly within this class, both for series and scalar operands. Multiplication of series by
 * scalar types is also implemented in this class, whereas series-by-series multiplication is provided via the external helper class
 * piranha::series_multiplier (whose behaviour can be specialised to provide fast multiplication capabilities).
 * Division by scalar types is also supported.
 * 
 * Support for comparison with series and scalar types is also provided.
 * 
 * ## Type requirements ##
 * 
 * - \p Term must satisfy piranha::is_term.
 * - \p Derived must derive from piranha::series of \p Term and \p Derived.
 * - \p Derived must satisfy piranha::is_series.
 * 
 * ## Exception safety guarantee ##
 * 
 * Unless otherwise specified, this class provides the strong exception safety guarantee for all operations.
 * 
 * ## Move semantics ##
 * 
 * Moved-from series are left in a state equivalent to an empty series.
 *
 * ## Serialization ##
 *
 * This class supports serialization if its term type does.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
 /* TODO:
 * \todo cast operator, to series and non-series types.
 * \todo cast operator would allow to define in-place operators with fundamental types as first operand.
 * \todo review the handling of incompatible terms: it seems like in some places we are considering the possibility that operations on
 * coefficients could make the term incompatible, but this is not the case as in base_term only the key matters for compatibility. Propagating
 * this through the code would solve the issue of what to do if a term becomes incompatible during multiplication/merging/insertion etc. (now
 * it cannot become incompatible any more if it is already in series).
 * \todo filter and transform can probably take arbitrary functors as input, instead of std::function. Just assert the function object's signature.
 * \todo probably apply_cf_functor can be folded into transform for those few uses.
 * \todo transform needs sfinaeing.
 * \todo the output of pow() for series needs to be recomputed.
 * \todo series generic ctor/assignment should use convert_to.
 * TODO new operators:
 * - merge terms and insertion should probably now accept only term_type and not generic terms insertions;
 * - review and beautify the generic ctor and assignment operator;
 * - probably the swap-for-merge thing overlaps with the swapping we do already in the new operator+. We need only on of these.
 * - test with mock_cfs that are not addable to scalars.
 */
template <typename Term, typename Derived>
class series: detail::series_tag, series_operators
{
		PIRANHA_TT_CHECK(is_term,Term);
	public:
		/// Alias for term type.
		typedef Term term_type;
	private:
		// Make friend with all series.
		template <typename, typename>
		friend class series;
		// Make friend with debugging class.
		template <typename>
		friend class debug_access;
		// Make friend with series multiplier class.
		template <typename, typename>
		friend class series_multiplier;
		// Make friend with the operators class.
		friend class series_operators;
		// Partial need access to the custom derivatives.
		template <typename, typename>
		friend struct math::partial_impl;
		// NOTE: this friendship is related to the bug workaround above.
		template <typename Term2, typename Derived2>
		friend std::pair<typename Term2::cf_type,Derived2> detail::pair_from_term(const symbol_set &, const Term2 &);
	protected:
		/// Container type for terms.
		typedef hash_set<term_type,detail::term_hasher<Term>> container_type;
	private:
		// Avoid confusing doxygen.
		typedef decltype(std::declval<container_type>().evaluate_sparsity()) sparsity_info_type;
		// Overload for completely different term type: copy-convert to term_type and proceed.
		template <bool Sign, typename T>
		void dispatch_insertion(T &&term, typename std::enable_if<
			!std::is_same<typename std::decay<T>::type,term_type>::value &&
			!is_nonconst_rvalue_ref<T &&>::value
			>::type * = nullptr)
		{
			dispatch_insertion<Sign>(term_type(typename term_type::cf_type(term.m_cf),
				typename term_type::key_type(term.m_key,m_symbol_set)));
		}
		// Overload for completely different term type: move-convert to term_type and proceed.
		template <bool Sign, typename T>
		void dispatch_insertion(T &&term, typename std::enable_if<
			!std::is_same<typename std::decay<T>::type,term_type>::value &&
			is_nonconst_rvalue_ref<T &&>::value
			>::type * = nullptr)
		{
			dispatch_insertion<Sign>(term_type(typename term_type::cf_type(std::move(term.m_cf)),
				typename term_type::key_type(std::move(term.m_key),m_symbol_set)));
		}
		// Overload for term_type.
		template <bool Sign, typename T>
		void dispatch_insertion(T &&term, typename std::enable_if<
			std::is_same<typename std::decay<T>::type,term_type>::value
			>::type * = nullptr)
		{
			// Debug checks.
			piranha_assert(empty() || m_container.begin()->is_compatible(m_symbol_set));
			// Generate error if term is not compatible.
			if (unlikely(!term.is_compatible(m_symbol_set))) {
				piranha_throw(std::invalid_argument,"cannot insert incompatible term");
			}
			// Discard ignorable term.
			if (unlikely(term.is_ignorable(m_symbol_set))) {
				return;
			}
			insertion_impl<Sign>(std::forward<T>(term));
		}
		template <bool Sign, typename Iterator>
		static void insertion_cf_arithmetics(Iterator &it, const term_type &term)
		{
			if (Sign) {
				it->m_cf += term.m_cf;
			} else {
				it->m_cf -= term.m_cf;
			}
		}
		template <bool Sign, typename Iterator>
		static void insertion_cf_arithmetics(Iterator &it, term_type &&term)
		{
			if (Sign) {
				it->m_cf += std::move(term.m_cf);
			} else {
				it->m_cf -= std::move(term.m_cf);
			}
		}
		// Insert compatible, non-ignorable term.
		template <bool Sign, typename T>
		void insertion_impl(T &&term)
		{
			// NOTE: here we are basically going to reconstruct hash_set::insert() with the goal
			// of optimising things by avoiding one branch.
			// Handle the case of a table with no buckets.
			if (unlikely(!m_container.bucket_count())) {
				m_container._increase_size();
			}
			// Try to locate the element.
			auto bucket_idx = m_container._bucket(term);
			const auto it = m_container._find(term,bucket_idx);
			// Cleanup function that checks ignorability and compatibility of an element in the hash set,
			// and removes it if necessary.
			auto cleanup = [this](const typename container_type::const_iterator &it) {
				if (unlikely(!it->is_compatible(this->m_symbol_set) || it->is_ignorable(this->m_symbol_set))) {
					this->m_container.erase(it);
				}
			};
			if (it == m_container.end()) {
				if (unlikely(m_container.size() == std::numeric_limits<size_type>::max())) {
					piranha_throw(std::overflow_error,"maximum number of elements reached");
				}
				// Term is new. Handle the case in which we need to rehash because of load factor.
				if (unlikely(static_cast<double>(m_container.size() + size_type(1u)) / static_cast<double>(m_container.bucket_count()) >
					m_container.max_load_factor()))
				{
					m_container._increase_size();
					// We need a new bucket index in case of a rehash.
					bucket_idx = m_container._bucket(term);
				}
				const auto new_it = m_container._unique_insert(std::forward<T>(term),bucket_idx);
				m_container._update_size(m_container.size() + size_type(1u));
				// Insertion was successful, change sign if requested.
				if (!Sign) {
					try {
						math::negate(new_it->m_cf);
						cleanup(new_it);
					} catch (...) {
						// Run the cleanup function also in case of exceptions, as we do not know
						// in which state the modified term is.
						cleanup(new_it);
						throw;
					}
				}
			} else {
				// Assert the existing term is not ignorable and it is compatible.
				piranha_assert(!it->is_ignorable(m_symbol_set) && it->is_compatible(m_symbol_set));
				try {
					// The term exists already, update it.
					insertion_cf_arithmetics<Sign>(it,std::forward<T>(term));
					// Check if the term has become ignorable or incompatible after the modification.
					cleanup(it);
				} catch (...) {
					cleanup(it);
					throw;
				}
			}
		}
		// Terms merging
		// =============
		// NOTE: ideas to improve the algorithm:
		// - optimization when merging with self: add each coefficient to itself, instead of copying and merging.
		// - optimization when merging with series with same bucket size: avoid computing the destination bucket,
		//   as it will be the same as the original.
		// Overload in case that T derives from the same type as this (series<Term,Derived>).
		template <bool Sign, typename T>
		void merge_terms_impl0(T &&s,
			typename std::enable_if<std::is_base_of<series<Term,Derived>,typename std::decay<T>::type>::value>::type * = nullptr)
		{
			// NOTE: here we can take the pointer to series and compare it to this because we know from enable_if that
			// series is an instance of the type of this.
			if (unlikely(&s == this)) {
				// If the two series are the same object, we need to make a copy.
				// NOTE: we do not forward here, when making the copy, because if T is a non-const
				// rvalue reference we might actually erase this: with Derived(series), a move
				// constructor might end up being called.
				merge_terms_impl1<Sign>(series<Term,Derived>(s));
			} else {
				merge_terms_impl1<Sign>(std::forward<T>(s));
			}
		}
		// Overload in case that T is not an instance of the type of this.
		template <bool Sign, typename T>
		void merge_terms_impl0(T &&s,
			typename std::enable_if<!std::is_base_of<series<Term,Derived>,typename std::decay<T>::type>::value>::type * = nullptr)
		{
			// No worries about same object, just forward.
			merge_terms_impl1<Sign>(std::forward<T>(s));
		}
		// Overload if we cannot move objects from series.
		template <bool Sign, typename T>
		void merge_terms_impl1(T &&s,
			typename std::enable_if<!is_nonconst_rvalue_ref<T &&>::value>::type * = nullptr)
		{
			const auto it_f = s.m_container.end();
			try {
				for (auto it = s.m_container.begin(); it != it_f; ++it) {
					insert<Sign>(*it);
				}
			} catch (...) {
				// In case of any insertion error, zero out this series.
				m_container.clear();
				throw;
			}
		}
		static void swap_for_merge(container_type &&c1, container_type &&c2, bool &swap)
		{
			piranha_assert(!swap);
			// Do not do anything in case of overflows.
			if (unlikely(c1.size() > std::numeric_limits<size_type>::max() - c2.size())) {
				return;
			}
			// This is the maximum number of terms in the return value.
			const auto max_size = c1.size() + c2.size();
			// This will be the maximum required number of buckets.
			size_type max_n_buckets;
			try {
				piranha_assert(c1.max_load_factor() > 0);
				max_n_buckets = boost::numeric_cast<size_type>(std::trunc(static_cast<double>(max_size) / c1.max_load_factor()));
			} catch (...) {
				// Ignore any error on conversions.
				return;
			}
			// Now we swap the containers, if the first one is not enough to contain the expected number of terms
			// and if the second one is larger.
			if (c1.bucket_count() < max_n_buckets && c2.bucket_count() > c1.bucket_count()) {
				container_type tmp(std::move(c1));
				c1 = std::move(c2);
				c2 = std::move(tmp);
				swap = true;
			}
		}
		// This overloads the above in the case we are dealing with two different container types:
		// in such a condition, we won't do any swapping.
		template <typename OtherContainerType>
		static void swap_for_merge(container_type &&, OtherContainerType &&, bool &)
		{}
		// Overload if we can move objects from series.
		template <bool Sign, typename T>
		void merge_terms_impl1(T &&s,
			typename std::enable_if<is_nonconst_rvalue_ref<T &&>::value>::type * = nullptr)
		{
			bool swap = false;
			// Try to steal memory from other.
			swap_for_merge(std::move(m_container),std::move(s.m_container),swap);
			try {
				const auto it_f = s.m_container._m_end();
				for (auto it = s.m_container._m_begin(); it != it_f; ++it) {
					insert<Sign>(std::move(*it));
				}
				// If we swapped the operands and a negative merge was performed, we need to change
				// the signs of all coefficients.
				if (swap && !Sign) {
					const auto it_f2 = m_container.end();
					for (auto it = m_container.begin(); it != it_f2;) {
						math::negate(it->m_cf);
						if (unlikely(!it->is_compatible(m_symbol_set) || it->is_ignorable(m_symbol_set))) {
							// Erase the invalid term.
							it = m_container.erase(it);
						} else {
							++it;
						}
					}
				}
			} catch (...) {
				// In case of any insertion error, zero out both series.
				m_container.clear();
				s.m_container.clear();
				throw;
			}
			// The other series must alway be cleared, since we moved out the terms.
			s.m_container.clear();
		}
		// Merge all terms from another series. Works if s is this (in which case a copy is made). Basic exception safety guarantee.
		template <bool Sign, typename T>
		void merge_terms(T &&s,
			typename std::enable_if<is_series<typename std::decay<T>::type>::value>::type * = nullptr)
		{
			merge_terms_impl0<Sign>(std::forward<T>(s));
		}
		// Generic construction
		// ====================
		// TMP for the generic constructor.
		// NOTE: this logic is a slight repetition of the helper methods below. However, since the enabling
		// conditions below are already quite complicated and split in lvalue/rvalue,
		// it might be better to leave this as is.
		template <typename T, typename U, typename = void>
		struct generic_ctor_enabler
		{
			static const bool value = std::is_constructible<typename term_type::cf_type,T>::value;
		};
		template <typename T, typename U>
		struct generic_ctor_enabler<T,U,typename std::enable_if<
			series_recursion_index<T>::value != 0 && series_recursion_index<U>::value == series_recursion_index<T>::value
			>::type>
		{
			typedef typename std::decay<T>::type::term_type other_term_type;
			typedef typename other_term_type::cf_type other_cf_type;
			typedef typename other_term_type::key_type other_key_type;
			// NOTE: this essentially is an is_term_insertable check. Keep it in mind if we implement
			// enable/disable of insertion method in the future.
			static const bool value = std::is_same<term_type,other_term_type>::value ||
				(std::is_constructible<typename term_type::cf_type,other_cf_type>:: value &&
				std::is_constructible<typename term_type::key_type,other_key_type,symbol_set>:: value);
		};
		template <typename T, typename U>
		struct generic_ctor_enabler<T,U,typename std::enable_if<
			(series_recursion_index<U>::value < series_recursion_index<T>::value)
			>::type>
		{
			static const bool value = false;
		};
		// Series with same echelon size, same term type, move.
		template <typename Series, typename U = series, typename std::enable_if<
			series_recursion_index<U>::value == series_recursion_index<Series>::value &&
			std::is_same<term_type,typename std::decay<Series>::type::term_type>::value &&
			is_nonconst_rvalue_ref<Series &&>::value,int>::type = 0>
		void dispatch_generic_construction(Series &&s)
		{
			static_assert(!std::is_same<series,typename std::decay<Series>::type>::value,"Invalid series type for generic construction.");
			m_symbol_set = std::move(s.m_symbol_set);
			m_container = std::move(s.m_container);
		}
		// Series with same echelon size, same term type, copy.
		template <typename Series, typename U = series, typename std::enable_if<
			series_recursion_index<U>::value == series_recursion_index<Series>::value &&
			std::is_same<term_type,typename std::decay<Series>::type::term_type>::value &&
			!is_nonconst_rvalue_ref<Series &&>::value,int>::type = 0>
		void dispatch_generic_construction(Series &&s)
		{
			static_assert(!std::is_same<series,typename std::decay<Series>::type>::value,"Invalid series type for generic construction.");
			m_symbol_set = s.m_symbol_set;
			m_container = s.m_container;
		}
		// Series with same echelon size and different term type, move.
		template <typename Series, typename U = series, typename std::enable_if<
			series_recursion_index<U>::value == series_recursion_index<Series>::value &&
			!std::is_same<term_type,typename std::decay<Series>::type::term_type>::value &&
			is_nonconst_rvalue_ref<Series &&>::value,int>::type = 0>
		void dispatch_generic_construction(Series &&s)
		{
			m_symbol_set = std::move(s.m_symbol_set);
			merge_terms<true>(std::forward<Series>(s));
		}
		// Series with same echelon size and different term type, copy.
		template <typename Series, typename U = series, typename std::enable_if<
			series_recursion_index<U>::value == series_recursion_index<Series>::value &&
			!std::is_same<term_type,typename std::decay<Series>::type::term_type>::value &&
			!is_nonconst_rvalue_ref<Series &&>::value,int>::type = 0>
		void dispatch_generic_construction(Series &&s)
		{
			m_symbol_set = s.m_symbol_set;
			merge_terms<true>(s);
		}
		// Object with smaller recursion index.
		template <typename T, typename U = series, typename std::enable_if<(series_recursion_index<U>::value > series_recursion_index<T>::value),int>::type = 0>
		void dispatch_generic_construction(T &&x)
		{
			typedef typename term_type::cf_type cf_type;
			typedef typename term_type::key_type key_type;
			cf_type cf(std::forward<T>(x));
			key_type key(m_symbol_set);
			insert(term_type(std::move(cf),std::move(key)));
		}
		// Exponentiation.
		template <typename T>
		Derived pow_impl(const T &x) const
		{
			integer n;
			try {
				n = safe_cast<integer>(x);
			} catch (const std::invalid_argument &) {
				piranha_throw(std::invalid_argument,"invalid argument for series exponentiation: non-integral value");
			}
			if (n.sign() < 0) {
				piranha_throw(std::invalid_argument,"invalid argument for series exponentiation: negative integral value");
			}
			// NOTE: for series it seems like it is better to run the dumb algorithm instead of, e.g.,
			// exponentiation by squaring - the growth in number of terms seems to be slower.
			Derived retval(*static_cast<Derived const *>(this));
			for (integer i(1); i < n; ++i) {
				retval *= *static_cast<Derived const *>(this);
			}
			return retval;
		}
		// Iterator utilities.
		typedef boost::transform_iterator<std::function<std::pair<typename term_type::cf_type,Derived>(const term_type &)>,
			typename container_type::const_iterator> const_iterator_impl;
		// Evaluation utilities.
		// NOTE: here we need the Series template because otherwise, in case of missing eval on key or coefficient, the decltype
		// will fail as series is not a template parameter.
		template <typename Series, typename U, typename = void>
		struct eval_type_ {};
		template <typename Series, typename U>
		using e_type = decltype(math::evaluate(std::declval<typename Series::term_type::cf_type const &>(),std::declval<std::unordered_map<std::string,U> const &>()) *
			std::declval<const typename Series::term_type::key_type &>().evaluate(std::declval<const symbol_set::positions_map<U> &>(),
			std::declval<const symbol_set &>()));
		template <typename Series, typename U>
		struct eval_type_<Series,U,typename std::enable_if<has_multiply_accumulate<e_type<Series,U>,
			decltype(math::evaluate(std::declval<typename Series::term_type::cf_type const &>(),std::declval<std::unordered_map<std::string,U> const &>())),
			decltype(std::declval<const typename Series::term_type::key_type &>().evaluate(std::declval<const symbol_set::positions_map<U> &>(),std::declval<const symbol_set &>()))>::value &&
			std::is_constructible<e_type<Series,U>,int>::value
			>::type>
		{
			using type = e_type<Series,U>;
		};
		// Final typedef.
		template <typename Series, typename U>
		using eval_type = typename eval_type_<Series,U>::type;
		// Print utilities.
		template <bool TexMode, typename Iterator>
		static std::ostream &print_helper_1(std::ostream &os, Iterator start, Iterator end, const symbol_set &args)
		{
			piranha_assert(start != end);
			const auto limit = settings::get_max_term_output();
			size_type count = 0u;
			std::ostringstream oss;
			auto it = start;
			for (; it != end;) {
				if (limit && count == limit) {
					break;
				}
				std::ostringstream oss_cf;
				if (TexMode) {
					print_tex_coefficient(oss_cf,it->m_cf);
				} else {
					print_coefficient(oss_cf,it->m_cf);
				}
				auto str_cf = oss_cf.str();
				std::ostringstream oss_key;
				if (TexMode) {
					it->m_key.print_tex(oss_key,args);
				} else {
					it->m_key.print(oss_key,args);
				}
				auto str_key = oss_key.str();
				if (str_cf == "1" && !str_key.empty()) {
					str_cf = "";
				} else if (str_cf == "-1" && !str_key.empty()) {
					str_cf = "-";
				}
				oss << str_cf;
				if (str_cf != "" && str_cf != "-" && !str_key.empty() && !TexMode) {
					oss << "*";
				}
				oss << str_key;
				++it;
				if (it != end) {
					oss << "+";
				}
				++count;
			}
			auto str = oss.str();
			// If we reached the limit without printing all terms in the series, print the ellipsis.
			if (limit && count == limit && it != end) {
				if (TexMode) {
					str += "\\ldots";
				} else {
					str += "...";
				}
			}
			std::string::size_type index = 0u;
			while (true) {
				index = str.find("+-",index);
				if (index == std::string::npos) {
					break;
				}
				str.replace(index,2u,"-");
				++index;
			}
			os << str;
			return os;
		}
		// Merge arguments using new_ss as new symbol set.
		Derived merge_arguments(const symbol_set &new_ss) const
		{
			piranha_assert(new_ss.size() > m_symbol_set.size());
			piranha_assert(std::includes(new_ss.begin(),new_ss.end(),m_symbol_set.begin(),m_symbol_set.end()));
			using cf_type = typename term_type::cf_type;
			using key_type = typename term_type::key_type;
			Derived retval;
			retval.m_symbol_set = new_ss;
			const auto it_f = m_container.end();
			for (auto it = m_container.begin(); it != it_f; ++it) {
				cf_type new_cf(it->m_cf);
				key_type new_key(it->m_key.merge_args(m_symbol_set,new_ss));
				retval.insert(term_type(std::move(new_cf),std::move(new_key)));
			}
			return retval;
		}
		// Set of checks to be run on destruction in debug mode.
		bool destruction_checks() const
		{
			// Run destruction checks only if we are not in shutdown.
			if (environment::shutdown()) {
				return true;
			}
			for (auto it = m_container.begin(); it != m_container.end(); ++it) {
				if (!it->is_compatible(m_symbol_set)) {
					std::cout << "Term not compatible.\n";
					return false;
				}
				if (it->is_ignorable(m_symbol_set)) {
					std::cout << "Term not ignorable.\n";
					return false;
				}
			}
			return true;
		}
		template <typename T>
		static T trim_cf_impl(const T &s, typename std::enable_if<
			is_series<T>::value>::type * = nullptr)
		{
			return s.trim();
		}
		template <typename T>
		static const T &trim_cf_impl(const T &s, typename std::enable_if<
			!is_series<T>::value>::type * = nullptr)
		{
			return s;
		}
		// Typedef for pow().
		template <typename T, typename U>
		using pow_ret_type = typename std::enable_if<std::is_same<decltype(math::pow(std::declval<typename term_type::cf_type const &>(),std::declval<T const &>())),
			typename term_type::cf_type>::value && has_is_zero<T>::value && has_safe_cast<integer,T>::value && is_multipliable_in_place<U>::value,U
			>::type;
		// Metaprogramming bits for partial derivative.
		template <typename Cf>
		using cf_diff_type = decltype(math::partial(std::declval<const Cf &>(),std::string()));
		// NOTE: decltype on a member is the type of that member:
		// http://thbecker.net/articles/auto_and_decltype/section_06.html
		template <typename Key>
		using key_diff_type = decltype(std::declval<const Key &>().partial(std::declval<const symbol_set::positions &>(),
			std::declval<const symbol_set &>()).first);
		// Shortcuts to get cf/key from series.
		template <typename Series>
		using cf_t = typename Series::term_type::cf_type;
		template <typename Series>
		using key_t = typename Series::term_type::key_type;
		// We have different implementations, depending on whether the computed derivative type is the same as the original one
		// (in which case we will use faster term insertions) or not (in which case we resort to series arithmetics). Adopt the usual schema
		// of providing an unspecialised value that sfinaes out if the enabler condition is malformed, and two specialisations
		// that implement the logic above.
		template <typename Series, typename = void>
		struct partial_type_ {};
		// NOTE: the enabler conditions below are one the negation of the other, not sure if it is possible to avoid repetition without macro as we need
		// them explicitly for sfinae.
		#define PIRANHA_SERIES_PARTIAL_ENABLER (std::is_same<cf_diff_type<cf_t<Series>>,cf_t<Series>>::value && \
			std::is_same<decltype(std::declval<const cf_t<Series> &>() * std::declval<const key_diff_type<key_t<Series>> &>()),cf_t<Series>>::value)
		template <typename Series>
		struct partial_type_<Series,typename std::enable_if<PIRANHA_SERIES_PARTIAL_ENABLER>::type>
		{
			using type = Series;
			// Record the algorithm to be adopted for later use.
			static const int algo = 0;
		};
		// This is the type resulting from the second algorithm.
		template <typename Series>
		using partial_type_1 = decltype(std::declval<const cf_diff_type<cf_t<Series>> &>() * std::declval<const Series &>() +
				std::declval<const cf_t<Series> &>() * std::declval<const key_diff_type<key_t<Series>> &>() * std::declval<const Series &>());
		// NOTE: in addition to check that we cannot use the optimised algorithm, we must also check that the resulting type
		// is constructible from int (for the accumulation of retval), that it is addable and that the add type is itself.
		template <typename Series>
		struct partial_type_<Series,typename std::enable_if<!PIRANHA_SERIES_PARTIAL_ENABLER &&
			std::is_constructible<partial_type_1<Series>,int>::value &&
			std::is_same<decltype(std::declval<const partial_type_1<Series> &>()
			+ std::declval<const partial_type_1<Series> &>()),partial_type_1<Series>>::value>::type>
		{
			using type = partial_type_1<Series>;
			static const int algo = 1;
		};
		#undef PIRANHA_SERIES_PARTIAL_ENABLER
		// The final typedef.
		template <typename Series>
		using partial_type = typename partial_type_<Series>::type;
		// The implementation of the two partial() algorithms.
		template <typename Series = Derived, typename std::enable_if<partial_type_<Series>::algo == 0,int>::type = 0>
		partial_type<Series> partial_impl(const std::string &name) const
		{
			static_assert(std::is_same<Derived,partial_type<Series>>::value,"Invalid type.");
			// This is the faster algorithm.
			Derived retval;
			retval.m_symbol_set = this->m_symbol_set;
			const auto it_f = this->m_container.end();
			const symbol_set::positions p(retval.m_symbol_set,symbol_set{symbol(name)});
			for (auto it = this->m_container.begin(); it != it_f; ++it) {
				// NOTE: here t0 cannot be incompatible as it->m_key is coming from a series
				// with the same symbol set. Worst that can happen is something going awry
				// in the derivative of the coefficient. If the derivative becomes zero,
				// the insertion routine will not insert anything.
				typename Derived::term_type t0(math::partial(it->m_cf,name),it->m_key);
				retval.insert(std::move(t0));
				// Here we are assuming that the partial of the key returns always a compatible key,
				// otherwise an error will be raised.
				auto p_key = it->m_key.partial(p,retval.m_symbol_set);
				typename Derived::term_type t1(it->m_cf * p_key.first,p_key.second);
				retval.insert(std::move(t1));
			}
			return retval;
		}
		template <typename Series = Derived, typename std::enable_if<partial_type_<Series>::algo == 1,int>::type = 0>
		partial_type<Series> partial_impl(const std::string &name) const
		{
			using term_type = typename Derived::term_type;
			using cf_type = typename term_type::cf_type;
			const auto it_f = this->m_container.end();
			const symbol_set::positions p(this->m_symbol_set,symbol_set{symbol(name)});
			partial_type<Series> retval(0);
			for (auto it = this->m_container.begin(); it != it_f; ++it) {
				// Construct the two pieces of the derivative relating to the key
				// in the original term.
				Derived tmp0;
				tmp0.m_symbol_set = this->m_symbol_set;
				tmp0.insert(term_type(cf_type(1),it->m_key));
				auto p_key = it->m_key.partial(p,this->m_symbol_set);
				Derived tmp1;
				tmp1.m_symbol_set = this->m_symbol_set;
				tmp1.insert(term_type(cf_type(1),p_key.second));
				// Assemble everything into the return value.
				retval = retval + (math::partial(it->m_cf,name) * tmp0 + it->m_cf * p_key.first * tmp1);
			}
			return retval;
		}
		// Custom derivatives boilerplate.
		template <typename Series>
		using cp_map_type = std::unordered_map<std::string,std::function<partial_type<Series>(const Derived &)>>;
		// NOTE: here the initialisation of the static variable inside the body of the function
		// is guaranteed to be thread-safe. It should not matter too much as we always protect the access to it.
		template <typename Series = Derived>
		static cp_map_type<Series> &get_cp_map()
		{
			static cp_map_type<Series> cp_map;
			return cp_map;
		}
		template <typename F, typename Series>
		using custom_partial_enabler = typename std::enable_if<std::is_constructible<std::function<partial_type<Series>(const Derived &)>,F>::value,int>::type;
		// Serialization support.
		friend class boost::serialization::access;
		template <class Archive>
		void save(Archive &ar, unsigned int) const
		{
			// Serialize the symbol set.
			ar & m_symbol_set;
			// Serialize the size.
			const auto s = size();
			ar & s;
			// Serialize all the terms one by one.
			const auto it_f = m_container.end();
			for (auto it = m_container.begin(); it != it_f; ++it) {
				ar & (*it);
			}
		}
		template <class Archive>
		void load(Archive &ar, unsigned int)
		{
			// Erase this.
			*this = series();
			// Recover the symbol set.
			symbol_set ss;
			ar & ss;
			m_symbol_set = std::move(ss);
			// Recover the size.
			size_type s;
			ar & s;
			// Re-insert all the terms.
			for (size_type i = 0u; i < s; ++i) {
				term_type t;
				ar & t;
				insert(std::move(t));
			}
		}
		BOOST_SERIALIZATION_SPLIT_MEMBER()
	public:
		/// Size type.
		/**
		 * Used to represent the number of terms in the series. Equivalent to piranha::hash_set::size_type.
		 */
		typedef typename container_type::size_type size_type;
		/// Const iterator.
		/**
		 * Iterator type that can be used to iterate over the terms of the series.
		 * The object returned upon dereferentiation is an \p std::pair in which the first element
		 * is a copy of the coefficient of the term, the second element a single-term instance of \p Derived constructed from
		 * the term's key and a unitary coefficient.
		 *
		 * This iterator is an input iterator which additionally offers the multi-pass guarantee.
		 * 
		 * @see piranha::series::begin() and piranha::series::end().
		 */
		typedef const_iterator_impl const_iterator;
		/// Defaulted default constructor.
		series() = default;
		/// Defaulted copy constructor.
		/**
		 * @throws unspecified any exception thrown by the copy constructor of piranha::hash_set.
		 */
		series(const series &) = default;
		/// Defaulted move constructor.
		series(series &&) = default;
		/// Generic constructor.
		/**
		 * \note
		 * This constructor is enabled only if the decay type of \p T is different from piranha::series and
		 * the algorithm outlined below is supported by the involved types.
		 *
		 * The generic construction algorithm works as follows:
		 * - if \p T is an instance of piranha::series with the same echelon size as the calling type:
		 *   - if the term type of \p T is the same as that of <tt>this</tt>:
		 *     - the internal terms container and symbol set of \p x are forwarded to construct \p this;
		 *   - else:
		 *     - the symbol set of \p x is forwarded to construct the symbol set of this and all terms from \p x are inserted into \p this
		 *       (after conversion to \p term_type via binary constructor from coefficient and key);
		 * - else:
		 *   - \p x is used to construct a new term as follows:
		 *     - \p x is forwarded to construct a coefficient;
		 *     - an empty arguments set will be used to construct a key;
		 *     - coefficient and key are used to construct the new term instance;
		 *   - the new term is inserted into \p this.
		 *
		 * @param[in] x object to construct from.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the copy assignment operators of piranha::symbol_set and piranha::hash_set,
		 * - the construction of a coefficient from \p x or of a key from piranha::symbol_set,
		 * - the construction of a term from a coefficient-key pair,
		 * - insert(),
		 * - the copy constructor of piranha::series.
		 */
		template <typename T, typename U = series, typename std::enable_if<!std::is_same<series,typename std::decay<T>::type>::value &&
			generic_ctor_enabler<T,U>::value,int>::type = 0>
		explicit series(T &&x)
		{
			dispatch_generic_construction(std::forward<T>(x));
		}
		/// Trivial destructor.
		~series()
		{
			PIRANHA_TT_CHECK(std::is_base_of,series,Derived);
			PIRANHA_TT_CHECK(is_series,Derived);
			// Static checks on the iterator types.
			PIRANHA_TT_CHECK(is_input_iterator,const_iterator);
			piranha_assert(destruction_checks());
		}
		/// Copy-assignment operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the copy constructor.
		 */
		series &operator=(const series &other)
		{
			if (likely(this != &other)) {
				series tmp(other);
				*this = std::move(tmp);
			}
			return *this;
		}
		/// Defaulted move assignment operator.
		series &operator=(series &&) = default;
		/// Generic assignment operator.
		/**
		 * This template constructor is enabled only if \p T is different from the type of \p this,
		 * and \p x can be used to construct piranha::series.
		 * That is, generic assignment is equivalent to assignment to a piranha::series constructed
		 * via the generic constructor.
		 * 
		 * @param[in] x assignment argument.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the generic constructor.
		 */
		template <typename T>
		typename std::enable_if<!std::is_same<series,typename std::decay<T>::type>::value &&
			std::is_constructible<series,T>::value,
			series &>::type operator=(T &&x)
		{
			return operator=(series(std::forward<T>(x)));
		}
		/// Symbol set getter.
		/**
		 * @return const reference to the piranha::symbol_set associated to the series.
		 */
		const symbol_set &get_symbol_set() const
		{
			return m_symbol_set;
		}
		/// Series size.
		/**
		 * @return the number of terms in the series.
		 */
		size_type size() const
		{
			return m_container.size();
		}
		/// Empty test.
		/**
		 * @return \p true if size() is nonzero, \p false otherwise.
		 */
		bool empty() const
		{
			return !size();
		}
		/// Test for single-coefficient series.
		/**
		 * A series is considered to be <em>single-coefficient</em> when it is symbolically equivalent to a coefficient.
		 * That is, the series is either empty (in which case it is considered to be equivalent to a coefficient constructed
		 * from zero) or consisting of a single term with unitary key (in which case the series is considered equivalent to
		 * its only coefficient).
		 * 
		 * @return \p true in case of single-coefficient series, \p false otherwise.
		 * 
		 * @throws unspecified any exception thrown by the <tt>is_unitary()</tt> method of the key type.
		 */
		bool is_single_coefficient() const
		{
			return (empty() || (size() == 1u && m_container.begin()->m_key.is_unitary(m_symbol_set)));
		}
		/// Insert generic term.
		/**
		 * This method will insert \p term into the series using internally piranha::hash_set::insert. The method is enabled only
		 * if \p term derives from piranha::base_term.
		 * 
		 * The insertion algorithm proceeds as follows:
		 * - if \p term is not of type series::term_type, its coefficient and key are forwarded to construct a series::term_type
		 *   as follows:
		 *     - <tt>term</tt>'s coefficient is forwarded to construct a coefficient of type series::term_type::cf_type;
		 *     - <tt>term</tt>'s key is forwarded, together with the series' piranha::symbol_set,
		 *       to construct a key of type series::term_type::key_type;
		 *     - the newly-constructed coefficient and key are used to construct an instance of series::term_type, which will replace \p term as the
		 *       argument of insertion for the remaining portion of the algorithm;
		 * - if the term is not compatible for insertion, an \p std::invalid_argument exception is thrown;
		 * - if the term is ignorable, the method will return;
		 * - if the term is already in the series, then:
		 *   - its coefficient is added (if \p Sign is \p true) or subtracted (if \p Sign is \p false)
		 *     to the existing term's coefficient;
		 *   - if, after the addition/subtraction the existing term is ignorable, it will be erased;
		 * - else:
		 *   - the term is inserted into the term container and, if \p Sign is \p false, its coefficient is negated.
		 * 
		 * After any modification to an existing term in the series (e.g., via insertion with negative \p Sign or via in-place addition
		 * or subtraction of existing coefficients), the term will be checked again for compatibility and ignorability, and, in case
		 * the term has become incompatible or ignorable, it will be erased from the series.
		 * 
		 * The exception safety guarantee upon insertion is that the series will be left in an undefined but valid state. Such a guarantee
		 * relies on the fact that the addition/subtraction and negation methods of the coefficient type will leave the coefficient in a valid
		 * (possibly undefined) state in face of exceptions.
		 * 
		 * @param[in] term term to be inserted.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the constructors of series::term_type, and of its coefficient and key types,
		 *   invoked by any necessary conversion;
		 * - piranha::hash_set::insert(),
		 * - piranha::hash_set::find(),
		 * - piranha::hash_set::erase(),
		 * - piranha::math::negate(), in-place addition/subtraction on coefficient types.
		 * @throws std::invalid_argument if \p term is incompatible.
		 */
		template <bool Sign, typename T>
		typename std::enable_if<is_instance_of<typename std::decay<T>::type,base_term>::value,void>::type insert(T &&term)
		{
			dispatch_insertion<Sign>(std::forward<T>(term));
		}
		/// Insert generic term with <tt>Sign = true</tt>.
		/**
		 * Convenience wrapper for the generic insert() method, with \p Sign set to \p true.
		 * 
		 * @param[in] term term to be inserted.
		 * 
		 * @throws unspecified any exception thrown by generic insert().
		 */
		template <typename T>
		void insert(T &&term)
		{
			insert<true>(std::forward<T>(term));
		}
		/// Identity operator.
		/**
		 * @return copy of \p this, cast to \p Derived.
		 * 
		 * @throws unspecified any exception thrown by the copy constructor.
		 */
		Derived operator+() const
		{
			return Derived(*static_cast<Derived const *>(this));
		}
		/// Negation operator.
		/**
		 * @return a copy of \p this on which negate() has been called.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - negate(),
		 * - the copy constructor of \p Derived.
		 */
		Derived operator-() const
		{
			Derived retval(*static_cast<Derived const *>(this));
			retval.negate();
			return retval;
		}
		/// Negate series in-place.
		/**
		 * This method will call math::negate() on the coefficients of all terms. In case of exceptions,
		 * the basic exception safety guarantee is provided.
		 * 
		 * If any term becomes ignorable or incompatible after negation, it will be erased from the series.
		 * 
		 * @throws unspecified any exception thrown by math::negate() on the coefficient type.
		 */
		void negate()
		{
			try {
				const auto it_f = m_container.end();
				for (auto it = m_container.begin(); it != it_f;) {
					math::negate(it->m_cf);
					if (unlikely(!it->is_compatible(m_symbol_set) || it->is_ignorable(m_symbol_set))) {
						it = m_container.erase(it);
					} else {
						++it;
					}
				}
			} catch (...) {
				m_container.clear();
				throw;
			}
		}
		/** @name Table-querying methods
		 * Methods to query the properties of the internal container used to store the terms.
		 */
		//@{
		/// Table sparsity.
		/**
		 * Will call piranha::hash_set::evaluate_sparsity() on the internal terms container
		 * and return the result.
		 * 
		 * @return the output of piranha::hash_set::evaluate_sparsity().
		 * 
		 * @throws unspecified any exception thrown by piranha::hash_set::evaluate_sparsity().
		 */
		sparsity_info_type table_sparsity() const
		{
			return m_container.evaluate_sparsity();
		}
		/// Table load factor.
		/**
		 * Will call piranha::hash_set::load_factor() on the internal terms container
		 * and return the result.
		 * 
		 * @return the load factor of the internal container.
		 */
		double table_load_factor() const
		{
			return m_container.load_factor();
		}
		/// Table bucket count.
		/**
		 * @return the bucket count of the internal container.
		 */
		size_type table_bucket_count() const
		{
			return m_container.bucket_count();
		}
		//@}
		/// Exponentiation.
		/**
		 * \note
		 * This method is enabled only if:
		 * - the coefficient type is exponentiable to the power of \p x, and the return type is the coefficient type itself,
		 * - \p T can be used as argument for piranha::math::is_zero() and piranha::safe_cast() to piranha::integer,
		 * - \p Derived is multipliable in place.
		 *
		 * Return \p this raised to the <tt>x</tt>-th power.
		 * 
		 * The exponentiation algorithm proceeds as follows:
		 * - if the series is single-coefficient, a call to apply_cf_functor() is attempted, using a functor that calls piranha::math::pow() on
		 *   the coefficient. Otherwise, the algorithm proceeds;
		 * - if \p x is zero (as established by piranha::math::is_zero()), a series with a single term
		 *   with unitary key and coefficient constructed from the integer numeral "1" is returned (i.e., any series raised to the power of zero
		 *   is 1 - including empty series);
		 * - if \p x represents a non-negative integral value, the return value is constructed via repeated multiplications;
		 * - otherwise, an exception will be raised.
		 * 
		 * @param[in] x exponent.
		 * 
		 * @return \p this raised to the power of \p x.
		 * 
		 * @throws std::invalid_argument if exponentiation is computed via repeated series multiplications and
		 * \p x does not represent a non-negative integer.
		 * @throws unspecified any exception thrown by:
		 * - series, term, coefficient and key construction,
		 * - insert(),
		 * - is_single_coefficient(),
		 * - apply_cf_functor(),
		 * - piranha::math::pow(), piranha::math::is_zero() and piranha::safe_cast(),
		 * - series multiplication.
		 */
		template <typename T, typename U = Derived>
		pow_ret_type<T,U> pow(const T &x) const
		{
			typedef typename term_type::cf_type cf_type;
			typedef typename term_type::key_type key_type;
			if (is_single_coefficient()) {
				auto pow_functor = [&x](const cf_type &cf) {return math::pow(cf,x);};
				return apply_cf_functor(pow_functor);
			}
			if (math::is_zero(x)) {
				Derived retval;
				retval.insert(term_type(cf_type(1),key_type(symbol_set{})));
				return retval;
			}
			return pow_impl(x);
		}
		/// Apply functor to single-coefficient series.
		/**
		 * This method can be called successfully only on single-coefficient series.
		 * 
		 * If the series is empty, the return value will be a series with single term and unitary key in which the coefficient
		 * is the result of applying the functor \p f on a coefficient instance constructed from the integral constant "0".
		 * 
		 * If the series has a single term with unitary key, the return value will be a series with single term and unitary key in which the coefficient
		 * is the result of applying the functor \p f to the only coefficient of \p this.
		 * 
		 * @param[in] f coefficient functor.
		 * 
		 * @return a series constructed via the application of \p f to a coefficient instance as described above.
		 * 
		 * @throws std::invalid_argument if the series is not single-coefficient.
		 * @throws unspecified any exception thrown by:
		 * - the call operator of \p f,
		 * - construction of coefficient, key and term instances,
		 * - insert(),
		 * - is_single_coefficient().
		 */
		Derived apply_cf_functor(std::function<typename term_type::cf_type (const typename term_type::cf_type &)> f) const
		{
			if (!is_single_coefficient()) {
				piranha_throw(std::invalid_argument,"cannot apply functor, series is not single-coefficient");
			}
			typedef typename term_type::cf_type cf_type;
			typedef typename term_type::key_type key_type;
			Derived retval;
			if (empty()) {
				retval.insert(term_type(f(cf_type(0)),key_type(symbol_set{})));
			} else {
				retval.insert(term_type(f(m_container.begin()->m_cf),key_type(symbol_set{})));
			}
			return retval;
		}
		/// Partial derivative.
		/**
		 * \note
		 * This method is enabled only if the coefficient and key are differentiable (i.e., they satisfy the piranha::is_differentiable
		 * and piranha::key_is_differentiable type traits), and if the arithmetic operations
		 * needed to compute the partial derivative are supported by all the involved types.
		 *
		 * This method will return the partial derivative of \p this with respect to the variable called \p name. The method will construct
		 * the return value from the output of the differentiation methods of coefficient and key, and via arithmetic and/or term insertion
		 * operations.
		 *
		 * Note that, contrary to the specialisation of piranha::math::partial() for series types, this method will not take into account
		 * custom derivatives registered via piranha::series::register_custom_derivative().
		 *
		 * @param[in] name name of the argument with respect to which the derivative will be calculated.
		 *
		 * @return partial derivative of \p this with respect to the symbol named \p name.
		 *
		 * @throws unspecified any exception thrown by:
		 * - the differentiation methods of coefficient and key,
		 * - term construction and insertion,
		 * - arithmetic operations on the involved types,
		 * - construction of the return type.
		 */
		template <typename Series = Derived>
		partial_type<Series> partial(const std::string &name) const
		{
			return partial_impl(name);
		}
		/// Register custom partial derivative.
		/**
		 * \note
		 * This method is enabled only if piranha::series::partial() is enabled for \p Derived, and if \p F
		 * is a type that can be used to construct <tt>std::function<partial_type(const Derived &)</tt>, where
		 * \p partial_type is the type resulting from the partial derivative of \p Derived.
		 *
		 * Register a copy of a callable \p func associated to the symbol called \p name for use by piranha::math::partial().
		 * \p func will be used to compute the partial derivative of instances of type \p Derived with respect to
		 * \p name in place of the default partial differentiation algorithm.
		 * 
		 * It is safe to call this method from multiple threads.
		 * 
		 * @param[in] name symbol for which the custom partial derivative function will be registered.
		 * @param[in] func custom partial derivative function.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - failure(s) in threading primitives,
		 * - lookup and insertion operations on \p std::unordered_map,
		 * - construction and move-assignment of \p std::function.
		 */
		template <typename F, typename Series = Derived, custom_partial_enabler<F,Series> = 0>
		static void register_custom_derivative(const std::string &name, F func)
		{
			using p_type = decltype(std::declval<const Derived &>().partial(name));
			using f_type = std::function<p_type(const Derived &)>;
			std::lock_guard<std::mutex> lock(cp_mutex);
			get_cp_map()[name] = f_type(func);
		}
		/// Unregister custom partial derivative.
		/**
		 * \note
		 * This method is enabled only if piranha::series::partial() is enabled for \p Derived.
		 *
		 * Unregister the custom partial derivative function associated to the symbol called \p name. If no custom
		 * partial derivative was previously registered using register_custom_derivative(), calling this function will be a no-op.
		 * 
		 * It is safe to call this method from multiple threads.
		 * 
		 * @param[in] name symbol for which the custom partial derivative function will be unregistered.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - failure(s) in threading primitives,
		 * - lookup and erase operations on \p std::unordered_map.
		 */
		// NOTE: the additional template parameter is to disable the method in case the partial
		// type is malformed.
		template <typename Series = Derived, typename Partial = partial_type<Series>>
		static void unregister_custom_derivative(const std::string &name)
		{
			std::lock_guard<std::mutex> lock(cp_mutex);
			auto it = get_cp_map().find(name);
			if (it != get_cp_map().end()) {
				get_cp_map().erase(it);
			}
		}
		/// Unregister all custom partial derivatives.
		/**
		 * \note
		 * This method is enabled only if piranha::series::partial() is enabled for \p Derived.
		 *
		 * Will unregister all custom derivatives currently registered via register_custom_derivative().
		 * It is safe to call this method from multiple threads.
		 * 
		 * @throws unspecified any exception thrown by failure(s) in threading primitives.
		 */
		template <typename Series = Derived, typename Partial = partial_type<Series>>
		static void unregister_all_custom_derivatives()
		{
			std::lock_guard<std::mutex> lock(cp_mutex);
			get_cp_map().clear();
		}
		/// Begin iterator.
		/**
		 * Return an iterator to the first term of the series. The returned iterator will
		 * provide, when dereferenced, an \p std::pair in which the first element is a copy of the coefficient of
		 * the term, whereas the second element is a single-term instance of \p Derived built from the term's key
		 * and a unitary coefficient.
		 * 
		 * Note that terms are stored unordered in the series, hence it is not defined which particular
		 * term will be returned by calling this method. The only guarantee is that the iterator can be used to transverse
		 * all the series' terms until piranha::series::end() is eventually reached.
		 * 
		 * Calling any non-const method on the series will invalidate the iterators obtained via piranha::series::begin() and piranha::series::end().
		 * 
		 * @return an iterator to the first term of the series.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - construction and assignment of piranha::symbol_set,
		 * - insert(),
		 * - construction of term, coefficient and key instances.
		 */
		const_iterator begin() const
		{
			typedef std::function<std::pair<typename term_type::cf_type,Derived>(const term_type &)>
				func_type;
			auto func = [this](const term_type &t) {
				return detail::pair_from_term<term_type,Derived>(this->m_symbol_set,t);
			};
			return boost::make_transform_iterator(m_container.begin(),func_type(std::move(func)));
		}
		/// End iterator.
		/**
		 * Return an iterator one past the last term of the series. See the documentation of piranha::series::begin()
		 * on how the returned iterator can be used.
		 * 
		 * @return an iterator to the end of the series.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - construction and assignment of piranha::symbol_set,
		 * - insert(),
		 * - construction of term, coefficient and key instances.
		 */
		const_iterator end() const
		{
			typedef std::function<std::pair<typename term_type::cf_type,Derived>(const term_type &)>
				func_type;
			auto func = [this](const term_type &t) {
				return detail::pair_from_term<term_type,Derived>(this->m_symbol_set,t);
			};
			return boost::make_transform_iterator(m_container.end(),func_type(std::move(func)));
		}
		/// Term filtering.
		/**
		 * This method will apply the functor \p func to each term in the series, and produce a return series
		 * containing all terms in \p this for which \p func returns \p true.
		 * Terms are passed to \p func in the format resulting from dereferencing the iterators obtained
		 * via piranha::series::begin().
		 * 
		 * @param[in] func filtering functor.
		 * 
		 * @return filtered series.
		 * 
		 * @throw unspecified any exception thrown by:
		 * - the call operator of \p func,
		 * - insert(),
		 * - the assignment operator of piranha::symbol_set,
		 * - term, coefficient, key construction.
		 */
		Derived filter(std::function<bool(const std::pair<typename term_type::cf_type,Derived> &)> func) const
		{
			Derived retval;
			retval.m_symbol_set = m_symbol_set;
			const auto it_f = this->m_container.end();
			for (auto it = this->m_container.begin(); it != it_f; ++it) {
				if (func(detail::pair_from_term<term_type,Derived>(m_symbol_set,*it))) {
					retval.insert(*it);
				}
			}
			return retval;
		}
		/// Term transformation.
		/**
		 * This method will apply the functor \p func to each term in the series, and will use the return
		 * value of the functor to construct a new series. Terms are passed to \p func in the same format
		 * resulting from dereferencing the iterators obtained via piranha::series::begin(), and \p func is expected to produce
		 * a return value of the same type.
		 * 
		 * The return series is first initialised as an empty series. For each input term \p t, the return value
		 * of \p func is used to construct a new temporary series from the multiplication of \p t.first and
		 * \p t.second. Each temporary series is then added to the return value series.
		 * 
		 * This method requires the coefficient type to be multipliable by \p Derived.
		 * 
		 * @param[in] func transforming functor.
		 * 
		 * @return transformed series.
		 *
		 * @throw unspecified any exception thrown by:
		 * - the call operator of \p func,
		 * - insert(),
		 * - the assignment operator of piranha::symbol_set,
		 * - term, coefficient, key construction,
		 * - series multiplication and addition.
		 * 
		 * \todo require multipliability of cf * Derived and addability of the result to Derived in place.
		 */
		Derived transform(std::function<std::pair<typename term_type::cf_type,Derived>
			(const std::pair<typename term_type::cf_type,Derived> &)> func) const
		{
			Derived retval;
			std::pair<typename term_type::cf_type,Derived> tmp;
			const auto it_f = this->m_container.end();
			for (auto it = this->m_container.begin(); it != it_f; ++it) {
				tmp = func(detail::pair_from_term<term_type,Derived>(m_symbol_set,*it));
				// NOTE: here we could use multadd, but it seems like there's not
				// much benefit (plus, the types involved are different).
				retval += tmp.first * tmp.second;
			}
			return retval;
		}
		/// Evaluation.
		/**
		 * \note
		 * This method is enabled only if:
		 * - both the coefficient and the key types are evaluable,
		 * - the evaluated types are suitable for use in piranha::math::multiply_accumulate(),
		 * - the return type is constructible from \p int.
		 *
		 * Series evaluation starts with a zero-initialised instance of the return type, which is determined
		 * according to the evaluation types of coefficient and key. The return value accumulates the evaluation
		 * of all terms in the series via the product of the evaluations of the coefficient-key pairs in each term.
		 * The input dictionary \p dict specifies with which value each symbolic quantity will be evaluated.
		 * 
		 * @param[in] dict dictionary of that will be used for evaluation.
		 * 
		 * @return evaluation of the series according to the evaluation dictionary \p dict.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - coefficient and key evaluation,
		 * - insertion operations on \p std::unordered_map,
		 * - piranha::math::multiply_accumulate().
		 */
		template <typename T, typename Series = series>
		eval_type<Series,T> evaluate(const std::unordered_map<std::string,T> &dict) const
		{
			// NOTE: possible improvement: if the evaluation type is less-than comparable,
			// build a vector of evaluated terms, sort it and accumulate (to minimise accuracy loss
			// with fp types and maybe improve performance - e.g., for integers).
			using return_type = eval_type<Series,T>;
			// Transform the string dict into symbol dict for use in keys.
			std::unordered_map<symbol,T> s_dict;
			for (auto it = dict.begin(); it != dict.end(); ++it) {
				s_dict[symbol(it->first)] = it->second;
			}
			// Convert to positions map.
			symbol_set::positions_map<T> pmap(this->m_symbol_set,s_dict);
			// Init return value and accumulate it.
			return_type retval = return_type(0);
			const auto it_f = this->m_container.end();
			for (auto it = this->m_container.begin(); it != it_f; ++it) {
				math::multiply_accumulate(retval,math::evaluate(it->m_cf,dict),it->m_key.evaluate(pmap,m_symbol_set));
			}
			return retval;
		}
		/// Trim.
		/**
		 * This method will return a series mathematically equivalent to \p this in which discardable arguments
		 * have been removed from the internal set of symbols. Which symbols are removed depends on the trimming
		 * method \p trim_identify() of the key type (e.g., in a polynomial a symbol can be discarded if its exponent
		 * is zero in all monomials).
		 * 
		 * If the coefficient type is an instance of piranha::series, trim() will be called recursively on the coefficients
		 * while building the return value.
		 * 
		 * @return trimmed version of \p this.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - operations on piranha::symbol_set,
		 * - the trimming methods of coefficient and/or key,
		 * - insert(),
		 * - term, coefficient and key type construction.
		 */
		Derived trim() const
		{
			// Build the set of symbols that can be removed.
			const auto it_f = this->m_container.end();
			symbol_set trim_ss(m_symbol_set);
			for (auto it = this->m_container.begin(); it != it_f; ++it) {
				it->m_key.trim_identify(trim_ss,m_symbol_set);
			}
			// Determine the new set.
			Derived retval;
			retval.m_symbol_set = m_symbol_set.diff(trim_ss);
			for (auto it = this->m_container.begin(); it != it_f; ++it) {
				retval.insert(term_type(trim_cf_impl(it->m_cf),it->m_key.trim(trim_ss,m_symbol_set)));
			}
			return retval;
		}
		/// Print in TeX mode.
		/**
		 * Print series to stream \p os in TeX mode. The representation is constructed in the same way as explained in
		 * piranha::series::operator<<(), but using piranha::print_tex_coefficient() and the key's TeX printing method instead of the plain
		 * printing functions.
		 * 
		 * @param os target stream.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::print_tex_coefficient(),
		 * - the TeX printing method of the key type,
		 * - memory allocation errors in standard containers,
		 * - piranha::settings::get_max_term_output(),
		 * - streaming to \p os or to instances of \p std::ostringstream.
		 * 
		 * @see operator<<().
		 */
		void print_tex(std::ostream &os) const
		{
			// If the series is empty, print zero and exit.
			if (empty()) {
				os << "0";
				return;
			}
			print_helper_1<true>(os,m_container.begin(),m_container.end(),m_symbol_set);
		}
		/// Overloaded stream operator for piranha::series.
		/**
		 * Will direct to stream a human-readable representation of the series.
		 * 
		 * The human-readable representation of the series is built as follows:
		 * 
		 * - the coefficient and key of each term are printed adjacent to each other separated by the character "*",
		 *   the former via the piranha::print_coefficient() function, the latter via its <tt>print()</tt> method;
		 * - terms are separated by a "+" sign.
		 * 
		 * The following additional transformations take place on the printed output:
		 * 
		 * - if the printed output of a coefficient is the string "1" and the printed output of its key
		 *   is not empty, the coefficient and the "*" sign are not printed;
		 * - if the printed output of a coefficient is the string "-1" and the printed output of its key
		 *   is not empty, the printed output of the coefficient is transformed into "-" and the sign "*" is not printed;
		 * - if the key output is empty, the sign "*" is not printed;
		 * - the sequence of characters "+-" is transformed into "-";
		 * - at most piranha::settings::get_max_term_output() terms are printed, and terms in excess are
		 *   represented with ellipsis "..." at the end of the output; if piranha::settings::get_max_term_output()
		 *   is zero, all the terms will be printed.
		 * 
		 * Note that the print order of the terms will be undefined.
		 * 
		 * @param[in,out] os target stream.
		 * @param[in] s piranha::series argument.
		 * 
		 * @return reference to \p os.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::print_coefficient(),
		 * - the <tt>print()</tt> method of the key type,
		 * - memory allocation errors in standard containers,
		 * - piranha::settings::get_max_term_output(),
		 * - streaming to \p os or to instances of \p std::ostringstream.
		 */
		friend std::ostream &operator<<(std::ostream &os, const series &s)
		{
			// If the series is empty, print zero and exit.
			if (s.empty()) {
				os << "0";
				return os;
			}
			return print_helper_1<false>(os,s.m_container.begin(),s.m_container.end(),s.m_symbol_set);
		}
	protected:
		/// Symbol set.
		symbol_set	m_symbol_set;
		/// Terms container.
		container_type	m_container;
	private:
		static std::mutex	cp_mutex;
};

// Static initialisation.
template <typename Term, typename Derived>
std::mutex series<Term,Derived>::cp_mutex;

/// Specialisation of piranha::print_coefficient_impl for series.
/**
 * This specialisation is enabled if \p Series is an instance of piranha::series.
 */
template <typename Series>
struct print_coefficient_impl<Series,typename std::enable_if<
	is_series<Series>::value>::type>
{
	/// Call operator.
	/**
	 * Equivalent to the stream operator overload of piranha::series, apart from a couple
	 * of parentheses '()' enclosing the coefficient series if its size is larger than 1.
	 * 
	 * @param[in] os target stream.
	 * @param[in] s coefficient series to be printed.
	 * 
	 * @throws unspecified any exception thrown by the stream operator overload of piranha::series.
	 */
	void operator()(std::ostream &os, const Series &s) const
	{
		if (s.size() > 1u) {
			os << '(';
		}
		os << s;
		if (s.size() > 1u) {
			os << ')';
		}
	}
};

/// Specialisation of piranha::print_tex_coefficient_impl for series.
/**
 * This specialisation is enabled if \p Series is an instance of piranha::series.
 */
template <typename Series>
struct print_tex_coefficient_impl<Series,typename std::enable_if<
	is_series<Series>::value>::type>
{
	/// Call operator.
	/**
	 * Equivalent to piranha::series::print_tex(), apart from a couple
	 * of parentheses '()' enclosing the coefficient series if its size is larger than 1.
	 * 
	 * @param[in] os target stream.
	 * @param[in] s coefficient series to be printed.
	 * 
	 * @throws unspecified any exception thrown by piranha::series::print_tex().
	 */
	void operator()(std::ostream &os, const Series &s) const
	{
		if (s.size() > 1u) {
			os << "\\left(";
		}
		s.print_tex(os);
		if (s.size() > 1u) {
			os << "\\right)";
		}
	}
};

namespace math
{

/// Specialisation of the piranha::math::negate() functor for piranha::series.
/**
 * This specialisation is enabled if \p T is an instance of piranha::series.
 */
template <typename T>
struct negate_impl<T,typename std::enable_if<is_series<T>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in,out] s piranha::series to be negated.
	 * 
	 * @return the return value of piranha::series::negate()..
	 * 
	 * @throws unspecified any exception thrown by piranha::series::negate().
	 */
	template <typename U>
	auto operator()(U &s) const -> decltype(s.negate())
	{
		return s.negate();
	}
};

/// Specialisation of the piranha::math::is_zero() functor for piranha::series.
/**
 * This specialisation is activated when \p Series is an instance of piranha::series.
 * The result will be computed via the series' <tt>empty()</tt> method.
 */
template <typename Series>
struct is_zero_impl<Series,typename std::enable_if<is_series<Series>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] s piranha::series to be tested.
	 * 
	 * @return \p true if \p s is empty, \p false otherwise.
	 */
	bool operator()(const Series &s) const
	{
		return s.empty();
	}
};

/// Specialisation of the piranha::math::pow() functor for piranha::series.
/**
 * This specialisation is activated when \p Series is an instance of piranha::series.
 * The result will be computed via the series' <tt>pow()</tt> method.
 */
template <typename Series, typename T>
struct pow_impl<Series,T,typename std::enable_if<is_series<Series>::value>::type>
{
	/// Call operator.
	/**
	 * The exponentiation will be computed via the series' <tt>pow()</tt> method.
	 * 
	 * @param[in] s base.
	 * @param[in] x exponent.
	 * 
	 * @return \p s to the power of \p x.
	 * 
	 * @throws unspecified any exception resulting from the series' <tt>pow()</tt> method.
	 */
	template <typename S, typename U>
	auto operator()(const S &s, const U &x) const -> decltype(s.pow(x))
	{
		return s.pow(x);
	}
};

}

namespace detail
{

// All this gook can go back into the private impl methods once we switch to GCC 4.8.
template <typename T>
class series_has_sin: sfinae_types
{
		template <typename U>
		static auto test(const U *t) -> decltype(t->sin());
		static no test(...);
	public:
		static const bool value = std::is_same<decltype(test((const T *)nullptr)),T>::value;
};

template <typename T>
inline T series_sin_call_impl(const T &s, typename std::enable_if<series_has_sin<T>::value>::type * = nullptr)
{
	return s.sin();
}

// NOTE: this is similar to the approach that we use in trigonometric series. It is one possible way of accomplishing this,
// but this particular form seems to work ok across a variety of compilers - especially in GCC, the interplay between template
// aliases, decltype() and sfinae seems to be brittle in early versions. Variations of this are used also in power_series
// and t_subs series.
// NOTE: template aliases are dispatched immediately where they are used, after that normal SFINAE rules apply. See, e.g., the usage
// in this example:
// http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_active.html#1558
template <typename T>
using sin_cf_enabler = typename std::enable_if<!series_has_sin<T>::value &&
	std::is_same<typename T::term_type::cf_type,decltype(piranha::math::sin(std::declval<typename T::term_type::cf_type>()))>::value>::type;

template <typename T>
inline T series_sin_call_impl(const T &s, sin_cf_enabler<T> * = nullptr)
{
	typedef typename T::term_type::cf_type cf_type;
	auto f = [](const cf_type &cf) {return piranha::math::sin(cf);};
	try {
		return s.apply_cf_functor(f);
	} catch (const std::invalid_argument &) {
		piranha_throw(std::invalid_argument,"series is unsuitable for the calculation of sine");
	}
}

template <typename T>
class series_has_cos: sfinae_types
{
		template <typename U>
		static auto test(const U *t) -> decltype(t->cos());
		static no test(...);
	public:
		static const bool value = std::is_same<decltype(test((const T *)nullptr)),T>::value;
};

template <typename T>
inline T series_cos_call_impl(const T &s, typename std::enable_if<series_has_cos<T>::value>::type * = nullptr)
{
	return s.cos();
}

template <typename T>
using cos_cf_enabler = typename std::enable_if<!series_has_cos<T>::value &&
	std::is_same<typename T::term_type::cf_type,decltype(piranha::math::cos(std::declval<typename T::term_type::cf_type>()))>::value>::type;

template <typename T>
inline T series_cos_call_impl(const T &s, cos_cf_enabler<T> * = nullptr)
{
	typedef typename T::term_type::cf_type cf_type;
	auto f = [](const cf_type &cf) {return piranha::math::cos(cf);};
	try {
		return s.apply_cf_functor(f);
	} catch (const std::invalid_argument &) {
		piranha_throw(std::invalid_argument,"series is unsuitable for the calculation of cosine");
	}
}

}

namespace math
{

/// Specialisation of the piranha::math::sin() functor for piranha::series.
/**
 * This specialisation is activated when \p Series is an instance of piranha::series.
 * If the series type provides a const <tt>Series::sin()</tt> method returning an object of type \p Series,
 * it will be used for the computation of the result.
 * 
 * Otherwise, a call to piranha::series::apply_cf_functor() will be attempted with a functor that
 * calculates piranha::math::sin().
 */
template <typename Series>
struct sin_impl<Series,typename std::enable_if<is_series<Series>::value>::type>
{
	/// Call operator.
	/**
	 * \note
	 * This operator is enabled if one of these conditions apply:
	 * - the input series type has a const <tt>sin()</tt> method, or
	 * - the coefficient type of the series satisfies piranha::has_sine, returning an instance of the
	 *   coefficient type as result.
	 *
	 * @param[in] s argument.
	 *
	 * @return sine of \p s.
	 *
	 * @throws unspecified any exception thrown by:
	 * - the <tt>Series::sin()</tt> method,
	 * - piranha::math::sin(),
	 * - piranha::series::apply_cf_functor().
	 */
	template <typename T>
	auto operator()(const T &s) const -> decltype(detail::series_sin_call_impl(s))
	{
		return detail::series_sin_call_impl(s);
	}
};

/// Specialisation of the piranha::math::cos() functor for piranha::series.
/**
 * This specialisation is activated when \p Series is an instance of piranha::series.
 * If the series type provides a const <tt>Series::cos()</tt> method returning an object of type \p Series,
 * it will be used for the computation of the result.
 * 
 * Otherwise, a call to piranha::series::apply_cf_functor() will be attempted with a functor that
 * calculates piranha::math::cos().
 */
template <typename Series>
struct cos_impl<Series,typename std::enable_if<is_series<Series>::value>::type>
{
	/// Call operator.
	/**
	 * \note
	 * This operator is enabled if one of these conditions apply:
	 * - the input series type has a const <tt>cos()</tt> method, or
	 * - the coefficient type of the series satisfies piranha::has_cosine, returning an instance of the
	 *   coefficient type as result.
	 *
	 * @param[in] s argument.
	 *
	 * @return cosine of \p s.
	 *
	 * @throws unspecified any exception thrown by:
	 * - the <tt>Series::cos()</tt> method,
	 * - piranha::math::cos(),
	 * - piranha::series::apply_cf_functor().
	 */
	template <typename T>
	auto operator()(const T &s) const -> decltype(detail::series_cos_call_impl(s))
	{
		return detail::series_cos_call_impl(s);
	}
};

/// Specialisation of the piranha::math::partial() functor for series types.
/**
 * This specialisation is activated when \p Series is an instance of piranha::series.
 */
template <typename Series>
struct partial_impl<Series,typename std::enable_if<is_series<Series>::value>::type>
{
	/// Call operator.
	/**
	 * The call operator will first check whether a custom partial derivative for \p Series was registered
	 * via piranha::series::register_custom_derivative(). In such a case, the custom derivative function will be used
	 * to compute the return value. Otherwise, the output of piranha::series::partial() will be returned.
	 * 
	 * @param[in] s input series.
	 * @param[in] name name of the argument with respect to which the differentiation will be calculated.
	 * 
	 * @return the partial derivative of \p s with respect to \p name.
	 * 
	 * @throws unspecified any exception thrown by:
	 * - piranha::series::partial(),
	 * - failure(s) in threading primitives,
	 * - lookup operations on \p std::unordered_map,
	 * - the copy assignment and call operators of the registered custom partial derivative function.
	 */
	template <typename T>
	auto operator()(const T &s, const std::string &name) -> decltype(s.partial(name))
	{
		using partial_type = decltype(s.partial(name));
		bool custom = false;
		std::function<partial_type(const T &)> func;
		// Try to locate a custom partial derivative and copy it into func, if found.
		{
			std::lock_guard<std::mutex> lock(T::cp_mutex);
			auto it = s.get_cp_map().find(name);
			if (it != s.get_cp_map().end()) {
				func = it->second;
				custom = true;
			}
		}
		if (custom) {
			return func(s);
		}
		return s.partial(name);
	}
};

/// Specialisation of the piranha::math::evaluate() functor for series types.
/**
 * This specialisation is activated when \p Series is an instance of piranha::series.
 */
template <typename Series>
struct evaluate_impl<Series,typename std::enable_if<is_series<Series>::value>::type>
{
	/// Call operator.
	/**
	 * The implementation will use piranha::series::evaluate().
	 *
	 * @param[in] s evaluation argument.
	 * @param[in] dict evaluation dictionary.
	 *
	 * @return output of piranha::series::evaluate().
	 *
	 * @throws unspecified any exception thrown by piranha::series::evaluate().
	 */
	template <typename T>
	auto operator()(const Series &s, const std::unordered_map<std::string,T> &dict) const -> decltype(s.evaluate(dict))
	{
		return s.evaluate(dict);
	}
};

}

}

#endif
