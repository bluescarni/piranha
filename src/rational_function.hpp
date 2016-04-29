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

#include <algorithm>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "config.hpp"
#include "exceptions.hpp"
#include "is_cf.hpp"
#include "math.hpp"
#include "mp_integer.hpp"
#include "mp_rational.hpp"
#include "polynomial.hpp"
#include "pow.hpp"
#include "print_tex_coefficient.hpp"
#include "serialization.hpp"
#include "series.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

struct rational_function_tag {};

}

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
 * Internally, a piranha::rational_function consists of a numerator and a denominator represented as piranha::polynomial with piranha::integer coefficients.
 * Rational functions are always kept in a canonical form defined by the following properties:
 * - numerator and denominator are coprime,
 * - zero is always represented as <tt>0 / 1</tt>,
 * - the denominator is never zero and its leading term is always positive.
 *
 * This class satisfies the piranha::is_cf type trait.
 *
 * ## Interoperability with other types ##
 *
 * Instances of piranha::rational_function can interoperate with:
 * - piranha::integer,
 * - piranha::rational,
 * - the polynomial type representing numerator and denominator (that is, piranha::rational_function::p_type) and its
 *   counterpart with rational coefficients (that is, piranha::rational_function::q_type).
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
 * Move operations will leave objects of this class in a state which is destructible and assignable.
 *
 * ## Serialization ##
 *
 * This class supports serialization.
 */
