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

#ifndef PIRANHA_SERIES_MULTIPLIER_H
#define PIRANHA_SERIES_MULTIPLIER_H

#include <algorithm>
#include <boost/concept/assert.hpp>
#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <vector>

#include "concepts/multipliable_term.hpp"
#include "concepts/series.hpp"
#include "echelon_descriptor.hpp"
#include "echelon_size.hpp"
#include "utils.hpp"

namespace piranha
{

/// Default series multiplier.
/**
 * This class is used within piranha::base_series::multiply_by_series() to multiply two series as follows:
 * 
 * - an instance of series multiplier is created using two series (possibly of different types) as construction arguments,
 * - operator()() is called with a generic piranha::echelon_descriptor as argument, returning an instance
 *   of \p Series1 as result of the multiplication of the two series used for construction.
 * 
 * Any specialisation of this class must respect the protocol described above (i.e., construction from series
 * instances and operator()() to compute the result).
 * 
 * \section type_requirements Type requirements
 * 
 * - \p Series1 and \p Series2 must be models of piranha::concept::Series. Additionally, the echelon sizes of
 *   \p Series1 and \p Series2 must be the same, otherwise a compile-time assertion will fail;
 * - the term type of \p Series1 must be a model of piranha::concept::MultipliableTerm.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo series multiplier concept?
 */
template <typename Series1, typename Series2, typename Enable = void>
class series_multiplier
{
		BOOST_CONCEPT_ASSERT((concept::Series<Series1>));
		BOOST_CONCEPT_ASSERT((concept::Series<Series2>));
		BOOST_CONCEPT_ASSERT((concept::MultipliableTerm<typename Series1::term_type>));
		static_assert(echelon_size<typename Series1::term_type>::value == echelon_size<typename Series2::term_type>::value,
			"Mismatch in echelon sizes.");
	public:
		/// Constructor.
		/**
		 * @param[in] s1 first series.
		 * @param[in] s2 second series.
		 */
		series_multiplier(const Series1 &s1, const Series2 &s2) : m_s1(s1),m_s2(s2)
		{}
		/// Compute result of series multiplication.
		/**
		 * @param[in] ed piranha::echelon_descriptor that will be used to build (via repeated insertions) the result of the series multiplication.
		 * 
		 * @return the result of multiplying the two series used for construction.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - memory allocation errors in standard containers,
		 * - piranha::base_series::insert(),
		 * - the <tt>multiply()</tt> method of the term type of \p Series1.
		 */
		template <typename Term>
		Series1 operator()(const echelon_descriptor<Term> &ed) const
		{
			typedef typename Series1::term_type term_type1;
			typedef typename Series2::term_type term_type2;
			// Cache the pointers.
			// NOTE: here maybe it is possible to improve performance by using std::array when number of
			// terms is small.
			std::vector<term_type1 const *> v1;
			std::vector<term_type2 const *> v2;
			v1.reserve(m_s1.size());
			v2.reserve(m_s2.size());
			// Fill in the vectors of pointers.
			std::back_insert_iterator<decltype(v1)> bii1(v1);
			std::transform(m_s1.m_container.begin(),m_s1.m_container.end(),bii1,[](const term_type1 &t) {return &t;});
			std::back_insert_iterator<decltype(v2)> bii2(v2);
			std::transform(m_s2.m_container.begin(),m_s2.m_container.end(),bii2,[](const term_type2 &t) {return &t;});
			return mult_impl(v1.begin(),v1.end(),v2.begin(),v2.end(),ed);
		}
	private:
		template <typename Iterator1, typename Iterator2, typename Term>
		static Series1 mult_impl(const Iterator1 &it_i1, const Iterator1 &it_f1, const Iterator2 &it_i2, const Iterator2 &it_f2, const echelon_descriptor<Term> &ed)
		{
			Series1 retval;
			for (auto it1 = it_i1; it1 != it_f1; ++it1) {
				for (auto it2 = it_i2; it2 != it_f2; ++it2) {
					insert_impl(retval,(*it1)->multiply(*(*it2),ed),ed);
				}
			}
			return retval;
		}
		template <typename Tuple, std::size_t N = 0, typename Enable2 = void>
		struct inserter
		{
			template <typename Term>
			static void run(Tuple &t, Series1 &retval, const echelon_descriptor<Term> &ed)
			{
				retval.insert(std::move(std::get<N>(t)),ed);
				inserter<Tuple,N + static_cast<std::size_t>(1)>::run(t,retval,ed);
			}
		};
		template <typename Tuple, std::size_t N>
		struct inserter<Tuple,N,typename std::enable_if<N == std::tuple_size<Tuple>::value>::type>
		{
			template <typename Term>
			static void run(Tuple &, Series1 &, const echelon_descriptor<Term> &)
			{}
		};
		template <typename Term, typename... Args>
		static void insert_impl(Series1 &retval,std::tuple<Args...> &&mult_res, const echelon_descriptor<Term> &ed)
		{
			inserter<std::tuple<Args...>>::run(mult_res,retval,ed);
		}
		template <typename Term>
		static void insert_impl(Series1 &retval,typename Series1::term_type &&mult_res, const echelon_descriptor<Term> &ed)
		{
			retval.insert(std::move(mult_res),ed);
		}
	private:
		const Series1 &m_s1;
		const Series2 &m_s2;
};

}

#endif
