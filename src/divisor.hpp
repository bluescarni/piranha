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

#ifndef PIRANHA_DIVISOR_HPP
#define PIRANHA_DIVISOR_HPP

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "config.hpp"
#include "detail/gcd.hpp"
#include "detail/prepare_for_print.hpp"
#include "detail/vector_merge_args.hpp"
#include "exceptions.hpp"
#include "hash_set.hpp"
#include "is_key.hpp"
#include "mp_integer.hpp"
#include "safe_cast.hpp"
#include "serialization.hpp"
#include "small_vector.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Divisor class.
/**
 * This class is used to represent keys of the form
 * \f[
 * \prod_j\frac{1}{\left(a_{0,j}x_0+a_{1,j}x_1+\ldots+a_{n,j}x_n\right)^{e_j}},
 * \f]
 * where \f$ a_{i,j} \f$ are integers, \f$ x_i \f$ are symbols, and \f$ e_j \f$ are positive integers. The type
 * of \f$ a_{i,j} \f$ and \f$ e_j \f$ is \p T. The terms of the product are stored in a piranha::hash_set and
 * they are guaranteed to be in a canonical form defined by the following properties:
 * - the values of \f$ a_{i,j} \f$ and \f$ e_j \f$ are within an implementation-defined range,
 * - \f$ e_j \f$ is always strictly positive,
 * - the first nonzero \f$ a_{i,j} \f$ in each term is positive,
 * - the \f$ a_{i,j} \f$ in each term have no non-unitary common divisor.
 *
 * ## Type requirements ##
 *
 * \p T must be either a C++ integral type or piranha::mp_integer.
 *
 * ## Exception safety guarantee ##
 *
 * Unless otherwise specified, this class provides the strong exception safety guarantee for all operations.
 *
 * ## Move semantics ##
 *
 * Move semantics is equivalent to the move semantics of piranha::hash_set.
 *
 * ## Serialization ##
 *
 * This class supports serialization.
 */
