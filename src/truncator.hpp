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

#ifndef PIRANHA_TRUNCATOR_HPP
#define PIRANHA_TRUNCATOR_HPP

#include <boost/concept/assert.hpp>
#include <iostream>
#include <tuple>

#include "concepts/series.hpp"
#include "concepts/truncator.hpp"
#include "config.hpp"
#include "detail/sfinae_types.hpp"
#include "detail/truncator_fwd.hpp"
#include "echelon_size.hpp"

namespace piranha
{

/// Default truncator class.
/**
 * A truncator is an object that is intended to establish an order over the terms of a series and truncate the result of series multiplication.
 * The default series truncator has a minimal interface that will not truncate or sort terms. Specialisations of this class
 * can be unary or binary truncators.
 * 
 * <em>Unary</em> truncators are used to operate on the terms of a single series, generally with ranking purposes.
 * A unary truncator will be parametrized over a single type \p Series1 (the series type on which the truncator
 * is going to operate) and will provide a constructor from \p Series1, in addition to a streaming operator overload that will be used
 * to print to stream a human-readable representation of the truncator. Additionally, the following optional methods can be provided
 * by a unary truncator:
 * 
 * - a <tt>bool compare_terms(const term_type &t1, const term_type &t2) const</tt> method, which must be a strict
 *   weak ordering comparison function usable on the term types of \p Series1
 *   that returns \p true if \p t1 comes before \p t2, \p false otherwise. This method
 *   is intended to be used to rank the terms of the series used for construction. A truncator implementing
 *   this method is a <em>sorting</em> truncator;
 * - a <tt>bool filter(const term_type &t) const</tt> method, which shall return \p true if term \p t
 *   can be discarded given the current truncator settings, \p false otherwise.
 *   A truncator implementing this method is a <em>filtering</em> truncator.
 * 
 * <em>Binary</em> truncators are used during series multiplication, and they are parametrised over the operand series types
 * \p Series1 and \p Series2. Binary truncators will provide a binary constructor from \p Series1 and \p Series2 objects,
 * in addition to a streaming operator overload that will be used
 * to print to stream a human-readable representation of the truncator.
 * In a binary truncator, the term types of \p Series1 and \p Series2 must have equal echelon sizes.
 * Additionally, the following optional methods can be provided
 * by a binary truncator:
 * 
 * - a <tt>bool compare_terms(const term_type &t1, const term_type &t2) const</tt> method equivalent to the method with the same name
 *   for unary truncators. The binary version of this method must be able to compare the term types of both \p Series1 and \p Series2.
 *   A binary truncator implementing this method is a <em>sorting</em> truncator;
 * - a <tt>bool filter(const term_type &t) const</tt> method, equivalent to the unary version and able to operate on the term types of \p Series1.
 *   A truncator implementing this method is a <em>filtering</em> truncator.
 * - a <tt>bool skip(const term_type1 &t1, const term_type2 &t2) const</tt> method, which can be used during series multiplication
 *   after the terms of the series operands have been sorted using the <tt>compare_terms()</tt> method.
 *   The <tt>skip()</tt> method will return \p true if the result of the multiplication of \p t1 by \p t2 and of
 *   any couple of terms following \p t1 and \p t2 in the ranking established by <tt>compare_terms()</tt>
 *   can be discarded given the current truncation settings,
 *   \p false otherwise. For instance, a typical truncator for polynomials will sort the terms by their (partial) degree and will be
 *   able to skip all term-by-term multiplications after the degree limit has been reached.
 *   A truncator implementing this method is a <em>skipping</em> truncator. For consistency reasons, a skipping truncator
 *   must also be a sorting and filtering truncator, otherwise compile-time errors will be produced.
 * 
 * All truncators must provide a <tt>bool is_active() const</tt> method that returns \p true if the truncator is in effect, \p false if it is not.
 * This method is provided for optimization purposes, in order to allow an implementation to skip all truncator-related operations when there
 * is no need to (e.g., the truncator has a global option set to 'disabled'). It will be assumed that the <tt>skip()</tt> and
 * <tt>filter()</tt> methods of an inactive truncator object will always return \p false. It will also be assumed that during a series multiplication
 * a skipping tuncator does all the necessary filtering (i.e., it will not be necessary to filter terms when using a skipping truncator).
 * 
 * All truncators must be copy-constructible. Note that a truncator object used together with the default piranha::series_multiplier implementation
 * must be usable concurrently from multiple threads.
 * 
 * The presence of the optional methods can be queried at compile time using the piranha::truncator_traits class. The default
 * implementation of piranha::truncator is a valid unary and binary truncator that
 * does not implement any of the optional methods, and whose is_active() method will always
 * return \p false.
 * 
 * \section type_requirements Type requirements
 * 
 * <tt>Series ...</tt> must be a pack of 1 or 2 types modelling the piranha::concept::Series concept. In case the number of template parameters
 * is 2, the term types of the series must have equal echelon size.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the strong exception safety guarantee for all operations.
 * 
 * \section move_semantics Move semantics
 * 
 * This class is stateless and hence provides trivial move semantics.
 * 
 * \todo require unset() method?
 */
template <typename... Series>
class truncator
{
		static_assert(sizeof...(Series) == 1u || sizeof...(Series) == 2u,"Invalid number of template parameters for piranha::truncator.");
		template <typename T>
		static void concept_check()
		{
			BOOST_CONCEPT_ASSERT((concept::Series<T>));
		}
		template <typename T, typename U>
		static void concept_check()
		{
			BOOST_CONCEPT_ASSERT((concept::Series<T>));
			BOOST_CONCEPT_ASSERT((concept::Series<U>));
			static_assert(echelon_size<typename T::term_type>::value == echelon_size<typename U::term_type>::value,"Inconsistent echelon sizes.");
		}
	public:
		/// Constructor from series.
		explicit truncator(const Series & ...) {}
		/// Trivial destructor.
		~truncator()
		{
			concept_check<Series...>();
			BOOST_CONCEPT_ASSERT((concept::Truncator<Series...>));
		}
		/// Status flag.
		/**
		 * @return \p false.
		 */
		bool is_active() const
		{
			return false;
		}
		/// Stream operator overload for the default implementation of piranha::truncator.
		/**
		 * Will send to stream \p os a human-readable representation of the truncator.
		 * 
		 * @param[in] os target stream.
		 * @param[in] t piranha::truncator argument.
		 * 
		 * @return reference to \p os.
		 */
		friend std::ostream &operator<<(std::ostream &os, const truncator &t)
		{
			(void)t;
			os << "Default null truncator.\n";
			return os;
		}
};

/// Binary truncator traits.
/**
 * This traits class is used to query which optional methods are implemented in a binary piranha::truncator.
 * 
 * \section type_requirements Type requirements
 * 
 * - <tt>Series ...</tt> must be a pack 2 types suitable for use in piranha::truncator.
 * - piranha::truncator of <tt>Series ...</tt> must be a model of the piranha::concept::Truncator concept.
 * 
 * @see piranha::truncator for the description of the optional interface.
 */
template <typename... Series>
class truncator_traits: detail::sfinae_types
{
		static_assert(sizeof...(Series) == 2u,"Invalid number of template parameters for piranha::truncator_traits.");
		typedef std::tuple<Series...> _ttype;
		BOOST_CONCEPT_ASSERT((concept::Series<typename std::tuple_element<0u,_ttype>::type>));
		BOOST_CONCEPT_ASSERT((concept::Series<typename std::tuple_element<1u,_ttype>::type>));
		static_assert(echelon_size<typename std::tuple_element<0u,_ttype>::type::term_type>::value ==
			echelon_size<typename std::tuple_element<1u,_ttype>::type::term_type>::value,"Inconsistent echelon sizes.");
		BOOST_CONCEPT_ASSERT((concept::Truncator<Series...>));
		typedef truncator<Series...> truncator_type;
		typedef typename std::tuple_element<0u,_ttype>::type::term_type term_type1;
		typedef typename std::tuple_element<1u,_ttype>::type::term_type term_type2;
		template <typename T>
		static auto test_sorting1(const T *t) -> decltype(t->compare_terms(std::declval<term_type1>(),std::declval<term_type1>()),yes());
		static no test_sorting1(...);
		template <typename T>
		static auto test_sorting2(const T *t) -> decltype(t->compare_terms(std::declval<term_type2>(),std::declval<term_type2>()),yes());
		static no test_sorting2(...);
		template <typename T>
		static auto test_filtering(const T *t) -> decltype(t->filter(std::declval<term_type1>()),yes());
		static no test_filtering(...);
		template <typename T>
		static auto test_skipping(const T *t) -> decltype(t->skip(std::declval<term_type1>(),std::declval<term_type2>()),yes());
		static no test_skipping(...);
		static const bool is_sorting_impl = (sizeof(test_sorting1((const truncator_type *)piranha_nullptr)) == sizeof(yes) &&
			sizeof(test_sorting2((const truncator_type *)piranha_nullptr)) == sizeof(yes));
		static const bool is_filtering_impl = (sizeof(test_filtering((const truncator_type *)piranha_nullptr)) == sizeof(yes));
		static const bool is_skipping_impl = (sizeof(test_skipping((const truncator_type *)piranha_nullptr)) == sizeof(yes));
	public:
		/// Sorting flag.
		/**
		 * Will be \p true if the truncator is a sorting truncator, \p false otherwise.
		 */
		static const bool is_sorting = is_sorting_impl;
		/// Filtering flag.
		/**
		 * Will be \p true if the truncator is a filtering truncator, \p false otherwise.
		 */
		static const bool is_filtering = is_filtering_impl;
		/// Skipping flag.
		/**
		 * Will be \p true if the truncator type of \p Series is a skipping truncator, \p false otherwise.
		 */
		static const bool is_skipping = is_skipping_impl;
	private:
		static_assert(!is_skipping || (is_skipping && is_sorting && is_filtering),
			"A skipping truncator must also be a sorting and filtering truncator.");
};

template <typename... Series>
const bool truncator_traits<Series...>::is_sorting;

template <typename... Series>
const bool truncator_traits<Series...>::is_filtering;

template <typename... Series>
const bool truncator_traits<Series...>::is_skipping;

/// Unary truncator traits.
/**
 * This traits class is used to query which optional methods are implemented in a unary piranha::truncator.
 * 
 * \section type_requirements Type requirements
 * 
 * - <tt>Series</tt> must be a type suitable for use in piranha::truncator.
 * - piranha::truncator of <tt>Series</tt> must be a model of the piranha::concept::Truncator concept.
 * 
 * @see piranha::truncator for the description of the optional interface.
 */
template <typename Series>
class truncator_traits<Series>: detail::sfinae_types
{
		BOOST_CONCEPT_ASSERT((concept::Series<Series>));
		BOOST_CONCEPT_ASSERT((concept::Truncator<Series>));
		typedef truncator<Series> truncator_type;
		typedef typename Series::term_type term_type;
		template <typename T>
		static auto test_sorting(const T *t) -> decltype(t->compare_terms(std::declval<term_type>(),std::declval<term_type>()),yes());
		static no test_sorting(...);
		static const bool is_sorting_impl = (sizeof(test_sorting((const truncator_type *)piranha_nullptr)) == sizeof(yes));
		template <typename T>
		static auto test_filtering(const T *t) -> decltype(t->filter(std::declval<term_type>()),yes());
		static no test_filtering(...);
		static const bool is_filtering_impl = (sizeof(test_filtering((const truncator_type *)piranha_nullptr)) == sizeof(yes));
	public:
		/// Sorting flag.
		/**
		 * Will be \p true if the truncator is a sorting truncator, \p false otherwise.
		 */
		static const bool is_sorting = is_sorting_impl;
		/// Filtering flag.
		/**
		 * Will be \p true if the truncator is a filtering truncator, \p false otherwise.
		 */
		static const bool is_filtering = is_filtering_impl;
};

template <typename Series>
const bool truncator_traits<Series>::is_sorting;

template <typename Series>
const bool truncator_traits<Series>::is_filtering;

}

#endif