template <typename Key>
class rational_function: public detail::rational_function_tag
{
		// Make friend with the math::partial() specialisation functor.
		template <typename, typename>
		friend struct math::partial_impl;
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
		// Detect types interoperable for arithmetics.
		template <typename T>
		using is_interoperable = std::integral_constant<bool,
			is_integral<T>::value || std::is_same<T,rational>::value ||
			std::is_same<T,p_type>::value || std::is_same<T,q_type>::value
		>;
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
			// NOTE: it would make sense here to deal with the single coefficient denominator case as well.
			// This should give good performance when one is using rational_function as a rational polynomial,
			// no need to go through a costly canonicalisation.
			// If the denominator is unitary, just return the input values.
			if (math::is_unitary(d)) {
				return std::make_pair(n,d);
			}
			// Handle single-coefficient polys.
			if (n.is_single_coefficient() && d.is_single_coefficient()) {
				piranha_assert(n.size() == 1u && d.size() == 1u);
				// Use a rational to construct the canonical form.
				rational tmp{n._container().begin()->m_cf,d._container().begin()->m_cf};
				return std::make_pair(p_type{tmp.num()},p_type{tmp.den()});
			}
			// Compute the GCD and create the return values.
			auto tup_res = p_type::gcd(n,d,true);
			auto retval = std::make_pair(std::move(std::get<1u>(tup_res)),std::move(std::get<2u>(tup_res)));
			// Check if we need to adjust the sign of the leading term
			// of the denominator.
			// NOTE: here we ask for the lterm into something that potentially might be zero
			// if truncation is active. Keep this in mind.
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
		// Binary ctor enabler.
		template <typename T, typename U, typename V>
		using binary_ctor_enabler = typename std::enable_if<std::is_constructible<V,T &&>::value &&
			std::is_constructible<V,U &&>::value,int>::type;
		// Enabler for generic assignment.
		// Here we are re-using the enabler for generic unary construction. This should make sure that
		// the generic assignment operator is never used when T is rational_function.
		template <typename T, typename U>
		using generic_assignment_enabler = unary_ctor_enabler<T,U>;
		// Addition.
		static rational_function dispatch_binary_add(const rational_function &a, const rational_function &b)
		{
			// Detect unitary denominators.
			const bool uda = math::is_unitary(a.den()), udb = math::is_unitary(b.den());
			rational_function retval;
			if (uda && udb) {
				// With unitary denominators, just add the numerators and set den to 1.
				retval.m_num = a.m_num + b.m_num;
				retval.m_den = 1;
			} else if (uda) {
				// Only a is unitary.
				retval.m_num = a.m_num*b.m_den + b.m_num;
				retval.m_den = b.m_den;
			} else if (udb) {
				// Only b is unitary.
				retval.m_num = a.m_num + b.m_num*a.m_den;
				retval.m_den = a.m_den;
			} else {
				// The general case.
				retval.m_num = a.m_num*b.m_den + a.m_den*b.m_num;
				retval.m_den = a.m_den*b.m_den;
				// NOTE: if the canonicalisation fails for some reason, it is not a problem
				// as retval is a local variable that will just be destroyed. The destructor
				// does not run any check.
				retval.canonicalise();
			}
			return retval;
		}
		template <typename T, typename std::enable_if<is_interoperable<T>::value,int>::type = 0>
		static rational_function dispatch_binary_add(const rational_function &a, const T &b)
		{
			return a + rational_function{b};
		}
		template <typename T, typename std::enable_if<is_interoperable<T>::value,int>::type = 0>
		static rational_function dispatch_binary_add(const T &a, const rational_function &b)
		{
			return rational_function{a} + b;
		}
		template <typename T, typename U>
		using binary_add_enabler = typename std::enable_if<detail::true_tt<
			// NOTE: here the rational_function:: specifier is apprently needed only in GCC, probably a bug.
			decltype(rational_function::dispatch_binary_add(std::declval<const decay_t<T> &>(),
			std::declval<const decay_t<U> &>()))>::value,int>::type;
		template <typename T, typename U>
		using in_place_add_enabler = typename std::enable_if<detail::true_tt<decltype(std::declval<const decay_t<T> &>()
			+ std::declval<const decay_t<U> &>())>::value,int>::type;
		// Subtraction.
		static rational_function dispatch_binary_sub(const rational_function &a, const rational_function &b)
		{
			const bool uda = math::is_unitary(a.den()), udb = math::is_unitary(b.den());
			rational_function retval;
			if (uda && udb) {
				retval.m_num = a.m_num - b.m_num;
				retval.m_den = 1;
			} else if (uda) {
				retval.m_num = a.m_num*b.m_den - b.m_num;
				retval.m_den = b.m_den;
			} else if (udb) {
				retval.m_num = a.m_num - b.m_num*a.m_den;
				retval.m_den = a.m_den;
			} else {
				retval.m_num = a.m_num*b.m_den - a.m_den*b.m_num;
				retval.m_den = a.m_den*b.m_den;
				retval.canonicalise();
			}
			return retval;
		}
		template <typename T, typename std::enable_if<is_interoperable<T>::value,int>::type = 0>
		static rational_function dispatch_binary_sub(const rational_function &a, const T &b)
		{
			return a - rational_function{b};
		}
		template <typename T, typename std::enable_if<is_interoperable<T>::value,int>::type = 0>
		static rational_function dispatch_binary_sub(const T &a, const rational_function &b)
		{
			return rational_function{a} - b;
		}
		template <typename T, typename U>
		using binary_sub_enabler = typename std::enable_if<detail::true_tt<
			decltype(rational_function::dispatch_binary_sub(std::declval<const decay_t<T> &>(),
			std::declval<const decay_t<U> &>()))>::value,int>::type;
		template <typename T, typename U>
		using in_place_sub_enabler = typename std::enable_if<detail::true_tt<decltype(std::declval<const decay_t<T> &>()
			- std::declval<const decay_t<U> &>())>::value,int>::type;
		// Multiplication.
		static rational_function dispatch_binary_mul(const rational_function &a, const rational_function &b)
		{
			const bool uda = math::is_unitary(a.den()), udb = math::is_unitary(b.den());
			rational_function retval;
			if (uda && udb) {
				retval.m_num = a.m_num * b.m_num;
				retval.m_den = 1;
			} else {
				retval.m_num = a.m_num*b.m_num;
				retval.m_den = a.m_den*b.m_den;
				retval.canonicalise();
			}
			return retval;
		}
		template <typename T, typename std::enable_if<is_interoperable<T>::value,int>::type = 0>
		static rational_function dispatch_binary_mul(const rational_function &a, const T &b)
		{
			return a * rational_function{b};
		}
		template <typename T, typename std::enable_if<is_interoperable<T>::value,int>::type = 0>
		static rational_function dispatch_binary_mul(const T &a, const rational_function &b)
		{
			return rational_function{a} * b;
		}
		template <typename T, typename U>
		using binary_mul_enabler = typename std::enable_if<detail::true_tt<
			decltype(rational_function::dispatch_binary_mul(std::declval<const decay_t<T> &>(),
			std::declval<const decay_t<U> &>()))>::value,int>::type;
		template <typename T, typename U>
		using in_place_mul_enabler = typename std::enable_if<detail::true_tt<decltype(std::declval<const decay_t<T> &>()
			* std::declval<const decay_t<U> &>())>::value,int>::type;
		// Division.
		static rational_function dispatch_binary_div(const rational_function &a, const rational_function &b)
		{
			// NOTE: division is like a multiplication with inverted second argument, so we need
			// to check the num of b.
			const bool uda = math::is_unitary(a.den()), udb = math::is_unitary(b.num());
			rational_function retval;
			if (uda && udb) {
				retval.m_num = a.m_num * b.m_den;
				retval.m_den = 1;
			} else {
				retval.m_num = a.m_num*b.m_den;
				retval.m_den = a.m_den*b.m_num;
				retval.canonicalise();
			}
			return retval;
		}
		template <typename T, typename std::enable_if<is_interoperable<T>::value,int>::type = 0>
		static rational_function dispatch_binary_div(const rational_function &a, const T &b)
		{
			return a / rational_function{b};
		}
		template <typename T, typename std::enable_if<is_interoperable<T>::value,int>::type = 0>
		static rational_function dispatch_binary_div(const T &a, const rational_function &b)
		{
			return rational_function{a} / b;
		}
		template <typename T, typename U>
		using binary_div_enabler = typename std::enable_if<detail::true_tt<
			decltype(rational_function::dispatch_binary_div(std::declval<const decay_t<T> &>(),
			std::declval<const decay_t<U> &>()))>::value,int>::type;
		template <typename T, typename U>
		using in_place_div_enabler = typename std::enable_if<detail::true_tt<decltype(std::declval<const decay_t<T> &>()
			/ std::declval<const decay_t<U> &>())>::value,int>::type;
		// Comparison.
		static bool dispatch_equality(const rational_function &a, const rational_function &b)
		{
			return a.m_num == b.m_num && a.m_den == b.m_den;
		}
		template <typename T, typename std::enable_if<is_interoperable<T>::value,int>::type = 0>
		static bool dispatch_equality(const rational_function &a, const T &b)
		{
			return a == rational_function{b};
		}
		template <typename T, typename std::enable_if<is_interoperable<T>::value,int>::type = 0>
		static bool dispatch_equality(const T &a, const rational_function &b)
		{
			return b == rational_function{a};
		}
		template <typename T, typename U>
		using eq_enabler = typename std::enable_if<detail::true_tt<
			decltype(rational_function::dispatch_equality(std::declval<const T &>(),
			std::declval<const U &>()))>::value,int>::type;
		// subs().
		template <typename T>
		using has_special_subs = std::integral_constant<bool,
			is_interoperable<T>::value || std::is_same<T,rational_function>::value>;
		// Type resulting from the normal substitution method.
		template <typename T>
		using normal_subs_type = decltype(math::subs(std::declval<const p_type &>(),std::string{},
			std::declval<const T &>()) / math::subs(std::declval<const p_type &>(),std::string{},
			std::declval<const T &>()));
		template <typename T>
		using has_normal_subs = std::integral_constant<bool,
			!has_special_subs<T>::value &&
			is_returnable<normal_subs_type<T>>::value>;
		template <typename T, typename std::enable_if<has_special_subs<T>::value,int>::type = 0>
		static rational_function subs_impl(const rational_function &r, const std::string &name, const T &x)
		{
			return rational_function{math::subs(r.num(),name,x),math::subs(r.den(),name,x)};
		}
		template <typename T, typename std::enable_if<has_normal_subs<T>::value,int>::type = 0>
		static normal_subs_type<T> subs_impl(const rational_function &r, const std::string &name, const T &x)
		{
			return math::subs(r.num(),name,x) / math::subs(r.den(),name,x);
		}
		template <typename T>
		using subs_type = decltype(subs_impl(std::declval<const rational_function &>(),std::string{},
			std::declval<const T &>()));
		// ipow_subs().
		template <typename T>
		using has_special_ipow_subs = has_special_subs<T>;
		// Type resulting from the normal substitution method.
		template <typename T>
		using normal_ipow_subs_type = decltype(math::ipow_subs(std::declval<const p_type &>(),std::string{},integer{},
			std::declval<const T &>()) / math::ipow_subs(std::declval<const p_type &>(),std::string{},integer{},
			std::declval<const T &>()));
		template <typename T>
		using has_normal_ipow_subs = std::integral_constant<bool,
			!has_special_ipow_subs<T>::value &&
			is_returnable<normal_ipow_subs_type<T>>::value>;
		template <typename T, typename std::enable_if<has_special_ipow_subs<T>::value,int>::type = 0>
		static rational_function ipow_subs_impl(const rational_function &r, const std::string &name, const integer &n, const T &x)
		{
			return rational_function{math::ipow_subs(r.num(),name,n,x),math::ipow_subs(r.den(),name,n,x)};
		}
		template <typename T, typename std::enable_if<has_normal_ipow_subs<T>::value,int>::type = 0>
		static normal_ipow_subs_type<T> ipow_subs_impl(const rational_function &r, const std::string &name, const integer &n, const T &x)
		{
			return math::ipow_subs(r.num(),name,n,x) / math::ipow_subs(r.den(),name,n,x);
		}
		template <typename T>
		using ipow_subs_type = decltype(ipow_subs_impl(std::declval<const rational_function &>(),std::string{},
			integer{},std::declval<const T &>()));
		// Serialization support.
		friend class boost::serialization::access;
		template <class Archive>
		void save(Archive &ar, unsigned int) const
		{
			// NOTE: here in principle we do not need the split member implementation,
			// this syntax could be used for both load and save. However, for load
			// we use an implementation that gives better exception safety: load num/den
			// into local variables and the move them in. So if something goes wrong in the
			// deserialization of one of the ints, we do not modify this.
			ar & m_num;
			ar & m_den;
		}
		template <class Archive>
		void load(Archive &ar, unsigned int)
		{
			p_type num, den;
			ar & num;
			ar & den;
			// This ensures that if we load from a bad archive with non-coprime
			// num and den or negative den, or... we get anyway a canonicalised
			// rational_function or an error.
			*this = rational_function{num,den};
		}
		BOOST_SERIALIZATION_SPLIT_MEMBER()
		// Hashing utils.
		struct rf_hasher
		{
			template <typename T>
			std::size_t operator()(const T &s) const
			{
				return s.hash();
			}
		};
		struct rf_equal_to
		{
			template <typename T>
			bool operator()(const T &a, const T &b) const
			{
				return a.is_identical(b);
			}
		};
		// Pow machinery, with cache.
		template <typename T>
		using pow_enabler = typename std::enable_if<is_integral<T>::value,int>::type;
		template <typename RF>
		using pow_map_type = std::unordered_map<RF,std::vector<RF>,rf_hasher,rf_equal_to>;
		template <typename RF = rational_function>
		static pow_map_type<RF> &get_pow_cache()
		{
			static pow_map_type<RF> s_pow_cache;
			return s_pow_cache;
		}
		// The mutex for access to the pow cache.
		static std::mutex s_pow_mutex;
		// Custom derivatives boilerplate.
		template <typename T>
		using cp_map_type = std::unordered_map<std::string,std::function<T(const T &)>>;
		template <typename T = rational_function>
		static cp_map_type<T> &get_cp_map()
		{
			static cp_map_type<T> cp_map;
			return cp_map;
		}
		template <typename F>
		using custom_partial_enabler = typename std::enable_if<std::is_constructible<
			std::function<rational_function(const rational_function &)>,F>::value,int>::type;
		// The mutex for access to the custom derivatives.
		static std::mutex s_cp_mutex;
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
		 * This constructor is enabled if the decay type of \p T is either an interoperable type or a string type.
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
		 * This constructor is enabled only if piranha::rational_function can be constructed from \p T and \p U.
		 *
		 * The body of this constructor is equivalent to:
		 * @code
		 * *this = rational_function(std::forward<T>(x)) / rational_function(std::forward<U>(y));
		 * @endcode
		 *
		 * @param[in] x first argument.
		 * @param[in] y second argument.
		 *
		 * @throws unspecified any exception thrown by:
		 * - the invoked unary constructors,
		 * - the division operator of piranha::rational_function.
		 */
		template <typename T, typename U, typename V = rational_function, binary_ctor_enabler<T,U,V> = 0>
		explicit rational_function(T &&x, U &&y)
		{
			*this = rational_function(std::forward<T>(x)) / rational_function(std::forward<U>(y));
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
		/// Trivial destructor.
		~rational_function()
		{
			PIRANHA_TT_CHECK(is_cf,rational_function);
		}
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
					(r.m_den.size() == 1u && math::is_unitary(r.m_den._container().begin()->m_cf) &&
					integer{r.m_den.degree()} == 1))
				{
					// If the denominator is a single coefficient or it has a single term with unitary coefficient
					// and degree 1 (that is, it is of the form "x"), don't print the brackets.
					os << r.m_den;
				} else {
					os << '(' << r.m_den << ')';
				}
			}
			return os;
		}
		/// Print in TeX mode.
		/**
		 * This method will stream to \p os a TeX representation of \p this.
		 *
		 * @param[in] os target stream.
		 *
		 * @throws unspecified any exception thrown by:
		 * - piranha::rational_function::p_type::print_tex(),
		 * - the unary negation operator of piranha::rational_function::p_type.
		 */
		void print_tex(std::ostream &os) const
		{
			if (math::is_zero(*this)) {
				// Special case for zero.
				os << "0";
			} else if (math::is_unitary(m_den)) {
				// If the denominator is 1, print just the numerator.
				m_num.print_tex(os);
			} else {
				// The idea here is to have the first term of num and den positive.
				// NOTE: here we are relying on the fact that negation of poly proceeds
				// by copy + in-place negation, and thus the order of the terms is preserved.
				piranha_assert(m_num.size() >= 1u);
				piranha_assert(m_den.size() >= 1u);
				const bool negate_num = m_num._container().begin()->m_cf.sign() < 0;
				const bool negate_den = m_den._container().begin()->m_cf.sign() < 0;
				if (negate_num != negate_den) {
					// If we need to negate only one of num/den,
					// then we need to prepend the minus sign.
					os << '-';
				}
				os << "\\frac{";
				if (negate_num) {
					(-m_num).print_tex(os);
				} else {
					m_num.print_tex(os);
				}
				os << "}{";
				if (negate_den) {
					(-m_den).print_tex(os);
				} else {
					m_den.print_tex(os);
				}
				os << '}';
			}
		}
		/** @name Low-level interface
		 * Low-level methods.
		 */
		//@{
		/// Mutable reference to the numerator.
		/**
		 * @return a mutable reference to the numerator.
		 */
		p_type &_num()
		{
			return m_num;
		}
		/// Mutable reference to the denominator.
		/**
		 * @return a mutable reference to the denominator.
		 */
		p_type &_den()
		{
			return m_den;
		}
		//@}
		/// Canonicality check.
		/**
		 * This method will return \p true if \p this is in canonical form, \p false otherwise.
		 * Unless low-level methods are used, non-canonical rational functions can be generated only
		 * by move operations (after which the moved-from object is left with zero numerator and denominator).
		 *
		 * @return \p true if \p this is in canonical form, \p false otherwise.
		 *
		 * @throws unspecified any exception thrown by piranha::math::gcd() or the comparison operator
		 * of piranha::rational_function::p_type.
		 */
		bool is_canonical() const
		{
			auto g = math::gcd(m_num,m_den);
			if (g != 1 && g != -1) {
				return false;
			}
			// NOTE: this catches only the case (0,-1), which gives a GCD
			// of -1. (0,1) is canonical and (0,n) gives a GCD of n, which
			// is filtered above.
			if (math::is_zero(m_num) && !math::is_unitary(m_den)) {
				return false;
			}
			if (math::is_zero(m_den) || detail::poly_lterm(m_den)->m_cf.sign() < 0) {
				return false;
			}
			return true;
		}
		/// Equality operator.
		/**
		 * \note
		 * This operator is enabled only if one of the arguments is piranha::rational_function
		 * and the other argument is either piranha::rational_function or an interoperable type.
		 *
		 * The numerator and denominator of \p a and \p b will be compared (after any necessary conversion
		 * of \p a or \p b to piranha::rational_function).
		 *
		 * @param[in] a first argument.
		 * @param[in] b second argument.
		 *
		 * @return \p true if the numerator and denominator of \p this are equal to the numerator and
		 * denominator of \p other, \p false otherwise.
		 *
		 * @throws unspecified any exception thrown by the generic unary constructor, if a
		 * parameter is not piranha::rational_function.
		 */
		template <typename T, typename U, eq_enabler<T,U> = 0>
		friend bool operator==(const T &a, const U &b)
		{
			return dispatch_equality(a,b);
		}
		/// Inequality operator.
		/**
		 * \note
		 * This operator is enabled only if one of the arguments is piranha::rational_function
		 * and the other argument is either piranha::rational_function or an interoperable type.
		 *
		 * This operator is the opposite of operator==().
		 *
		 * @param[in] a first argument.
		 * @param[in] b second argument.
		 *
		 * @return the opposite of operator==().
		 *
		 * @throws unspecified any exception thrown by operator==().
		 */
		template <typename T, typename U, eq_enabler<T,U> = 0>
		friend bool operator!=(const T &a, const U &b)
		{
			return !dispatch_equality(a,b);
		}
		/// Identity operator.
		/**
		 * @return a copy of \p this.
		 *
		 * @throws unspecified any exception thrown by the copy constructor.
		 */
		rational_function operator+() const
		{
			return *this;
		}
		/// Binary addition.
		/**
		 * \note
		 * This operator is enabled only if one of the arguments is piranha::rational_function
		 * and the other argument is either piranha::rational_function or an interoperable type.
		 *
		 * This operator will compute the result of adding \p a to \p b.
		 *
		 * @param[in] a first argument.
		 * @param[in] b second argument.
		 *
		 * @return <tt>a + b</tt>.
		 *
		 * @throws unspecified any exception thrown by:
		 * - construction, assignment, multiplication, addition of piranha::rational_function::p_type,
		 * - the generic unary constructor of piranha::rational_function,
		 * - canonicalise().
		 */
		template <typename T, typename U, binary_add_enabler<T,U> = 0>
		friend rational_function operator+(T &&a, U &&b)
		{
			return dispatch_binary_add(a,b);
		}
		/// In-place addition.
		/**
		 * \note
		 * This operator is enabled only if \p T is an interoperable type.
		 *
		 * This operator is equivalent to the expression:
		 * @code
		 * return *this = *this + other;
		 * @endcode
		 *
		 * @param[in] other argument.
		 *
		 * @return a reference to \p this.
		 *
		 * @throws unspecified any exception thrown by the corresponding binary operator.
		 */
		template <typename T, typename U = rational_function, in_place_add_enabler<T,U> = 0>
		rational_function &operator+=(T &&other)
		{
			// NOTE: *this + other will always return rational_function.
			// NOTE: we should consider in the future std::move(*this) + std::forward<T>(other) to potentially improve performance,
			// if the addition operator is also modified to take advantage of rvalues.
			return *this = *this + other;
		}
		/// Negated copy.
		/**
		 * @return a copy of <tt>-this</tt>.
		 *
		 * @throws unspecified any exception thrown by the copy constructor.
		 */
		rational_function operator-() const
		{
			rational_function retval{*this};
			math::negate(retval.m_num);
			return retval;
		}
		/// Binary subtraction.
		/**
		 * \note
		 * This operator is enabled only if one of the arguments is piranha::rational_function
		 * and the other argument is either piranha::rational_function or an interoperable type.
		 *
		 * This operator will compute the result of subtracting \p b from \p a.
		 *
		 * @param[in] a first argument.
		 * @param[in] b second argument.
		 *
		 * @return <tt>a - b</tt>.
		 *
		 * @throws unspecified any exception thrown by:
		 * - construction, assignment, multiplication, subtraction of piranha::rational_function::p_type,
		 * - the generic unary constructor of piranha::rational_function,
		 * - canonicalise().
		 */
		template <typename T, typename U, binary_sub_enabler<T,U> = 0>
		friend rational_function operator-(T &&a, U &&b)
		{
			return dispatch_binary_sub(a,b);
		}
		/// In-place subtraction.
		/**
		 * \note
		 * This operator is enabled only if \p T is an interoperable type.
		 *
		 * This operator is equivalent to the expression:
		 * @code
		 * return *this = *this - other;
		 * @endcode
		 *
		 * @param[in] other argument.
		 *
		 * @return a reference to \p this.
		 *
		 * @throws unspecified any exception thrown by the corresponding binary operator.
		 */
		template <typename T, typename U = rational_function, in_place_sub_enabler<T,U> = 0>
		rational_function &operator-=(T &&other)
		{
			return *this = *this - other;
		}
		/// Binary multiplication.
		/**
		 * \note
		 * This operator is enabled only if one of the arguments is piranha::rational_function
		 * and the other argument is either piranha::rational_function or an interoperable type.
		 *
		 * This operator will compute the result of multiplying \p a by \p b.
		 *
		 * @param[in] a first argument.
		 * @param[in] b second argument.
		 *
		 * @return <tt>a * b</tt>.
		 *
		 * @throws unspecified any exception thrown by:
		 * - construction, assignment, multiplication of piranha::rational_function::p_type,
		 * - the generic unary constructor of piranha::rational_function,
		 * - canonicalise().
		 */
		template <typename T, typename U, binary_mul_enabler<T,U> = 0>
		friend rational_function operator*(T &&a, U &&b)
		{
			return dispatch_binary_mul(a,b);
		}
		/// In-place multiplication.
		/**
		 * \note
		 * This operator is enabled only if \p T is an interoperable type.
		 *
		 * This operator is equivalent to the expression:
		 * @code
		 * return *this = *this * other;
		 * @endcode
		 *
		 * @param[in] other argument.
		 *
		 * @return a reference to \p this.
		 *
		 * @throws unspecified any exception thrown by the corresponding binary operator.
		 */
		template <typename T, typename U = rational_function, in_place_mul_enabler<T,U> = 0>
		rational_function &operator*=(T &&other)
		{
			return *this = *this * other;
		}
		/// Binary division.
		/**
		 * \note
		 * This operator is enabled only if one of the arguments is piranha::rational_function
		 * and the other argument is either piranha::rational_function or an interoperable type.
		 *
		 * This operator will compute the result of dividing \p a by \p b.
		 *
		 * @param[in] a first argument.
		 * @param[in] b second argument.
		 *
		 * @return <tt>a / b</tt>.
		 *
		 * @throws unspecified any exception thrown by:
		 * - construction, assignment, multiplication of piranha::rational_function::p_type,
		 * - the generic unary constructor of piranha::rational_function,
		 * - canonicalise().
		 */
		template <typename T, typename U, binary_div_enabler<T,U> = 0>
		friend rational_function operator/(T &&a, U &&b)
		{
			return dispatch_binary_div(a,b);
		}
		/// In-place division.
		/**
		 * \note
		 * This operator is enabled only if \p T is an interoperable type.
		 *
		 * This operator is equivalent to the expression:
		 * @code
		 * return *this = *this / other;
		 * @endcode
		 *
		 * @param[in] other argument.
		 *
		 * @return a reference to \p this.
		 *
		 * @throws unspecified any exception thrown by the corresponding binary operator.
		 */
		template <typename T, typename U = rational_function, in_place_div_enabler<T,U> = 0>
		rational_function &operator/=(T &&other)
		{
			return *this = *this / other;
		}
		/// Exponentiation.
		/**
		 * \note
		 * This method is enabled only if \p T is a C++ integral type or piranha::integer.
		 *
		 * Similarly to piranha::series::pow(), this method keeps a cache of natural powers of
		 * rational functions in order to expedite computations when the same powers of rational
		 * functions are requested repeatedly (e.g., during substitution operations). The cache is
		 * thread-safe and it can be cleared with piranha::rational_function::clear_pow_cache().
		 *
		 * @param[in] n an integral exponent.
		 *
		 * @return \p this raised to the power of \p n.
		 *
		 * @throws piranha::zero_division_error if \p n is negative and \p this is zero.
		 * @throws unspecified any exception thrown by:
		 * - threading primitives,
		 * - memory errors in standard containers,
		 * - construction, assignment and arithmetic operations on piranha::rational_function.
		 */
		template <typename T, pow_enabler<T> = 0>
		rational_function pow(const T &n) const
		{
			// NOTE: here we are renouncing the pow() optimisation
			// implemented for the polynomials. Consider bringing
			// it back if it becomes important.
			const integer n_int{n}, un_int = n_int.abs();
			rational_function retval;
			{
			// Lock the cache.
			std::lock_guard<std::mutex> lock(s_pow_mutex);
			auto &v = get_pow_cache()[*this];
			using s_type = decltype(v.size());
			// Init the vector, if needed.
			if (!v.size()) {
				v.push_back(rational_function{1});
			}
			// Fill in the missing powers.
			while (v.size() <= un_int) {
				// NOTE: avoid canonicalisation by setting directly
				// the num/den.
				// NOTE: this will have to be replaced by explicit untruncated
				// multiplication.
				rational_function tmp;
				tmp.m_num = v.back().m_num * m_num;
				tmp.m_den = v.back().m_den * m_den;
				v.push_back(std::move(tmp));
			}
			retval = v[static_cast<s_type>(un_int)];
			}
			// We need to fix retval in case of negative powers.
			if (n_int.sign() < 0) {
				if (unlikely(math::is_zero(retval.m_num))) {
					piranha_throw(zero_division_error,"zero denominator in rational_function exponentiation");
				}
				// Swap num/den.
				std::swap(retval.m_num,retval.m_den);
				// The only canonicalisation we need is checking the sign of the new
				// denominator.
				if (detail::poly_lterm(retval.m_den)->m_cf.sign() < 0) {
					math::negate(retval.m_num);
					math::negate(retval.m_den);
				}
			}
			return retval;
		}
		/// Clear the internal cache of natural powers.
		/**
		 * This method can be used to clear the cache of natural powers of series maintained by piranha::rational_function::pow().
		 *
		 * @throws unspecified any exception thrown by threading primitives.
		 */
		static void clear_pow_cache()
		{
			std::lock_guard<std::mutex> lock(s_pow_mutex);
			get_pow_cache().clear();
		}
		/// Substitution.
		/**
		 * \note
		 * This method is enabled only if either:
		 * - \p T is an interoperable type or piranha::rational_function, or
		 * - the expression <tt>math::subs(num(),name,x) / math::subs(den(),name,x)</tt>
		 *   is well-formed, yielding a type that satisfies piranha::is_returnable.
		 *
		 * If \p T is an interoperable type or piranha::rational_function, the body of this method is equivalent to:
		 * @code
		 * return rational_function{math::subs(num(),name,x),math::subs(den(),name,x)};
		 * @endcode
		 * Otherwise, the body of this method is equivalent to:
		 * @code
		 * return math::subs(num(),name,x) / math::subs(den(),name,x);
		 * @endcode
		 *
		 * @param[in] name name of the variable to be substituted.
		 * @param[in] x substitution argument.
		 *
		 * @return the result of substituting \p name with \p x in \p this.
		 *
		 * @throws unspecified any exception thrown by:
		 * - piranha::math::subs(),
		 * - the binary constructor of piranha::rational_function,
		 * - the division operator on the type resulting from the substitution
		 *   in the numerator and denominator.
		 */
		template <typename T>
		subs_type<T> subs(const std::string &name, const T &x) const
		{
			return subs_impl(*this,name,x);
		}
		/// Substitution of integral power.
		/**
		 * \note
		 * This method is enabled only if either:
		 * - \p T is an interoperable type or piranha::rational_function, or
		 * - the expression <tt>math::ipow_subs(num(),name,n,x) / math::ipow_subs(den(),name,n,x)</tt>
		 *   is well-formed, yielding a type that satisfies piranha::is_returnable.
		 *
		 * If \p T is an interoperable type or piranha::rational_function, the body of this method is equivalent to:
		 * @code
		 * return rational_function{math::ipow_subs(num(),name,n,x),math::ipow_subs(den(),name,n,x)};
		 * @endcode
		 * Otherwise, the body of this method is equivalent to:
		 * @code
		 * return math::ipow_subs(num(),name,n,x) / math::ipow_subs(den(),name,n,x);
		 * @endcode
		 *
		 * @param[in] name name of the variable to be substituted.
		 * @param[in] n power of \p name to be substituted.
		 * @param[in] x substitution argument.
		 *
		 * @return the result of substituting \p name to the power of \p n with \p x in \p this.
		 *
		 * @throws unspecified any exception thrown by:
		 * - piranha::math::ipow_subs(),
		 * - the binary constructor of piranha::rational_function,
		 * - the division operator on the type resulting from the substitution
		 *   in the numerator and denominator.
		 */
		template <typename T>
		ipow_subs_type<T> ipow_subs(const std::string &name, const integer &n, const T &x) const
		{
			return ipow_subs_impl(*this,name,n,x);
		}
		/// Trim.
		/**
		 * This method will return a copy of \p this whose numerator and denominator have been generated
		 * by calling piranha::series::trim() on the numerator and denominator of \p this.
		 *
		 * @return a copy of \p this with the ignorable arguments removed.
		 *
		 * @throws unspecified any exception thrown by piranha::series::trim().
		 */
		rational_function trim() const
		{
			// Don't use the ctor, as the result will be canonical by construction.
			rational_function retval;
			retval.m_num = m_num.trim();
			retval.m_den = m_den.trim();
			return retval;
		}
		/// Hash value.
		/**
		 * The hash of a rational function is computed by combining the hashes of
		 * numerator and denominator.
		 *
		 * @return a hash value for \p this.
		 */
		std::size_t hash() const
		{
			return static_cast<std::size_t>(num().hash() + den().hash());
		}
		/// Identical check.
		/**
		 * Two rational functions are identical if their numerators and denominators are, via
		 * piranha::rational_function::p_type::is_identical().
		 *
		 * @param[in] other comparison argument.
		 *
		 * @return \p true if \p this is identical to \p other, \p false otherwise.
		 */
		bool is_identical(const rational_function &other) const
		{
			return m_num.is_identical(other.m_num) && m_den.is_identical(other.m_den);
		}
		/// Register custom partial derivative.
		/**
		 * \note
		 * This method is enabled only if \p F is a type that can be used to construct
		 * <tt>std::function<rational_function(const rational_function &)</tt>.
		 *
		 * Register a copy of a callable \p func associated to the symbol called \p name for use by piranha::math::partial().
		 * \p func will be used to compute the partial derivative of piranha::rational_function with respect to
		 * \p name in place of the default partial differentiation algorithm.
		 *
		 * It is safe to call this method from multiple threads.
		 *
		 * @param[in] name symbol for which the custom partial derivative function will be registered.
		 * @param[in] func custom partial derivative function.
		 *
		 * @throws unspecified any exception thrown by:
		 * - failure(s) in threading primitives,
		 * - lookup and insertion operations on \p std::unordered_map,
		 * - construction and move-assignment of \p std::function.
		 */
		template <typename F, custom_partial_enabler<F> = 0>
		static void register_custom_derivative(const std::string &name, F func)
		{
			using f_type = std::function<rational_function(const rational_function &)>;
			std::lock_guard<std::mutex> lock(s_cp_mutex);
			get_cp_map()[name] = f_type(func);
		}
		/// Unregister custom partial derivative.
		/**
		 * Unregister the custom partial derivative function associated to the symbol called \p name. If no custom
		 * partial derivative was previously registered using register_custom_derivative(), calling this function will be a no-op.
		 *
		 * It is safe to call this method from multiple threads.
		 *
		 * @param[in] name symbol for which the custom partial derivative function will be unregistered.
		 *
		 * @throws unspecified any exception thrown by:
		 * - failure(s) in threading primitives,
		 * - lookup and erase operations on \p std::unordered_map.
		 */
		static void unregister_custom_derivative(const std::string &name)
		{
			std::lock_guard<std::mutex> lock(s_cp_mutex);
			auto it = get_cp_map().find(name);
			if (it != get_cp_map().end()) {
				get_cp_map().erase(it);
			}
		}
		/// Unregister all custom partial derivatives.
		/**
		 * Will unregister all custom derivatives currently registered via register_custom_derivative().
		 * It is safe to call this method from multiple threads.
		 *
		 * @throws unspecified any exception thrown by failure(s) in threading primitives.
		 */
		static void unregister_all_custom_derivatives()
		{
			std::lock_guard<std::mutex> lock(s_cp_mutex);
			get_cp_map().clear();
		}
		/// Partial derivative.
		/**
		 * The result is computed via the quotient rule. Internally, this method will call
		 * piranha::math::partial() on the numerator and denominator of \p this.
		 *
		 * @param[in] name name of the variable with respect to which to differentiate.
		 *
		 * @return the partial derivative of \p r with respect to \p name.
		 *
		 * @throws unspecified any exception thrown by:
		 * - the partial derivative of numerator and denominator,
		 * - arithmetic operations on piranha::rational_function::p_type,
		 * - the binary constructor of piranha::rational_function.
		 */
		rational_function partial(const std::string &name) const
		{
			using math::partial;
			return rational_function{partial(num(),name)*den()-num()*partial(den(),name),
				den()*den()};
		}
	private:
		p_type	m_num;
		p_type	m_den;
};

