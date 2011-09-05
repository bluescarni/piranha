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

#include <boost/concept/assert.hpp>

#include "concepts/array_key_value_type.hpp"
#include "concepts/key.hpp"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include "config.hpp"
#include "exceptions.hpp"
#include "math.hpp"
#include "symbol.hpp"

namespace piranha
{

/// Univariate monomial.
/**
 * This class represents a univariate monomial with exponent of type \p T. The exponent is represented
 * by an instance of \p T stored within the object.
 * 
 * This class is a model of the piranha::concept::Key concept.
 * 
 * \section type_requirements Type requirements
 * 
 * \p T must be a model of piranha::concept::ArrayKeyValueType.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the same exception safety guarantee as the type \p T.
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics is equivalent to <tt>T</tt>'s move semantics.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename T>
class univariate_monomial
{
		BOOST_CONCEPT_ASSERT((concept::ArrayKeyValueType<T>));
	public:
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
		/// Constructor from vector of symbols.
		/**
		 * This constructor will initialise the value of the exponent to 0.
		 * 
		 * @param[in] args vector of symbols used for construction.
		 * 
		 * @throws std::invalid_argument if the size of \p args is greater than 1.
		 * @throws unspecified any exception thrown by the construction of an instance of \p T from 0.
		 */
		explicit univariate_monomial(const std::vector<symbol> &args):m_value(0)
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
		~univariate_monomial() piranha_noexcept_spec(true)
		{
			BOOST_CONCEPT_ASSERT((concept::Key<univariate_monomial>));
		}
		/// Defaulted copy assignment operator.
		/**
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the copy assignment operator of \p T.
		 */
		univariate_monomial &operator=(const univariate_monomial &) = default;
		/// Defaulted move assignment operator.
		univariate_monomial &operator=(univariate_monomial &&other) piranha_noexcept_spec(true)
		{
			m_value = std::move(other.m_value);
			return *this;
		}
		/// Hash value.
		/**
		 * @return the hash value of the exponent, calculated via \p std::hash.
		 * 
		 * @throws unspecified any exception thrown by \p std::hash of type \p T.
		 */
		std::size_t hash() const
		{
			return std::hash<T>()(m_value);
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
		 * @param[in] args reference arguments vector.
		 * 
		 * @return \p true if the size of \p args is 1 or if the size of \p args is 0 and piranha::math::is_zero()
		 * on the exponent returns \p true.
		 */
		bool is_compatible(const std::vector<symbol> &args) const piranha_noexcept_spec(true)
		{
			const auto size = args.size();
			return (size == 1u || (!size && math::is_zero(m_value)));
		}
		/// Ignorability test.
		/**
		 * @return \p false (a monomial is never ignorable).
		 */
		bool is_ignorable(const std::vector<symbol> &) const piranha_noexcept_spec(true)
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
		 * @throws std::invalid_argument if the size of \p new_args is greater than 1.
		 * @throws unspecified any exception thrown by the default constructor of piranha::univariate_monomial.
		 */
		univariate_monomial merge_args(const std::vector<symbol> &orig_args, const std::vector<symbol> &new_args) const
		{
			(void)orig_args;
			piranha_assert(orig_args.size() <= 1u);
			piranha_assert(orig_args.size() || math::is_zero(m_value));
			piranha_assert(new_args.size() > orig_args.size());
			piranha_assert(std::is_sorted(orig_args.begin(),orig_args.end()));
			piranha_assert(std::is_sorted(new_args.begin(),new_args.end()));
			if (unlikely(new_args.size() > 1u)) {
				piranha_throw(std::invalid_argument,"excessive number of symbols for univariate monomial");
			}
			// The only valid possibility here is that a monomial with zero args is extended
			// to one arg. Default construction is ok.
			return univariate_monomial();
		}
		/// Check if monomial is unitary.
		/**
		 * A monomial is unitary if piranha::math::is_zero() on its exponent returns \p true.
		 * 
		 * @param[in] args reference vector of piranha::symbol.
		 * 
		 * @return \p true if the monomial is unitary, \p false otherwise.
		 * 
		 * @throws unspecified any exception thrown by piranha::math::is_zero().
		 */
		bool is_unitary(const std::vector<symbol> &args) const
		{
			(void)args;
			piranha_assert(args.size() == 1u || (!args.size() && math::is_zero(m_value)));
			return math::is_zero(m_value);
		}
		/// Degree.
		/**
		 * @param[in] args reference vector of piranha::symbol.
		 * 
		 * @return degree of the monomial.
		 *
		 * @throws unspecified any exception thrown by the copy constructor of \p T.
		 */
		T degree(const std::vector<symbol> &args) const
		{
			(void)args;
			piranha_assert(args.size() == 1u || (!args.size() && math::is_zero(m_value)));
			return m_value;
		}
		/// Multiply monomials.
		/**
		 * @param[out] retval object that will store the result of the multiplication.
		 * @param[in] other multiplication argument.
		 * @param[in] args reference arguments vector.
		 * 
		 * @throws unspecified any exception thrown by copy-assignment of \p T and by in-place addition of \p T with \p U.
		 */
		template <typename U>
		void multiply(univariate_monomial &retval, const univariate_monomial<U> &other, const std::vector<symbol> &args) const
		{
			(void)args;
			piranha_assert(args.size() == 1u || (!args.size() && math::is_zero(m_value)));
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
		/// Stream operator for piranha::univariate_monomial.
		/**
		 * Will output to stream a human-readable representation of the monomial.
		 * 
		 * @param[in] os target stream.
		 * @param[in] m input piranha::univariate_monomial.
		 * 
		 * @return reference to \p os.
		 * 
		 * @throws unspecified any exception thrown by streaming to \p os an instance of the type
		 * of the exponent of the monomial.
		 */
		friend std::ostream &operator<<(std::ostream &os, const univariate_monomial &m)
		{
			os << '[' << m.m_value << ']';
			return os;
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
	result_type operator()(const argument_type &m) const
	{
		return m.hash();
	}
};

}

#endif
