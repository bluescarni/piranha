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

#ifndef PIRANHA_POWER_SERIES_TRUNCATOR_HPP
#define PIRANHA_POWER_SERIES_TRUNCATOR_HPP

#include <boost/concept/assert.hpp>
#include <tuple>

#include "concepts/power_series_term.hpp"
#include "degree_truncator_settings.hpp"
#include "symbol_set.hpp"

namespace piranha
{

/// Power series truncator.
/**
 * This class provides the building blocks to write specialisations of piranha::truncator for use with
 * piranha::power_series series types.
 * 
 * The truncator has global settings inherited from piranha::degree_truncator_settings, and provides methods
 * for ranking terms according to their low degree. Most of the methods of this class will operate on terms
 * satisfying the piranha::concept::PowerSeriesTerm concept.
 * 
 * For performance reasons, this class will atomically take a snapshot of the current degree truncation settings
 * on construction and will use this snapshot
 * in its operations. As such, the settings of an instance of this truncator might differ from the global settings if these are modified
 * after construction.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the same exception safety guarantee as the type of piranha::degree_truncator_settings::get_state().
 */
class power_series_truncator: public degree_truncator_settings
{
		typedef decltype(degree_truncator_settings::get_state()) state_type;
	public:
		/// Default constructor.
		/**
		 * Will take an atomic snapshot of the current state of piranha::degree_truncator_settings (i.e., truncation
		 * mode, limit and arguments), and will store it internally for future use.
		 * 
		 * @throws unspecified any exception thrown by threading primitives or memory allocation
		 * errors in standard containers.
		 */
		power_series_truncator():m_state(degree_truncator_settings::get_state()) {}
		/// Deleted copy assignment operator.
		power_series_truncator &operator=(const power_series_truncator &) = delete;
		/// Deleted move assignment operator.
		power_series_truncator &operator=(power_series_truncator &&) = delete;
		/// Truncator status.
		/**
		 * @return \p true if the truncator mode is total or partial, \p false otherwise.
		 */
		bool is_active() const
		{
			return std::get<0u>(m_state) != inactive;
		}
	protected:
		/// Compare terms by low degree.
		/**
		 * Will return \p true if the total low degree of \p t1 is less than \p t2, \p false otherwise.
		 * 
		 * If \p Term is not a model of the piranha::concept::PowerSeriesTerm concept, a compile-time
		 * error will be produced.
		 * 
		 * @param[in] t1 first term operand.
		 * @param[in] t2 second term operand.
		 * @param[in] args reference set of symbols that will be used in the computation of the degree.
		 * 
		 * @return result of the comparison.
		 * 
		 * @throws unspecified any exception resulting from the computation and comparison of the degree.
		 */
		template <typename Term>
		bool compare_ldegree(const Term &t1, const Term &t2, const symbol_set &args) const
		{
			BOOST_CONCEPT_ASSERT((concept::PowerSeriesTerm<Term>));
			return t1.ldegree(args) < t2.ldegree(args);
		}
		/// Compare terms by partial low degree.
		/**
		 * Will return \p true if the partial low degree of \p t1 is less than \p t2, \p false otherwise.
		 * The arguments considered for the computation are those copied from piranha::degree_truncator_settings
		 * upon construction.
		 * 
		 * If \p Term is not a model of the piranha::concept::PowerSeriesTerm concept, a compile-time
		 * error will be produced.
		 * 
		 * @param[in] t1 first term operand.
		 * @param[in] t2 second term operand.
		 * @param[in] args reference set of symbols that will be used in the computation of the degree.
		 * 
		 * @return result of the comparison.
		 * 
		 * @throws unspecified any exception resulting from the computation and comparison of the degree.
		 */
		template <typename Term>
		bool compare_pldegree(const Term &t1, const Term &t2, const symbol_set &args) const
		{
			BOOST_CONCEPT_ASSERT((concept::PowerSeriesTerm<Term>));
			return t1.ldegree(std::get<2u>(m_state),args) < t2.ldegree(std::get<2u>(m_state),args);
		}
		/// Filter term.
		/**
		 * Will return \p true if the (partial) low degree of \p t is greater than or equal to the current truncation limit,
		 * \p false otherwise.
		 * 
		 * If \p Term is not a model of the piranha::concept::PowerSeriesTerm concept, a compile-time
		 * error will be produced.
		 * 
		 * @param[in] t term argument.
		 * @param[in] args reference set of symbols that will be used in the computation of the degree.
		 * 
		 * @return \p true is the term can be filtered out, \p false otherwise.
		 * 
		 * @throws unspecified any exception resulting from the computation and comparison of the degree.
		 */
		template <typename Term>
		bool filter_term(const Term &t, const symbol_set &args) const
		{
			BOOST_CONCEPT_ASSERT((concept::PowerSeriesTerm<Term>));
			switch (std::get<0u>(m_state)) {
				case total:
					return t.ldegree(args) >= std::get<1u>(m_state);
				case partial:
					return t.ldegree(std::get<2u>(m_state),args) >= std::get<1u>(m_state);
				default:
					return false;
			}
		}
		/// State of the truncator settings.
		/**
		 * Equivalent to the return type of piranha::degree_truncator_settings::get_state().
		 */
		const state_type m_state;
};

}

#endif