template <typename Key>
std::mutex rational_function<Key>::s_pow_mutex;

template <typename Key>
std::mutex rational_function<Key>::s_cp_mutex;

/// Specialisation of piranha::print_tex_coefficient() for piranha::rational_function.
/**
 * This specialisation is enabled if \p T is an instance of piranha::rational_function.
 */
template <typename T>
struct print_tex_coefficient_impl<T,typename std::enable_if<std::is_base_of<detail::rational_function_tag,T>::value>::type>
{
	/// Call operator.
	/**
	 * This operator will call internally piranha::rational_function::print_tex().
	 *
	 * @param[in] os target stream.
	 * @param[in] r piranha::rational_function argument.
	 *
	 * @throws unspecified any exception thrown by piranha::rational_function::print_tex().
	 */
	void operator()(std::ostream &os, const T &r) const
	{
		r.print_tex(os);
	}
};

namespace math
{

/// Specialisation of piranha::math::is_zero() for piranha::rational_function.
/**
 * This specialisation is enabled if \p T is an instance of piranha::rational_function.
 */
template <typename T>
struct is_zero_impl<T,typename std::enable_if<std::is_base_of<detail::rational_function_tag,T>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] r the piranha::rational_function to be tested.
	 *
	 * @return the result of calling piranha::math::is_zero() on the numerator of \p r.
	 */
	bool operator()(const T &r) const
	{
		return is_zero(r.num());
	}
};

