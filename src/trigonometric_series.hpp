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

#ifndef PIRANHA_TRIGONOMETRIC_SERIES_HPP
#define PIRANHA_TRIGONOMETRIC_SERIES_HPP

#include <functional>
#include <set>
#include <string>
#include <type_traits>
#include <utility>

#include "detail/degree_commons.hpp"
#include "forwarding.hpp"
#include "math.hpp"
#include "series.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Trigonometric series toolbox.
/**
 * This toolbox extends a series class with properties of trigonometric series. Specifically, this class will conditionally add methods
 * to query the trigonometric (partial,low) degree and order of a series. This augmentation takes places if either the coefficient or the
 * key satisfy the relevant type traits: piranha::has_t_degree and related for the coefficient type, piranha::key_has_t_degree and related
 * for the key type.
 * 
 * Note that in order for the trigonometric methods to be enabled, coefficient and key type cannot satisfy these type traits at the same time,
 * and all degree/order type traits need to be satisfied for the coefficient/key type. As an additional requirement, the types returned when
 * querying the degree and order must be constructible from \p int, copy or move constructible, and less-than comparable.
 *
 * If the above requirements are not satisfied, this class will not add any new functionality to the \p Series class and
 * will just provide generic constructors and assignment operators that will forward their arguments to \p Series.
 * 
 * This class satisfies the piranha::is_series type trait.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the same guarantee as \p Series.
 * 
 * \section type_requirements Type requirements
 *
 * \p Series must be an instance of piranha::series.
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics is equivalent to the move semantics of \p Series.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename Series>
class trigonometric_series: public Series
{
		typedef Series base;
		PIRANHA_TT_CHECK(is_instance_of,base,series);
		// Utilities to detect trigonometric terms.
		template <typename Cf>
		struct cf_trig_score
		{
			static const int value = has_t_degree<Cf>::value + has_t_ldegree<Cf>::value +
				has_t_order<Cf>::value + has_t_lorder<Cf>::value;
		};
		template <typename Key>
		struct key_trig_score
		{
			static const int value = key_has_t_degree<Key>::value + key_has_t_ldegree<Key>::value +
				key_has_t_order<Key>::value + key_has_t_lorder<Key>::value;
		};
		// Type checks for the degree/order type.
		template <typename T>
		struct common_type_checks
		{
			static const bool value = is_less_than_comparable<T>::value &&
						  (std::is_copy_constructible<T>::value || std::is_move_constructible<T>::value) &&
						  std::is_constructible<T,int>::value;
		};
		template <typename Term, typename = void>
		struct t_get {};
		template <typename Term>
		struct t_get<Term,typename std::enable_if<cf_trig_score<typename Term::cf_type>::value == 4 &&
			key_trig_score<typename Term::key_type>::value == 0>::type>
		{
			// NOTE: the use of the template alias in here is a workaround for what seems a GCC 4.7 (and 4.8?) problem. GCC 4.9
			// and clang 3.3 seem to deal with this just ok.
			#define PIRANHA_DECLARE_CF_GETTER(property) \
			template <typename ... Args> \
			using return_type_##property = typename std::enable_if<common_type_checks<decltype(math::property(std::declval<typename Term::cf_type const &>(), \
				std::declval<Args const &>()...))>::value,decltype(math::property(std::declval<typename Term::cf_type const &>(),std::declval<Args const &>()...))>::type; \
			template <typename ... Args> \
			static return_type_##property<Args...> property(const Term &t, const symbol_set &, const Args & ... args) \
			{ \
				return math::property(t.m_cf,args...); \
			}
			PIRANHA_DECLARE_CF_GETTER(t_degree)
			PIRANHA_DECLARE_CF_GETTER(t_ldegree)
			PIRANHA_DECLARE_CF_GETTER(t_order)
			PIRANHA_DECLARE_CF_GETTER(t_lorder)
			#undef PIRANHA_DECLARE_CF_GETTER
		};
		template <typename Term>
		struct t_get<Term,typename std::enable_if<cf_trig_score<typename Term::cf_type>::value == 0 &&
			key_trig_score<typename Term::key_type>::value == 4>::type>
		{
			#define PIRANHA_DECLARE_KEY_GETTER(property) \
			template <typename ... Args> \
			using return_type_##property = typename std::enable_if<common_type_checks<decltype(std::declval<typename Term::key_type const &>().property( \
				std::declval<Args const &>()...,std::declval<symbol_set const &>()))>::value, \
				decltype(std::declval<typename Term::key_type const &>().property( \
				std::declval<Args const &>()...,std::declval<symbol_set const &>()))>::type; \
			template <typename ... Args> \
			static return_type_##property<Args...> property(const Term &t, const symbol_set &s, const Args & ... args) \
			{ \
				return t.m_key.property(args...,s); \
			}
			PIRANHA_DECLARE_KEY_GETTER(t_degree)
			PIRANHA_DECLARE_KEY_GETTER(t_ldegree)
			PIRANHA_DECLARE_KEY_GETTER(t_order)
			PIRANHA_DECLARE_KEY_GETTER(t_lorder)
			#undef PIRANHA_DECLARE_KEY_GETTER
		};
	public:
		/// Defaulted default constructor.
		trigonometric_series() = default;
		/// Defaulted copy constructor.
		trigonometric_series(const trigonometric_series &) = default;
		/// Defaulted move constructor.
		trigonometric_series(trigonometric_series &&) = default;
		PIRANHA_FORWARDING_CTOR(trigonometric_series,base)
		/// Defaulted copy assignment operator.
		trigonometric_series &operator=(const trigonometric_series &) = default;
		/// Defaulted move assignment operator.
		trigonometric_series &operator=(trigonometric_series &&) = default;
		/// Trivial destructor.
		~trigonometric_series()
		{
			PIRANHA_TT_CHECK(is_series,trigonometric_series);
		}
		PIRANHA_FORWARDING_ASSIGNMENT(trigonometric_series,base)
		/// Trigonometric degree.
		/**
		 * \note
		 * This method is enabled only if the requirements outlined in piranha::trigonometric_series are satisfied.
		 *
		 * This method can compute both the total and the partial trigonometric degree (the former is computed when the
		 * variadic pack is empty, the latter when an <tt>std::set<std::string></tt> is passed as the only argument - in
		 * all other cases, the method will be disabled).
		 *
		 * @param[in] args variadic argument pack.
		 * 
		 * @return trigonometric degree of the series.
		 * 
		 * @throws unspecified any exception resulting from the computation and comparison of the degree of the individual terms.
		 */
		template <typename ... Args, typename T = Series>
		auto t_degree(const Args & ... args) const -> decltype(t_get<typename T::term_type>::t_degree(std::declval<typename T::term_type>(),std::declval<symbol_set>(),args...))
		{
			// NOTE: according to the documentation, the placeholder will generate a binder in which the corresponding argument will be forwarded
			// perfectly to the underlying function - so there should be no risk of useless copying.
			// http://en.cppreference.com/w/cpp/utility/functional/bind
			auto g = std::bind(t_get<typename Series::term_type>::template t_degree<Args...>,std::placeholders::_1,std::cref(this->m_symbol_set),std::cref(args)...);
			return detail::generic_series_degree<0>(this->m_container,g);
		}
		/// Trigonometric low degree.
		/**
		 * \note
		 * This method is enabled only if the requirements outlined in piranha::trigonometric_series are satisfied.
		 *
		 * This method can compute both the total and the partial trigonometric low degree (the former is computed when the
		 * variadic pack is empty, the latter when an <tt>std::set<std::string></tt> is passed as the only argument - in
		 * all other cases, the method will be disabled).
		 *
		 * @param[in] args variadic argument pack.
		 *
		 * @return trigonometric low degree of the series.
		 *
		 * @throws unspecified any exception resulting from the computation and comparison of the low degree of the individual terms.
		 */
		template <typename ... Args, typename T = Series>
		auto t_ldegree(const Args & ... args) const -> decltype(t_get<typename T::term_type>::t_ldegree(std::declval<typename T::term_type>(),std::declval<symbol_set>(),args...))
		{
			auto g = std::bind(t_get<typename Series::term_type>::template t_ldegree<Args...>,std::placeholders::_1,std::cref(this->m_symbol_set),std::cref(args)...);
			return detail::generic_series_degree<1>(this->m_container,g);
		}
		/// Trigonometric order.
		/**
		 * \note
		 * This method is enabled only if the requirements outlined in piranha::trigonometric_series are satisfied.
		 *
		 * This method can compute both the total and the partial trigonometric order (the former is computed when the
		 * variadic pack is empty, the latter when an <tt>std::set<std::string></tt> is passed as the only argument - in
		 * all other cases, the method will be disabled).
		 *
		 * @param[in] args variadic argument pack.
		 *
		 * @return trigonometric order of the series.
		 *
		 * @throws unspecified any exception resulting from the computation and comparison of the order of the individual terms.
		 */
		template <typename ... Args, typename T = Series>
		auto t_order(const Args & ... args) const -> decltype(t_get<typename T::term_type>::t_order(std::declval<typename T::term_type>(),std::declval<symbol_set>(),args...))
		{
			auto g = std::bind(t_get<typename Series::term_type>::template t_order<Args...>,std::placeholders::_1,std::cref(this->m_symbol_set),std::cref(args)...);
			return detail::generic_series_degree<0>(this->m_container,g);
		}
		/// Trigonometric low order.
		/**
		 * \note
		 * This method is enabled only if the requirements outlined in piranha::trigonometric_series are satisfied.
		 *
		 * This method can compute both the total and the partial trigonometric low order (the former is computed when the
		 * variadic pack is empty, the latter when an <tt>std::set<std::string></tt> is passed as the only argument - in
		 * all other cases, the method will be disabled).
		 *
		 * @param[in] args variadic argument pack.
		 *
		 * @return trigonometric low order of the series.
		 *
		 * @throws unspecified any exception resulting from the computation and comparison of the low order of the individual terms.
		 */
		template <typename ... Args, typename T = Series>
		auto t_lorder(const Args & ... args) const -> decltype(t_get<typename T::term_type>::t_lorder(std::declval<typename T::term_type>(),std::declval<symbol_set>(),args...))
		{
			auto g = std::bind(t_get<typename Series::term_type>::template t_lorder<Args...>,std::placeholders::_1,std::cref(this->m_symbol_set),std::cref(args)...);
			return detail::generic_series_degree<1>(this->m_container,g);
		}
};

