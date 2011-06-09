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
#include <boost/integer_traits.hpp>
#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <vector>

#include "concepts/multipliable_term.hpp"
#include "concepts/series.hpp"
#include "echelon_descriptor.hpp"
#include "echelon_size.hpp"
#include "integer.hpp"

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
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the strong exception safety guarantee.
 * 
 * \section move_semantics Move semantics
 * 
 * Move construction and move assignment will have no effect on an existing object.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo series multiplier concept?
 * \todo evaluate the opportunity of using std::move when inserting into retval.
 */
template <typename Series1, typename Series2, typename Enable = void>
class series_multiplier
{
		BOOST_CONCEPT_ASSERT((concept::Series<Series1>));
		BOOST_CONCEPT_ASSERT((concept::Series<Series2>));
		BOOST_CONCEPT_ASSERT((concept::MultipliableTerm<typename Series1::term_type>));
		static_assert(echelon_size<typename Series1::term_type>::value == echelon_size<typename Series2::term_type>::value,
			"Mismatch in echelon sizes.");
		typedef typename Series1::term_type term_type1;
		typedef typename Series2::term_type term_type2;
		typedef typename Series1::base_series_type return_type;
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
		typename Series1::base_series_type operator()(const echelon_descriptor<Term> &ed) const
		{
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
			typename term_type1::multiplication_result_type tmp;
			return_type retval;
			const auto size1 = v1.size(), size2 = v2.size();
			if (size1 > 2000u && size2 > 2000u) {
				// NOTE: here we could have (very unlikely) some overflow or memory error in the computation
				// of the estimate. In such a case, just ignore the rehashing and proceed.
				try {
					retval.m_container.rehash(estimate_size(&v1[0u],size1,&v2[0u],size2,ed));
				} catch (...) {}
			}
			const auto f = [&v1,&v2,&tmp,&ed,&retval](const decltype(v1.size()) &i, const decltype(v1.size()) &j) -> void {
				(v1[i])->multiply(tmp,*(v2[j]),ed);
				series_multiplier::insert_impl(retval,tmp,ed);
			};
			blocked_multiplication(f,decltype(v1.size())(0u),size1,decltype(v2.size())(0u),size2);
			return retval;
		}
	private:
		template <typename Size, typename Term>
		static auto estimate_size(const term_type1 **t1, const Size &size1, const term_type2 **t2,
			const Size &size2, const echelon_descriptor<Term> &ed) ->
			decltype(std::declval<return_type>().m_container.bucket_count())
		{
			std::vector<typename std::decay<decltype(*t1)>::type> copy1(t1,t1 + size1);
			std::vector<typename std::decay<decltype(*t2)>::type> copy2(t2,t2 + size2);
			integer mean(0);
			// NOTE: hard-coded number of trials: 10. This could be easily parallelised, but not
			// sure if it is worth.
			const int ntrials = 10;
			typename term_type1::multiplication_result_type tmp;
			for (int n = 0; n < ntrials; ++n) {
				std::random_shuffle(copy1.begin(),copy1.end());
				std::random_shuffle(copy2.begin(),copy2.end());
				return_type tmp_retval;
				Size i = 0u;
				for (; i < std::min<Size>(size1,size2); ++i) {
					(copy1[i])->multiply(tmp,*(copy2[i]),ed);
					insert_impl(tmp_retval,tmp,ed);
					if (tmp_retval.size() != i + 1u) {
						break;
					}
				}
				mean += i;
			}
			mean /= ntrials;
			// NOTE: heuristic from experiments.
			const integer M = (mean * mean * 4) / 3;
			return static_cast<decltype(std::declval<return_type>().m_container.bucket_count())>(M);
		}
		template <typename Functor, typename Size>
		static void blocked_multiplication(const Functor &f, const Size &start1, const Size &size1,
			const Size &start2, const Size &size2)
		{
			// NOTE: hard-coded block size of 256.
			static_assert(std::is_unsigned<Size>::value && boost::integer_traits<Size>::const_max >= 256u, "Invalid size type.");
			const Size bsize1 = 256u, nblocks1 = size1 / bsize1, bsize2 = bsize1, nblocks2 = size2 / bsize2;
			// Start and end of last (possibly irregular) blocks.
			const Size i_ir_start = start1 + nblocks1 * bsize1, i_ir_end = start1 + size1;
			const Size j_ir_start = start2 + nblocks2 * bsize2, j_ir_end = start2 + size2;
			for (Size n1 = 0u; n1 < nblocks1; ++n1) {
				const Size i_start = start1 + n1 * bsize1, i_end = i_start + bsize1;
				// regulars1 * regulars2
				for (Size n2 = 0u; n2 < nblocks2; ++n2) {
					const Size j_start = start2 + n2 * bsize2, j_end = j_start + bsize2;
					for (Size i = i_start; i < i_end; ++i) {
						for (Size j = j_start; j < j_end; ++j) {
							f(i,j);
						}
					}
				}
				// regulars1 * rem2
				for (Size i = i_start; i < i_end; ++i) {
					for (Size j = j_ir_start; j < j_ir_end; ++j) {
						f(i,j);
					}
				}
			}
			// rem1 * regulars2
			for (Size n2 = 0u; n2 < nblocks2; ++n2) {
				const Size j_start = start2 + n2 * bsize2, j_end = j_start + bsize2;
				for (Size i = i_ir_start; i < i_ir_end; ++i) {
					for (Size j = j_start; j < j_end; ++j) {
						f(i,j);
					}
				}
			}
			// rem1 * rem2.
			for (Size i = i_ir_start; i < i_ir_end; ++i) {
				for (Size j = j_ir_start; j < j_ir_end; ++j) {
					f(i,j);
				}
			}
		}
		template <typename Tuple, std::size_t N = 0, typename Enable2 = void>
		struct inserter
		{
			template <typename Term>
			static void run(Tuple &t, return_type &retval, const echelon_descriptor<Term> &ed)
			{
				retval.insert(std::get<N>(t),ed);
				inserter<Tuple,N + static_cast<std::size_t>(1)>::run(t,retval,ed);
			}
		};
		template <typename Tuple, std::size_t N>
		struct inserter<Tuple,N,typename std::enable_if<N == std::tuple_size<Tuple>::value>::type>
		{
			template <typename Term>
			static void run(Tuple &, return_type &, const echelon_descriptor<Term> &)
			{}
		};
		template <typename Term, typename... Args>
		static void insert_impl(return_type &retval,std::tuple<Args...> &mult_res, const echelon_descriptor<Term> &ed)
		{
			inserter<std::tuple<Args...>>::run(mult_res,retval,ed);
		}
		template <typename Term>
		static void insert_impl(return_type &retval, typename Series1::term_type &mult_res, const echelon_descriptor<Term> &ed)
		{
			retval.insert(mult_res,ed);
		}
	private:
		const Series1 &m_s1;
		const Series2 &m_s2;
};

}

#endif