/// Specialisation of piranha::math::pow() for piranha::rational_function bases.
/**
 * This specialisation is enabled if \p T is an instance of piranha::rational_function.
 */
template <typename T, typename U>
struct pow_impl<T,U,typename std::enable_if<std::is_base_of<detail::rational_function_tag,T>::value>::type>
{
	private:
		template <typename V>
		using pow_enabler = typename std::enable_if<detail::true_tt<
			decltype(std::declval<const T &>().pow(std::declval<const V &>()))>::value,int>::type;
	public:
		/// Call operator.
		/**
		 * \note
		 * The call operator is enabled only if <tt>return r.pow(n)</tt> is a valid expression.
		 *
		 * @param[in] r the piranha::rational_function base.
		 * @param[in] n the integral exponent.
		 *
		 * @return <tt>r.pow(n)</tt>.
		 *
		 * @throws unspecified any exception thrown by piranha::rational_function::pow().
		 */
		template <typename V, pow_enabler<V> = 0>
		T operator()(const T &r, const V &n) const
		{
			return r.pow(n);
		}
};

/// Specialisation of piranha::math::subs() for piranha::rational_function.
/**
 * This specialisation is enabled if \p T is an instance of piranha::rational_function.
 */
template <typename T, typename U>
struct subs_impl<T,U,typename std::enable_if<std::is_base_of<detail::rational_function_tag,T>::value>::type>
{
	private:
		template <typename V>
		using subs_type = decltype(std::declval<const T &>().subs(std::string{},std::declval<const V &>()));
	public:
		/// Call operator.
		/**
		 * \note
		 * The call operator is enabled only if <tt>return r.subs(s,x)</tt> is a valid expression.
		 *
		 * @param[in] r the piranha::rational_function argument.
		 * @param[in] s the name of the variable to be substituted.
		 * @param[in] x the substitution argument.
		 *
		 * @return <tt>r.subs(s,x)</tt>.
		 *
		 * @throws unspecified any exception thrown by piranha::rational_function::subs().
		 */
		template <typename V>
		subs_type<V> operator()(const T &r, const std::string &s, const V &x) const
		{
			return r.subs(s,x);
		}
};

