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

#ifndef PIRANHA_NUMERICAL_COEFFICIENT_HPP
#define PIRANHA_NUMERICAL_COEFFICIENT_HPP

#include <boost/utility/enable_if.hpp>
#include <type_traits>

#include "config.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Numerical coefficient.
/**
 * This class is intended as a thin wrapper around C++ arithmetic types and types behaving like them. Construction,
 * assignment, arithmetic operations, etc. will be forwarded to an instance of type \p T stored as a member of the class.
 * A set of additional methods is provided, so that this class can be used as coefficient type in series.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename T>
class numerical_coefficient
{
		// Type trait to detect numerical coefficient class.
		template <typename U>
		struct is_numerical_coefficient: std::false_type {};
		template <typename U>
		struct is_numerical_coefficient<numerical_coefficient<U>>: std::true_type {};
		template <typename U>
		void assign(U &&other, typename boost::enable_if_c<
			is_numerical_coefficient<typename strip_cv_ref<U>::type>::value &&
			std::is_rvalue_reference<U &&>::value>::type * = piranha_nullptr)
		{
			m_value = std::move(other.m_value);
		}
		template <typename U>
		void assign(const U &other, typename boost::enable_if<
			is_numerical_coefficient<typename strip_cv_ref<U>::type>>::type * = piranha_nullptr)
		{
			m_value = other.m_value;
		}
		template <class U>
		void assign(U &&other, typename boost::disable_if<is_numerical_coefficient<typename strip_cv_ref<U>::type>>::type * = piranha_nullptr)
		{
			m_value = std::forward<U>(other);
		}
	public:
		/// Underlying numerical type.
		typedef T type;
		/// Default constructor.
		/**
		 * Will explicitly call numerical_coefficient::type's default constructor.
		 * 
		 * @throws unspecified any exception thrown by numerical_coefficient::type's default constructor.
		 */
		numerical_coefficient():m_value() {}
		/// Default copy constructor.
		/**
		 * @throws unspecified any exception thrown by numerical_coefficient::type's copy constructor.
		 */
		numerical_coefficient(const numerical_coefficient &) = default;
		/// Default move constructor.
		/**
		 * @throws unspecified any exception thrown by numerical_coefficient::type's move constructor.
		 */
		numerical_coefficient(numerical_coefficient &&) = default;
		/// Generic constructor.
		/**
		 * Will try to construct the underlying numerical type with the forwarded arguments.
		 * 
		 * @param[in] params arguments used for the construction of the underlying numerical type.
		 * 
		 * @throws unspecified any exception thrown by numerical_coefficient::type's constructor.
		 */
		template <typename... Args>
		explicit numerical_coefficient(Args && ... params):m_value(std::forward<Args>(params)...) {}
		/// Default destructor.
		/**
		 * @throws unspecified any exception thrown by numerical_coefficient::type's destructor.
		 */
		~numerical_coefficient() = default;
		/// Generic assignment operator.
		/**
		 * If \p other is an instance of numerical_coefficient, its numerical value will be assigned to the numerical value of \p this. Otherwise, \p other
		 * will be assigned directly to the numerical value of \p this.
		 * 
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by numerical_coefficient::type's copy/move assignment operators.
		 */
		template <typename U>
		numerical_coefficient &operator=(U &&other)
		{
			assign(std::forward<U>(other));
			return *this;
		}
	private:
		type m_value;
};

}

#endif
