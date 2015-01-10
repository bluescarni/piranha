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
#include <functional>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "array_key.hpp"
#include "detail/prepare_for_print.hpp"
#include "config.hpp"
#include "forwarding.hpp"
#include "is_key.hpp"
#include "math.hpp"
#include "mp_integer.hpp"
#include "mp_rational.hpp"
#include "safe_cast.hpp"
#include "serialization.hpp"
#include "symbol_set.hpp"
#include "symbol.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Monomial class.
/**
 * This class extends piranha::array_key to define a series key type suitable as monomial in polynomial terms.
 * 
 * This class satisfies the piranha::is_key type trait.
 * 
 * ## Type requirements ##
 * 
 * \p T and \p S must be suitable for use as first and third template arguments in piranha::array_key. Additionally,
 * \p T must satisfy the piranha::has_is_zero type trait.
 * 
 * ## Exception safety guarantee ##
 * 
 * This class provides the same exception safety guarantee as piranha::array_key.
 * 
 * ## Move semantics ##
 * 
 * Move semantics is equivalent to piranha::array_key's move semantics.
 * 
 * ## Serialization ##
 *
 * This class supports serialization if the base class supports it.
 *
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
/*
 * \todo the linear argument method should not probably be conditionally enabled, as we rely on it for the
 * polynomial to poisson series stuff. Its requirements should become class requirements.
 */