/// Specialisation of piranha::math::ipow_subs() for piranha::rational_function.
/**
 * This specialisation is enabled if \p T is an instance of piranha::rational_function.
 */
template <typename T, typename U>
struct ipow_subs_impl<T,U,typename std::enable_if<std::is_base_of<detail::rational_function_tag,T>::value>::type>
{
	private:
		template <typename V>
		using ipow_subs_type = decltype(std::declval<const T &>().ipow_subs(std::string{},integer{},
			std::declval<const V &>()));
	public:
		/// Call operator.
		/**
		 * \note
		 * The call operator is enabled only if <tt>return r.ipow_subs(s,n,x)</tt> is a valid expression.
		 *
		 * @param[in] r the piranha::rational_function argument.
		 * @param[in] s the name of the variable to be substituted.
		 * @param[in] n the power of \p s to be substituted.
		 * @param[in] x the substitution argument.
		 *
		 * @return <tt>r.ipow_subs(s,n,x)</tt>.
		 *
		 * @throws unspecified any exception thrown by piranha::rational_function::ipow_subs().
		 */
		template <typename V>
		ipow_subs_type<V> operator()(const T &r, const std::string &s, const integer &n, const V &x) const
		{
			return r.ipow_subs(s,n,x);
		}
};

/// Specialisation of the piranha::math::partial() functor for piranha::rational_function.
/**
 * This specialisation is activated when \p T is an instance of piranha::rational_function.
 */
