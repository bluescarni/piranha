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

#ifndef PIRANHA_POWER_SERIES_HPP
#define PIRANHA_POWER_SERIES_HPP

#include <set>
#include <string>
#include <type_traits>
#include <utility>

#include "forwarding.hpp"
#include "math.hpp"
#include "series.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Power series toolbox.
/**
 * This toolbox is intended to extend the \p Series type with properties of formal power series.
 *
 * Specifically, the toolbox will conditionally augment a \p Series type by adding methods to query the total and partial (low) degree
 * of a \p Series object. Such augmentation takes place if the series' coefficient and/or key types expose methods to query
 * their degree properties (as established by the piranha::has_degree, piranha::key_has_degree and similar type traits).
 *
 * As an additional requirement, the types returned when querying the degree must be default-constructible, constructible from \p int,
 * copy-constructible, assignable, move-assignable, and less-than comparable. If these additional requirements are not satisfied,
 * the degree-querying methods will be disabled.
 *
 * This class satisfies the piranha::is_series type trait.
 *
 * \section type_requirements Type requirements
 *
 * \p Series must satisfy the piranha::is_series type trait.
 *
 * \section exception_safety Exception safety guarantee
 *
 * This class provides the same guarantee as \p Series.
 *
 * \section move_semantics Move semantics
 *
 * Move semantics is equivalent to the move semantics of \p Series.
 *
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename Series>
class power_series: public Series
{
		PIRANHA_TT_CHECK(is_series,Series);
		typedef Series base;
		// Detect power series terms.
		template <typename T>
		struct term_score
		{
			typedef typename T::term_type::cf_type cf_type;
			typedef typename T::term_type::key_type key_type;
			static const unsigned value = static_cast<unsigned>(has_degree<cf_type>::value && has_ldegree<cf_type>::value) |
						      (static_cast<unsigned>(key_has_degree<key_type>::value && key_has_ldegree<key_type>::value) << 1u);
		};
		// Common checks on degree/ldegree type for use in enabling conditions below.
		template <typename T>
		struct common_type_checks
		{
			static const bool value = std::is_default_constructible<T>::value &&
						  std::is_constructible<T,int>::value &&
						  std::is_copy_constructible<T>::value &&
						  std::is_assignable<T &,T>::value &&
						  std::is_move_assignable<T>::value &&
						  is_less_than_comparable<T>::value;
		};
		// Utilities to compute the degree.
		// Unspecialised version will not define any member, will SFINAE out.
		template <typename T, typename = void>
		struct degree_utils
		{
			static_assert(term_score<T>::value == 0u,"Invalid power series term score.");
		};
		// Case 1: only coefficient as degree/ldegree.
		template <typename T>
		struct degree_utils<T,typename std::enable_if<term_score<T>::value == 1u>::type>
		{
			#define PIRANHA_TMP_RETURN math::degree(t.m_cf,std::forward<Args>(args)...)
			template <typename Term, typename ... Args>
			static auto get(const Term &t, const symbol_set &, Args && ... args) ->
				typename std::enable_if<common_type_checks<decltype(PIRANHA_TMP_RETURN)>::value,decltype(PIRANHA_TMP_RETURN)>::type
			{
				return PIRANHA_TMP_RETURN;
			}
			#undef PIRANHA_TMP_RETURN
			#define PIRANHA_TMP_RETURN math::ldegree(t.m_cf,std::forward<Args>(args)...)
			template <typename Term, typename ... Args>
			static auto lget(const Term &t, const symbol_set &, Args && ... args) ->
				typename std::enable_if<common_type_checks<decltype(PIRANHA_TMP_RETURN)>::value,decltype(PIRANHA_TMP_RETURN)>::type
			{
				return PIRANHA_TMP_RETURN;
			}
			#undef PIRANHA_TMP_RETURN
		};
		// Case 2: only key has degree/ldegree.
		template <typename T>
		struct degree_utils<T,typename std::enable_if<term_score<T>::value == 2u>::type>
		{
			#define PIRANHA_TMP_RETURN t.m_key.degree(std::forward<Args>(args)...,s)
			template <typename Term, typename ... Args>
			static auto get(const Term &t, const symbol_set &s, Args && ... args) ->
				typename std::enable_if<common_type_checks<decltype(PIRANHA_TMP_RETURN)>::value,decltype(PIRANHA_TMP_RETURN)>::type
			{
				return PIRANHA_TMP_RETURN;
			}
			#undef PIRANHA_TMP_RETURN
			#define PIRANHA_TMP_RETURN t.m_key.ldegree(std::forward<Args>(args)...,s)
			template <typename Term, typename ... Args>
			static auto lget(const Term &t, const symbol_set &s, Args && ... args) ->
				typename std::enable_if<common_type_checks<decltype(PIRANHA_TMP_RETURN)>::value,decltype(PIRANHA_TMP_RETURN)>::type
			{
				return PIRANHA_TMP_RETURN;
			}
			#undef PIRANHA_TMP_RETURN
		};
		// Case 3: cf and key have both degree/ldegree.
		template <typename T>
		struct degree_utils<T,typename std::enable_if<term_score<T>::value == 3u>::type>
		{
			#define PIRANHA_TMP_RETURN math::degree(t.m_cf,std::forward<Args>(args)...) + t.m_key.degree(std::forward<Args>(args)...,s)
			// NOTE: this one is just a hack to work around what seems an issue in GCC 4.7 (4.8 and clang compile it just fine).
			// Remove it in next versions.
			#define PIRANHA_TMP_RETURN2 math::degree(std::declval<const Term &>().m_cf,std::declval<Args>()...) + std::declval<const Term &>().m_key.degree(std::declval<Args>()...,std::declval<const symbol_set &>())
			template <typename Term, typename ... Args>
			using degree_return_type = typename std::enable_if<common_type_checks<decltype(PIRANHA_TMP_RETURN2)>::value,decltype(PIRANHA_TMP_RETURN2)>::type;
			template <typename Term, typename ... Args>
			static auto get(const Term &t, const symbol_set &s, Args && ... args) -> degree_return_type<Term,Args...>
			{
				return PIRANHA_TMP_RETURN;
			}
			#undef PIRANHA_TMP_RETURN
			#undef PIRANHA_TMP_RETURN2
			#define PIRANHA_TMP_RETURN math::ldegree(t.m_cf,std::forward<Args>(args)...) + t.m_key.ldegree(std::forward<Args>(args)...,s)
			#define PIRANHA_TMP_RETURN2 math::ldegree(std::declval<const Term &>().m_cf,std::declval<Args>()...) + std::declval<const Term &>().m_key.ldegree(std::declval<Args>()...,std::declval<const symbol_set &>())
			template <typename Term, typename ... Args>
			using ldegree_return_type = typename std::enable_if<common_type_checks<decltype(PIRANHA_TMP_RETURN2)>::value,decltype(PIRANHA_TMP_RETURN2)>::type;
			template <typename Term, typename ... Args>
			static auto lget(const Term &t, const symbol_set &s, Args && ... args) -> ldegree_return_type<Term,Args...>
			{
				return PIRANHA_TMP_RETURN;
			}
			#undef PIRANHA_TMP_RETURN
			#undef PIRANHA_TMP_RETURN2
		};
	public:
		/// Defaulted default constructor.
		power_series() = default;
		/// Defaulted copy constructor.
		power_series(const power_series &) = default;
		/// Defaulted move constructor.
		power_series(power_series &&) = default;
		PIRANHA_FORWARDING_CTOR(power_series,base)
		/// Defaulted copy assignment operator.
		power_series &operator=(const power_series &) = default;
		/// Defaulted move assignment operator.
		power_series &operator=(power_series &&) = default;
		PIRANHA_FORWARDING_ASSIGNMENT(power_series,base)
		/// Trivial destructor.
		~power_series() noexcept(true)
		{
			PIRANHA_TT_CHECK(is_series,power_series);
		}
		/// Total and partial degree.
		/**
		 * \note
		 * This method is available only if the requisites outlined in piranha::power_series are satisfied.
		 *
		 * The degree of the series is the maximum degree of its terms. If the series is empty, zero will be returned.
		 * If the parameter pack has a size of zero, the total degree will be returned. If the parameter pack consists
		 * of a set of strings, the partial degree (i.e., calculated considering only the variables in the set) will be returned.
		 * In all other cases, the call is malformed and the method will be disabled.
		 *
		 * @param[in] args variadic parameter pack.
		 *
		 * @return the total or partial degree of the series.
		 *
		 * @throws unspecified any exception thrown by:
		 * - the construction of return type,
		 * - the calculation of the degree of each term,
		 * - the assignment and less-than operators for the return type.
		 */
		template <typename ... Args>
		auto degree(Args && ... args) const ->
			decltype(degree_utils<power_series>::get(std::declval<typename power_series::term_type>(),std::declval<symbol_set>(),std::forward<Args>(args)...))
		{
			// NOTE: here (and ldegree() as well) this could be implemented with std::max_element, but for this to work we need
			// a lambda that can capture the variadic arguments. Check back with more recent versions of GCC.
			typedef decltype(degree_utils<power_series>::get(std::declval<typename power_series::term_type>(),
				std::declval<symbol_set>(),std::forward<Args>(args)...)) return_type;
			if (this->empty()) {
				return return_type(0);
			}
			auto it = this->m_container.begin();
			const auto it_f = this->m_container.end();
			return_type retval = degree_utils<power_series>::get(*it,this->m_symbol_set,std::forward<Args>(args)...);
			++it;
			return_type tmp;
			for (; it != it_f; ++it) {
				tmp = degree_utils<power_series>::get(*it,this->m_symbol_set,std::forward<Args>(args)...);
				if (retval < tmp) {
					retval = std::move(tmp);
				}
			}
			return retval;
		}
		/// Total and partial low degree.
		/**
		 * \note
		 * This method is available only if the requisites outlined in piranha::power_series are satisfied.
		 *
		 * The low degree of the series is the minimum low degree of its terms. If the series is empty, zero will be returned.
		 * If the parameter pack has a size of zero, the total low degree will be returned. If the parameter pack consists
		 * of a set of strings, the partial low degree (i.e., calculated considering only the variables in the set) will be returned.
		 * In all other cases, the call is malformed and the method will be disabled.
		 *
		 * @param[in] args variadic parameter pack.
		 *
		 * @return the total or partial low degree of the series.
		 *
		 * @throws unspecified any exception thrown by:
		 * - the construction of return type,
		 * - the calculation of the low degree of each term,
		 * - the assignment and less-than operators for the return type.
		 */
		template <typename ... Args>
		auto ldegree(Args && ... args) const ->
			decltype(degree_utils<power_series>::lget(std::declval<typename power_series::term_type>(),std::declval<symbol_set>(),std::forward<Args>(args)...))
		{
			typedef decltype(degree_utils<power_series>::lget(std::declval<typename power_series::term_type>(),
				std::declval<symbol_set>(),std::forward<Args>(args)...)) return_type;
			if (this->empty()) {
				return return_type(0);
			}
			auto it = this->m_container.begin();
			const auto it_f = this->m_container.end();
			return_type retval = degree_utils<power_series>::lget(*it,this->m_symbol_set,std::forward<Args>(args)...);
			++it;
			return_type tmp;
			for (; it != it_f; ++it) {
				tmp = degree_utils<power_series>::lget(*it,this->m_symbol_set,std::forward<Args>(args)...);
				if (tmp < retval) {
					retval = std::move(tmp);
				}
			}
			return retval;
		}
};

namespace math
{

template <typename Series>
struct degree_impl<Series,typename std::enable_if<is_instance_of<Series,power_series>::value>::type>
{
	template <typename ... Args>
	auto operator()(const Series &s, Args && ... args) const -> decltype(s.degree(std::forward<Args>(args)...))
	{
		return s.degree(std::forward<Args>(args)...);
	}
};

template <typename Series>
struct ldegree_impl<Series,typename std::enable_if<is_instance_of<Series,power_series>::value>::type>
{
	template <typename ... Args>
	auto operator()(const Series &s, Args && ... args) const -> decltype(s.ldegree(std::forward<Args>(args)...))
	{
		return s.ldegree(std::forward<Args>(args)...);
	}
};

}

}

#endif
