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
#include "exceptions.hpp"
#include "math.hpp"
#include "mp_integer.hpp"
#include "mp_rational.hpp"
#include "polynomial.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Rational function.
/**
 * This class represents rational functions, that is, mathematical objects of the form
 * \f[
 * \frac{f\left(x_0,x_1,\ldots\right)}{g\left(x_0,x_1,\ldots\right)},
 * \f]
 * where \f$f\left(x_0,x_1,\ldots\right)\f$ and \f$g\left(x_0,x_1,\ldots\right)\f$ are polynomials in the variables \f$x_0,x_1,\ldots\f$ over \f$\mathbb{Z}\f$.
 * The monomial representation is determined by the \p Key template parameter. Only monomial types with integral exponents are allowed; signed exponent
 * types are allowed, but if a negative exponent is ever generated or encountered while operating on a rational function an error will be produced.
 *
 * Internally, a piranha::rational_function consists of a numerator and a denominator represented via piranha::polynomial with piranha::integer coefficients.
 * Rational functions are always kept in a canonical form defined by the following properties:
 * - numerator and denominator are coprime,
 * - zero is always represented as <tt>0 / 1</tt>,
 * - the coefficient of the leading term of the denominator is always positive.
 *
 * ## Type requirements ##
 *
 * \p Key must be usable as second template parameter for piranha::polynomial, and the exponent type must be a C++ integral type or piranha::integer.
 *
 * ## Exception safety guarantee ##
 *
 * Unless noted otherwise, this class provides the strong exception safety guarantee.
 *
 * ## Move semantics ##
 *
 * Move operations will leave object of this class in a stat which is destructible and assignable.
 *
 * ## Serialization ##
 *
 * This class supports serialization.
 */