template <typename T>
struct partial_impl<T,typename std::enable_if<std::is_base_of<detail::rational_function_tag,T>::value>::type>
{
	/// Call operator.
	/**
	 * The call operator will first check whether a custom partial derivative for \p T was registered
	 * via piranha::rational_function::register_custom_derivative(). In such a case, the custom derivative function will be used
	 * to compute the return value. Otherwise, the output of piranha::rational_function::partial() will be returned.
	 *
	 * @param[in] r input piranha::rational_function.
	 * @param[in] name name of the argument with respect to which the differentiation will be calculated.
	 *
	 * @return the partial derivative of \p r with respect to \p name.
	 *
	 * @throws unspecified any exception thrown by:
	 * - piranha::rational_function::partial(),
	 * - failure(s) in threading primitives,
	 * - lookup operations on \p std::unordered_map,
	 * - the copy assignment and call operators of the registered custom partial derivative function.
	 */
	T operator()(const T &r, const std::string &name) const
	{
		bool custom = false;
		std::function<T(const T &)> func;
		// Try to locate a custom partial derivative and copy it into func, if found.
		{
			std::lock_guard<std::mutex> lock(T::s_cp_mutex);
			auto it = T::get_cp_map().find(name);
			if (it != T::get_cp_map().end()) {
				func = it->second;
				custom = true;
			}
		}
		if (custom) {
			return func(r);
		}
		return r.partial(name);
	}
};

