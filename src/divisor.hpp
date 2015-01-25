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
#include <iterator>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "config.hpp"
#include "detail/gcd.hpp"
#include "exceptions.hpp"
#include "hash_set.hpp"
#include "mp_integer.hpp"
#include "safe_cast.hpp"
#include "serialization.hpp"
#include "small_vector.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"

namespace piranha
{

template <typename T = short>
class divisor
{
		static_assert((std::is_signed<T>::value && std::is_integral<T>::value) || detail::is_mp_integer<T>::value,
			"The value type must be a signed integer or an mp_integer");
	public:
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
				// hit assertion failures in debug mode.
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
		using size_type = typename container_type::size_type;
		divisor() = default;
		divisor(const divisor &) = default;
		divisor(divisor &&) = default;
		explicit divisor(const divisor &other, const symbol_set &args):m_container(other.m_container)
		{
			if (unlikely(!is_compatible(args))) {
				piranha_throw(std::invalid_argument,"the constructed divisor is incompatible with the "
					"input symbol set");
			}
		}
		explicit divisor(const symbol_set &) {}
		~divisor()
		{
			piranha_assert(destruction_checks());
		}
		divisor &operator=(const divisor &) = default;
		divisor &operator=(divisor &&) = default;
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
		size_type size() const
		{
			return m_container.size();
		}
		void clear()
		{
			m_container.clear();
		}
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
		bool operator!=(const divisor &other) const
		{
			return !((*this) == other);
		}
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
		bool is_compatible(const symbol_set &args) const noexcept
		{
			if (m_container.empty()) {
				return true;
			}
			return m_container.begin()->v.size() == args.size();
		}
		bool is_ignorable(const symbol_set &) const noexcept
		{
			return false;
		}
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
