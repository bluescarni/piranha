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

#include <algorithm>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "forwarding.hpp"
#include "math.hpp"
#include "serialization.hpp"
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
 * As an additional requirement, the types returned when querying the degree must be constructible from \p int,
 * copy or move constructible, and less-than comparable. If these additional requirements are not satisfied,
 * the degree-querying methods will be disabled.
 *
 * This class satisfies the piranha::is_series type trait.
 *
 * ## Type requirements ##
 *
 * - \p Series must satisfy the piranha::is_series type trait,
 * - \p Derived must derive from power_series of \p Series and \p Derived.
 *
 * ## Exception safety guarantee ##
 *
 * This class provides the same guarantee as \p Series.
 *
 * ## Move semantics ##
 *
 * Move semantics is equivalent to the move semantics of \p Series.
 *
 * ## Serialization ##
 *
 * This class supports serialization if \p Series does.
 *
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename Series, typename Derived>
class power_series: public Series
{
		PIRANHA_TT_CHECK(is_series,Series);
		typedef Series base;
		// Detect power series terms.
		template <typename T>
		struct ps_term_score
		{
			typedef typename T::cf_type cf_type;
			typedef typename T::key_type key_type;
			static const unsigned value = static_cast<unsigned>(has_degree<cf_type>::value && has_ldegree<cf_type>::value) +
						      (static_cast<unsigned>(key_has_degree<key_type>::value && key_has_ldegree<key_type>::value) << 1u);
		};
		// Common checks on degree/ldegree type for use in enabling conditions below.
		template <typename T>
		struct common_type_checks
		{
			static const bool value = std::is_constructible<T,int>::value &&
						  (std::is_copy_constructible<T>::value || std::is_move_constructible<T>::value) &&
						  is_less_than_comparable<T>::value;
		};
		// Total (low) degree computation.
		#define PIRANHA_DEFINE_PS_PROPERTY_GETTER(property) \
		template <typename Term, typename std::enable_if<ps_term_score<Term>::value == 1u,int>::type = 0> \
		static auto get_##property(const Term &t, const symbol_set &) -> decltype(math::property(t.m_cf)) \
		{ \
			return math::property(t.m_cf); \
		} \
		template <typename Term, typename std::enable_if<ps_term_score<Term>::value == 2u,int>::type = 0> \
		static auto get_##property(const Term &t, const symbol_set &s) -> decltype(t.m_key.property(s)) \
		{ \
			return t.m_key.property(s); \
		} \
		template <typename Term, typename std::enable_if<ps_term_score<Term>::value == 3u,int>::type = 0> \
		static auto get_##property(const Term &t, const symbol_set &s) -> decltype(math::property(t.m_cf) + t.m_key.property(s)) \
		{ \
			return math::property(t.m_cf) + t.m_key.property(s); \
		} \
		template <typename T> \
		using property##_type_ = decltype(get_##property(std::declval<const typename T::term_type &>(),std::declval<const symbol_set &>())); \
		template <typename T> \
		using property##_type = typename std::enable_if<common_type_checks<property##_type_<T>>::value,property##_type_<T>>::type;
		PIRANHA_DEFINE_PS_PROPERTY_GETTER(degree)
		PIRANHA_DEFINE_PS_PROPERTY_GETTER(ldegree)
		#undef PIRANHA_DEFINE_PS_PROPERTY_GETTER
		// Partial (low) degree computation.
		#define PIRANHA_DEFINE_PARTIAL_PS_PROPERTY_GETTER(property) \
		template <typename Term, typename std::enable_if<ps_term_score<Term>::value == 1u,int>::type = 0> \
		static auto get_##property(const Term &t, const std::vector<std::string> &names, \
			const symbol_set::positions &, const symbol_set &) -> decltype(math::property(t.m_cf,names)) \
		{ \
			return math::property(t.m_cf,names); \
		} \
		template <typename Term, typename std::enable_if<ps_term_score<Term>::value == 2u,int>::type = 0> \
		static auto get_##property(const Term &t, const std::vector<std::string> &, \
			const symbol_set::positions &p, const symbol_set &s) -> decltype(t.m_key.property(p,s)) \
		{ \
			return t.m_key.property(p,s); \
		} \
		template <typename Term, typename std::enable_if<ps_term_score<Term>::value == 3u,int>::type = 0> \
		static auto get_##property(const Term &t, const std::vector<std::string> &names, \
			const symbol_set::positions &p, const symbol_set &s) -> decltype(math::property(t.m_cf,names) + t.m_key.property(p,s)) \
		{ \
			return math::property(t.m_cf,names) + t.m_key.property(p,s); \
		} \
		template <typename T> \
		using p##property##_type_ = decltype(get_##property(std::declval<const typename T::term_type &>(), \
			std::declval<const std::vector<std::string> &>(), std::declval<const symbol_set::positions &>(), std::declval<const symbol_set &>())); \
		template <typename T> \
		using p##property##_type = typename std::enable_if<common_type_checks<p##property##_type_<T>>::value,p##property##_type_<T>>::type;
		PIRANHA_DEFINE_PARTIAL_PS_PROPERTY_GETTER(degree)
		PIRANHA_DEFINE_PARTIAL_PS_PROPERTY_GETTER(ldegree)
		#undef PIRANHA_DEFINE_PARTIAL_PS_PROPERTY_GETTER
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
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
		~power_series()
		{
			PIRANHA_TT_CHECK(is_series,power_series);
			PIRANHA_TT_CHECK(std::is_base_of,power_series,Derived);
		}
		/// Total degree.
		/**
		 * \note
		 * This method is available only if the requisites outlined in piranha::power_series are satisfied.
		 *
		 * The degree of the series is the maximum degree of its terms. If the series is empty, zero will be returned.
		 *
		 * @return the total degree of the series.
		 *
		 * @throws unspecified any exception thrown by:
		 * - the construction of return type,
		 * - the calculation of the degree of each term,
		 * - the assignment and less-than operators for the return type.
		 */
		template <typename T = power_series>
		degree_type<T> degree() const
		{
			using term_type = typename T::term_type;
			auto it = std::max_element(this->m_container.begin(),this->m_container.end(),[this](const term_type &t1, const term_type &t2) {
				return this->get_degree(t1,this->m_symbol_set) < this->get_degree(t2,this->m_symbol_set);
			});
			return (it == this->m_container.end()) ? degree_type<T>(0) : get_degree(*it,this->m_symbol_set);
		}
		/// Total low degree.
		/**
		 * \note
		 * This method is available only if the requisites outlined in piranha::power_series are satisfied.
		 *
		 * The low degree of the series is the minimum low degree of its terms. If the series is empty, zero will be returned.
		 *
		 * @return the total low degree of the series.
		 *
		 * @throws unspecified any exception thrown by:
		 * - the construction of return type,
		 * - the calculation of the low degree of each term,
		 * - the assignment and less-than operators for the return type.
		 */
		template <typename T = power_series>
		ldegree_type<T> ldegree() const
		{
			using term_type = typename T::term_type;
			auto it = std::min_element(this->m_container.begin(),this->m_container.end(),[this](const term_type &t1, const term_type &t2) {
				return this->get_ldegree(t1,this->m_symbol_set) < this->get_ldegree(t2,this->m_symbol_set);
			});
			return (it == this->m_container.end()) ? ldegree_type<T>(0) : get_ldegree(*it,this->m_symbol_set);
		}
		/// Partial degree.
		/**
		 * \note
		 * This method is available only if the requisites outlined in piranha::power_series are satisfied.
		 *
		 * The partial degree of the series is the maximum partial degree of its terms. If the series is empty, zero will be returned.
		 *
		 * @param[in] names names of the variables to be considered in the computation of the degree.
		 *
		 * @return the partial degree of the series.
		 *
		 * @throws unspecified any exception thrown by:
		 * - the construction of return type,
		 * - the calculation of the degree of each term,
		 * - the assignment and less-than operators for the return type.
		 */
		template <typename T = power_series>
		pdegree_type<T> degree(const std::vector<std::string> &names) const
		{
			using term_type = typename T::term_type;
			const symbol_set::positions p(this->m_symbol_set,symbol_set(names.begin(),names.end()));
			auto it = std::max_element(this->m_container.begin(),this->m_container.end(),[this,&p,&names](const term_type &t1, const term_type &t2) {
				return this->get_degree(t1,names,p,this->m_symbol_set) < this->get_degree(t2,names,p,this->m_symbol_set);
			});
			return (it == this->m_container.end()) ? pdegree_type<T>(0) : get_degree(*it,names,p,this->m_symbol_set);
		}
		/// Partial low degree.
		/**
		 * \note
		 * This method is available only if the requisites outlined in piranha::power_series are satisfied.
		 *
		 * The partial low degree of the series is the minimum partial low degree of its terms. If the series is empty, zero will be returned.
		 *
		 * @param[in] names names of the variables to be considered in the computation of the low degree.
		 *
		 * @return the partial low degree of the series.
		 *
		 * @throws unspecified any exception thrown by:
		 * - the construction of return type,
		 * - the calculation of the low degree of each term,
		 * - the assignment and less-than operators for the return type.
		 */
		template <typename T = power_series>
		pldegree_type<T> ldegree(const std::vector<std::string> &names) const
		{
			using term_type = typename T::term_type;
			const symbol_set::positions p(this->m_symbol_set,symbol_set(names.begin(),names.end()));
			auto it = std::min_element(this->m_container.begin(),this->m_container.end(),[this,&p,&names](const term_type &t1, const term_type &t2) {
				return this->get_ldegree(t1,names,p,this->m_symbol_set) < this->get_ldegree(t2,names,p,this->m_symbol_set);
			});
			return (it == this->m_container.end()) ? pldegree_type<T>(0) : get_ldegree(*it,names,p,this->m_symbol_set);
		}
};

