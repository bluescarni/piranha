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

#ifndef PIRANHA_ECHELON_DESCRIPTOR_HPP
#define PIRANHA_ECHELON_DESCRIPTOR_HPP

#include <algorithm>
#include <boost/concept/assert.hpp>
#include <boost/integer_traits.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <cstddef>
#include <deque>
#include <iterator>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <vector>

#include "concepts/term.hpp"
#include "config.hpp"
#include "detail/echelon_descriptor_fwd.hpp"
#include "exceptions.hpp"
#include "symbol.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Echelon descriptor.
/**
 * This class describes the echelon structure of a term of type \p TopLevelTerm by means of 
 * an arguments tuple containing, for each level of the hierarchy, a vector of piranha::symbol instances representing the symbolic
 * arguments in that echelon level. The vectors in the arguments tuple are always kept sorted alphabetically, and no duplicate
 * symbols are allowed in the same echelon level. Getters method to
 * access and manipulate the arguments tuple are provided.
 * 
 * \section type_requirements Type requirements
 * 
 * \p TopLevelTerm must be a model of piranha::concept::Term.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class offers the strong exception safety guarantee for all operations.
 * 
 * \section move_semantics Move semantics
 * 
 * Move construction and move assignment will leave the moved-from object in the same state of a moved-from \p std::tuple of \p std::vector.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename TopLevelTerm>
class echelon_descriptor
{
		BOOST_CONCEPT_ASSERT((concept::Term<TopLevelTerm>));
		// Make friends with all kinds of descriptors.
		template <typename TopLevelTerm2>
		friend class echelon_descriptor;
		// Template MP to define a tuple of uniform types.
		template <typename T, std::size_t Size>
		struct ntuple
		{
			static_assert(Size > 0,"Size must be strictly positive.");
			typedef decltype(std::tuple_cat(std::tuple<T>{},typename ntuple<T,Size - static_cast<std::size_t>(1)>::type{})) type;
		};
		template <typename T>
		struct ntuple<T,static_cast<std::size_t>(1)>
		{
			typedef std::tuple<T> type;
		};
	public:
		/// Alias for \p TopLevelTerm.
		typedef TopLevelTerm top_level_term_type;
		/// Tuple of vectors of piranha::symbol.
		typedef typename ntuple<std::vector<symbol>,echelon_size<top_level_term_type>::value>::type args_tuple_type;
		/// Tuple for the representation of differences between two echelon descriptors.
		/**
		 * Each element of the tuple
		 * is a vector of vectors of indices, which, for each echelon level, represents how symbols from one descriptor must
		 * be inserted into the other in order to merge the two descriptors.
		 * 
		 * @see echelon_descriptor::difference() for an explanation of how this type is constructed and used.
		 */
		typedef typename ntuple<std::vector<std::vector<std::vector<symbol>::size_type>>,
			echelon_size<top_level_term_type>::value>::type diff_tuple_type;
	private:
		static std::vector<std::vector<std::vector<symbol>::size_type>>
			get_symbol_diff(const std::vector<symbol> &v1, const std::vector<symbol> &v2)
		{
			piranha_assert(std::is_sorted(v1.begin(),v1.end()));
			piranha_assert(std::is_sorted(v2.begin(),v2.end()));
			// Functor to locate the index of a symbol in v2.
			struct f_type
			{
				typedef std::vector<symbol>::size_type result_type;
				f_type(const std::vector<symbol> &v2):m_v2(v2) {}
				result_type operator()(const symbol &s) const
				{
					result_type retval = 0;
					for (; retval < m_v2.size(); ++retval) {
						if (m_v2[retval] == s) {
							break;
						}
					}
					piranha_assert(retval != m_v2.size());
					return retval;
				}
				const std::vector<symbol> &m_v2;
			};
			f_type f{v2};
			std::vector<std::vector<std::vector<symbol>::size_type>> retval;
			// Find the difference between the two sets of symbols: all elements in v2 that are not in v1.
			std::deque<symbol> difference;
			std::set_difference(v2.begin(),v2.end(),v1.begin(),v1.end(),std::back_insert_iterator<std::deque<symbol>>{difference});
			// Build the return value.
			for (decltype(v1.size()) index = 0; index < v1.size(); ++index) {
				retval.push_back(std::vector<std::vector<symbol>::size_type>{});
				while (difference.size() && !(v1[index] < difference.front())) {
					retval.back().push_back(f(difference.front()));
					difference.pop_front();
				}
			}
			// Push back at the end any remaining symbol in the difference.
			typedef boost::transform_iterator<decltype(f),decltype(difference.begin())> t_iter;
			retval.push_back(std::vector<std::vector<symbol>::size_type>{});
			retval.back().insert(retval.back().end(),t_iter(difference.begin(),f),t_iter(difference.end(),f));
			piranha_assert(retval.size() > 0);
			return retval;
		}
		template <std::size_t N = 0, typename Enable = void>
		struct difference_impl
		{
			static typename ntuple<std::vector<std::vector<std::vector<symbol>::size_type>>,echelon_size<top_level_term_type>::value - N>::type
				run(const args_tuple_type &args_tuple1, const args_tuple_type &args_tuple2)
			{
				return std::tuple_cat(std::make_tuple(get_symbol_diff(std::get<N>(args_tuple1),std::get<N>(args_tuple2))),
					difference_impl<N + static_cast<std::size_t>(1)>::run(args_tuple1,args_tuple2));
			}
		};
		template <std::size_t N>
		struct difference_impl<N,typename std::enable_if<N == echelon_size<TopLevelTerm>::value - static_cast<std::size_t>(1)>::type>
		{
			static std::tuple<std::vector<std::vector<std::vector<symbol>::size_type>>>
				run(const args_tuple_type &args_tuple1, const args_tuple_type &args_tuple2)
			{
				return std::make_tuple(get_symbol_diff(
					std::get<echelon_size<TopLevelTerm>::value - static_cast<std::size_t>(1)>(args_tuple1),
					std::get<echelon_size<TopLevelTerm>::value - static_cast<std::size_t>(1)>(args_tuple2))
				);
			}
		};
		template <std::size_t N = 0, typename Enable = void>
		struct apply_difference_impl
		{
			static void run(args_tuple_type &new_args_tuple, const diff_tuple_type &diff_tuple, const args_tuple_type &other_args_tuple)
			{
				auto &new_args = std::get<N>(new_args_tuple);
				const auto &diff = std::get<N>(diff_tuple);
				const auto &other_args = std::get<N>(other_args_tuple);
				piranha_assert(diff.size() > 0);
				piranha_assert(new_args.size() == diff.size() - decltype(diff.size())(1));
				decltype(new_args.size()) offset = 0;
				for (decltype(diff.size()) i = 0; i < diff.size(); ++i) {
					piranha_assert(std::is_sorted(diff[i].begin(),diff[i].end()));
					for (auto it = diff[i].begin(); it != diff[i].end(); ++it) {
						// First let's go to the position into which we want to insert.
						auto new_args_it = new_args.begin();
						std::advance(new_args_it,offset);
						std::advance(new_args_it,i);
						piranha_assert(new_args_it <= new_args.end());
						// Second, let's identify what we want to insert.
						auto other_args_it = other_args.begin();
						std::advance(other_args_it,*it);
						piranha_assert(other_args_it <= other_args.end());
						new_args.insert(new_args_it,*other_args_it);
						++offset;
					}
				}
				apply_difference_impl<N + static_cast<std::size_t>(1)>::run(new_args_tuple,diff_tuple,other_args_tuple);
			}
		};
		template <std::size_t N>
		struct apply_difference_impl<N,typename std::enable_if<N == echelon_size<TopLevelTerm>::value>::type>
		{
			static void run(args_tuple_type &, const diff_tuple_type &, const args_tuple_type &) {}
		};
		template <std::size_t N = 0, typename Enable = void>
		struct sort_check_impl
		{
			static bool run(const args_tuple_type &args_tuple)
			{
				if (!std::is_sorted(std::get<N>(args_tuple).begin(),std::get<N>(args_tuple).end())) {
					return false;
				} else {
					return sort_check_impl<N + static_cast<std::size_t>(1)>::run(args_tuple);
				}
			}
		};
		template <std::size_t N>
		struct sort_check_impl<N,typename std::enable_if<N == echelon_size<TopLevelTerm>::value - static_cast<std::size_t>(1)>::type>
		{
			static bool run(const args_tuple_type &args_tuple)
			{
				return std::is_sorted(
					std::get<echelon_size<TopLevelTerm>::value - static_cast<std::size_t>(1)>(args_tuple).begin(),
					std::get<echelon_size<TopLevelTerm>::value - static_cast<std::size_t>(1)>(args_tuple).end()
				);
			}
		};
	public:
		/// Defaulted default constructor.
		echelon_descriptor() = default;
		/// Defaulted copy constructor.
		/**
		 * @throw unspecified any exception thrown by the copy constructor of \p std::vector of piranha::symbol.
		 */
		echelon_descriptor(const echelon_descriptor &) = default;
		/// Move constructor.
		/**
		 * @param[in] other descriptor to move from.
		 */
		echelon_descriptor(echelon_descriptor &&other) piranha_noexcept_spec(true) : m_args_tuple(std::move(other.m_args_tuple)) {}
		/// Destructor.
		/**
		 * Equivalent to the default destructor, apart from checks being performed in debug mode.
		 */
		~echelon_descriptor() piranha_noexcept_spec(true)
		{
			piranha_assert(destruction_checks());
		}
		/// Copy assignment operator.
		/**
		 * @param[in] other descriptor to be copied.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the copy constructor of \p std::vector of piranha::symbol.
		 */
		echelon_descriptor &operator=(const echelon_descriptor &other)
		{
			// Use copy-and-move to make it exception-safe.
			if (this != &other) {
				auto tmp(other);
				*this = std::move(tmp);
			}
			return *this;
		}
		/// Move assignment operator.
		/**
		 * Will move the members of \p other into \p this.
		 * 
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 */
		echelon_descriptor &operator=(echelon_descriptor &&other) piranha_noexcept_spec(true)
		{
			m_args_tuple = std::move(other.m_args_tuple);
			return *this;
		}
		/// Arguments getter.
		/**
		 * @return const reference to the arguments vector at the echelon level corresponding
		 * to type \p Term. If \p Term does not appear in the echelon hierarchy of echelon_descriptor::top_level_term_type,
		 * a compile-time error will be produced.
		 */
		template <typename Term>
		const std::vector<symbol> &get_args() const
		{
			return std::get<echelon_position<top_level_term_type,Term>::value>(m_args_tuple);
		}
		/// Arguments tuple getter.
		/**
		 * @return const reference to the arguments tuple.
		 */
		const args_tuple_type &get_args_tuple() const
		{
			return m_args_tuple;
		}
		/// Calculate difference between echelon descriptors.
		/**
		 * This template method is enabled iff the echelon sizes of \p TopLevelTerm and \p TopLevelTerm2 coincide.
		 * 
		 * The difference between two echelon descriptors is described for each level of the echelon hierarchy in terms of the
		 * differences between the corresponding arguments vectors. Given two argument vectors \p a1 and \p a2, the difference of \p a1
		 * with respect to \p a2 is given as a structure describing how elements of \p a2 not appearing in \p a1 must be inserted into \p a1
		 * so that, after such merge operation, \p a1 is an ordered set representing the union of the arguments in \p a1 and \p a2. This structure
		 * is a vector \p d - whose size is <tt>a1</tt>'s size plus one - of vectors of indices. The element at index \p n in \p d contains
		 * the indices of those elements in \p a2 which must be inserted into \p a1 before \p n in order to merge \p a1 and \p a2.
		 * 
		 * For instance, given the two vectors of arguments <tt>a1 = ['c','e']</tt> and <tt>a2 = ['a','b','c','f']</tt>, vector \p d will be
		 * <tt>d = [[0,1],[],[3]]</tt>. This means that arguments <tt>['a','b']</tt> must be inserted before the first position in
		 * \p a1, that no arguments must be inserted before <tt>'e'</tt> in \p a1 and that argument <tt>'f'</tt> must be inserted after the end of \p a1.
		 * 
		 * @param[in] other piranha::echelon_descriptor with respect to which the difference will be calculated.
		 * 
		 * @return tuple of difference vectors.
		 * 
		 * @throws unspecified any exception thrown in case of memory allocation errors by standard containers.
		 */
		template <typename TopLevelTerm2>
		diff_tuple_type difference(const echelon_descriptor<TopLevelTerm2> &other,
			typename std::enable_if<echelon_size<TopLevelTerm>::value == echelon_size<TopLevelTerm2>::value>::type * = piranha_nullptr) const
		{
			return difference_impl<>::run(m_args_tuple,other.m_args_tuple);
		}
		/// Merge with other descriptor.
		/**
		 * This template method is enabled iff the echelon sizes of \p TopLevelTerm and \p TopLevelTerm2 coincide.
		 * 
		 * Returns a new piranha::echelon_descriptor resulting from applying the difference with respect to
		 * another piranha::echelon_descriptor calculated using difference(): output descriptor will contain
		 * all symbols from \p this plus the symbols in \p other not appearing in \p this.
		 * 
		 * If the echelon size of \p TopLevelTerm2 is different from that of \p TopLevelTerm, a compile-time error will be produced.
		 * 
		 * @param[in] other piranha::echelon_descriptor to be merged into \p this.
		 * 
		 * @return merged piranha::echelon_descriptor.
		 * 
		 * @throws unspecified any exception thrown in case of memory allocation errors by standard containers.
		 */
		template <typename TopLevelTerm2>
		echelon_descriptor merge(const echelon_descriptor<TopLevelTerm2> &other,
			typename std::enable_if<echelon_size<TopLevelTerm>::value == echelon_size<TopLevelTerm2>::value>::type * = piranha_nullptr) const
		{
			const diff_tuple_type diff = difference(other);
			args_tuple_type new_args_tuple = m_args_tuple;
			apply_difference_impl<>::run(new_args_tuple,diff,other.m_args_tuple);
			echelon_descriptor retval;
			retval.m_args_tuple = std::move(new_args_tuple);
			return retval;
		}
		/// Add symbol.
		/**
		 * The piranha::symbol \p s will be inserted into the arguments vector at the echelon position corresponding to \p Term.
		 * \p s must not be already present in the echelon position.
		 * 
		 * If \p Term does not appear in the echelon hierarchy of echelon_descriptor::top_level_term_type,
		 * a compile-time error will be produced.
		 * 
		 * @param[in] s piranha::symbol to be inserted into the descriptor.
		 * 
		 * @throws unspecified any exception thrown in case of memory allocation errors by standard containers.
		 * @throws std::invalid_argument if the symbol being added is already present in the destination echelon position.
		 */
		template <typename Term>
		void add_symbol(const symbol &s)
		{
			auto &args_vector = std::get<echelon_position<top_level_term_type,Term>::value>(m_args_tuple);
			auto new_args_vector = args_vector;
			const auto it = std::lower_bound(new_args_vector.begin(),new_args_vector.end(),s);
			if (unlikely(it != new_args_vector.end() && *it == s)) {
				piranha_throw(std::invalid_argument,"symbol already present in this echelon level");
			}
			new_args_vector.insert(it,s);
			// Move in the new args vector.
			args_vector = std::move(new_args_vector);
		}
	private:
		bool destruction_checks() const
		{
			return sort_check_impl<>::run(m_args_tuple);
		}
	private:
		args_tuple_type m_args_tuple;
};

}

#endif