namespace math
{

/// Specialisation of the piranha::math::t_degree() functor for instances of piranha::trigonometric_series.
/**
 * This specialisation is activated if \p Series is an instance of piranha::trigonometric_series.
 * The corrsponding method in piranha::trigonometric_series will be used for the computation, if available.
 */
template <typename Series>
struct t_degree_impl<Series,typename std::enable_if<is_instance_of<Series,trigonometric_series>::value>::type>
{
	/// Trigonometric degree operator.
	/**
	 * \note
	 * This operator is enabled only if the invoked method in piranha::trigonometric_series is enabled.
	 *
	 * @param[in] ts input series.
	 * @param[in] args variadic argument pack.
	 * 
	 * @return trigonometric degree of \p ts.
	 *
	 * @throws unspecified any exception thrown by piranha::trigonometric_series::t_degree().
	 */
	template <typename ... Args>
	auto operator()(const Series &ts, const Args & ... args) const -> decltype(ts.t_degree(args...))
	{
		return ts.t_degree(args...);
	}
};

/// Specialisation of the piranha::math::t_ldegree() functor for instances of piranha::trigonometric_series.
/**
 * This specialisation is activated if \p Series is an instance of piranha::trigonometric_series.
 * The corrsponding method in piranha::trigonometric_series will be used for the computation, if available.
 */
template <typename Series>
struct t_ldegree_impl<Series,typename std::enable_if<is_instance_of<Series,trigonometric_series>::value>::type>
{
	/// Trigonometric low degree operator.
	/**
	 * \note
	 * This operator is enabled only if the invoked method in piranha::trigonometric_series is enabled.
	 *
	 * @param[in] ts input series.
	 * @param[in] args variadic argument pack.
	 *
	 * @return trigonometric low degree of \p ts.
	 *
	 * @throws unspecified any exception thrown by piranha::trigonometric_series::t_ldegree().
	 */
	template <typename ... Args>
	auto operator()(const Series &ts, const Args & ... args) const -> decltype(ts.t_ldegree(args...))
	{
		return ts.t_ldegree(args...);
	}
};

/// Specialisation of the piranha::math::t_order() functor for instances of piranha::trigonometric_series.
/**
 * This specialisation is activated if \p Series is an instance of piranha::trigonometric_series.
 * The corrsponding method in piranha::trigonometric_series will be used for the computation, if available.
 */
template <typename Series>
struct t_order_impl<Series,typename std::enable_if<is_instance_of<Series,trigonometric_series>::value>::type>
{
	/// Trigonometric order operator.
	/**
	 * \note
	 * This operator is enabled only if the invoked method in piranha::trigonometric_series is enabled.
	 *
	 * @param[in] ts input series.
	 * @param[in] args variadic argument pack.
	 *
	 * @return trigonometric order of \p ts.
	 *
	 * @throws unspecified any exception thrown by piranha::trigonometric_series::t_order().
	 */
	template <typename ... Args>
	auto operator()(const Series &ts, const Args & ... args) const -> decltype(ts.t_order(args...))
	{
		return ts.t_order(args...);
	}
};

/// Specialisation of the piranha::math::t_lorder() functor for instances of piranha::trigonometric_series.
/**
 * This specialisation is activated if \p Series is an instance of piranha::trigonometric_series.
 * The corrsponding method in piranha::trigonometric_series will be used for the computation, if available.
 */
template <typename Series>
struct t_lorder_impl<Series,typename std::enable_if<is_instance_of<Series,trigonometric_series>::value>::type>
{
	/// Trigonometric low order operator.
	/**
	 * \note
	 * This operator is enabled only if the invoked method in piranha::trigonometric_series is enabled.
	 *
	 * @param[in] ts input series.
	 * @param[in] args variadic argument pack.
	 *
	 * @return trigonometric low order of \p ts.
	 *
	 * @throws unspecified any exception thrown by piranha::trigonometric_series::t_lorder().
	 */
	template <typename ... Args>
	auto operator()(const Series &ts, const Args & ... args) const -> decltype(ts.t_lorder(args...))
	{
		return ts.t_lorder(args...);
	}
};

}

}

#endif
