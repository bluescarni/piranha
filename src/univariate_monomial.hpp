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

#ifndef PIRANHA_UNIVARIATE_MONOMIAL_HPP
#define PIRANHA_UNIVARIATE_MONOMIAL_HPP

#include <algorithm>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "array_key.hpp"
#include "config.hpp"
#include "exceptions.hpp"
#include "math.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Univariate monomial.
/**
 * This class represents a univariate monomial with exponent of type \p T. The exponent is represented
 * by an instance of \p T stored within the object.
 * 
 * This class satisfies the piranha::is_key, piranha::key_has_degree and piranha::key_has_ldegree type traits.
 * 
 * \section type_requirements Type requirements
 * 
 * \p T must satisfy piranha::is_array_key_value_type.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * Unless otherwise specified, this class provides the strong exception safety guarantee for all operations.
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics is equivalent to <tt>T</tt>'s move semantics.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 *
 * \todo this has been left behind a bit feature-wise.
 * \todo needs sfinaeing
 * \todo review type requirements.
 */
template <typename T>
class univariate_monomial
{
		PIRANHA_TT_CHECK(is_array_key_value_type,T);
	public:
		/// Value type.
		/**
		 * Alias for \p T.
		 */
		typedef T value_type;
		/// Size type.
		typedef std::size_t size_type;
		/// Default constructor.
		/**
		 * Will initialise the exponent to 0.
		 * 
		 * @throws unspecified any exception thrown by constructing an instance of \p T from 0.
		 */
		univariate_monomial():m_value(0) {}
		/// Defaulted copy constructor.
		/**
		 * @throws unspecified any exception thrown by the copy constructor of \p T.
		 */
		univariate_monomial(const univariate_monomial &) = default;
		/// Defaulted move constructor.
		univariate_monomial(univariate_monomial &&) = default;
		/// Converting constructor.
		/**
		 * This constructor is for use when converting from one term type to another in piranha::series. It will
		 * set the internal integer instance to the same value of \p m, after having checked that
		 * \p m is compatible with \p args.
		 * 
		 * @param[in] m construction argument.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @throws std::invalid_argument if \p m is not compatible with \p args.
		 */
		explicit univariate_monomial(const univariate_monomial &m, const symbol_set &args):m_value(m.m_value)
		{
			if (unlikely(!m.is_compatible(args))) {
				piranha_throw(std::invalid_argument,"incompatible arguments set");
			}
		}
		/// Constructor from set of symbols.
		/**
		 * This constructor will initialise the value of the exponent to 0.
		 * 
		 * @param[in] args set of symbols used for construction.
		 * 
		 * @throws std::invalid_argument if the size of \p args is greater than 1.
		 * @throws unspecified any exception thrown by the construction of an instance of \p T from 0.
		 */
		explicit univariate_monomial(const symbol_set &args):m_value(0)
		{
			if (unlikely(args.size() > 1u)) {
				piranha_throw(std::invalid_argument,"excessive number of symbols for univariate monomial");
			}
		}
		/// Constructor from initialiser list.
		/**
		 * This constructor will initialise the value of the exponent to 0 if the list is empty,
		 * to the first element of the list if the list is not empty.
		 * 
		 * @param[in] list list of values used for initialisation.
		 * 
		 * @throws std::invalid_argument if the size of \p list is greater than 1.
		 * @throws unspecified any exception thrown by the construction of an instance of \p T from 0,
		 * or by copy-assigning an instance of \p U to an instance of \p T.
		 */
		template <typename U>
		explicit univariate_monomial(std::initializer_list<U> list):m_value(0)
		{
			if (unlikely(list.size() > 1u)) {
				piranha_throw(std::invalid_argument,"excessive number of symbols for univariate monomial");
			}
			if (list.size()) {
				m_value = *list.begin();
			}
		}
		/// Trivial destructor.
		~univariate_monomial()
		{
			PIRANHA_TT_CHECK(is_key,univariate_monomial);
			PIRANHA_TT_CHECK(key_has_degree,univariate_monomial);
			PIRANHA_TT_CHECK(key_has_ldegree,univariate_monomial);
		}
		/// Copy assignment operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the copy constructor of \p T.
		 */
		univariate_monomial &operator=(const univariate_monomial &other)
		{
			if (likely(this != &other)) {
				univariate_monomial tmp(other);
				*this = std::move(tmp);
			}
			return *this;
		}
		/// Defaulted move assignment operator.
		univariate_monomial &operator=(univariate_monomial &&) = default;
		/// Hash value.
		/**
		 * @return the hash value of the exponent, calculated via \p std::hash in case \p T is not an integral
		 * type, via direct casting of the exponent to \p std::size_t otherwise.
		 * 
		 * @throws unspecified any exception thrown by \p std::hash of type \p T.
		 */
		std::size_t hash() const
		{
			return compute_hash(m_value);
		}
		/// Equality operator.
		/**
		 * @param[in] m comparison argument.
		 * 
		 * @return \p true if the exponents of \p this and \p m are the same, \p false otherwise.
		 * 
		 * @throws unspecified any exception thrown by the comparison operator of \p T.
		 */
		bool operator==(const univariate_monomial &m) const
		{
			return m_value == m.m_value;
		}
		/// Inequality operator.
		/**
		 * @param[in] m comparison argument.
		 * 
		 * @return the opposite of operator==().
		 * 
		 * @throws unspecified any exception thrown by operator==().
		 */
		bool operator!=(const univariate_monomial &m) const
		{
			return !(*this == m);
		}
		/// Compatibility test.
		/** 
		 * @param[in] args reference arguments set.
		 * 
		 * @return \p true if the size of \p args is 1 or if the size of \p args is 0 and piranha::math::is_zero()
		 * on the exponent returns \p true.
		 */
		bool is_compatible(const symbol_set &args) const noexcept(true)
		{
			const auto size = args.size();
			return (size == 1u || (!size && math::is_zero(m_value)));
		}
		/// Ignorability test.
		/**
		 * @return \p false (a monomial is never ignorable).
		 */
		bool is_ignorable(const symbol_set &) const noexcept(true)
		{
			return false;
		}
		/// Merge arguments.
		/**
		 * Arguments merging in case of univariate monomial is meaningful only when extending a zero-arguments
		 * monomial to one argument. Therefore, this method will either throw or return a default-constructed monomial.
		 * 
		 * @param[in] orig_args original arguments.
		 * @param[in] new_args new arguments.
		 * 
		 * @return a default-constructed instance of piranha::univariate_monomial.
		 * 
		 * @throws std::invalid_argument if the size of \p new_args is different from 1 or the size of \p orig_args is not zero.
		 * @throws unspecified any exception thrown by the default constructor of piranha::univariate_monomial.
		 */
		univariate_monomial merge_args(const symbol_set &orig_args, const symbol_set &new_args) const
		{
			piranha_assert(std::is_sorted(orig_args.begin(),orig_args.end()));
			piranha_assert(std::is_sorted(new_args.begin(),new_args.end()));
			if (unlikely(new_args.size() != 1u || orig_args.size())) {
				piranha_throw(std::invalid_argument,"invalid symbol set");
			}
			piranha_assert(math::is_zero(m_value));
			// The only valid possibility here is that a monomial with zero args is extended
			// to one arg. Default construction is ok.
			return univariate_monomial();
		}
		/// Check if monomial is unitary.
		/**
		 * A monomial is unitary if piranha::math::is_zero() on its exponent returns \p true.
		 * 
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return \p true if the monomial is unitary, \p false otherwise.
		 * 
		 * @throws std::invalid_argument if the size of \p args is greater than one or
		 * if the size is zero and the exponent is not zero.
		 * @throws unspecified any exception thrown by piranha::math::is_zero().
		 */
		bool is_unitary(const symbol_set &args) const
		{
			const bool is_zero = math::is_zero(m_value);
			if (unlikely(args.size() > 1u || (!args.size() && !is_zero))) {
				piranha_throw(std::invalid_argument,"invalid symbol set");
			}
			return is_zero;
		}
		/// Degree.
		/**
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return degree of the monomial.
		 * 
		 * @throws std::invalid_argument if the size of \p args is greater than one or
		 * if the size is zero and the exponent is not zero.
		 * @throws unspecified any exception thrown by piranha::math::is_zero().
		 */
		T degree(const symbol_set &args) const
		{
			if (unlikely(args.size() > 1u || (!args.size() && !math::is_zero(m_value)))) {
				piranha_throw(std::invalid_argument,"invalid symbol set");
			}
			return m_value;
		}
		/// Low degree.
		/**
		 * Analogous to the degree.
		 * 
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return low degree of the monomial.
		 * 
		 * @throws unspecified any exception thrown by degree().
		 */
		T ldegree(const symbol_set &args) const
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
		 * @throws std::invalid_argument if the size of \p args is greater than one or
		 * if the size is zero and the exponent is not zero.
		 * @throws unspecified any exception thrown by piranha::math::is_zero() or by constructing an instance of \p T from zero.
		 */
		T degree(const std::set<std::string> &active_args, const symbol_set &args) const
		{
			if (unlikely(args.size() > 1u || (!args.size() && !math::is_zero(m_value)))) {
				piranha_throw(std::invalid_argument,"invalid symbol set");
			}
			if (!args.size()) {
				return T(0);
			}
			piranha_assert(args.size() == 1u);
			// Look for the only symbol in active args, if we find it return its exponent.
			const bool is_present = std::binary_search(active_args.begin(),active_args.end(),args[0].get_name());
			if (is_present) {
				return m_value;
			} else {
				return T(0);
			}
		}
		/// Partial low degree.
		/**
		 * Equivalent to the partial degree.
		 * 
		 * @param[in] active_args names of the symbols that will be considered in the computation of the partial low degree of the monomial.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return partial low degree of the monomial.
		 * 
		 * @throws unspecified any exception thrown by degree().
		 */
		T ldegree(const std::set<std::string> &active_args, const symbol_set &args) const
		{
			return degree(active_args,args);
		}
		/// Multiply monomials.
		/**
		 * In case of exceptions, this method will leave \p retval in a valid but undefined state.
		 * 
		 * @param[out] retval object that will store the result of the multiplication.
		 * @param[in] other multiplication argument.
		 * @param[in] args reference arguments set.
		 * 
		 * @throws std::invalid_argument if the size of \p args is greater than one or
		 * if the size is zero and one of the two exponents is not zero.
		 * @throws unspecified any exception thrown by:
		 * - copy-assignment of \p T,
		 * - in-place addition of \p T with \p U,
		 * - piranha::math::is_zero().
		 */
		template <typename U>
		void multiply(univariate_monomial &retval, const univariate_monomial<U> &other, const symbol_set &args) const
		{
			if (unlikely(args.size() > 1u || (!args.size() && (!math::is_zero(m_value) || !math::is_zero(other.m_value))))) {
				piranha_throw(std::invalid_argument,"invalid symbol set");
			}
			retval.m_value = m_value;
			retval.m_value += other.m_value;
		}
		/// Get exponent.
		/**
		 * @return const reference to the exponent of the monomial.
		 */
		const T &get_exponent() const
		{
			return m_value;
		}
		/// Set exponent.
		/**
		 * Will assign input argument to the exponent of the monomial.
		 * 
		 * @param[in] x input argument.
		 * 
		 * @throws unspecified any exception thrown by assigning a perfectly-forwarded instance of \p U to \p T.
		 */
		template <typename U>
		void set_exponent(U &&x)
		{
			m_value = std::forward<U>(x);
		}
		/// Print.
		/**
		 * Will print to stream a human-readable representation of the monomial.
		 * 
		 * @param[in] os target stream.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @throws std::invalid_argument if the size of \p args is greater than one or
		 * if the size is zero and the exponent is not zero.
		 * @throws unspecified any exception thrown by:
		 * - piranha::math::is_zero(),
		 * - construction of the exponent type from \p int,
		 * - comparison of exponents,
		 * - printing exponents to stream.
		 */
		void print(std::ostream &os, const symbol_set &args) const
		{
			if (unlikely(args.size() > 1u || (!args.size() && !math::is_zero(m_value)))) {
				piranha_throw(std::invalid_argument,"invalid symbol set");
			}
			const value_type zero(0), one(1);
			if (args.size() && m_value != zero) {
				os << args[0u].get_name();
				if (m_value != one) {
					os << "**" << m_value;
				}
			}
		}
		/// Print in TeX mode.
		/**
		 * Will print to stream a TeX representation of the monomial.
		 * 
		 * @param[in] os target stream.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @throws std::invalid_argument if the size of \p args is greater than one or
		 * if the size is zero and the exponent is not zero.
		 * @throws unspecified any exception thrown by:
		 * - piranha::math::is_zero() and piranha::math::negate(),
		 * - construction of the exponent type,
		 * - comparison of exponents,
		 * - printing exponents to stream.
		 */
		void print_tex(std::ostream &os, const symbol_set &args) const
		{
			if (unlikely(args.size() > 1u || (!args.size() && !math::is_zero(m_value)))) {
				piranha_throw(std::invalid_argument,"invalid symbol set");
			}
			const value_type zero(0), one(1);
			value_type copy(m_value);
			bool sign_change = false;
			if (args.size() && copy != zero) {
				if (copy < zero) {
					math::negate(copy);
					os << "\\frac{1}{";
					sign_change = true;
				}
				os << "{" << args[0u].get_name() << "}";
				if (copy != one) {
					os << "^{" << copy << "}";
				}
				if (sign_change) {
					os << "}";
				}
			}
		}
	private:
		template <typename U>
		static std::size_t compute_hash(const U &value,typename std::enable_if<std::is_integral<U>::value>::type * = nullptr)
		{
			return static_cast<std::size_t>(value);
		}
		template <typename U>
		static std::size_t compute_hash(const U &value,typename std::enable_if<!std::is_integral<U>::value>::type * = nullptr)
		{
			return std::hash<U>()(value);
		}
	private:
		T m_value;
};

}

namespace std
{

/// Specialisation of \p std::hash for piranha::univariate_monomial.
template <typename T>
struct hash<piranha::univariate_monomial<T>>
{
	/// Result type.
	typedef size_t result_type;
	/// Argument type.
	typedef piranha::univariate_monomial<T> argument_type;
	/// Hash operator.
	/**
	 * @param[in] m piranha::univariate_monomial whose hash value will be returned.
	 * 
	 * @return piranha::univariate_monomial::hash().
	 * 
	 * @throws unspecified any exception thrown by piranha::univariate_monomial::hash().
	 */
	result_type operator()(const argument_type &m) const noexcept(true)
	{
		return m.hash();
	}
};

}

#endif
