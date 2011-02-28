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

#include <initializer_list>
#include <unordered_set>

#include "array_key.hpp"

namespace piranha
{

/// Monomial class.
/**
 * This class extends piranha::array_key to define a series key type suitable as monomial in polynomial terms.
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
		monomial(std::initializer_list<T> list):base(list) {}
		/// Generic constructor.
		/**
		 * Will forward its arguments to one of the constructors of piranha::array_key.
		 * 
		 * @param[in] params arguments used for construction.
		 * 
		 * @throws unspecified any exception thrown by the invoked piranha::array_key's constructor.
		 */
		template <typename... Args>
		explicit monomial(Args && ... params):base(std::forward<Args>(params)...) {}
		/// Defaulted destructor.
		~monomial() = default;
		/// Generic assignment operator.
		/**
		 * Will forward the assignment operator to the base class.
		 * 
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the invoked piranha::array_key's assignment operator.
		 */
		template <typename U>
		monomial &operator=(U &&other)
		{
			base::operator=(std::forward<U>(other));
			return *this;
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