/// Specialisation of the piranha::math::integrate() functor for piranha::rational_function.
/**
 * This specialisation is enabled if \p T is an instance of piranha::rational_function.
 */
template <typename T>
struct integrate_impl<T,typename std::enable_if<std::is_base_of<detail::rational_function_tag,T>::value>::type>
{
	/// Call operator.
	/**
	 * The integration will be successful only if the denominator of \p r does not depend on the integration variable.
	 *
	 * @param[in] r piranha::rational_function argument.
	 * @param[in] name name of the integration variable.
	 *
	 * @return an antiderivative of \p r with respect to \p name.
	 *
	 * @throws std::invalid_argument if the denominator of \p r depends on the integration variable.
	 * @throws unspecified any exception thrown by:
	 * - piranha::math::integrate(),
	 * - the constructor of piranha::rational_function::q_type from piranha::rational_function::p_type,
	 * - the binary constructor of piranha::rational_function.
	 */
	T operator()(const T &r, const std::string &name)
	{
		if (!is_zero(degree(r.den(),{name}))) {
			piranha_throw(std::invalid_argument,"cannot compute the integral of a rational function whose "
				"denominator depends on the integration variable");
		}
		return T{integrate(typename T::q_type{r.num()},name),r.den()};
	}
};

/// Specialisation of the piranha::math::evaluate() functor for piranha::rational_function.
/**
 * This specialisation is enabled if \p T is an instance of piranha::rational_function.
 */