template <typename T, typename S = std::integral_constant<std::size_t,0u>>
class monomial: public array_key<T,monomial<T,S>,S>
{
		PIRANHA_TT_CHECK(has_is_zero,T);
		using base = array_key<T,monomial<T,S>,S>;
		// Eval and subs type definition.
		template <typename U, typename = void>
		struct eval_type_ {};
		template <typename U>
		using e_type = decltype(math::pow(std::declval<U const &>(),std::declval<T const &>()));
		template <typename U>
		struct eval_type_<U,typename std::enable_if<is_multipliable_in_place<e_type<U>>::value &&
			std::is_constructible<e_type<U>,int>::value && detail::is_pmappable<U>::value>::type>
		{
			using type = e_type<U>;
		};
		template <typename U>
		using eval_type = typename eval_type_<U>::type;
		// Functors to shut off conversion warnings.
		template <typename U, typename = void>
		struct in_place_adder
		{
			void operator()(U &x, const U &y) const
			{
				x += y;
			}
		};
		template <typename U>
		struct in_place_adder<U,typename std::enable_if<std::is_integral<U>::value>::type>
		{
			void operator()(U &x, const U &y) const
			{
				x = static_cast<U>(x + y);
			}
		};
		// Enabler for ctor from init list.
		template <typename U>
		using init_list_enabler = typename std::enable_if<std::is_constructible<base,std::initializer_list<U>>::value,int>::type;
		// Enabler for multiplication.
		template <typename U>
		using multiply_enabler = decltype(std::declval<U const &>().vector_add(std::declval<U &>(),std::declval<U const &>()));
		// Enabler for linear argument.
		template <typename U>
		using linarg_enabler = typename std::enable_if<has_safe_cast<integer,U>::value,int>::type;
		// Enabler and machinery for pow.
		// When the exponent is an integral, promote to integer during exponentiation
		// for safe operations.
		template <typename U, typename std::enable_if<std::is_integral<U>::value,int>::type = 0>
		static integer get_pow_arg(const U &n)
		{
			return integer(n);
		}
		// Otherwise, just return the unchanged reference.
		template <typename U, typename std::enable_if<!std::is_integral<U>::value,int>::type = 0>
		static const U &get_pow_arg(const U &x)
		{
			return x;
		}
		using pow_type = typename std::conditional<std::is_integral<T>::value,integer &&,const T &>::type;
		template <typename U>
		using pow_enabler = typename std::enable_if<has_safe_cast<T,
			decltype(std::declval<pow_type>() * std::declval<const U &>())>::value,int>::type;
		// Machinery to determine the degree type.
		template <typename U>
		using add_type = decltype(std::declval<const U &>() + std::declval<const U &>());
		// No type defined in here, will sfinae out.
		template <typename U, typename = void>
		struct degree_type_
		{};
		template <typename U>
		struct degree_type_<U,typename std::enable_if<std::is_integral<U>::value>::type>
		{
			using type = integer;
		};
		template <typename U>
		struct degree_type_<U,typename std::enable_if<!std::is_integral<U>::value && std::is_constructible<add_type<U>,int>::value &&
			is_addable_in_place<add_type<U>,U>::value>::type>
		{
			using type = add_type<U>;
		};
		// The final alias.
		template <typename U>
		using degree_type = typename degree_type_<U>::type;
		// In-place decrement by one, checked for integral types.
		template <typename U, typename std::enable_if<std::is_integral<U>::value,int>::type = 0>
		static void ip_dec(U &x)
		{
			if (unlikely(x == std::numeric_limits<U>::min())) {
				piranha_throw(std::invalid_argument,"negative overflow error in the calculation of the "
					"partial derivative of a monomial");
			}
			x = static_cast<U>(x - U(1));
		}
		template <typename U, typename std::enable_if<!std::is_integral<U>::value,int>::type = 0>
		static void ip_dec(U &x)
		{
			x = x - U(1);
		}
		template <typename U>
		using partial_enabler = typename std::enable_if<std::is_assignable<U &,decltype(
			std::declval<U &>() - std::declval<U>())>::value,int>::type;
		// Serialization support.
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
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
		template <typename U, init_list_enabler<U> = 0>
		explicit monomial(std::initializer_list<U> list):base(list) {}
		PIRANHA_FORWARDING_CTOR(monomial,base)
		/// Trivial destructor.
		~monomial()
		{
			PIRANHA_TT_CHECK(is_key,monomial);
		}
		/// Defaulted copy assignment operator.
		monomial &operator=(const monomial &) = default;
		/// Defaulted move assignment operator.
		monomial &operator=(monomial &&) = default;
		/// Compatibility check
		/**
		 * A monomial and a set of arguments are compatible if their sizes coincide.
		 * 
		 * @param[in] args reference arguments set.
		 * 
		 * @return <tt>this->size() == args.size()</tt>.
		 */
		bool is_compatible(const symbol_set &args) const noexcept
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
		bool is_ignorable(const symbol_set &args) const noexcept
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
			if(unlikely(args.size() != this->size())) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			return std::all_of(this->begin(),this->end(),
				[](const T &element) {return math::is_zero(element);});
		}
		/// Degree.
		/**
		 * \note
		 * This method is enabled only if \p T is addable and the type resulting from the addition is constructible from \p int
		 * and monomial::value_type can be added in-place to it.
		 *
		 * This method will return the degree of the monomial, computed via the summation of the exponents of the monomial.
		 * If \p T is a C++ integral type, then the type of the degree will be piranha::integer. Otherwise, the type of the degree
		 * is the type resulting from the addition of the exponents.
		 *
		 * @param[in] args reference set of piranha::symbol.
		 *
		 * @return the degree of the monomial.
		 *
		 * @throws std::invalid_argument if the sizes of \p args and \p this differ.
		 * @throws unspecified any exception thrown by the invoked constructor or arithmetic operators.
		 */
		template <typename U = T>
		degree_type<U> degree(const symbol_set &args) const
		{
			// This is a fast check, better always to keep it.
			if (unlikely(args.size() != this->size())) {
				piranha_throw(std::invalid_argument,"invalid arguments set");
			}
			degree_type<U> retval(0);
			for (const auto &x: *this) {
				retval += x;
			}
			return retval;
		}
		/// Partial degree.
		/**
		 * \note
		 * This method is enabled only if \p T is addable and the type resulting from the addition is constructible from \p int
		 * and monomial::value_type can be added in-place to it.
		 *
		 * This method will return the partial degree of the monomial, computed via the summation of the exponents of the monomial.
		 * If \p T is a C++ integral type, then the type of the degree will be piranha::integer. Otherwise, the type of the degree
		 * is the type resulting from the addition of the exponents.
		 *
		 * The \p p argument is used to indicate which exponents are to be taken into account when computing the partial degree.
		 * Exponents not in \p p will be discarded during the computation of the partial degree.
		 *
		 * @param[in] p positions of the symbols to be considered.
		 * @param[in] args reference set of piranha::symbol.
		 *
		 * @return the partial degree of the monomial.
		 *
		 * @throws std::invalid_argument if the sizes of \p args and \p this differ, or if \p p is
		 * not compatible with the monomial.
		 * @throws unspecified any exception thrown by the invoked constructor or arithmetic operators.
		 */
		template <typename U = T>
		degree_type<U> degree(const symbol_set::positions &p, const symbol_set &args) const
		{
			if (unlikely(args.size() != this->size() || (p.size() && p.back() >= this->size()))) {
				piranha_throw(std::invalid_argument,"invalid arguments set or positions");
			}
			auto cit = this->begin();
			degree_type<U> retval(0);
			for (const auto &i: p) {
				retval += cit[i];
			}
			return retval;
		}
		/// Low degree (equivalent to the degree).
		template <typename U = T>
		degree_type<U> ldegree(const symbol_set &args) const
		{
			return degree(args);
		}
		/// Partial low degree (equivalent to the partial degree).
		template <typename U = T>
		degree_type<U> ldegree(const symbol_set::positions &p, const symbol_set &args) const
		{
			return degree(p,args);
		}
		/// Multiply monomial.
		/**
		 * \note
		 * This method is enabled only if the underlying call to piranha::array_key::vector_add(()
		 * is well-formed.
		 *
		 * Multiplies \p this by \p other and stores the result in \p retval. The exception safety
		 * guarantee is the same as for piranha::array_key::vector_add().
		 * 
		 * @param[out] retval return value.
		 * @param[in] other argument of multiplication.
		 * @param[in] args reference set of arguments.
		 * 
		 * @throws std::invalid_argument if the sizes of \p args and \p this differ.
		 * @throws unspecified any exception thrown by piranha::array_key::vector_add().
		 *
		 * @return the return value of piranha::array_key::vector_add().
		 */
		template <typename U = monomial, typename = multiply_enabler<U>>
		void multiply(monomial &retval, const monomial &other, const symbol_set &args) const
		{
			if(unlikely(other.size() != args.size())) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			this->vector_add(retval,other);
		}
		/// Name of the linear argument.
		/**
		 * \note
		 * This method is enabled only if the exponent type supports piranha::safe_cast() to piranha::integer.
		 *
		 * If the monomial is linear in a variable (i.e., all exponents are zero apart from a single unitary
		 * exponent), the name of the variable will be returned. Otherwise, an error will be raised.
		 * 
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return name of the linear variable.
		 * 
		 * @throws std::invalid_argument if the monomial is not linear or if the sizes of \p args and \p this differ.
		 */
		template <typename U = T, linarg_enabler<U> = 0>
		std::string linear_argument(const symbol_set &args) const
		{
			if (!is_compatible(args)) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			typedef typename base::size_type size_type;
			const size_type size = this->size();
			size_type n_linear = 0u, candidate = 0u;
			for (size_type i = 0u; i < size; ++i) {
				integer tmp;
				try {
					tmp = safe_cast<integer>((*this)[i]);
				} catch (const std::invalid_argument &) {
					piranha_throw(std::invalid_argument,"exponent is not an integer");
				}
				if (tmp == 0) {
					continue;
				}
				if (tmp != 1) {
					piranha_throw(std::invalid_argument,"exponent is not unitary");
				}
				candidate = i;
				++n_linear;
			}
			if (n_linear != 1u) {
				piranha_throw(std::invalid_argument,"monomial is not linear");
			}
			return args[candidate].get_name();
		}
		/// Monomial exponentiation.
		/**
		 * \note
		 * This method is enabled if the exponent type (or its promoted piranha::integer counterpart)
		 * is multipliable by \p U and the result type can be cast safely back to the exponent type.
		 *
		 * Will return a monomial corresponding to \p this raised to the <tt>x</tt>-th power. The exponentiation
		 * is computed via the multiplication of the exponents by \p x. If the exponent type is a C++
		 * integral type, each exponent will be promoted to piranha::integer before the exponentiation
		 * takes place.
		 * 
		 * @param[in] x exponent.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return \p this to the power of \p x.
		 * 
		 * @throws std::invalid_argument if the sizes of \p args and \p this differ.
		 * @throws unspecified any exception thrown by monomial copy construction
		 * or in-place multiplication of exponents by \p x, and by the invoked
		 * operations on piranha::integer, if any.
		 */
		template <typename U, pow_enabler<U> = 0>
		monomial pow(const U &x, const symbol_set &args) const
		{
			typedef typename base::size_type size_type;
			if (!is_compatible(args)) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			// Init with zeroes.
			monomial retval(args);
			const size_type size = retval.size();
			for (decltype(retval.size()) i = 0u; i < size; ++i) {
				retval[i] = safe_cast<T>(get_pow_arg((*this)[i]) * x);
			}
			return retval;
		}
		/// Partial derivative.
		/**
		 * \note
		 * This method is enabled only if the exponent type is subtractable and the result of the operation
		 * can be assigned back to the exponent type.
		 *
		 * This method will return the partial derivative of \p this with respect to the symbol at the position indicated by \p p.
		 * The result is a pair consisting of the exponent associated to \p p before differentiation and the monomial itself
		 * after differentiation. If \p p is empty or if the exponent associated to it is zero,
		 * the returned pair will be <tt>(0,monomial{args})</tt>.
		 *
		 * If the exponent type is an integral type, then the decrement-by-one operation on the affected exponent is checked
		 * for negative overflow.
		 * 
		 * @param[in] p position of the symbol with respect to which the differentiation will be calculated.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return result of the differentiation.
		 * 
		 * @throws std::invalid_argument if the sizes of \p args and \p this differ, if the size of \p p is
		 * greater than one, if the position specified by \p p is invalid or if the computation on integral exponents
		 * results in an overflow error.
		 * @throws unspecified any exception thrown by:
		 * - monomial and exponent construction,
		 * - the exponent type's subtraction operator,
		 * - piranha::math::is_zero().
		 */
		template <typename U = T, partial_enabler<U> = 0>
		std::pair<U,monomial> partial(const symbol_set::positions &p, const symbol_set &args) const
		{
			if (!is_compatible(args)) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			// Cannot take derivative wrt more than one variable, and the position of that variable
			// must be compatible with the monomial.
			if (p.size() > 1u || (p.size() == 1u && p.back() >= args.size())) {
				piranha_throw(std::invalid_argument,"invalid size of symbol_set::positions");
			}
			// Derivative wrt a variable not in the monomial: position is empty, or refers to a
			// variable with zero exponent.
			// NOTE: safe to take this->begin() here, as the checks on the positions above ensure
			// there is a valid position and hence the size must be not zero.
			if (!p.size() || math::is_zero(this->begin()[*p.begin()])) {
				return std::make_pair(U(0),monomial(args));
			}
			// Copy of the original monomial.
			monomial m(*this);
			auto m_b = m.begin();
			// Original exponent.
			U v(m_b[*p.begin()]);
			// Decrement the exponent in the monomial.
			ip_dec(m_b[*p.begin()]);
			return std::make_pair(std::move(v),std::move(m));
		}
		/// Integration.
		/**
		 * Will return the antiderivative of \p this with respect to symbol \p s. The result is a pair
		 * consisting of the exponent associated to \p s and the monomial itself
		 * after integration. If \p s is not in \p args, the returned monomial will have an extra exponent
		 * set to 1 in the same position \p s would have if it were added to \p args.
		 * 
		 * If the exponent corresponding to \p s is -1, an error will be produced.
		 * 
		 * @param[in] s symbol with respect to which the integration will be calculated.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return result of the integration.
		 * 
		 * @throws std::invalid_argument if the sizes of \p args and \p this differ
		 * or if the exponent associated to \p s is -1.
		 * @throws unspecified any exception thrown by:
		 * - piranha::math::is_zero(),
		 * - exponent construction,
		 * - push_back(),
		 * - the exponent type's addition and assignment operators.
		 */
		std::pair<T,monomial> integrate(const symbol &s, const symbol_set &args) const
		{
			typedef typename base::size_type size_type;
			if (!is_compatible(args)) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			monomial retval;
			T expo(0), one(1);
			in_place_adder<T> adder;
			for (size_type i = 0u; i < args.size(); ++i) {
				if (math::is_zero(expo) && s < args[i]) {
					// If we went past the position of s in args and still we
					// have not performed the integration, it means that we need to add
					// a new exponent.
					retval.push_back(one);
					expo = one;
				}
				retval.push_back((*this)[i]);
				if (args[i] == s) {
					// NOTE: here using i is safe: if retval gained an extra exponent in the condition above,
					// we are never going to land here as args[i] is at this point never going to be s.
					adder(retval[i],one);
					if (math::is_zero(retval[i])) {
						piranha_throw(std::invalid_argument,"unable to perform monomial integration: negative unitary exponent");
					}
					expo = retval[i];
				}
			}
			// If expo is still zero, it means we need to add a new exponent at the end.
			if (math::is_zero(expo)) {
				retval.push_back(one);
				expo = one;
			}
			return std::make_pair(std::move(expo),std::move(retval));
		}
		/// Print.
		/**
		 * Will print to stream a human-readable representation of the monomial.
		 * 
		 * @param[in] os target stream.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @throws std::invalid_argument if the sizes of \p args and \p this differ.
		 * @throws unspecified any exception resulting from:
		 * - construction of the exponent type from \p int,
		 * - comparison of exponents,
		 * - printing exponents to stream.
		 */
		void print(std::ostream &os, const symbol_set &args) const
		{
			if (unlikely(args.size() != this->size())) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			const T zero(0), one(1);
			bool empty_output = true;
			for (typename base::size_type i = 0u; i < this->size(); ++i) {
				if ((*this)[i] != zero) {
					// If we are going to print a symbol, and something has been printed before,
					// then we are going to place the multiplication sign.
					if (!empty_output) {
						os << '*';
					}
					os << args[i].get_name();
					empty_output = false;
					if ((*this)[i] != one) {
						os << "**" << detail::prepare_for_print((*this)[i]);
					}
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
		 * @throws std::invalid_argument if the sizes of \p args and \p this differ.
		 * @throws unspecified any exception resulting from:
		 * - construction, comparison and assignment of exponents,
		 * - piranha::math::negate(),
		 * - streaming to \p os.
		 */
		void print_tex(std::ostream &os, const symbol_set &args) const
		{
			if (unlikely(args.size() != this->size())) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			std::ostringstream oss_num, oss_den, *cur_oss;
			const T zero(0), one(1);
			T cur_value;
			for (typename base::size_type i = 0u; i < this->size(); ++i) {
				cur_value = (*this)[i];
				if (cur_value != zero) {
					cur_oss = (cur_value > zero) ? std::addressof(oss_num) : (math::negate(cur_value),std::addressof(oss_den));
					(*cur_oss) << "{" << args[i].get_name() << "}";
					if (cur_value != one) {
						(*cur_oss) << "^{" << detail::prepare_for_print(cur_value) << "}";
					}
				}
			}
			const std::string num_str = oss_num.str(), den_str = oss_den.str();
			if (!num_str.empty() && !den_str.empty()) {
				os << "\\frac{" << num_str << "}{" << den_str << "}";
			} else if (!num_str.empty() && den_str.empty()) {
				os << num_str;
			} else if (num_str.empty() && !den_str.empty()) {
				os << "\\frac{1}{" << den_str << "}";
			}
		}
		/// Evaluation.
		/**
		 * \note
		 * This method is available only if \p U satisfies the following requirements:
		 * - it can be used in piranha::symbol_set::positions_map,
		 * - it can be used in piranha::math::pow() with the monomial exponents as powers, yielding a type \p eval_type,
		 * - \p eval_type is constructible from \p int,
		 * - \p eval_type is multipliable in place.
		 *
		 * The return value will be built by iteratively applying piranha::math::pow() using the values provided
		 * by \p pmap as bases and the values in the monomial as exponents. If the size of the monomial is zero, 1 will be
		 * returned. If \p args is not compatible with \p this and \p pmap, or the positions in \p pmap do not reference
		 * only and all the exponents in the monomial, an error will be thrown.
		 *
		 * @param[in] pmap piranha::symbol_set::positions_map that will be used for substitution.
		 * @param[in] args reference set of piranha::symbol.
		 *
		 * @return the result of evaluating \p this with the values provided in \p pmap.
		 *
		 * @throws std::invalid_argument if there exist an incompatibility between \p this,
		 * \p args or \p pmap.
		 * @throws unspecified any exception thrown by:
		 * - construction of the return type,
		 * - piranha::math::pow() or the in-place multiplication operator of the return type.
		 */
		template <typename U>
		eval_type<U> evaluate(const symbol_set::positions_map<U> &pmap, const symbol_set &args) const
		{
			using return_type = eval_type<U>;
			using size_type = typename base::size_type;
			// NOTE: the positions map must have the same number of elements as this, and it must
			// be made of consecutive positions [0,size - 1].
			if (unlikely(pmap.size() != this->size() || (pmap.size() && pmap.back().first != pmap.size() - 1u))) {
				piranha_throw(std::invalid_argument,"invalid positions map for evaluation");
			}
			// Args must be compatible with this.
			if (unlikely(args.size() != this->size())) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			return_type retval(1);
			auto it = pmap.begin();
			for (size_type i = 0u; i < this->size(); ++i, ++it) {
				piranha_assert(it != pmap.end() && it->first == i);
				retval *= math::pow(it->second,(*this)[i]);
			}
			piranha_assert(it == pmap.end());
			return retval;
		}
		/// Substitution.
		/**
		 * Substitute the symbol \p s in the monomial with quantity \p x. The return value is a pair in which the first
		 * element is the result of substituting \p s with \p x (i.e., \p x raised to the power of the exponent corresponding
		 * to \p s), and the second element the monomial after the substitution has been performed (i.e., with the exponent
		 * corresponding to \p s removed). If \p s is not in \p args, the return value will be <tt>(1,this)</tt> (i.e., the
		 * monomial is unchanged and the substitution yields 1).
		 * 
		 * @param[in] s symbol that will be substituted.
		 * @param[in] x quantity that will be substituted in place of \p s.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return the result of substituting \p x for \p s.
		 * 
		 * @throws std::invalid_argument if the sizes of \p args and \p this differ.
		 * @throws unspecified any exception thrown by:
		 * - construction and assignment of the return value,
		 * - piranha::math::pow(),
		 * - piranha::array_key::push_back().
		 * 
		 * \todo review and check the requirements on type - should be the same as eval.
		 */
		template <typename U>
		std::pair<eval_type<U>,monomial> subs(const symbol &s, const U &x, const symbol_set &args) const
		{
			using s_type = eval_type<U>;
			if (unlikely(args.size() != this->size())) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			s_type retval_s(1);
			monomial retval_key;
			for (typename base::size_type i = 0u; i < this->size(); ++i) {
				if (args[i] == s) {
					retval_s = math::pow(x,(*this)[i]);
				} else {
					retval_key.push_back((*this)[i]);
				}
			}
			piranha_assert(retval_key.size() == this->size() || retval_key.size() == this->size() - 1u);
			return std::make_pair(std::move(retval_s),std::move(retval_key));
		}
		/// Substitution of integral power.
		/**
		 * Substitute \p s to the power of \p n with quantity \p x. The return value is a pair in which the first
		 * element is the result of the substitution, and the second element the monomial after the substitution has been performed.
		 * If \p s is not in \p args, the return value will be <tt>(1,this)</tt> (i.e., the
		 * monomial is unchanged and the substitution yields 1).
		 * 
		 * In order for the substitution to be successful, the exponent type must be convertible to piranha::integer
		 * via piranha::safe_cast(). The method will substitute also \p s to powers higher than \p n in absolute value.
		 * For instance, substitution of <tt>y**2</tt> with \p a in <tt>y**7</tt> will produce <tt>a**3 * y</tt>, and
		 * substitution of <tt>y**-2</tt> with \p a in <tt>y**-7</tt> will produce <tt>a**3 * y**-1</tt>.
		 * 
		 * Note that, contrary to normal substitution, this method will never eliminate an exponent from the monomial, even if the
		 * exponent becomes zero after substitution.
		 * 
		 * @param[in] s symbol that will be substituted.
		 * @param[in] n power of \p s that will be substituted.
		 * @param[in] x quantity that will be substituted in place of \p s to the power of \p n.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return the result of substituting \p x for \p s to the power of \p n.
		 * 
		 * @throws std::invalid_argument if the sizes of \p args and \p this differ.
		 * @throws unspecified any exception thrown by:
		 * - construction and assignment of the return value,
		 * - construction of piranha::rational,
		 * - piranha::safe_cast(),
		 * - piranha::math::pow(),
		 * - piranha::array_key::push_back(),
		 * - the in-place subtraction operator of the exponent type.
		 * 
		 * \todo require constructability from int, exponentiability, subtractability, safe_cast.
		 */
		template <typename U>
		std::pair<eval_type<U>,monomial> ipow_subs(const symbol &s, const integer &n, const U &x, const symbol_set &args) const
		{
			using s_type = eval_type<U>;
			if (unlikely(args.size() != this->size())) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			s_type retval_s(1);
			monomial retval_key;
			for (typename base::size_type i = 0u; i < this->size(); ++i) {
				retval_key.push_back((*this)[i]);
				if (args[i] == s) {
					const rational tmp(safe_cast<integer>((*this)[i]),n);
					if (tmp >= 1) {
						const auto tmp_t = static_cast<integer>(tmp);
						retval_s = math::pow(x,tmp_t);
						retval_key[i] -= tmp_t * n;
					}
				}
			}
			return std::make_pair(std::move(retval_s),std::move(retval_key));
		}
};

}

namespace std
{

/// Specialisation of \p std::hash for piranha::monomial.
/**
 * Functionally equivalent to the \p std::hash specialisation for piranha::array_key.
 */
template <typename T, typename S>
struct hash<piranha::monomial<T,S>>: public hash<piranha::array_key<T,piranha::monomial<T,S>,S>> {};

}

#endif
