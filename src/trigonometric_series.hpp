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

#include <algorithm>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

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
		// Total versions.
		#define PIRANHA_DEFINE_TRIG_PROPERTY_GETTER(property) \
		template <typename Term, typename std::enable_if<cf_trig_score<typename Term::cf_type>::value == 4u && \
			key_trig_score<typename Term::key_type>::value == 0u,int>::type = 0> \
		static auto get_t_##property(const Term &t, const symbol_set &) -> decltype(math::t_##property(t.m_cf)) \
		{ \
			return math::t_##property(t.m_cf); \
		} \
		template <typename Term, typename std::enable_if<cf_trig_score<typename Term::cf_type>::value == 0u && \
			key_trig_score<typename Term::key_type>::value == 4u,int>::type = 0> \
		static auto get_t_##property(const Term &t, const symbol_set &s) -> decltype(t.m_key.t_##property(s)) \
		{ \
			return t.m_key.t_##property(s); \
		} \
		template <typename T> \
		using t_##property##_type_ = decltype(get_t_##property(std::declval<const typename T::term_type &>(),std::declval<const symbol_set &>())); \
		template <typename T> \
		using t_##property##_type = typename std::enable_if<common_type_checks<t_##property##_type_<T>>::value,t_##property##_type_<T>>::type;
		PIRANHA_DEFINE_TRIG_PROPERTY_GETTER(degree)
		PIRANHA_DEFINE_TRIG_PROPERTY_GETTER(ldegree)
		PIRANHA_DEFINE_TRIG_PROPERTY_GETTER(order)
		PIRANHA_DEFINE_TRIG_PROPERTY_GETTER(lorder)
		#undef PIRANHA_DEFINE_TRIG_PROPERTY_GETTER
		// Partial versions.
		#define PIRANHA_DEFINE_PARTIAL_TRIG_PROPERTY_GETTER(property) \
		template <typename Term, typename std::enable_if<cf_trig_score<typename Term::cf_type>::value == 4u && \
			key_trig_score<typename Term::key_type>::value == 0u,int>::type = 0> \
		static auto get_t_##property(const Term &t, const std::vector<std::string> &names, \
			const symbol_set::positions &, const symbol_set &) -> decltype(math::t_##property(t.m_cf,names)) \
		{ \
			return math::t_##property(t.m_cf,names); \
		} \
		template <typename Term, typename std::enable_if<cf_trig_score<typename Term::cf_type>::value == 0u && \
			key_trig_score<typename Term::key_type>::value == 4u,int>::type = 0> \
		static auto get_t_##property(const Term &t, const std::vector<std::string> &, \
			const symbol_set::positions &p, const symbol_set &s) -> decltype(t.m_key.t_##property(p,s)) \
		{ \
			return t.m_key.t_##property(p,s); \
		} \
		template <typename T> \
		using pt_##property##_type_ = decltype(get_t_##property(std::declval<const typename T::term_type &>(), \
			std::declval<const std::vector<std::string> &>(), std::declval<const symbol_set::positions &>(), std::declval<const symbol_set &>())); \
		template <typename T> \
		using pt_##property##_type = typename std::enable_if<common_type_checks<pt_##property##_type_<T>>::value,pt_##property##_type_<T>>::type;
		PIRANHA_DEFINE_PARTIAL_TRIG_PROPERTY_GETTER(degree)
		PIRANHA_DEFINE_PARTIAL_TRIG_PROPERTY_GETTER(ldegree)
		PIRANHA_DEFINE_PARTIAL_TRIG_PROPERTY_GETTER(order)
		PIRANHA_DEFINE_PARTIAL_TRIG_PROPERTY_GETTER(lorder)
		#undef PIRANHA_DEFINE_PARTIAL_TRIG_PROPERTY_GETTER
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
		 * @return total trigonometric degree of the series.
		 *
		 * @throws unspecified any exception resulting from the computation and comparison of the degree of the individual terms.
		 */
		template <typename T = trigonometric_series>
		t_degree_type<T> t_degree() const
		{
			using term_type = typename T::term_type;
			auto it = std::max_element(this->m_container.begin(),this->m_container.end(),[this](const term_type &t1, const term_type &t2) {
				return this->get_t_degree(t1,this->m_symbol_set) < this->get_t_degree(t2,this->m_symbol_set);
			});
			return (it == this->m_container.end()) ? t_degree_type<T>(0) : get_t_degree(*it,this->m_symbol_set);
		}
		/// Partial trigonometric degree.
		/**
		 * \note
		 * This method is enabled only if the requirements outlined in piranha::trigonometric_series are satisfied.
		 *
		 * @param[in] names names of the variables to be considered in the computation.
		 *
		 * @return partial trigonometric degree of the series.
		 *
		 * @throws unspecified any exception resulting from the computation and comparison of the degree of the individual terms.
		 */
		template <typename T = trigonometric_series>
		pt_degree_type<T> t_degree(const std::vector<std::string> &names) const
		{
			using term_type = typename T::term_type;
			symbol_set::positions p(this->m_symbol_set,symbol_set(names.begin(),names.end()));
			auto it = std::max_element(this->m_container.begin(),this->m_container.end(),[this,&p,&names](const term_type &t1, const term_type &t2) {
				return this->get_t_degree(t1,names,p,this->m_symbol_set) < this->get_t_degree(t2,names,p,this->m_symbol_set);
			});
			return (it == this->m_container.end()) ? pt_degree_type<T>(0) : get_t_degree(*it,names,p,this->m_symbol_set);
		}
		/// Trigonometric low degree.
		/**
		 * \note
		 * This method is enabled only if the requirements outlined in piranha::trigonometric_series are satisfied.
		 *
		 * @return total trigonometric low degree of the series.
		 *
		 * @throws unspecified any exception resulting from the computation and comparison of the degree of the individual terms.
		 */
		template <typename T = trigonometric_series>
		t_ldegree_type<T> t_ldegree() const
		{
			using term_type = typename T::term_type;
			auto it = std::min_element(this->m_container.begin(),this->m_container.end(),[this](const term_type &t1, const term_type &t2) {
				return this->get_t_ldegree(t1,this->m_symbol_set) < this->get_t_ldegree(t2,this->m_symbol_set);
			});
			return (it == this->m_container.end()) ? t_ldegree_type<T>(0) : get_t_ldegree(*it,this->m_symbol_set);
		}
		/// Partial trigonometric low degree.
		/**
		 * \note
		 * This method is enabled only if the requirements outlined in piranha::trigonometric_series are satisfied.
		 *
		 * @param[in] names names of the variables to be considered in the computation.
		 *
		 * @return partial trigonometric low degree of the series.
		 *
		 * @throws unspecified any exception resulting from the computation and comparison of the degree of the individual terms.
		 */
		template <typename T = trigonometric_series>
		pt_ldegree_type<T> t_ldegree(const std::vector<std::string> &names) const
		{
			using term_type = typename T::term_type;
			symbol_set::positions p(this->m_symbol_set,symbol_set(names.begin(),names.end()));
			auto it = std::min_element(this->m_container.begin(),this->m_container.end(),[this,&p,&names](const term_type &t1, const term_type &t2) {
				return this->get_t_ldegree(t1,names,p,this->m_symbol_set) < this->get_t_ldegree(t2,names,p,this->m_symbol_set);
			});
			return (it == this->m_container.end()) ? pt_ldegree_type<T>(0) : get_t_ldegree(*it,names,p,this->m_symbol_set);
		}
		/// Trigonometric order.
		/**
		 * \note
		 * This method is enabled only if the requirements outlined in piranha::trigonometric_series are satisfied.
		 *
		 * @return total trigonometric order of the series.
		 *
		 * @throws unspecified any exception resulting from the computation and comparison of the order of the individual terms.
		 */
		template <typename T = trigonometric_series>
		t_order_type<T> t_order() const
		{
			using term_type = typename T::term_type;
			auto it = std::max_element(this->m_container.begin(),this->m_container.end(),[this](const term_type &t1, const term_type &t2) {
				return this->get_t_order(t1,this->m_symbol_set) < this->get_t_order(t2,this->m_symbol_set);
			});
			return (it == this->m_container.end()) ? t_order_type<T>(0) : get_t_order(*it,this->m_symbol_set);
		}
		/// Partial trigonometric order.
		/**
		 * \note
		 * This method is enabled only if the requirements outlined in piranha::trigonometric_series are satisfied.
		 *
		 * @param[in] names names of the variables to be considered in the computation.
		 *
		 * @return partial trigonometric order of the series.
		 *
		 * @throws unspecified any exception resulting from the computation and comparison of the order of the individual terms.
		 */
		template <typename T = trigonometric_series>
		pt_order_type<T> t_order(const std::vector<std::string> &names) const
		{
			using term_type = typename T::term_type;
			symbol_set::positions p(this->m_symbol_set,symbol_set(names.begin(),names.end()));
			auto it = std::max_element(this->m_container.begin(),this->m_container.end(),[this,&p,&names](const term_type &t1, const term_type &t2) {
				return this->get_t_order(t1,names,p,this->m_symbol_set) < this->get_t_order(t2,names,p,this->m_symbol_set);
			});
			return (it == this->m_container.end()) ? pt_order_type<T>(0) : get_t_order(*it,names,p,this->m_symbol_set);
		}
		/// Trigonometric low order.
		/**
		 * \note
		 * This method is enabled only if the requirements outlined in piranha::trigonometric_series are satisfied.
		 *
		 * @return total trigonometric low order of the series.
		 *
		 * @throws unspecified any exception resulting from the computation and comparison of the order of the individual terms.
		 */
		template <typename T = trigonometric_series>
		t_lorder_type<T> t_lorder() const
		{
			using term_type = typename T::term_type;
			auto it = std::min_element(this->m_container.begin(),this->m_container.end(),[this](const term_type &t1, const term_type &t2) {
				return this->get_t_lorder(t1,this->m_symbol_set) < this->get_t_lorder(t2,this->m_symbol_set);
			});
			return (it == this->m_container.end()) ? t_lorder_type<T>(0) : get_t_lorder(*it,this->m_symbol_set);
		}
		/// Partial trigonometric low order.
		/**
		 * \note
		 * This method is enabled only if the requirements outlined in piranha::trigonometric_series are satisfied.
		 *
		 * @param[in] names names of the variables to be considered in the computation.
		 *
		 * @return partial trigonometric low order of the series.
		 *
		 * @throws unspecified any exception resulting from the computation and comparison of the order of the individual terms.
		 */
		template <typename T = trigonometric_series>
		pt_lorder_type<T> t_lorder(const std::vector<std::string> &names) const
		{
			using term_type = typename T::term_type;
			symbol_set::positions p(this->m_symbol_set,symbol_set(names.begin(),names.end()));
			auto it = std::min_element(this->m_container.begin(),this->m_container.end(),[this,&p,&names](const term_type &t1, const term_type &t2) {
				return this->get_t_lorder(t1,names,p,this->m_symbol_set) < this->get_t_lorder(t2,names,p,this->m_symbol_set);
			});
			return (it == this->m_container.end()) ? pt_lorder_type<T>(0) : get_t_lorder(*it,names,p,this->m_symbol_set);
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
