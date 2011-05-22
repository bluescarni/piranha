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

#include <boost/concept/assert.hpp>
#include <iostream>
#include <type_traits>

#include "concepts/container_element.hpp"
#include "config.hpp"
#include "echelon_descriptor.hpp"
#include "math.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Numerical coefficient.
/**
 * This class is intended as a thin wrapper around C++ arithmetic types and types behaving like them. Construction,
 * assignment, arithmetic operations, etc. will be forwarded to an instance of type \p T stored as a member of the class.
 * A set of additional methods is provided, so that this class can be used as coefficient type in series.
 * This class is a model of the piranha::concept::Cf concept.
 * 
 * \section type_requirements Type requirements
 * 
 * \p T must be a model of piranha::concept::ContainerElement.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the same exception safety guarantee as the underlying type \p T.
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics is equivalent to <tt>T</tt>'s move semantics.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename T>
class numerical_coefficient
{
		BOOST_CONCEPT_ASSERT((concept::ContainerElement<T>));
	public:
		/// Underlying numerical type.
		typedef T type;
	private:
		// Make friend with all instances of numerical_coefficient.
		template <typename U>
		friend class numerical_coefficient;
		// Type trait to detect numerical coefficient class.
		template <typename U>
		struct is_numerical_coefficient: std::false_type {};
		template <typename U>
		struct is_numerical_coefficient<numerical_coefficient<U>>: std::true_type {};
		// Generic constructor from non numerical coefficient type.
		template <typename U, typename std::enable_if<!is_numerical_coefficient<typename strip_cv_ref<U>::type>::value>::type*& = enabler>
		explicit numerical_coefficient(U &&x):
			m_value(std::forward<U>(x)) {}
		template <typename U>
		void assign(U &&other, typename std::enable_if<
			is_numerical_coefficient<typename strip_cv_ref<U>::type>::value &&
			std::is_rvalue_reference<U &&>::value>::type * = piranha_nullptr)
		{
			m_value = std::move(other.m_value);
		}
		template <typename U>
		void assign(const U &other, typename std::enable_if<
			is_numerical_coefficient<typename strip_cv_ref<U>::type>::value>::type * = piranha_nullptr)
		{
			m_value = other.m_value;
		}
		template <typename U>
		void assign(U &&other, typename std::enable_if<!is_numerical_coefficient<typename strip_cv_ref<U>::type>::value>::type * = piranha_nullptr)
		{
			m_value = std::forward<U>(other);
		}
		template <bool Sign, typename U>
		void dispatch_add(U &&other, typename std::enable_if<
			is_numerical_coefficient<typename strip_cv_ref<U>::type>::value &&
			std::is_rvalue_reference<U &&>::value>::type * = piranha_nullptr)
		{
			// NOTE: here it is enough the rvalue ref check. If it is a *const* rvalue, the move
			// will preserve constness and will be equivalent to method below.
			if (Sign) {
				m_value += std::move(other.m_value);
			} else {
				m_value -= std::move(other.m_value);
			}
		}
		template <bool Sign, typename U>
		void dispatch_add(const U &other, typename std::enable_if<
			is_numerical_coefficient<typename strip_cv_ref<U>::type>::value>::type * = piranha_nullptr)
		{
			if (Sign) {
				m_value += other.m_value;
			} else {
				m_value -= other.m_value;
			}
		}
		template <bool Sign, typename U>
		void dispatch_add(U &&other, typename std::enable_if<!is_numerical_coefficient<typename strip_cv_ref<U>::type>::value>::type * = piranha_nullptr)
		{
			if (Sign) {
				m_value += std::forward<U>(other);
			} else {
				m_value -= std::forward<U>(other);
			}
		}
		template <typename U>
		numerical_coefficient dispatch_multiply(U &&other, typename std::enable_if<
			is_numerical_coefficient<typename strip_cv_ref<U>::type>::value &&
			std::is_rvalue_reference<U &&>::value>::type * = piranha_nullptr)
		{
			return numerical_coefficient{m_value * std::move(other.m_value)};
		}
		template <typename U>
		numerical_coefficient dispatch_multiply(const U &other, typename std::enable_if<
			is_numerical_coefficient<typename strip_cv_ref<U>::type>::value>::type * = piranha_nullptr)
		{
			return numerical_coefficient{m_value * other.m_value};
		}
		template <typename U>
		numerical_coefficient dispatch_multiply(U &&other, typename std::enable_if<!is_numerical_coefficient<typename strip_cv_ref<U>::type>::value>::type * = piranha_nullptr)
		{
			return numerical_coefficient{m_value * std::forward<U>(other)};
		}
		template <typename U>
		void dispatch_multiply_by(U &&other, typename std::enable_if<
			is_numerical_coefficient<typename strip_cv_ref<U>::type>::value &&
			std::is_rvalue_reference<U &&>::value>::type * = piranha_nullptr)
		{
			m_value *= std::move(other.m_value);
		}
		template <typename U>
		void dispatch_multiply_by(const U &other, typename std::enable_if<
			is_numerical_coefficient<typename strip_cv_ref<U>::type>::value>::type * = piranha_nullptr)
		{
			m_value *= other.m_value;
		}
		template <typename U>
		void dispatch_multiply_by(U &&other, typename std::enable_if<!is_numerical_coefficient<typename strip_cv_ref<U>::type>::value>::type * = piranha_nullptr)
		{
			m_value *= std::forward<U>(other);
		}
		template <typename U>
		static type forward_for_construction(U &&x,
			typename std::enable_if<!is_numerical_coefficient<typename strip_cv_ref<U>::type>::value>::type * = piranha_nullptr)
		{
			return type(std::forward<U>(x));
		}
		template <typename U>
		static type forward_for_construction(U &&x,
			typename std::enable_if<
			is_numerical_coefficient<typename strip_cv_ref<U>::type>::value &&
			is_nonconst_rvalue_ref<U &&>::value
			>::type * = piranha_nullptr)
		{
			return type(std::move(x.m_value));
		}
		template <typename U>
		static type forward_for_construction(U &&x,
			typename std::enable_if<
			is_numerical_coefficient<typename strip_cv_ref<U>::type>::value &&
			!is_nonconst_rvalue_ref<U &&>::value
			>::type * = piranha_nullptr)
		{
			return type(x.m_value);
		}
		// Equality operator with numerical coefficients.
		template <typename U>
		bool dispatch_equality(const U &nc,
			typename std::enable_if<is_numerical_coefficient<typename strip_cv_ref<U>::type>::value>::type * = piranha_nullptr) const
		{
			return m_value == nc.m_value;
		}
		// Equality operator with arbitrary type.
		template <typename U>
		bool dispatch_equality(const U &x,
			typename std::enable_if<!is_numerical_coefficient<typename strip_cv_ref<U>::type>::value>::type * = piranha_nullptr) const
		{
			return m_value == x;
		}
	public:
		/// Default constructor.
		/**
		 * Will explicitly call numerical_coefficient::type's default constructor.
		 * 
		 * @throws unspecified any exception thrown by numerical_coefficient::type's default constructor.
		 */
		numerical_coefficient():m_value() {}
		/// Defaulted copy constructor.
		/**
		 * @throws unspecified any exception thrown by numerical_coefficient::type's copy constructor.
		 */
		numerical_coefficient(const numerical_coefficient &) = default;
		/// Defaulted move constructor.
		numerical_coefficient(numerical_coefficient &&) = default;
		/// Generic constructor.
		/**
		 * Generic constructor for use in series.
		 * 
		 * The construction algorithm proceeds as follows:
		 * 
		 * - if \p x is an instance of piranha::numerical_coefficient:
		 *   - its numerical value is used to construct the numerical value of \p this;
		 * - else:
		 *   - \p x is used directly to construct the numerical value of \p this.
		 * 
		 * @param[in] x construction argument.
		 * 
		 * @throws unspecified any exception thrown by the construction of the internal numerical value from \p x or its
		 * internal numerical value.
		 */
		template <typename U, typename Term>
		explicit numerical_coefficient(U &&x, const echelon_descriptor<Term> &):m_value(forward_for_construction(std::forward<U>(x)))
		{}
		/// Defaulted destructor.
		~numerical_coefficient() = default;
		/// Defaulted copy assignment operator.
		numerical_coefficient &operator=(const numerical_coefficient &) = default;
		/// Move assignment operator.
		/**
		 * @param[in] other object to move from.
		 * 
		 * @return reference to \p this.
		 */
		numerical_coefficient &operator=(numerical_coefficient &&other) piranha_noexcept_spec(true)
		{
			if (likely(this != &other)) {
				m_value = std::move(other.m_value);
			}
			return *this;
		}
		/// Generic assignment operator.
		/**
		 * If \p other is an instance of numerical_coefficient, its numerical value will be assigned to the numerical value of \p this. Otherwise, \p other
		 * will be assigned directly to the numerical value of \p this.
		 * 
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by numerical_coefficient::type's copy assignment operators.
		 */
		template <typename U>
		numerical_coefficient &operator=(U &&other)
		{
			assign(std::forward<U>(other));
			return *this;
		}
		/// Value getter.
		/**
		 * @return const reference to the internal type.
		 */
		const type &get_value() const
		{
			return m_value;
		}
		/// Ignorability test.
		/**
		 * Note that this method is not allowed to throw: any exception resulting from invoking piranha::math::is_zero()
		 * will result in the termination of the program.
		 * 
		 * @return output of piranha::math::is_zero() on the internal numerical value.
		 */
		template <typename Term>
		bool is_ignorable(const echelon_descriptor<Term> &) const piranha_noexcept_spec(true)
		{
			return math::is_zero(m_value);
		}
		/// Compatibility test.
		/**
		 * A numerical coefficient is always compatible for insertion as in any series.
		 * 
		 * @return \p true.
		 */
		template <typename Term>
		bool is_compatible(const echelon_descriptor<Term> &) const piranha_noexcept_spec(true)
		{
			return true;
		}
		/// Generic in-place addition.
		/**
		 * If \p x is an instance of piranha::numerical_coefficient, then <tt>x</tt>'s numerical value will be added in-place
		 * to the numerical value of \p this. Otherwise, \p x will be directly added in-place to the numerical value of \p this.
		 * 
		 * @param[in] x argument of the addition.
		 * 
		 * @throws unspecified any exception thrown by numerical_coefficient::type's in-place addition operator.
		 */
		template <typename U, typename Term>
		void add(U &&x, const echelon_descriptor<Term> &)
		{
			dispatch_add<true>(std::forward<U>(x));
		}
		/// Generic in-place subtraction.
		/**
		 * Semantically equivalent to add().
		 * 
		 * @param[in] x argument of the subtraction.
		 * 
		 * @throws unspecified any exception thrown by numerical_coefficient::type's in-place subtraction operator.
		 */
		template <typename U, typename Term>
		void subtract(U &&x, const echelon_descriptor<Term> &)
		{
			dispatch_add<false>(std::forward<U>(x));
		}
		/// Generic multiplication.
		/**
		 * If \p x is an instance of piranha::numerical_coefficient, then the numerical value of \p this will be multiplied by
		 * <tt>x</tt>'s numerical value, and the result will be used to create the return value.
		 * 
		 * Otherwise, the numerical value of \p this will be directly multiplied by \p x and the result will be used to create
		 * the return value.
		 * 
		 * @param[in] x argument of the multiplication.
		 * 
		 * @return the result of multiplying \p this by \p x.
		 * 
		 * @throws unspecified any exception thrown by numerical_coefficient::type's multiplication operator.
		 */
		template <typename U, typename Term>
		numerical_coefficient multiply(U &&x, const echelon_descriptor<Term> &)
		{
			return dispatch_multiply(std::forward<U>(x));
		}
		/// Generic in-place multiplication.
		/**
		 * If \p x is an instance of piranha::numerical_coefficient, then the numerical value of \p this will be multiplied in-place by
		 * <tt>x</tt>'s numerical value. Otherwise, the numerical value of \p this will be directly multiplied in-place by \p x .
		 * 
		 * @param[in] x argument of the multiplication.
		 * 
		 * @throws unspecified any exception thrown by numerical_coefficient::type's in-place multiplication operator.
		 */
		template <typename U, typename Term>
		void multiply_by(U &&x, const echelon_descriptor<Term> &)
		{
			dispatch_multiply_by(std::forward<U>(x));
		}
		/// In-place negation.
		/**
		 * Will invoke piranha::math::negate() on the internal numerical value.
		 * 
		 * @throws unspecified any exception thrown by piranha::math::negate().
		 */
		template <typename Term>
		void negate(const echelon_descriptor<Term> &)
		{
			math::negate(m_value);
		}
		/// Merge new arguments.
		/**
		 * No special actions are required for arguments merging in piranha::numerical_coefficient.
		 * 
		 * @return copy of \p this.
		 */
		template <typename Term>
		numerical_coefficient merge_args(const echelon_descriptor<Term> &, const echelon_descriptor<Term> &) const
		{
			return *this;
		}
		/// Equality operator.
		/**
		 * The equality operator works as follow:
		 * 
		 * - if \p x is an instance of piranha::numerical_coefficient, the internal values
		 *   are compared;
		 * - else, the internal numerical value is compared to \p x.
		 * 
		 * @param[in] x argument for equality test.
		 * 
		 * @return \p true if \p this is equal to \p x, \p false otherwise.
		 * 
		 * @throws unspecified any exception thrown by the equality operator of the internal
		 * numerical type.
		 */
		template <typename U, typename Term>
		bool is_equal_to(const U &x, const echelon_descriptor<Term> &) const
		{
			return dispatch_equality(x);
		}
		/// Overload of stream operator for piranha::numerical_coefficient.
		/**
		 * Will direct to stream the internal value of the numerical coefficient.
		 * 
		 * @param[in,out] os target stream.
		 * @param[in] nc piranha::numerical_coefficient that will be directed to \p os.
		 * 
		 * @return reference to \p os.
		 * 
		 * @throws unspecified any exception thrown by directing to the stream the internal value
		 * of the numerical coefficient.
		 */
		friend inline std::ostream &operator<<(std::ostream &os, const numerical_coefficient<T> &nc)
		{
			os << nc.m_value;
			return os;
		}
	private:
		type m_value;
};

/// Specialisation of piranha::binary_op_promotion_rule for piranha::numerical_coefficient operands.
/**
 * Equivalent to piranha::binary_op_promotion_rule of the internal numerical types of the piranha::numerical_coefficient operands.
 */
template <typename T1, typename T2>
class binary_op_promotion_rule<numerical_coefficient<T1>,numerical_coefficient<T2>,void>
{
	public:
		/// Type-trait's value.
		static const bool value = binary_op_promotion_rule<T1,T2>::value;
};

}

#endif
