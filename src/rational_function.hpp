/* Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)

This file is part of the Piranha library.

The Piranha library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The Piranha library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the Piranha library.  If not,
see https://www.gnu.org/licenses/. */

#ifndef PIRANHA_RATIONAL_FUNCTION_HPP
#define PIRANHA_RATIONAL_FUNCTION_HPP

#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

#include "config.hpp"
#include "math.hpp"
#include "mp_integer.hpp"
#include "mp_rational.hpp"
#include "polynomial.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Rational function.
template <typename Key>
class rational_function
{
		// Shortcut for supported integral type.
		template <typename T>
		using is_integral = std::integral_constant<bool,
			detail::is_mp_integer<T>::value || std::is_integral<T>::value>;
		PIRANHA_TT_CHECK(detail::is_polynomial_key,Key);
		static_assert(is_integral<typename Key::value_type>::value,"The exponent type must be an integral type.");
		// Shortcut from C++14.
		template <typename T>
		using decay_t = typename std::decay<T>::type;
	public:
		/// The polynomial type of numerator and denominator.
		using p_type = polynomial<integer,Key>;
		/// The counterpart of rational_function::p_type with rational coefficients.
		using q_type = polynomial<rational,Key>;
	private:
		// Generic ctors utils.
		// Ctor from p_type, integrals and string.
		template <typename T, typename std::enable_if<std::is_same<decay_t<T>,p_type>::value || is_integral<decay_t<T>>::value ||
			std::is_same<decay_t<T>,std::string>::value ||
			std::is_same<decay_t<T>,char *>::value ||
			std::is_same<decay_t<T>,const char *>::value,int>::type = 0>
		void dispatch_unary_ctor(T &&x)
		{
			m_num = std::forward<T>(x);
			m_den = p_type{1};
			// Check for negative expos in the num if cting from a polynomial.
			if (std::is_same<decay_t<T>,p_type>::value) {
				detail::poly_expo_checker(m_num);
			}
		}
		// Ctor from rationals.
		template <typename T, typename std::enable_if<detail::is_mp_rational<decay_t<T>>::value,int>::type = 0>
		void dispatch_unary_ctor(const T &q)
		{
			m_num = q.num();
			m_den = q.den();
		}
		// Ctor from q_type.
		template <typename T, typename std::enable_if<std::is_same<decay_t<T>,q_type>::value,int>::type = 0>
		void dispatch_unary_ctor(const T &q)
		{
			// Compute the least common multiplier.
			integer lcm{1};
			// The GCD.
			integer g;
			for (const auto &t: q._container()) {
				math::gcd3(g,lcm,t.m_cf.den());
				math::mul3(lcm,lcm,t.m_cf.den());
				integer::_divexact(lcm,lcm,g);
			}
			// All these computations involve only positive numbers,
			// the GCD must always be positive.
			piranha_assert(lcm.sign() == 1);
			// Construct the numerator.
			// Rehash according to q.
			m_num._container().rehash(q._container().bucket_count());
			m_num.set_symbol_set(q.get_symbol_set());
			for (const auto &t: q._container()) {
				using term_type = typename p_type::term_type;
				// NOTE: possibility of unique insertion here.
				m_num.insert(term_type{lcm / t.m_cf.den() * t.m_cf.num(),t.m_key});
			}
			// Construct the denominator.
			m_den = std::move(lcm);
			// Check the numerator's exponents.
			detail::poly_expo_checker(m_num);
		}
		// Enabler for generic unary ctor.
		template <typename T, typename U>
		using unary_ctor_enabler = typename std::enable_if<
			// NOTE: this should disable the ctor if T derives from rational_function,
			// as the dispatcher does not support that.
			detail::true_tt<decltype(std::declval<U &>().dispatch_unary_ctor(std::declval<const decay_t<T> &>()))>::value
		,int>::type;
	public:
		/// Default constructor.
		/**
		 * The numerator will be set to zero, the denominator to 1.
		 * 
		 * @throws unspecified any exception thrown by the constructor of rational_function::p_type from \p int.
		 */
		rational_function():m_num(),m_den(1) {}
		/// Defaulted copy constructor.
		rational_function(const rational_function &) = default;
		/// Defaulted move constructor.
		rational_function(rational_function &&) = default;
		/// Generic unary constructor.
		/**
		 * 
		 * @throws std::invalid_argument if, when constructing from rational_function::p_type or rational_function::q_type, a negative exponent is
		 * encountered.
		 * @throws unspecified any exception thrown by:
		 * - the constructors and the assignment operators of rational_function::p_type,
		 * - the public interface of piranha::series and piranha::hash_set.
		 */
		template <typename T, typename U = rational_function, unary_ctor_enabler<T,U> = 0>
		explicit rational_function(T &&x)
		{
			dispatch_unary_ctor(std::forward<T>(x));
		}
		/// Copy-assignment operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @return a reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the copy constructor.
		 */
		rational_function &operator=(const rational_function &other)
		{
			if (likely(&other != this)) {
				*this = rational(other);
			}
			return *this;
		}
		/// Defaulted move-assignment operator.
		rational_function &operator=(rational_function &&) = default;
		/// Defaulted destructor.
		~rational_function() = default;
		/// Stream operator.
		/**
		 * Will stream to \p os a human-readable representation of \p r.
		 * 
		 * @param[in,out] os target stream.
		 * @param[in] r the piranha::rational_function to be streamed.
		 * 
		 * @return a reference to \p os.
		 * 
		 * @throws unspecified any exception thrown by the stream opertor of rational_function::p_type.
		 */
		friend std::ostream &operator<<(std::ostream &os, const rational_function &r)
		{
			if (math::is_zero(r.m_num)) {
				// Special case for zero.
				os << "0";
			} else if (math::is_unitary(r.m_den)) {
				// If the denominator is 1, print just the numerator.
				os << r.m_num;
			} else {
				// First let's deal with the numerator.
				if (r.m_num.size() == 1u) {
					// If on top there's only 1 term, don't print brackets.
					os  << r.m_num;
				} else {
					os << '(' << r.m_num << ')';
				}
				os << '/';
				if (r.m_den.is_single_coefficient()) {
					// If the denominator is a single coefficient, don't print brackets.
					os << r.m_den;
				} else {
					os << '(' << r.m_den << ')';
				}
			}
			return os;
		}
	private:
		p_type	m_num;
		p_type	m_den;
};

}

#endif
