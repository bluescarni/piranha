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

#include <boost/concept/assert.hpp>
#include <set>
#include <string>
#include <type_traits>

#include "concepts/power_series_term.hpp"
#include "concepts/series.hpp"
#include "detail/series_fwd.hpp"
#include "forwarding.hpp"
#include "power_series_term.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp" // For has_degree.

namespace piranha
{

namespace detail
{

struct power_series_tag {};

}

/// Power series toolbox.
/**
 * This toolbox is intended to extend the \p Series type with properties of formal power series.
 * 
 * Specifically, the toolbox will conditionally augment a \p Series type by adding methods to query the total and partial (low) degree
 * of a \p Series object. Such augmentation takes place if the series term satisfies the piranha::is_power_series_term type-trait.
 * 
 * As an additional requirement, the types returned when querying total and partial (low) degree must be default-constructible,
 * move-assignable, constructible from \p int, and less-than and greater-than comparable. If these additional requirements are not satisfied,
 * a compile-time error will be produced.
 * 
 * If the term type does not satisfy the piranha::is_power_series_term type-trait, this class will not add any new functionality to the \p Series class and
 * will just provide generic constructors that will forward their arguments to the constructors of \p Series.
 * 
 * This class is a model of the piranha::concept::Series concept and, in case the above requirements are satisfied, of the piranha::concept::PowerSeries
 * concept.
 * 
 * \section type_requirements Type requirements
 * 
 * \p Series must be an instance of piranha::series.
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
 * 
 * \todo specify in the docs that the methods are available only if the series is a power series (so that in the docs of, e.g., poisson
 * series there's no confusion).
 * \todo investigate beautification (for doc purposes) of degree() return types via auto and decltype(). Or maybe at least do the same as
 * done in power_series_term.
 */
// NOTE: here the tag is used explicitly to differentiate between the general implementation
// and the specialization below.
template <typename Series, typename Enable = void>
class power_series: public Series,detail::power_series_tag
{
		static_assert(std::is_base_of<detail::series_tag,Series>::value,"Base class must be an instance of piranha::series.");
		BOOST_CONCEPT_ASSERT((concept::PowerSeriesTerm<typename Series::term_type>));
		typedef Series base;
		template <typename... Args>
		struct degree_type
		{
			typedef decltype(std::declval<typename Series::term_type>().degree(
				std::declval<typename std::decay<Args>::type>()...,std::declval<symbol_set>())) type;
		};
		template <typename... Args>
		struct ldegree_type
		{
			typedef decltype(std::declval<typename Series::term_type>().ldegree(
				std::declval<typename std::decay<Args>::type>()...,std::declval<symbol_set>())) type;
		};
		template <typename... Args>
		typename degree_type<Args ...>::type degree_impl(Args && ... params) const
		{
			// NOTE: this code is partially re-used in Poisson series, keep it in mind
			// if it gets changed.
			typedef typename degree_type<Args ...>::type return_type;
			if (this->empty()) {
				return return_type(0);
			}
			auto it = this->m_container.begin();
			const auto it_f = this->m_container.end();
			return_type retval = it->degree(std::forward<Args>(params)...,this->m_symbol_set);
			++it;
			return_type tmp;
			for (; it != it_f; ++it) {
				tmp = it->degree(std::forward<Args>(params)...,this->m_symbol_set);
				if (tmp > retval) {
					retval = std::move(tmp);
				}
			}
			return retval;
		}
		template <typename... Args>
		typename ldegree_type<Args ...>::type ldegree_impl(Args && ... params) const
		{
			typedef typename ldegree_type<Args ...>::type return_type;
			if (this->empty()) {
				return return_type(0);
			}
			auto it = this->m_container.begin();
			const auto it_f = this->m_container.end();
			return_type retval = it->ldegree(std::forward<Args>(params)...,this->m_symbol_set);
			++it;
			return_type tmp;
			for (; it != it_f; ++it) {
				tmp = it->ldegree(std::forward<Args>(params)...,this->m_symbol_set);
				if (tmp < retval) {
					retval = std::move(tmp);
				}
			}
			return retval;
		}
	public:
		/// Defaulted default constructor.
		power_series() = default;
		/// Defaulted copy constructor.
		power_series(const power_series &) = default;
		/// Defaulted move constructor.
		power_series(power_series &&) = default;
		PIRANHA_FORWARDING_CTOR(power_series,base)
		/// Trivial destructor.
		~power_series() piranha_noexcept_spec(true)
		{
			BOOST_CONCEPT_ASSERT((concept::Series<power_series>));
		}
		/// Defaulted copy assignment operator.
		power_series &operator=(const power_series &) = default;
		/// Trivial move assignment operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 */
		power_series &operator=(power_series &&other) piranha_noexcept_spec(true)
		{
			base::operator=(std::move(other));
			return *this;
		}
		PIRANHA_FORWARDING_ASSIGNMENT(power_series,base)
		/// Total degree.
		/**
		 * The degree of the series is the maximum degree of its terms. The degree of each term is calculated using piranha::power_series_term::degree().
		 * 
		 * If the series is empty, zero will be returned.
		 * 
		 * @return the total degree of the series.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the construction of return type from zero,
		 * - the calculation of the degree of each term,
		 * - the assignment and greater-than operators for the return type.
		 */
		typename degree_type<>::type degree() const
		{
			return degree_impl();
		}
		/// Partial degree.
		/**
		 * The partial degree of the series is the maximum partial degree of its terms
		 * (i.e., the total degree when only variables with names in \p s are considered).
		 * The partial degree of each term is calculated using piranha::power_series_term::degree().
		 * 
		 * If the series is empty, zero will be returned.
		 * 
		 * @param[in] s the set of names of the variables that will be considered in the computation of the partial degree.
		 * 
		 * @return the partial degree of the series.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the construction of return type from zero,
		 * - the calculation of the degree of each term,
		 * - the assignment and greater-than operators for the return type.
		 */
		typename degree_type<std::set<std::string>>::type degree(const std::set<std::string> &s) const
		{
			return degree_impl(s);
		}
		/// Partial degree.
		/**
		 * Convenience overload that will call degree() with a set built from the
		 * single string \p name. This template method is activated only if \p Str is a string type (either C or C++).
		 * 
		 * @param[in] name name of the variable that will be considered in the computation of the partial degree.
		 * 
		 * @return the partial degree of the series.
		 * 
		 * @throws unspecified any exception thrown by degree(const std::set<std::string> &s) or by memory allocation
		 * errors in standard containers.
		 */
		template <typename Str>
		typename degree_type<std::set<std::string>>::type degree(Str &&name,
			typename std::enable_if<std::is_same<typename std::decay<Str>::type,std::string>::value ||
			std::is_same<typename std::decay<Str>::type,const char *>::value>::type * = piranha_nullptr) const
		{
			return degree(std::set<std::string>{std::string(name)});
		}
		/// Low degree.
		/**
		 * The low degree of the series is the minimum low degree of its terms. The low degree of each term is calculated
		 * using piranha::power_series_term::ldegree().
		 * 
		 * If the series is empty, zero will be returned.
		 * 
		 * @return the total low degree of the series.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the construction of return type from zero,
		 * - the calculation of the low degree of each term,
		 * - the assignment and greater-than operators for the return type.
		 */
		typename ldegree_type<>::type ldegree() const
		{
			return ldegree_impl();
		}
		/// Partial low degree.
		/**
		 * The partial low degree of the series is the minimum partial low degree of its terms
		 * (i.e., the total low degree when only variables with names in \p s are considered).
		 * The partial low degree of each term is calculated using piranha::power_series_term::degree().
		 * 
		 * If the series is empty, zero will be returned.
		 * 
		 * @param[in] s the set of names of the variables that will be considered in the computation of the partial low degree.
		 * 
		 * @return the partial low degree of the series.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the construction of return type from zero,
		 * - the calculation of the degree of each term,
		 * - the assignment and greater-than operators for the return type.
		 */
		typename ldegree_type<std::set<std::string>>::type ldegree(const std::set<std::string> &s) const
		{
			return ldegree_impl(s);
		}
		/// Partial low degree.
		/**
		 * Convenience overload that will call ldegree() with a set built from the
		 * single string \p name. This template method is activated only if \p Str is a string type (either C or C++).
		 * 
		 * @param[in] name name of the variable that will be considered in the computation of the partial low degree.
		 * 
		 * @return the partial low degree of the series.
		 * 
		 * @throws unspecified any exception thrown by ldegree(const std::set<std::string> &s) or by memory allocation
		 * errors in standard containers.
		 */
		template <typename Str>
		typename ldegree_type<std::set<std::string>>::type ldegree(Str &&name,
			typename std::enable_if<std::is_same<typename std::decay<Str>::type,std::string>::value ||
			std::is_same<typename std::decay<Str>::type,const char *>::value>::type * = piranha_nullptr) const
		{
			return ldegree(std::set<std::string>{std::string(name)});
		}
};

template <typename Series>
class power_series<Series,typename std::enable_if<!is_power_series_term<typename Series::term_type>::value>::type>:
	public Series
{
		static_assert(std::is_base_of<detail::series_tag,Series>::value,"Base class must derive from piranha::series.");
		typedef Series base;
	public:
		power_series() = default;
		power_series(const power_series &) = default;
		power_series(power_series &&) = default;
		PIRANHA_FORWARDING_CTOR(power_series,base)
		~power_series() piranha_noexcept_spec(true)
		{
			BOOST_CONCEPT_ASSERT((concept::Series<power_series>));
		}
		power_series &operator=(const power_series &) = default;
		power_series &operator=(power_series &&other) piranha_noexcept_spec(true)
		{
			base::operator=(std::move(other));
			return *this;
		}
		PIRANHA_FORWARDING_ASSIGNMENT(power_series,base)
};

/// Type-trait for power series.
/**
 * The value of the type-trait will be \p true if \p Series is an instance of piranha::power_series that provides the methods
 * for querying the degree of the series, \p false otherwise.
 */
template <typename Series>
class is_power_series
{
	public:
		/// Type-trait value.
		static const bool value = std::is_base_of<detail::power_series_tag,Series>::value;
};

template <typename Series>
const bool is_power_series<Series>::value;

/// Specialization of piranha::has_degree for power series.
/**
 * This specialization is enabled for types satisfying the type-trait piranha::is_power_series.
 * 
 * \section type_requirements Type requirements
 * 
 * \p Series must be a model of the piranha::concept::Series concept.
 */
template <typename PowerSeries>
class has_degree<PowerSeries,typename std::enable_if<is_power_series<PowerSeries>::value>::type>
{
		BOOST_CONCEPT_ASSERT((concept::Series<PowerSeries>));
	public:
		/// Type trait value.
		static const bool value = true;
		/// Getter for total degree.
		static auto get(const PowerSeries &ps) -> decltype(ps.degree())
		{
			return ps.degree();
		}
		/// Getter for partial degree.
		static auto get(const PowerSeries &ps, const std::set<std::string> &s) -> decltype(ps.degree(s))
		{
			return ps.degree(s);
		}
		/// Getter for low degree.
		static auto lget(const PowerSeries &ps) -> decltype(ps.ldegree())
		{
			return ps.ldegree();
		}
		/// Getter for partial low degree.
		static auto lget(const PowerSeries &ps, const std::set<std::string> &s) -> decltype(ps.ldegree(s))
		{
			return ps.ldegree(s);
		}
};

}

#endif
