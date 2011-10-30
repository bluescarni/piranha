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

#ifndef PIRANHA_MONOMIAL_HPP
#define PIRANHA_MONOMIAL_HPP

#include <algorithm>
#include <boost/concept/assert.hpp>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <unordered_set>

#include "array_key.hpp"
#include "concepts/key.hpp"
#include "config.hpp"
#include "math.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Monomial class.
/**
 * This class extends piranha::array_key to define a series key type suitable as monomial in polynomial terms.
 * This class is a model of the piranha::concept::Key concept.
 * 
 * \section type_requirements Type requirements
 * 
 * \p T must be suitable for use in piranha::array_key.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the same exception safety guarantee as piranha::array_key.
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics is equivalent to piranha::array_key's move semantics.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename T>
class monomial: public array_key<T,monomial<T>>
{
		typedef array_key<T,monomial<T>> base;
	public:
		/// Defaulted default constructor.
		monomial() = default;
		/// Defaulted copy constructor.
		monomial(const monomial &) = default;
		/// Defaulted move constructor.
		monomial(monomial &&) = default;
		/// Constructor from initializer list.
		/**
		 * @param[in] list initializer list.
		 * 
		 * @see piranha::array_key's constructor from initializer list.
		 */
		template <typename U>
		explicit monomial(std::initializer_list<U> list):base(list) {}
		/// Forwarding constructor.
		/**
		 * Will perfectly forward the arguments to a corresponding constructor in piranha::array_key.
		 * This constructor is enabled only if the number of variadic arguments is nonzero or
		 * if \p U is not of the same type as \p this.
		 * 
		 * @param[in] arg1 first argument to be forwarded to the base constructor.
		 * @param[in] params other arguments to be forwarded to the base constructor.
		 * 
		 * @throws unspecified any exception thrown by the invoked constructor from piranha::array_key.
		 */
		template <typename U, typename... Args, typename std::enable_if<sizeof...(Args) || !std::is_same<monomial,typename std::decay<U>::type>::value>::type*& = enabler>
		explicit monomial(U &&arg1, Args && ... params):
			base(std::forward<U>(arg1),std::forward<Args>(params)...) {}
		/// Trivial destructor.
		~monomial() piranha_noexcept_spec(true)
		{
			BOOST_CONCEPT_ASSERT((concept::Key<monomial>));
		}
		/// Defaulted copy assignment operator.
		monomial &operator=(const monomial &) = default;
		// NOTE: this can be defaulted in GCC >= 4.6.
		/// Move assignment operator.
		/**
		 * @param[in] other object to move from.
		 * 
		 * @return reference to \p this.
		 */
		monomial &operator=(monomial &&other) piranha_noexcept_spec(true)
		{
			base::operator=(std::move(other));
			return *this;
		}
		/// Compatibility check
		/**
		 * A monomial and a set of arguments are compatible if their sizes coincide.
		 * 
		 * @param[in] args reference arguments set.
		 * 
		 * @return <tt>this->size() == args.size()</tt>.
		 */
		bool is_compatible(const symbol_set &args) const piranha_noexcept_spec(true)
		{
			return (this->size() == args.size());
		}
		/// Ignorability check.
		/**
		 * A monomial is never ignorable by definition.
		 * 
		 * @param[in] args reference arguments set.
		 * 
		 * @return \p false.
		 */
		bool is_ignorable(const symbol_set &args) const piranha_noexcept_spec(true)
		{
			(void)args;
			piranha_assert(is_compatible(args));
			return false;
		}
		/// Merge arguments.
		/**
		 * Will forward the call to piranha::array_key::base_merge_args().
		 * 
		 * @param[in] orig_args original arguments set.
		 * @param[in] new_args new arguments set.
		 * 
		 * @return piranha::monomial with the new arguments merged in.
		 * 
		 * @throws unspecified any exception thrown by piranha::array_key::base_merge_args().
		 */
		monomial merge_args(const symbol_set &orig_args, const symbol_set &new_args) const
		{
			monomial retval;
			static_cast<base &>(retval) = this->base_merge_args(orig_args,new_args);
			return retval;
		}
		/// Check if monomial is unitary.
		/**
		 * A monomial is unitary if, for all its elements, piranha::math::is_zero() returns \p true.
		 * 
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return \p true if the monomial is unitary, \p false otherwise.
		 * 
		 * @throws std::invalid_argument if the sizes of \p args and \p this differ.
		 * @throws unspecified any exception thrown by piranha::math::is_zero().
		 */
		bool is_unitary(const symbol_set &args) const
		{
			typedef typename base::value_type value_type;
			if(unlikely(args.size() != this->size())) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			return std::all_of(this->m_container.begin(),this->m_container.end(),
				[](const value_type &element) {return math::is_zero(element);});
		}
		/// Degree.
		/**
		 * Degree of the monomial.
		 * 
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return the summation of all the exponents of the monomial, or <tt>value_type(0)</tt> if the size
		 * of the monomial is zero.
		 * 
		 * @throws std::invalid_argument if the sizes of \p args and \p this differ.
		 * @throws unspecified any exception thrown by the constructor and the addition and assignment operators of \p value_type.
		 */
		typename array_key<T,monomial<T>>::value_type degree(const symbol_set &args) const
		{
			typedef typename base::value_type value_type;
			if(unlikely(args.size() != this->size())) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			return std::accumulate(this->m_container.begin(),this->m_container.end(),value_type(0));
		}
		/// Low degree.
		/**
		 * Analogous to the degree.
		 * 
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return the output of degree().
		 * 
		 * @throws unspecified any exception thrown by degree().
		 */
		typename array_key<T,monomial<T>>::value_type ldegree(const symbol_set &args) const
		{
			return degree(args);
		}
		/// Partial degree.
		/**
		 * Partial degree of the monomial: only the symbols in \p active_args are considered during the computation
		 * of the degree. Symbols in \p active_args not appearing in \p args are not considered.
		 * 
		 * @param[in] active_args symbols that will be considered in the computation of the partial degree of the monomial.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return the summation of all the exponents of the monomial corresponding to the symbols in
		 * \p active_args, or <tt>value_type(0)</tt> if no symbols in \p active_args appear in \p args.
		 * 
		 * @throws std::invalid_argument if the sizes of \p args and \p this differ.
		 * @throws unspecified any exception thrown by the constructor and the addition and assignment operators of \p value_type.
		 */
		typename array_key<T,monomial<T>>::value_type degree(const symbol_set &active_args, const symbol_set &args) const
		{
			typedef typename base::value_type value_type;
			if(unlikely(args.size() != this->size())) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			value_type retval(0);
			auto it1 = args.begin(), it2 = active_args.begin();
			for (typename base::size_type i = 0u; i < this->size(); ++i, ++it1) {
				// Move forward the it2 iterator until it does not preceed the iterator in args,
				// or we run out of symbols.
				while (it2 != active_args.end() && *it2 < *it1) {
					++it2;
				}
				if (it2 == active_args.end()) {
					break;
				} else if (*it2 == *it1) {
					retval += (*this)[i];
				}
			}
			return retval;
		}
		/// Partial low degree.
		/**
		 * Analogous to the partial degree.
		 * 
		 * @param[in] active_args symbols that will be considered in the computation of the partial low degree of the monomial.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return the output of degree().
		 * 
		 * @throws unspecified any exception thrown by degree().
		 */
		typename array_key<T,monomial<T>>::value_type ldegree(const symbol_set &active_args, const symbol_set &args) const
		{
			return degree(active_args,args);
		}
		/// Multiply monomial.
		/**
		 * Multiplies \p this by \p other and stores the result in \p retval. The exception safety
		 * guarantee is the same as for piranha::array_key::add().
		 * 
		 * @param[out] retval return value.
		 * @param[in] other argument of multiplication.
		 * @param[in] args reference set of arguments.
		 * 
		 * @throws std::invalid_argument if the sizes of \p args and \p this differ.
		 * @throws unspecified any exception thrown by piranha::array_key::add().
		 */
		template <typename U>
		void multiply(monomial &retval, const monomial<U> &other, const symbol_set &args) const
		{
			if(unlikely(other.size() != args.size())) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			this->add(retval,other);
		}
};

}

namespace std
{

/// Specialisation of \p std::hash for piranha::monomial.
/**
 * Functionally equivalent to the \p std::hash specialisation for piranha::array_key.
 */
template <typename T>
struct hash<piranha::monomial<T>>: public hash<piranha::array_key<T,piranha::monomial<T>>> {};

}

#endif
