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
#include <boost/utility.hpp> // For addressof.
#include <initializer_list>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "array_key.hpp"
#include "concepts/degree_key.hpp"
#include "config.hpp"
#include "integer.hpp"
#include "math.hpp"
#include "symbol_set.hpp"
#include "symbol.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Monomial class.
/**
 * This class extends piranha::array_key to define a series key type suitable as monomial in polynomial terms.
 * 
 * This class is a model of the piranha::concept::DegreeKey concept.
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
 * \todo think about introducing a monomial concept that embeds maybe the degreekey concept, if the need to treat generically the various
 * monomial classes arises.
 * \todo think about fixing printing when T is (unsigned) char: cast to int, otherwise the output will be nasty.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename T>
class monomial: public array_key<T,monomial<T>>
{
		typedef array_key<T,monomial<T>> base;
		// Eval and subs type definition.
		template <typename U>
		struct eval_type
		{
			typedef decltype(math::pow(std::declval<U>(),std::declval<typename base::value_type>())) type;
		};
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
			BOOST_CONCEPT_ASSERT((concept::DegreeKey<monomial>));
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
		 * Partial degree of the monomial: only the symbols with names in \p active_args are considered during the computation
		 * of the degree. Symbols in \p active_args not appearing in \p args are not considered.
		 * 
		 * @param[in] active_args names of the symbols that will be considered in the computation of the partial degree of the monomial.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return the summation of all the exponents of the monomial corresponding to the symbols in
		 * \p active_args, or <tt>value_type(0)</tt> if no symbols in \p active_args appear in \p args.
		 * 
		 * @throws std::invalid_argument if the sizes of \p args and \p this differ.
		 * @throws unspecified any exception thrown by the constructor and the addition and assignment operators of \p value_type.
		 */
		typename array_key<T,monomial<T>>::value_type degree(const std::set<std::string> &active_args, const symbol_set &args) const
		{
			typedef typename base::value_type value_type;
			if(unlikely(args.size() != this->size())) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			value_type retval(0);
			auto it1 = args.begin();
			auto it2 = active_args.begin();
			for (typename base::size_type i = 0u; i < this->size(); ++i, ++it1) {
				// Move forward the it2 iterator until it does not preceed the iterator in args,
				// or we run out of symbols.
				while (it2 != active_args.end() && *it2 < it1->get_name()) {
					++it2;
				}
				if (it2 == active_args.end()) {
					break;
				} else if (*it2 == it1->get_name()) {
					retval += (*this)[i];
				}
			}
			return retval;
		}
		/// Partial low degree.
		/**
		 * Analogous to the partial degree.
		 * 
		 * @param[in] active_args names of the symbols that will be considered in the computation of the partial low degree of the monomial.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return the output of degree().
		 * 
		 * @throws unspecified any exception thrown by degree().
		 */
		typename array_key<T,monomial<T>>::value_type ldegree(const std::set<std::string> &active_args, const symbol_set &args) const
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
		/// Name of the linear argument.
		/**
		 * If the monomial is linear in a variable (i.e., all exponents are zero apart from a single unitary
		 * exponent), the name of the variable will be returned. Otherwise, an error will be raised.
		 * 
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return name of the linear variable.
		 * 
		 * @throws std::invalid_argument if the monomial is not linear or if the sizes of \p args and \p this differ.
		 */
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
					tmp = math::integral_cast((*this)[i]);
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
		 * Will return a monomial corresponding to \p this raised to the <tt>x</tt>-th power. The exponentiation
		 * is computed via in-place multiplication of the exponents by \p x.
		 * 
		 * @param[in] x exponent.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return \p this to the power of \p x.
		 * 
		 * @throws std::invalid_argument if the sizes of \p args and \p this differ.
		 * @throws unspecified any exception thrown by monomial copy construction
		 * or in-place multiplication of exponents by \p x.
		 */
		template <typename U>
		monomial pow(const U &x, const symbol_set &args) const
		{
			typedef typename base::size_type size_type;
			if (!is_compatible(args)) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			monomial retval(*this);
			const size_type size = retval.size();
			for (size_type i = 0u; i < size; ++i) {
				retval[i] *= x;
			}
			return retval;
		}
		/// Partial derivative.
		/**
		 * Will return the partial derivative of \p this with respect to symbol \p s. The result is a pair
		 * consisting of the exponent associated to \p s before differentiation and the monomial itself
		 * after differentiation. If \p s is not in \p args or if the exponent associated to it is zero,
		 * the returned pair will be <tt>(0,monomial{})</tt>.
		 * 
		 * @param[in] s symbol with respect to which the differentiation will be calculated.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return result of the differentiation.
		 * 
		 * @throws std::invalid_argument if the sizes of \p args and \p this differ.
		 * @throws unspecified any exception thrown by:
		 * - piranha::math::is_zero(),
		 * - monomial and exponent construction,
		 * - the exponent type's subtraction operator.
		 */
		std::pair<typename base::value_type,monomial> partial(const symbol &s, const symbol_set &args) const
		{
			typedef typename base::size_type size_type;
			typedef typename base::value_type value_type;
			if (!is_compatible(args)) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			for (size_type i = 0u; i < args.size(); ++i) {
				if (args[i] == s && !math::is_zero((*this)[i])) {
					monomial tmp_m(*this);
					value_type tmp_v(tmp_m[i]);
					tmp_m[i] = tmp_m[i] - value_type(1);
					return std::make_pair(std::move(tmp_v),std::move(tmp_m));
				}
			}
			return std::make_pair(value_type(0),monomial());
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
			typedef typename base::value_type value_type;
			const value_type zero(0), one(1);
			for (typename base::size_type i = 0u; i < this->size(); ++i) {
				if ((*this)[i] != zero) {
					os << args[i].get_name();
					if ((*this)[i] != one) {
						os << "**" << (*this)[i];
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
			typedef typename base::value_type value_type;
			std::ostringstream oss_num, oss_den, *cur_oss;
			const value_type zero(0), one(1);
			value_type cur_value;
			for (typename base::size_type i = 0u; i < this->size(); ++i) {
				cur_value = (*this)[i];
				if (cur_value != zero) {
					cur_oss = (cur_value > zero) ? boost::addressof(oss_num) : (math::negate(cur_value),boost::addressof(oss_den));
					(*cur_oss) << "{" << args[i].get_name() << "}";
					if (cur_value != one) {
						(*cur_oss) << "^{" << cur_value << "}";
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
		 * The return value will be built by iteratively applying piranha::math::pow() using the values provided
		 * by \p dict as bases and the values in the monomial as exponents. If a symbol in \p args is not found
		 * in \p dict, an error will be raised. If the size of the monomial is zero, 1 will be returned.
		 * 
		 * @param[in] dict dictionary that will be used for substitution.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return the result of evaluating \p this with the values provided in \p dict.
		 * 
		 * @throws std::invalid_argument if the sizes of \p args and \p this differ, or if
		 * a symbol in \p args is not found in \p dict.
		 * @throws unspecified any exception thrown by:
		 * - construction of the return type,
		 * - lookup operations in \p std::unordered_map,
		 * - piranha::math::pow() or the in-place multiplication operator of the return type.
		 * 
		 * \todo request constructability from 1, multipliability and exponentiability.
		 */
		template <typename U>
		typename eval_type<U>::type evaluate(const std::unordered_map<symbol,U> &dict, const symbol_set &args) const
		{
			typedef typename eval_type<U>::type return_type;
			if (unlikely(args.size() != this->size())) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			return_type retval(1);
			const auto it_f = dict.end();
			for (typename base::size_type i = 0u; i < this->size(); ++i) {
				const auto it = dict.find(args[i]);
				if (it == it_f) {
					piranha_throw(std::invalid_argument,
						std::string("cannot evaluate monomial: symbol \'") + args[i].get_name() +
						"\' does not appear in dictionary");
				}
				retval *= math::pow(it->second,(*this)[i]);
			}
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
		 * \todo require constructability from int and exponentiability.
		 */
		template <typename U>
		std::pair<typename eval_type<U>::type,monomial> subs(const symbol &s, const U &x, const symbol_set &args) const
		{
			typedef typename eval_type<U>::type s_type;
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