template <typename T>
struct evaluate_impl<T,typename std::enable_if<std::is_base_of<detail::rational_function_tag,T>::value>::type>
{
	private:
		// This is the real eval type.
		template <typename U, typename V>
		using eval_type_ = decltype(evaluate(std::declval<const U &>().num(),std::declval<const std::unordered_map<std::string,V> &>())
			/ evaluate(std::declval<const U &>().den(),std::declval<const std::unordered_map<std::string,V> &>()));
		template <typename U, typename V>
		using eval_type = typename std::enable_if<is_returnable<eval_type_<U,V>>::value,eval_type_<U,V>>::type;
	public:
		/// Call operator.
		/**
		 * \note
		 * This operator is enabled only if the algorithm described below is supported by the involved types,
		 * and it returns a type which satisfies piranha::is_returnable.
		 *
		 * The evaluation of a rational function is constructed from the ratio of the evaluation of its numerator
		 * by the evaluation of its denominator. The return type is the type resulting from this operation.
		 *
		 * @param[in] r the piranha::rational_function argument.
		 * @param[in] m the evaluation map.
		 *
		 * @return the evaluation of \p r according to \p m.
		 *
		 * @throws unspecified any exception thrown by:
		 * - the evaluation of the numerator and denominator of \p r,
		 * - the division of the results of the evaluations of numerator and denominator.
		 */
		template <typename U, typename V>
		eval_type<U,V> operator()(const U &r, const std::unordered_map<std::string,V> &m) const
		{
			return evaluate(r.num(),m) / evaluate(r.den(),m);
		}
};

/// Specialisation of the piranha::math::cos() functor for piranha::rational_function.
/**
 * This specialisation is enabled if \p T is an instance of piranha::rational_function.
 */
template <typename T>
struct cos_impl<T,typename std::enable_if<std::is_base_of<detail::rational_function_tag,T>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] r the piranha::rational_function argument.
	 *
	 * @return 1 if \p r is zero.
	 *
	 * @throws std::invalid_argument if \p r is not zero.
	 * @throws unspecified any exception thrown by the constructor of piranha::rational_function from \p int.
	 */
	T operator()(const T &r) const
	{
		if (!is_zero(r)) {
			piranha_throw(std::invalid_argument,"cannot compute the cosine of a nonzero rational function");
		}
		return T{1};
	}
};

/// Specialisation of the piranha::math::sin() functor for piranha::rational_function.
/**
 * This specialisation is enabled if \p T is an instance of piranha::rational_function.
 */
template <typename T>
struct sin_impl<T,typename std::enable_if<std::is_base_of<detail::rational_function_tag,T>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] r the piranha::rational_function argument.
	 *
	 * @return 0 if \p r is zero.
	 *
	 * @throws std::invalid_argument if \p r is not zero.
	 */
	T operator()(const T &r) const
	{
		if (!is_zero(r)) {
			piranha_throw(std::invalid_argument,"cannot compute the sine of a nonzero rational function");
		}
		return T{};
	}
};

/// Specialisation of the piranha::math::degree() functor for piranha::rational_function.
/**
 * This specialisation is enabled if \p T is an instance of piranha::rational_function.
 */
template <typename T>
struct degree_impl<T,typename std::enable_if<std::is_base_of<detail::rational_function_tag,T>::value>::type>
{
	/// Call operator.
	/**
	 * The (partial) degree of a rational function is defined as the maximum (partial)
	 * degree of numerator and denominator.
	 *
	 * @param[in] r the piranha::rational_function argument.
	 * @param[in] args either an empty pack, or a univariate pack containing a vector
	 * of strings representing the names of the variables with respect to which
	 * the partial degree is computed.
	 *
	 * @return the (partial) degree of \p r.
	 *
	 * @throws unspecified any exception thrown by the computation of the degrees
	 * of the numerator or denominator of \p r.
	 */
	template <typename ... Args>
	auto operator()(const T &r, const Args & ... args) const -> decltype(degree(r.num(),args...))
	{
		// NOTE: very important here, std::max returns a reference, so in the decltype()
		// above we must not use the std::max otherwise we will return a reference.
		return std::max(degree(r.num(),args...),degree(r.den(),args...));
	}
};

/// Specialisation of the piranha::math::divexact() functor for piranha::rational_function.
/**
 * This specialisation is enabled if \p T is an instance of piranha::rational_function.
 */
template <typename T>
struct divexact_impl<T,typename std::enable_if<std::is_base_of<detail::rational_function_tag,T>::value>::type>
{
	/// Call operator.
	/**
	 * This call operator is equivalent to:
	 * @code
	 * retval = n / d;
	 * @endcode
	 *
	 * @param[out] retval the result of exact division.
	 * @param[in] n the numerator.
	 * @param[in] d the denominator.
	 *
	 * @throws unspecified any exception thrown by piranha::rational_function::operator/().
	 */
	void operator()(T &retval, const T &n, const T &d) const
	{
		retval = n / d;
	}
};

}

namespace detail
{

template <typename T>
using sri_rf_enabler = typename std::enable_if<std::is_base_of<detail::rational_function_tag,
	typename std::decay<T>::type>::value>::type;

}

/// Specialisation of piranha::series_recursion_index for piranha::rational_function.
/**
 * This specialisation is enabled when \p T is an instance of piranha::rational_function.
 * Although piranha::rational_function is not a piranha::series, it is assigned a recursion index
 * of 1 in order to provide useful type coercion in a number of situations (e.g., adding
 * a Poisson series over rational functions to a polynomial).
 */
// NOTE: this was prompted by the necessity of allowing subs with generic objects in subs() and
// ipow_subs(). It seems to work ok and it does not seem to conflict with the general series operators
// (which is the only place where the recursion index is used), but we should tread carefully:
// rational_function is not a series and does not provide any term/cf/key types and the likes.
// The logic in the series operators is SFINAE friendly and does not error out due to the lack
// of these typedefs.
template <typename T>
class series_recursion_index<T,detail::sri_rf_enabler<T>>
{
	public:
		/// Value of the type trait.
		static const std::size_t value = 1u;
};

template <typename T>
const std::size_t series_recursion_index<T,detail::sri_rf_enabler<T>>::value;

}

#endif