namespace math
{

/// Specialisation of the piranha::math::degree() functor for instances of piranha::power_series.
/**
 * This specialisation is activated if \p Series is an instance of piranha::power_series. If \p Series
 * does not fulfill the requirements outlined in piranha::power_series, the call operator will be disabled.
 */
template <typename Series>
struct degree_impl<Series,typename std::enable_if<is_instance_of<Series,power_series>::value>::type>
{
	/// Call operator.
	/**
	 * If available, it will call piranha::power_series::degree().
	 * 
	 * @param[in] s input power series.
	 * @param[in] args additional arguments that will be passed to the series' method.
	 * 
	 * @return the degree of input series \p s.
	 * 
	 * @throws unspecified any exception thrown by the invoked method of the series.
	 */
	template <typename ... Args>
	auto operator()(const Series &s, const Args & ... args) const -> decltype(s.degree(args...))
	{
		return s.degree(args...);
	}
};

/// Specialisation of the piranha::math::ldegree() functor for instances of piranha::power_series.
/**
 * This specialisation is activated if \p Series is an instance of piranha::power_series. If \p Series
 * does not fulfill the requirements outlined in piranha::power_series, the call operator will be disabled.
 */
template <typename Series>
struct ldegree_impl<Series,typename std::enable_if<is_instance_of<Series,power_series>::value>::type>
{
	/// Call operator.
	/**
	 * If available, it will call piranha::power_series::ldegree().
	 * 
	 * @param[in] s input power series.
	 * @param[in] args additional arguments that will be passed to the series' method.
	 * 
	 * @return the low degree of input series \p s.
	 * 
	 * @throws unspecified any exception thrown by the invoked method of the series.
	 */
	template <typename ... Args>
	auto operator()(const Series &s, const Args & ... args) const -> decltype(s.ldegree(args...))
	{
		return s.ldegree(args...);
	}
};

}

}

#endif