template <typename T = short>
class divisor
{
		static_assert((std::is_signed<T>::value && std::is_integral<T>::value) || detail::is_mp_integer<T>::value,
			"The value type must be a signed integer or an mp_integer");
	public:
		/// Alias for \p T.
		using value_type = T;
	private:
		using v_type = small_vector<value_type>;
		// Pair vector-exponent, with the exponent made mutable.
		struct p_type
		{
			p_type() = default;
			p_type(const p_type &) = default;
			p_type(p_type &&) = default;
			explicit p_type(const v_type &v_, const value_type &e_):v(v_),e(e_) {}
			explicit p_type(v_type &&v_, value_type &&e_):v(std::move(v_)),e(std::move(e_)) {}
			~p_type() = default;
			p_type &operator=(const p_type &) = default;
			p_type &operator=(p_type &&) = default;
			bool operator==(const p_type &other) const
			{
				return v == other.v;
			}
			// Serialization support.
			template <class Archive>
			void serialize(Archive &ar, unsigned int)
			{
				// NOTE: there's no need here for exception safety, as this is not a public class: we
				// don't care if during deserialization we end up deserializing only part of the pair.
				ar & v;
				ar & e;
			}
			// Members.
			v_type			v;
			mutable value_type	e;
		};
		// Hasher for the pair type.
		struct p_type_hasher
		{
			std::size_t operator()(const p_type &p) const
			{
				return p.v.hash();
			}
		};
		// Underlying container type.
		using container_type = hash_set<p_type,p_type_hasher>;
		// Canonical term: the first nonzero element is positive and all the gcd of all elements is 1 or -1.
		// NOTE: this also includes the check for all zero elements, as gcd(0,0,...,0) = 0.
		static bool term_is_canonical(const p_type &p)
		{
			bool first_nonzero_found = false;
			value_type cd(0);
			for (const auto &n: p.v) {
				if (!first_nonzero_found && !math::is_zero(n)) {
					if (n < 0) {
						return false;
					}
					first_nonzero_found = true;
				}
				// NOTE: gcd(0,n) == n (or +-n, in our case) for all n, zero included.
				cd = detail::gcd(cd,n);
			}
			if (cd != 1 && cd != -1) {
				return false;
			}
			return true;
		}
		// Range check on a term - meaningful only if T is a C++ integral type.
		template <typename U = T, typename std::enable_if<std::is_integral<U>::value,int>::type = 0>
		static bool term_range_check(const p_type &p)
		{
			return std::all_of(p.v.begin(),p.v.end(),[](const value_type &x) {
				return x >= -detail::safe_abs_sint<value_type>::value &&
					x <= detail::safe_abs_sint<value_type>::value;
			});
		}
		template <typename U = T, typename std::enable_if<!std::is_integral<U>::value,int>::type = 0>
		static bool term_range_check(const p_type &)
		{
			return true;
		}
		bool destruction_checks() const
		{
			const auto it_f = m_container.end();
			auto it = m_container.begin();
			const typename v_type::size_type v_size = (it == it_f) ? 0u : it->v.size();
			for (; it != it_f; ++it) {
				// Check: the exponent must be greater than zero.
				if (it->e <= 0) {
					return false;
				}
				// Check: range.
				if (!term_range_check(*it)) {
					return false;
				}
				// Check: canonical.
				if (!term_is_canonical(*it)) {
					return false;
				}
				// Check: all vectors have the same size.
				if (it->v.size() != v_size) {
					return false;
				}
			}
			return true;
		}
		// Insertion machinery.
		template <typename Term>
		void insertion_impl(Term &&term)
		{
			// Handle the case of a table with no buckets.
			if (unlikely(!m_container.bucket_count())) {
				m_container._increase_size();
			}
			// Try to locate the term.
			auto bucket_idx = m_container._bucket(term);
			const auto it = m_container._find(term,bucket_idx);
			if (it == m_container.end()) {
				// New term.
				if (unlikely(m_container.size() == std::numeric_limits<size_type>::max())) {
					piranha_throw(std::overflow_error,"maximum number of elements reached");
				}
				// Term is new. Handle the case in which we need to rehash because of load factor.
				if (unlikely(static_cast<double>(m_container.size() + size_type(1u)) / static_cast<double>(m_container.bucket_count()) >
					m_container.max_load_factor()))
				{
					m_container._increase_size();
					// We need a new bucket index in case of a rehash.
					bucket_idx = m_container._bucket(term);
				}
				// Actually perform the insertion and finish by updating the size.
				m_container._unique_insert(std::forward<Term>(term),bucket_idx);
				m_container._update_size(m_container.size() + size_type(1u));
			} else {
				// Existing term - update the exponent.
				update_exponent(it->e,term.e);
			}
		}
		template <typename U = T, typename std::enable_if<std::is_integral<U>::value,int>::type = 0>
		static void update_exponent(value_type &a, const value_type &b)
		{
			// Check the range when the exponent is an integral type.
			piranha_assert(a > 0);
			piranha_assert(b > 0);
			// NOTE: this is safe as we require b to be a positive value.
			if (unlikely(a > std::numeric_limits<value_type>::max() - b)) {
				piranha_throw(std::invalid_argument,"overflow in the computation of the exponent of a divisor term");
			}
			a = static_cast<value_type>(a + b);
		}
		template <typename U = T, typename std::enable_if<!std::is_integral<U>::value,int>::type = 0>
		static void update_exponent(value_type &a, const value_type &b)
		{
			// For integer exponents, just do normal addition.
			a += b;
		}
		// Enabler for insertion.
		template <typename It, typename Exponent>
		using insert_enabler = typename std::enable_if<is_input_iterator<It>::value &&
			has_safe_cast<value_type,typename std::iterator_traits<It>::value_type>::value &&
			has_safe_cast<value_type,Exponent>::value,int>::type;
		// Serialization support.
		friend class boost::serialization::access;
		template <class Archive>
		void save(Archive &ar, unsigned int) const
		{
			ar & m_container;
		}
		template <class Archive>
		void load(Archive &ar, unsigned int)
		{
			divisor new_d;
			try {
				// NOTE: here we could throw either because the archive is garbage (Boost.Serialization throwing
				// in this case) or because the loaded terms are invalid. In the second case we must make sure
				// to destroy the content of new_d before exiting, otherwise the dtor of the divisor will
				// hit assertion failures in debug mode. We need the second clear() in the catch block
				// because we could be in the situation that the archive contains invaild terms and it is garbage
				// after a while - in thie case invalid terms coud be living in new_d and the first clear() is never
				// hit.
				ar & new_d.m_container;
				// Run the destruction checks, if they fail throw.
				if (unlikely(!new_d.destruction_checks())) {
					new_d.clear();
					piranha_throw(std::invalid_argument,"error during the deserialization of a divisor, the loaded data "
						"is invalid");
				}
			} catch (...) {
				new_d.clear();
				throw;
			}
			// Move in.
			*this = std::move(new_d);
		}
		BOOST_SERIALIZATION_SPLIT_MEMBER()
	public:
		/// Size type.
		/**
		 * It corresponds to the size type of the internal container.
		 */
		using size_type = typename container_type::size_type;
		/// Defaulted default constructor.
		/**
		 * This constructor will initialise an empty divisor.
		 */
		divisor() = default;
		/// Defaulted copy constructor.
		divisor(const divisor &) = default;
		/// Defaulted move constructor.
		divisor(divisor &&) = default;
		/// Converting constructor.
		/**
		 * This constructor is used in the generic constructor of piranha::series. It is equivalent
		 * to a copy constructor with extra checking.
		 *
		 * @param[in] other construction argument.
		 * @param[in] args reference symbol set.
		 *
		 * @throws std::invalid_argument if \p other is not compatible with \p args.
		 * @throws unspecified any exception thrown by the copy constructor.
		 */
		explicit divisor(const divisor &other, const symbol_set &args):m_container(other.m_container)
		{
			if (unlikely(!is_compatible(args))) {
				piranha_throw(std::invalid_argument,"the constructed divisor is incompatible with the "
					"input symbol set");
			}
		}
		/// Constructor from piranha::symbol_set.
		/**
		 * Equivalent to the default constructor.
		 */
		explicit divisor(const symbol_set &) {}
		/// Trivial destructor.
		~divisor()
		{
			piranha_assert(destruction_checks());
			PIRANHA_TT_CHECK(is_key,divisor);
		}
		/// Defaulted copy assignment operator.
		divisor &operator=(const divisor &) = default;
		/// Defaulted move assignment operator.
		divisor &operator=(divisor &&) = default;
		/// Create and insert a term from range and exponent.
		/**
		 * \note
		 * This method is enabled only if:
		 * - \p It is an input iterator,
		 * - the value type of \p It can be safely cast to piranha::divisor::value_type,
		 * - \p Exponent can be safely cast to piranha::divisor::value_type.
		 *
		 * This method will construct and insert a term into the divisor. The elements
		 * in the range <tt>[begin,end)</tt> will be used to construct the \f$ a_{i,j} \f$ of the term, while \p e
		 * will be used to construct the exponent (after a call to piranha::safe_cast()).
		 * If no term with the same set of \f$ a_{i,j} \f$ exists, then
		 * a new term will be inserted; otherwise, \p e will be added to the exponent of the existing term.
		 *
		 * @param[in] begin start of the range of \f$ a_{i,j} \f$.
		 * @param[in] end end of the range of \f$ a_{i,j} \f$.
		 * @param[in] e exponent.
		 *
		 * @throws std::invalid_argument if the term to be inserted is not in canonical form.
		 * @throws std::overflow_error if the insertion results in the container to be resized over
		 * an implementation-defined limit.
		 * @throws unspecified any exception thrown by:
		 * - piranha::safe_cast(),
		 * - manipulations of piranha::small_vector,
		 * - the public interface of piranha::hash_set,
		 * - arithmetic operations on the exponent.
		 */
		template <typename It, typename Exponent, insert_enabler<It,Exponent> = 0>
		void insert(It begin, It end, const Exponent &e)
		{
			p_type term;
			// Assign the exponent.
			term.e = safe_cast<value_type>(e);
			if (unlikely(term.e <= 0)) {
				piranha_throw(std::invalid_argument,"a term of a divisor must have a positive exponent");
			}
			// Build the vector to be inserted.
			for (; begin != end; ++begin) {
				term.v.push_back(safe_cast<value_type>(*begin));
			}
			// Range check.
			if (unlikely(!term_range_check(term))) {
				piranha_throw(std::invalid_argument,"an element in a term of a divisor is outside the allowed range");
			}
			// Check that the term is canonical.
			if (unlikely(!term_is_canonical(term))) {
				piranha_throw(std::invalid_argument,"term not in canonical form");
			}
			// Perform the insertion.
			insertion_impl(std::move(term));
		}
		/// Size.
		/**
		 * @return the size of the internal container - that is, the number of terms in the product.
		 */
		size_type size() const
		{
			return m_container.size();
		}
		/// Clear.
		/**
		 * This method will remove all terms from the divisor.
		 */
		void clear()
		{
			m_container.clear();
		}
		/// Equality operator.
		/**
		 * Two divisors are considered equal if:
		 * - they have the same number of terms, and
		 * - for each term in the first divisor there exist an identical term in the
		 *   second divisor.
		 *
		 * @param[in] other comparison argument.
		 *
		 * @return \p true if \p this is equal to \p other, \p false otherwise.
		 */
		bool operator==(const divisor &other) const
		{
			if (size() != other.size()) {
				return false;
			}
			const auto it_f_x = m_container.end(), it_f_y = other.m_container.end();
			for (auto it = m_container.begin(); it != it_f_x; ++it) {
				const auto tmp_it = other.m_container.find(*it);
				if (tmp_it == it_f_y || tmp_it->e != it->e) {
					return false;
				}
			}
			return true;
		}
		/// Inequality operator.
		/**
		 * @param[in] other comparison argument.
		 *
		 * @return the opposite of operator==().
		 */
		bool operator!=(const divisor &other) const
		{
			return !((*this) == other);
		}
		/// Hash value.
		/**
		 * The hash value is computed by combining the hash values of all terms. An empty divisor
		 * has a hash value of 0. Two equal divisors have the same hash value.
		 *
		 * @return a hash value for the divisor.
		 */
		std::size_t hash() const
		{
			std::size_t retval = 0u;
			p_type_hasher hasher;
			const auto it_f = m_container.end();
			for (auto it = m_container.begin(); it != it_f; ++it) {
				retval += hasher(*it);
			}
			return retval;
		}
		/// Compatibility check.
		/**
		 * An empty divisor is considered compatible with any set of symbols. Otherwise, a non-empty
		 * divisor is compatible if the number of variables in the terms is the same as the number
		 * of symbols in \p args.
		 *
		 * @param[in] args reference symbol set.
		 *
		 * @return \p true if \p this is compatible with \p args, \p false otherwise.
		 */
		bool is_compatible(const symbol_set &args) const noexcept
		{
			if (m_container.empty()) {
				return true;
			}
			return m_container.begin()->v.size() == args.size();
		}
		/// Ignorability check.
		/**
		 * A divisor is never considered ignorable (an empty divisor is equal to 1).
		 *
		 * @return \p false.
		 */
		bool is_ignorable(const symbol_set &) const noexcept
		{
			return false;
		}
		/// Check if divisor is unitary.
		/**
		 * Only an empty divisor is considered unitary.
		 *
		 * @param[in] args reference symbol set.
		 *
		 * @return \p true if \p this is empty, \p false otherwise.
		 *
		 * @throws std::invalid_argument if the number of variables in the divisor is different
		 * from the size of \p args.
		 */
		bool is_unitary(const symbol_set &args) const
		{
			if (m_container.empty()) {
				return true;
			}
			if (unlikely(m_container.begin()->v.size() != args.size())) {
				piranha_throw(std::invalid_argument,"invalid arguments set");
			}
			return false;
		}
		/// Merge arguments.
		/**
		 * This method will merge the new arguments set \p new_args into \p this, given the current reference arguments set
		 * \p orig_args. Arguments in \p new_args not appearing in \p orig_args will be inserted in the terms,
		 * with the corresponding \f$ a_{i,j} \f$ values constructed from the integral constant 0.
		 *
		 * @param[in] orig_args current reference arguments set for \p this.
		 * @param[in] new_args new arguments set.
		 *
		 * @return a divisor resulting from merging \p new_args into \p this.
		 *
		 * @throws std::invalid_argument in the following cases:
		 * - the number of variables in the terms of \p this is different from the size of \p orig_args,
		 * - the size of \p new_args is not greater than the size of \p orig_args,
		 * - not all elements of \p orig_args are included in \p new_args.
		 * @throws unspecified any exception thrown by:
		 * - piranha::small_vector::push_back(),
		 * - the copy assignment of piranha::divisor::value_type,
		 * - piranha::hash_set::insert().
		 */
		divisor merge_args(const symbol_set &orig_args, const symbol_set &new_args) const
		{
			divisor retval;
			const auto it_f = m_container.end();
			p_type tmp;
			for (auto it = m_container.begin(); it != it_f; ++it) {
				tmp.v = detail::vector_merge_args(it->v,orig_args,new_args);
				tmp.e = it->e;
				auto ret = retval.m_container.insert(std::move(tmp));
				(void)ret;
				piranha_assert(ret.first != retval.m_container.end());
				piranha_assert(ret.second);
			}
			return retval;
		}
		/// Print to stream.
		/**
		 * This method will print to the stream \p os a text representation of \p this.
		 *
		 * @param[in] os target stream.
		 * @param[in] args reference symbol set.
		 *
		 * @throws std::invalid_argument if the number of terms of \p this is different from the size of \p args.
		 * @throws unspecified any exception thrown by printing to \p os piranha::divisor::value_type, strings or characters.
		 */
		void print(std::ostream &os, const symbol_set &args) const
		{
			// Don't print anything if there are no terms.
			if (m_container.empty()) {
				return;
			}
			if (unlikely(m_container.begin()->v.size() != args.size())) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			const auto it_f = m_container.end();
			bool first_term = true;
			os << "1/[";
			for (auto it = m_container.begin(); it != it_f; ++it) {
				// If this is not the first term, print a leading '*' operator.
				if (first_term) {
					first_term = false;
				} else {
					os << '*';
				}
				bool printed_something = false;
				os << '(';
				for (typename v_type::size_type i = 0u; i < it->v.size(); ++i) {
					// If the aij is zero, don't print anything.
					if (math::is_zero(it->v[i])) {
						continue;
					}
					// A positive aij, in case previous output exists, must be preceded
					// by a "+" sign.
					if (it->v[i] > 0 && printed_something) {
						os << '+';
					}
					// Print the aij, unless it's "-1": in that case, just print the minus sign.
					if (it->v[i] == -1) {
						os << '-';
					} else if (it->v[i] != 1) {
						os << detail::prepare_for_print(it->v[i]) << '*';
					}
					// Finally, print name of variable.
					os << args[i].get_name();
					printed_something = true;
				}
				os << ')';
				// Print the exponent, if different from one.
				if (it->e != 1) {
					os << "**" << detail::prepare_for_print(it->e);
				}
			}
			os << ']';
		}
		/// Print to stream in TeX mode.
		/**
		 * This method will print to the stream \p os a TeX representation of \p this.
		 *
		 * @param[in] os target stream.
		 * @param[in] args reference symbol set.
		 *
		 * @throws std::invalid_argument if the number of terms of \p this is different from the size of \p args.
		 * @throws unspecified any exception thrown by printing to \p os piranha::divisor::value_type, strings or characters.
		 */
		void print_tex(std::ostream &os, const symbol_set &args) const
		{
			// Don't print anything if there are no terms.
			if (m_container.empty()) {
				return;
			}
			if (unlikely(m_container.begin()->v.size() != args.size())) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			const auto it_f = m_container.end();
			os << "\\frac{1}{";
			for (auto it = m_container.begin(); it != it_f; ++it) {
				bool printed_something = false;
				os << "\\left(";
				for (typename v_type::size_type i = 0u; i < it->v.size(); ++i) {
					// If the aij is zero, don't print anything.
					if (math::is_zero(it->v[i])) {
						continue;
					}
					// A positive aij, in case previous output exists, must be preceded
					// by a "+" sign.
					if (it->v[i] > 0 && printed_something) {
						os << '+';
					}
					// Print the aij, unless it's "-1": in that case, just print the minus sign.
					if (it->v[i] == -1) {
						os << '-';
					} else if (it->v[i] != 1) {
						os << detail::prepare_for_print(it->v[i]);
					}
					// Finally, print name of variable.
					os << args[i].get_name();
					printed_something = true;
				}
				os << "\\right)";
				// Print the exponent, if different from one.
				if (it->e != 1) {
					os << "^{" << detail::prepare_for_print(it->e) << "}";
				}
			}
			os << '}';
		}
	private:
		container_type m_container;
};

}

namespace std
{

template <typename T>
struct hash<piranha::divisor<T>>
{
	/// Result type.
	typedef size_t result_type;
	/// Argument type.
	typedef piranha::divisor<T> argument_type;
	/// Hash operator.
	/**
	 * @param[in] a piranha::divisor whose hash value will be returned.
	 *
	 * @return piranha::divisor::hash().
	 */
	result_type operator()(const argument_type &a) const
	{
		return a.hash();
	}
};

}

#endif