// TODO: what type traits does it satisfy?
// TODO: serialization.
template <typename Key>
class rational_function
{
		// Shortcut for supported integral type.
		template <typename T>
		using is_integral = std::integral_constant<bool,
			std::is_same<integer,T>::value || std::is_integral<T>::value>;
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
		// Canonicalisation.
		static std::pair<p_type,p_type> canonicalise_impl(const p_type &n, const p_type &d)
		{
			// First let's check for negative exponents.
			detail::poly_expo_checker(n);
			detail::poly_expo_checker(d);
			// Handle a zero divisor.
			if (unlikely(math::is_zero(d))) {
				piranha_throw(zero_division_error,"null denominator in rational function");
			}
			// If the numerator is null, return {0,1}.
			if (math::is_zero(n)) {
				return std::make_pair(p_type{},p_type{1});
			}
			// NOTE: maybe these checks should go directly into the poly GCD routine. Keep it
			// in mind for the future.
			// If the denominator is unitary, just return the input values.
			if (math::is_unitary(d)) {
				return std::make_pair(n,d);
			}
			// Handle single-coefficient polys.
			if (n.is_single_coefficient() && d.is_single_coefficient()) {
				piranha_assert(n.size() == 1u && d.size() == 1u);
				rational tmp{n._container().begin()->m_cf,d._container().begin()->m_cf};
				return std::make_pair(p_type{tmp.num()},p_type{tmp.den()});
			}
			// Compute the GCD and create the return values.
			auto g = math::gcd(n,d);
			auto retval = std::make_pair(n / g, d / g);
			// Check if we need to adjust the sign of the leading term
			// of the denominator.
			if (detail::poly_lterm(retval.second)->m_cf.sign() < 0) {
				math::negate(retval.first);
				math::negate(retval.second);
			}
			return retval;
		}
		// Utility function to convert a q_type to a pair (p_type,integer), representing the numerator
		// and denominator of a rational function.
		static std::pair<p_type,integer> q_to_p_type(const q_type &q)
		{
			// Init the numerator.
			p_type ret_p;
			ret_p._container().rehash(q._container().bucket_count());
			ret_p.set_symbol_set(q.get_symbol_set());
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
			// Fill in the numerator.
			for (const auto &t: q._container()) {
				using term_type = typename p_type::term_type;
				// NOTE: possibility of unique insertion here.
				// NOTE: possibility of exact division.
				ret_p.insert(term_type{lcm / t.m_cf.den() * t.m_cf.num(),t.m_key});
			}
			return std::make_pair(std::move(ret_p),std::move(lcm));
		}
		// Generic ctors utils.
		// Enabler for the constructor from p_type, integral and string.
		template <typename T>
		using pzs_enabler = std::integral_constant<bool,std::is_same<decay_t<T>,p_type>::value || is_integral<decay_t<T>>::value ||
			std::is_same<decay_t<T>,std::string>::value ||
			std::is_same<decay_t<T>,char *>::value ||
			std::is_same<decay_t<T>,const char *>::value>;
		// Ctor from p_type, integrals and string.
		template <typename T, typename std::enable_if<pzs_enabler<T>::value,int>::type = 0>
		void dispatch_unary_ctor(T &&x)
		{
			m_num = p_type{std::forward<T>(x)};
			m_den = p_type{1};
			// Check for negative expos in the num if cting from a polynomial.
			if (std::is_same<decay_t<T>,p_type>::value) {
				detail::poly_expo_checker(m_num);
			}
		}
		// Ctor from rationals.
		void dispatch_unary_ctor(const rational &q)
		{
			// NOTE: here we are assuming that q is canonicalised, as it should be.
			m_num = q.num();
			m_den = q.den();
		}
		// Ctor from q_type.
		void dispatch_unary_ctor(const q_type &q)
		{
			auto res = q_to_p_type(q);
			m_num = std::move(res.first);
			m_den = std::move(res.second);
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
		// Binary constructors (binary counterparts of the above above).
		// Enabler for construction from pzs. We require T and U to be the same, after decay.
		template <typename T, typename U>
		using pzs_enabler2 = std::integral_constant<bool,pzs_enabler<T>::value && std::is_same<decay_t<T>,decay_t<U>>::value>;
		template <typename T, typename U, typename std::enable_if<pzs_enabler2<T,U>::value,int>::type = 0>
		void dispatch_binary_ctor(T &&x, U &&y)
		{
			m_num = p_type{std::forward<T>(x)};
			m_den = p_type{std::forward<U>(y)};
			// Always need to canonicalise.
			// NOTE: normally we should *not* set m_num/m_den and canonicalise afterwards, as this
			// is not exception safe. Here it is a special case because this function is called from
			// a ctor only, thus if something goes wrong in the canonicalisation m_num/m_den will be destroyed
			// in the cleanup.
			canonicalise();
		}
		void dispatch_binary_ctor(const q_type &n, const q_type &d)
		{
			auto res_n = q_to_p_type(n);
			auto res_d = q_to_p_type(d);
			m_num = std::move(res_n.first);
			m_den = std::move(res_d.first);
			m_num *= std::move(res_d.second);
			m_den *= std::move(res_n.second);
			// Canonicalise.
			canonicalise();
		}
		void dispatch_binary_ctor(const rational &n, const rational &d)
		{
			auto tmp = n / d;
			m_num = tmp.num();
			m_den = tmp.den();
		}
		// Enabler for generic binary ctor.
		template <typename T, typename U, typename V>
		using binary_ctor_enabler = typename std::enable_if<
			// NOTE: this should disable the ctor if T derives from rational_function,
			// as the dispatcher does not support that.
			detail::true_tt<decltype(std::declval<V &>().dispatch_binary_ctor(std::declval<const decay_t<T> &>(),std::declval<const decay_t<U> &>()))>::value
		,int>::type;
		// Enabler for generic assignment.
		// Here we are re-using the enabler for generic unary construction. This should make sure that
		// the generic assignment operator is never used when T is rational_function.
		template <typename T, typename U>
		using generic_assignment_enabler = unary_ctor_enabler<T,U>;
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
		 * \note
		 * This constructor is enabled if the decay type of \p T is either:
		 * - rational_function::p_type,
		 * - rational_function::q_type,
		 * - a C++ integral type, piranha::integer or piranha::rational,
		 * - a string type.
		 *
		 * The constructor operates as follows:
		 * - if \p T is rational_function::p_type, a C++ integral type, piranha::integer or a string type, then the numerator
		 *   is constructed from \p x, and the denominator is set to 1;
		 * - if \p T is piranha::rational, then the numerator is constructed from the numerator of \p x, and the denominator
		 *   from the denominator of \p x;
		 * - if \p T is rational_function::q_type, then, after construction, \p this will be mathematically equivalent to \p x (that is, the coefficients
		 *   of \p x are multiplied by their LCM and the result is used to construct the numerator, and the denominator is constructed from the LCM
		 *   of the coefficients).
		 *
		 * @param[in] x construction argument.
		 *
		 * @throws std::invalid_argument if, when constructing from rational_function::p_type or rational_function::q_type, a negative exponent is
		 * encountered.
		 * @throws unspecified any exception thrown by:
		 * - the constructors and the assignment operators of rational_function::p_type,
		 * - the constructors of the term type of rational_function::p_type,
		 * - the public interface of piranha::series and piranha::hash_set.
		 */
		template <typename T, typename U = rational_function, unary_ctor_enabler<T,U> = 0>
		explicit rational_function(T &&x)
		{
			dispatch_unary_ctor(std::forward<T>(x));
		}
		/// Generic binary constructor.
		/**
		 * \note
		 * This constructor is enabled only if the decay types of \p T and \p U are the same type \p Td. \p Td must be either:
		 * - rational_function::p_type,
		 * - rational_function::q_type,
		 * - a C++ integral type, piranha::integer or piranha::rational,
		 * - a string type.
		 *
		 * The constructor operates as follows:
		 * - if \p Td is rational_function::p_type, a C++ integral type, piranha::integer or a string type, then the numerator
		 *   is constructed from \p x and the denominator from \p y;
		 * - if \p Td is piranha::rational, then the first rational is divided by the second and the result's numerator and denominator
		 *   are used to construct the numerator and denominator of \p this;
		 * - if \p Td is rational_function::q_type, then, after construction, \p this will be mathematically equivalent
		 *   to <tt>x / y</tt> (that is, \p x and \p y are both multiplied by <tt>lx * ly</tt>, the product of the LCMs of their coefficients;
		 *   the resulting polynomials are used to initialise the numerator and denominator of \p this).
		 *
		 * @param[in] x first argument.
		 * @param[in] y second argument.
		 *
		 * @throws std::invalid_argument if, when constructing from rational_function::p_type or rational_function::q_type, a negative exponent is
		 * encountered.
		 * @throws unspecified any exception thrown by:
		 * - the constructors, the assignment and the multiplication operators of rational_function::p_type,
		 * - the division operator of piranha::rational,
		 * - the constructors of the term type of rational_function::p_type,
		 * - the public interface of piranha::series and piranha::hash_set,
		 * - piranha::rational_function::canonicalise().
		 */
		template <typename T, typename U, typename V = rational_function, binary_ctor_enabler<T,U,V> = 0>
		explicit rational_function(T &&x, U &&y)
		{
			dispatch_binary_ctor(std::forward<T>(x),std::forward<U>(y));
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
				*this = rational_function(other);
			}
			return *this;
		}
		/// Defaulted move-assignment operator.
		rational_function &operator=(rational_function &&) = default;
		/// Generic assignment operator.
		/**
		 * \note
		 * This operator is enabled only if the corresponding generic unary constructor is enabled.
		 *
		 * The operator is implemented as the assignment from a piranha::rational_function constructed
		 * from \p x.
		 *
		 * @param[in] x assignment argument.
		 *
		 * @return a reference to \p this.
		 *
		 * @throws unspecified any exception thrown by the invoked constructor.
		 */
		template <typename T, typename U = rational_function, generic_assignment_enabler<T,U> = 0>
		rational_function &operator=(T &&x)
		{
			// Check this is never invoked when T is a rational function.
			static_assert(!std::is_base_of<rational_function,decay_t<T>>::value,"Type error.");
			return *this = rational_function(std::forward<T>(x));
		}
		/// Defaulted destructor.
		~rational_function() = default;
		/// Canonicalisation.
		/**
		 * This method will put \p this in canonical form. Normally, it is never necessary to call this method,
		 * unless low-level methods that do not keep \p this in canonical form have been invoked.
		 *
		 * If any exception is thrown by this method, \p this will not be modified.
		 *
		 * @throws piranha::zero_division_error if the numerator is zero.
		 * @throws std::invalid_argument if either the numerator or the denominator have a negative exponent.
		 * @throws unspecified any exception thrown by construction, division and GCD operations involving objects of
		 * type rational_function::p_type.
		 */
		void canonicalise()
		{
			auto res = canonicalise_impl(m_num,m_den);
			// This is noexcept.
			m_num = std::move(res.first);
			m_den = std::move(res.second);
		}
		/// Numerator.
		/**
		 * @return a const reference to the numerator.
		 */
		const p_type &num() const
		{
			return m_num;
		}
		/// Denominator.
		/**
		 * @return a const reference to the denominator.
		 */
		const p_type &den() const
		{
			return m_den;
		}
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
				if (r.m_den.is_single_coefficient() ||
					(r.m_den.size() == 1u && math::is_unitary(r.m_den._container().begin()->m_cf)))
				{
					// If the denominator is a single coefficient or it has a single term with unitary coefficient,
					// don't print the brackets.
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
