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

#ifndef PIRANHA_KRONECKER_MONOMIAL_HPP
#define PIRANHA_KRONECKER_MONOMIAL_HPP

#include <algorithm>
#include <boost/concept/assert.hpp>
#include <boost/integer_traits.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_set>

#include "concepts/degree_key.hpp"
#include "config.hpp"
#include "detail/kronecker_monomial_fwd.hpp"
#include "kronecker_array.hpp"
#include "static_vector.hpp"
#include "symbol_set.hpp"

namespace piranha
{

/// Kronecker monomial class.
/**
 * This class represents a multivariate monomial with integral exponents.
 * The values of the exponents are packed in a signed integer using Kronecker substitution, using the facilities provided
 * by piranha::kronecker_array.
 * 
 * This class is a model of the piranha::concept::DegreeKey concept.
 * 
 * \section type_requirements Type requirements
 * 
 * \p T must be suitable for use in piranha::kronecker_array. The default type for \p T is the signed counterpart of \p std::size_t.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * Unless otherwise specified, this class provides the strong exception safety guarantee for all operations.
 * 
 * \section move_semantics Move semantics
 * 
 * The move semantics of this class are equivalent to the move semantics of C++ signed integral types.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo investigate if we can leave the type parametrized or not: is it consistent with the size_type of hash set? What
 * are the effects on parallel kronecker multiplication (i.e., on the barriers that make the parallel algorithm work)?
 */
template <typename T = std::make_signed<std::size_t>::type>
class kronecker_monomial: detail::kronecker_monomial_tag
{
	public:
		/// Alias for \p T.
		typedef T value_type;
	private:
		typedef kronecker_array<value_type> ka;
		typedef typename std::decay<decltype(ka::get_limits())>::type _v_type;
		// Vector type used for temporary packing/unpacking.
		typedef static_vector<value_type,_v_type::max_size> v_type;
	public:
		/// Size type.
		/**
		 * Used to represent the number of variables in the monomial. Equivalent to the size type of
		 * piranha::kronecker_array.
		 */
		typedef typename ka::size_type size_type;
		/// Default constructor.
		/**
		 * After construction all exponents in the monomial will be zero.
		 */
		kronecker_monomial():m_value(0) {}
		/// Defaulted copy constructor.
		kronecker_monomial(const kronecker_monomial &) = default;
		/// Defaulted move constructor.
		kronecker_monomial(kronecker_monomial &&) = default;
		/// Constructor from initalizer list.
		/**
		 * The values in the initializer list are intended to represent the exponents of the monomial:
		 * they will be converted to type \p T (if \p T and \p U are not the same type),
		 * encoded using piranha::kronecker_array::encode() and the result assigned to the internal integer instance.
		 * 
		 * @param[in] list initializer list representing the exponents.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::kronecker_array::encode(),
		 * - \p boost::numeric_cast (in case \p U is not the same as \p T),
		 * - piranha::static_vector::push_back().
		 */
		template <typename U>
		explicit kronecker_monomial(std::initializer_list<U> list):m_value(0)
		{
			v_type tmp;
			for (auto it = list.begin(); it != list.end(); ++it) {
				tmp.push_back(boost::numeric_cast<value_type>(*it));
			}
			m_value = ka::encode(tmp);
		}
		/// Constructor from set of symbols.
		/**
		 * After construction all exponents in the monomial will be zero.
		 * 
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::kronecker_array::encode(),
		 * - piranha::static_vector::push_back().
		 */
		explicit kronecker_monomial(const symbol_set &args)
		{
			v_type tmp;
			for (auto it = args.begin(); it != args.end(); ++it) {
				tmp.push_back(value_type(0));
			}
			m_value = ka::encode(tmp);
		}
		/// Constructor from \p value_type.
		/**
		 * This constructor will initialise the internal integer instance
		 * to \p n.
		 * 
		 * @param[in] n initializer for the internal integer instance.
		 */
		explicit kronecker_monomial(const value_type &n):m_value(n) {}
		/// Trivial destructor.
		~kronecker_monomial() piranha_noexcept_spec(true)
		{
			BOOST_CONCEPT_ASSERT((concept::DegreeKey<kronecker_monomial>));
		}
		/// Defaulted copy assignment operator.
		kronecker_monomial &operator=(const kronecker_monomial &) = default;
		/// Trivial move assignment operator.
		/**
		 * Equivalent to copy assignment.
		 * 
		 * @param[in] other monomial to be assigned to this.
		 * 
		 * @return reference to \p this.
		 */
		kronecker_monomial &operator=(kronecker_monomial &&other) piranha_noexcept_spec(true)
		{
			// NOTE: replace with std::move and update docs if we ever allow for arbitrary integer types.
			m_value = other.m_value;
			return *this;
		}
		/// Set the internal integer instance.
		/**
		 * @param[in] n value to which the internal integer instance will be set.
		 */
		void set_int(const value_type &n)
		{
			m_value = n;
		}
		/// Get internal instance.
		/**
		 * @return value of the internal integer instance.
		 */
		value_type get_int() const
		{
			return m_value;
		}
		/// Compatibility check.
		/**
		 * Monomial is considered incompatible if any of these conditions holds:
		 * 
		 * - the size of \p args is zero and the internal integer is not zero,
		 * - the size of \p args is equal to or larger than the size of the output of piranha::kronecker_array::get_limits(),
		 * - the internal integer is not within the limits reported by piranha::kronecker_array::get_limits().
		 * 
		 * Otherwise, the monomial is considered to be compatible for insertion.
		 * 
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return compatibility flag for the monomial.
		 */
		bool is_compatible(const symbol_set &args) const piranha_noexcept_spec(true)
		{
			const auto s = args.size();
			// No args means the value must also be zero.
			if (s == 0u) {
				return !m_value;
			}
			const auto &limits = ka::get_limits();
			// If we overflow the maximum size available, we cannot use this object as key in series.
			if (s >= limits.size()) {
				return false;
			}
			const auto &l = limits[s];
			// Value is compatible if it is within the bounds for the given size.
			return (m_value >= std::get<3u>(l) && m_value <= std::get<4u>(l));
		}
		/// Ignorability check.
		/**
		 * A monomial is never considered ignorable.
		 * 
		 * @return \p false.
		 */
		bool is_ignorable(const symbol_set &) const piranha_noexcept_spec(true)
		{
			return false;
		}
		/// Merge arguments.
		/**
		 * Merge the new arguments set \p new_args into \p this, given the current reference arguments set
		 * \p orig_args.
		 * 
		 * @param[in] orig_args original arguments set.
		 * @param[in] new_args new arguments set.
		 * 
		 * @return monomial with merged arguments.
		 * 
		 * @throw std::invalid_argument if at least one of these conditions is true:
		 * - the size of \p new_args is not greater than the size of \p orig_args,
		 * - not all elements of \p orig_args are included in \p new_args.
		 * @throws unspecified any exception thrown by:
		 * - piranha::kronecker_array::encode(),
		 * - piranha::static_vector::push_back(),
		 * - unpack().
		 */
		kronecker_monomial merge_args(const symbol_set &orig_args, const symbol_set &new_args) const
		{
			if (unlikely(new_args.size() <= orig_args.size() ||
				!std::includes(new_args.begin(),new_args.end(),orig_args.begin(),orig_args.end())))
			{
				piranha_throw(std::invalid_argument,"invalid argument(s) for symbol set merging");
			}
			piranha_assert(std::is_sorted(orig_args.begin(),orig_args.end()));
			piranha_assert(std::is_sorted(new_args.begin(),new_args.end()));
			const auto old_vector = unpack(orig_args);
			v_type new_vector;
			auto it_new = new_args.begin();
			for (size_type i = 0u; i < old_vector.size(); ++i, ++it_new) {
				while (*it_new != orig_args[i]) {
					// NOTE: for arbitrary int types, value_type(0) might throw. Update docs
					// if needed.
					new_vector.push_back(value_type(0));
					piranha_assert(it_new != new_args.end());
					++it_new;
					piranha_assert(it_new != new_args.end());
				}
				new_vector.push_back(old_vector[i]);
			}
			// Fill up arguments at the tail of new_args but not in orig_args.
			for (; it_new != new_args.end(); ++it_new) {
				new_vector.push_back(value_type(0));
			}
			piranha_assert(new_vector.size() == new_args.size());
			// Return monomial with the new encoded vector.
			kronecker_monomial retval;
			retval.m_value = ka::encode(new_vector);
			return retval;
		}
		/// Check if monomial is unitary.
		/**
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return \p true if the internal integer instance is zero, \p false otherwise.
		 * 
		 * @throws std::invalid_argument if \p this is not compatible with \p args.
		 */
		bool is_unitary(const symbol_set &args) const
		{
			if (unlikely(!is_compatible(args))) {
				piranha_throw(std::invalid_argument,"invalid symbol set");
			}
			// A kronecker code will be zero if all components are zero.
			return !m_value;
		}
		/// Degree.
		/**
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return degree of the monomial.
		 * 
		 * @throws std::overflow_error if the computation of the degree overflows type \p value_type.
		 * @throws unspecified any exception thrown by unpack().
		 */
		value_type degree(const symbol_set &args) const
		{
			const auto tmp = unpack(args);
			return std::accumulate(tmp.begin(),tmp.end(),value_type(0),safe_adder);
		}
		/// Low degree.
		/**
		 * Equivalent to the degree.
		 * 
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return low degree of the monomial.
		 * 
		 * @throws unspecified any exception thrown by degree().
		 */
		value_type ldegree(const symbol_set &args) const
		{
			return degree(args);
		}
		/// Partial degree.
		/**
		 * Partial degree of the monomial: only the symbols with names in \p active_args are considered during the computation
		 * of the degree. Symbols in \p active_args not appearing in \p args are not considered.
		 * 
		 * @param[in] active_args names of the symbols that will be considered in the computation of the partial degree of the monomial.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return the summation of all the exponents of the monomial corresponding to the symbols in
		 * \p active_args, or <tt>value_type(0)</tt> if no symbols in \p active_args appear in \p args.
		 * 
		 * @throws std::overflow_error if the computation of the degree overflows type \p value_type.
		 * @throws unspecified any exception thrown by unpack().
		 */
		value_type degree(const std::set<std::string> &active_args, const symbol_set &args) const
		{
			const auto tmp = unpack(args);
			value_type retval(0);
			auto it1 = args.begin();
			auto it2 = active_args.begin();
			for (size_type i = 0u; i < tmp.size(); ++i, ++it1) {
				// Move forward the it2 iterator until it does not preceed the iterator in args,
				// or we run out of symbols.
				while (it2 != active_args.end() && *it2 < it1->get_name()) {
					++it2;
				}
				if (it2 == active_args.end()) {
					break;
				} else if (*it2 == it1->get_name()) {
					retval = safe_adder(retval,tmp[i]);
				}
			}
			return retval;
		}
		/// Partial low degree.
		/**
		 * Equivalent to the partial degree.
		 * 
		 * @param[in] active_args names of the symbols that will be considered in the computation of the partial low degree of the monomial.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return the partial low degree.
		 * 
		 * @throws unspecified any exception thrown by degree().
		 */
		value_type ldegree(const std::set<std::string> &active_args, const symbol_set &args) const
		{
			return degree(active_args,args);
		}
		/// Multiply monomial.
		/**
		 * The resulting monomial is computed by adding the exponents of \p this to the exponents of \p other.
		 * 
		 * @param[out] retval result of multiplying \p this by \p other.
		 * @param[in] other multiplicand.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @throws std::overflow_error if the computation of the result overflows type \p value_type.
		 * @throws unspecified any exception thrown by:
		 * - piranha::kronecker_array::encode(),
		 * - unpack(),
		 * - piranha::static_vector::push_back().
		 */
		void multiply(kronecker_monomial &retval, const kronecker_monomial &other, const symbol_set &args) const
		{
			const auto size = args.size();
			const auto tmp1 = unpack(args), tmp2 = other.unpack(args);
			v_type result;
			for (decltype(args.size()) i = 0u; i < size; ++i) {
				result.push_back(safe_adder(tmp1[i],tmp2[i]));
			}
			retval.m_value = ka::encode(result);
		}
		/// Hash value.
		/**
		 * @return the internal integer instance, cast to \p std::size_t.
		 */
		std::size_t hash() const
		{
			return static_cast<std::size_t>(m_value);
		}
		/// Equality operator.
		/**
		 * @param[in] other comparison argument.
		 * 
		 * @return \p true if the internal integral instance of \p this is equal to the integral instance of \p other,
		 * \p false otherwise.
		 */
		bool operator==(const kronecker_monomial &other) const
		{
			return m_value == other.m_value;
		}
		/// Inequality operator.
		/**
		 * @param[in] other comparison argument.
		 * 
		 * @return the opposite of operator==().
		 */
		bool operator!=(const kronecker_monomial &other) const
		{
			return m_value != other.m_value;
		}
		/// Unpack internal integer instance.
		/**
		 * Will decode the internal integral instance into a piranha::static_vector of size equal to the size of \p args.
		 * 
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return piranha::static_vector containing the result of decoding the internal integral instance via
		 * piranha::kronecker_array.
		 * 
		 * @throws std::invalid_argument if the size of \p args is larger than the maximum size of piranha::static_vector.
		 * @throws unspecified any exception thrown by piranha::kronecker_array::decode().
		 */
		v_type unpack(const symbol_set &args) const
		{
			v_type retval(args.size(),0);
			if (unlikely(retval.size() != args.size())) {
				piranha_throw(std::invalid_argument,"error creating the unpacked vector");
			}
			ka::decode(retval,m_value);
			return retval;
		}
		/// Print.
		/**
		 * Will print to stream a human-readable representation of the monomial.
		 * 
		 * @param[in] os target stream.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @throws unspecified any exception thrown by unpack() or by streaming instances of \p value_type.
		 */
		void print(std::ostream &os, const symbol_set &args) const
		{
			const auto tmp = unpack(args);
			piranha_assert(tmp.size() == args.size());
			const value_type zero(0), one(1);
			for (decltype(tmp.size()) i = 0u; i < tmp.size(); ++i) {
				if (tmp[i] != zero) {
					os << args[i].get_name();
					if (tmp[i] != one) {
						os << "**" << tmp[i];
					}
				}
			}
		}
	private:
		static value_type safe_adder(const value_type &a, const value_type &b)
		{
			if (b >= 0) {
				if (unlikely(a > boost::integer_traits<value_type>::const_max - b)) {
					piranha_throw(std::overflow_error,"overflow in the addition of two exponents in a Kronecker monomial");
				}
			} else {
				if (unlikely(a < boost::integer_traits<value_type>::const_min - b)) {
					piranha_throw(std::overflow_error,"overflow in the addition of two exponents in a Kronecker monomial");
				}
			}
			return a + b;
		}
	private:
		value_type m_value;
};

}

namespace std
{

/// Specialisation of \p std::hash for piranha::kronecker_monomial.
template <typename T>
struct hash<piranha::kronecker_monomial<T>>
{
	/// Result type.
	typedef size_t result_type;
	/// Argument type.
	typedef piranha::kronecker_monomial<T> argument_type;
	/// Hash operator.
	/**
	 * @param[in] a argument whose hash value will be computed.
	 * 
	 * @return hash value of \p a computed via piranha::kronecker_monomial::hash().
	 */
	result_type operator()(const argument_type &a) const
	{
		return a.hash();
	}
};

}

#endif
