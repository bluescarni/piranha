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

#ifndef PIRANHA_DYNAMIC_KRONECKER_MONOMIAL_HPP
#define PIRANHA_DYNAMIC_KRONECKER_MONOMIAL_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <ostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include "config.hpp"
#include "detail/prepare_for_print.hpp"
#include "exceptions.hpp"
#include "kronecker_array.hpp"
#include "mp_integer.hpp"
#include "small_vector.hpp"
#include "safe_cast.hpp"
#include "static_vector.hpp"
#include "symbol_set.hpp"

namespace piranha
{

// TODOs:
// - check homomorphic property of hash

// TODO: document that here NBits includes the sign bit as well.
// NOTE: this class kind of looks like kronecker_monomial, but in many ways it is a different beast.
// The major difference is that here we know exactly how many values are packed in a big int. As a
// consequence, we don't need the reference symbol set as much as in kronecker_monomial, and we generally
// use it only as a consistency check (similarly to monomial.hpp).

/// Dynamic Kronecker monomial class.
/**
 * This class represents a sequence of signed integral values as a vector of packed integers. That is, each
 * element of the vector represents a set of signed integral values encoded via piranha::kronecker_array. The
 * \p NBits template parameter establishes approximately how many bits of the signed integral type are devoted
 * to each packed element (including sign bit), and, consequently, how many values can be packed inside a single
 * signed integer.
 *
 * For instance, on a common 64 bit architecture, when \p NBits is 8 then 64 / 8 = 8 values are packed inside each
 * signed integer.
 *
 * ## Type requirements ##
 *
 * - \p SignedInt must be usable as first template parameter in piranha::kronecker_array.
 * - \p NBits must satisfy the following requirements:
 *   - it must be less than an implementation-defined maximum,
 *   - it must be greater than zero and less than the bit width of \p SignedInt (including sign bit).
 */
template <typename SignedInt = std::make_signed<std::size_t>::type, int NBits = 8>
class dynamic_kronecker_monomial
{
		// Type requirements for use in kronecker_array.
		static_assert(detail::ka_type_reqs<SignedInt>::value,"This class can be used only with signed integers.");
		// Is it even possible otherwise?
		static_assert(std::numeric_limits<SignedInt>::digits < std::numeric_limits<SignedInt>::max(),"Overflow error.");
		// NOTE: here the +1 is to count the sign bit.
		static_assert(NBits > 0 && NBits <= std::numeric_limits<SignedInt>::digits + 1,"Invalid number of bits.");
		// How many values to pack inside each element of the small vector.
		static const unsigned ksize_ = static_cast<unsigned>((std::numeric_limits<SignedInt>::digits + 1) / NBits);
		static_assert(ksize_ > 0u,"Error in the computation of ksize.");
		// Turn it into a size_t for ease of use.
		static_assert(ksize_ <= std::numeric_limits<std::size_t>::max(),"Overflow error.");
	public:
		/// The number of values packed in each signed integer.
		static const std::size_t ksize = static_cast<std::size_t>(ksize_);
		/// Alias for \p SignedInt.
		using value_type = SignedInt;
	private:
		// This is a small static vector used for the decodification of a single signed integral.
		using k_type = static_vector<value_type,ksize>;
		// The underlying container.
		using container_type = small_vector<value_type>;
		// The kronecker_array class that will be used to encode/decode.
		using ka = kronecker_array<value_type>;
		// Enabler for ctor from init list.
		template <typename T>
		using init_list_enabler = typename std::enable_if<has_safe_cast<value_type,T>::value,int>::type;
		// This is the maximum size of the temp decoding vector, and essentially
		// the maximum possible size of a monomial (in terms of real values stored - the number of packed elements
		// will be this divided by ksize). It is a multiple of ksize.
		static const std::size_t max_size =
			static_cast<std::size_t>((ksize < 255u ? 255u / ksize : 1u) * ksize);
		// Implementation of construction from iterators.
		template <typename It>
		void construct_from_iterators(It begin, It end)
		{
			k_type tmp;
			for (; begin != end; ++begin) {
				if (tmp.size() == ksize) {
					m_vec.push_back(ka::encode(tmp));
					tmp.clear();
				}
				tmp.push_back(safe_cast<value_type>(*begin));
			}
			// If we have a partially filled tmp vector, we need to pad it
			// with zeroes and add it.
			if (tmp.size()) {
				while (tmp.size() != ksize) {
					tmp.push_back(value_type(0));
				}
				m_vec.push_back(ka::encode(tmp));
			}
			// Now we need to check if we pushed too many elements.
			if (unlikely(m_vec.size() > max_size / ksize)) {
				piranha_throw(std::invalid_argument,"too many elements in the construction "
					"of a dynamic_kronecker_monomial");
			}
		}
		// Utilities for hashing.
		// This is an array of randomly selected primes in the range provided by size_t.
		static const std::array<std::size_t,static_cast<std::size_t>(max_size/ksize)> hash_mixer;
		// This is the function used to build the above array at program startup.
		static std::array<std::size_t,static_cast<std::size_t>(max_size/ksize)> get_hash_mixer()
		{
			std::mt19937 rng;
			std::uniform_int_distribution<std::size_t> dist;
			std::array<std::size_t,static_cast<std::size_t>(max_size/ksize)> retval;
			for (auto it = retval.begin(); it != retval.end();) {
				// NOTE: the idea here is: pick a random number, get the next prime and
				// try to downcast it back to size_t. If this overflows, just try again.
				// I have a gut feeling in theory this could fail a lot if the bit width
				// of size_t is large enough, due to the way prime numbers distribute.
				// But it does not seem to be a problem in practice so far. Just keep it
				// in mind.
				auto tmp = integer{dist(rng)};
				tmp = tmp.nextprime();
				try {
					*it = static_cast<std::size_t>(tmp);
				} catch (const std::overflow_error &) {
					continue;
				}
				++it;
			}
			return retval;
		}
	public:
		using v_type = static_vector<value_type,max_size>;
		using size_type = typename container_type::size_type;
		dynamic_kronecker_monomial() = default;
		dynamic_kronecker_monomial(const dynamic_kronecker_monomial &) = default;
		dynamic_kronecker_monomial(dynamic_kronecker_monomial &&) = default;
		template <typename T, init_list_enabler<T> = 0>
		explicit dynamic_kronecker_monomial(std::initializer_list<T> list)
		{
			construct_from_iterators(list.begin(),list.end());
		}
		// TODO enabler.
		template <typename It>
		explicit dynamic_kronecker_monomial(It begin, It end)
		{
			construct_from_iterators(begin,end);
		}
		~dynamic_kronecker_monomial()
		{
			// Check that we never went past the size limit in m_vec.
			piranha_assert(m_vec.size() <= max_size / ksize);
		}
		dynamic_kronecker_monomial &operator=(const dynamic_kronecker_monomial &) = default;
		dynamic_kronecker_monomial &operator=(dynamic_kronecker_monomial &&) = default;
		v_type unpack(const symbol_set &args) const
		{
			const auto as = args.size();
			const auto s = m_vec.size();
			v_type retval;
			// Special casing when s is null.
			if (s == 0u) {
				if (unlikely(as != 0u)) {
					piranha_throw(std::invalid_argument,"incompatible symbol set");
				}
				return retval;
			}
			// NOTE: we know that these computations on the right-hand sides are ok:
			// - s is at least 1,
			// - the maximum value of s is such that ksize * s is computable.
			// This is to check that the size of args is consistent with the size of m_vec.
			// The number of arguments has to be in the interval [ksize * (s - 1) + 1,ksize * s].
			if (unlikely(as <= ksize * (s - 1u) || as > ksize * s)) {
				piranha_throw(std::invalid_argument,"incompatible symbol set");
			}
			k_type tmp(ksize,0);
			for (auto t = m_vec.size_begin_end(); std::get<1u>(t) != std::get<2u>(t); ++std::get<1u>(t)) {
				ka::decode(tmp,*std::get<1u>(t));
				std::copy(tmp.begin(),tmp.end(),std::back_inserter(retval));
			}
			piranha_assert(retval.size() >= args.size());
			// Last, we check that all elements not corresponding to any argument are zero.
			if (unlikely(!std::all_of(retval.begin() + args.size(),retval.end(),[](const value_type &n) {return n == 0;}))) {
				piranha_throw(std::invalid_argument,"incompatible symbol set");
			}
			return retval;
		}
		bool is_compatible(const symbol_set &args) const noexcept
		{
			// NOTE: this is a subset of the checks which are run
			// in unpack().
			const auto as = args.size();
			const auto s = m_vec.size();
			// If this is empty, args has to be empty as well.
			if (s == 0u) {
				return as == 0u;
			}
			// The number of args must be compatible with the size of m_vec.
			if (as <= ksize * (s - 1u) || as > ksize * s) {
				return false;
			}
			return true;
		}
		std::size_t hash() const
		{
			auto t = m_vec.size_begin_end();
			switch (std::get<0u>(t)) {
				case 0u:
					// The hash of zero is always zero.
					return 0u;
				case 1u:
					// With only one packed element, do as k_monomial does.
					return static_cast<std::size_t>(*std::get<1u>(t));
			}
			// In general, cast each element to size_t, multiply it by a random prime and
			// return the accumulated value.
			std::size_t retval = 0u;
			for (std::size_t i = 0u; std::get<1u>(t) != std::get<2u>(t); ++std::get<1u>(t), ++i) {
				retval = static_cast<std::size_t>(retval + static_cast<std::size_t>(*std::get<1u>(t)) *
					hash_mixer[i]);
			}
			return retval;
		}
		bool operator==(const dynamic_kronecker_monomial &other) const
		{
			return m_vec == other.m_vec;
		}
		bool operator!=(const dynamic_kronecker_monomial &other) const
		{
			return !operator==(other);
		}
		friend std::ostream &operator<<(std::ostream &os, const dynamic_kronecker_monomial &dkm)
		{
			os << '[';
			for (size_type i = 0u; i < dkm.m_vec.size(); ++i) {
				os << detail::prepare_for_print(dkm.m_vec[i]);
				if (i != dkm.m_vec.size() - 1u) {
					os << ',';
				}
			}
			os << ']';
			return os;
		}
	private:
		container_type m_vec;
};

template <typename SignedInt, int NBits>
const std::size_t dynamic_kronecker_monomial<SignedInt,NBits>::ksize;

// Static initialisation of the hash mixer.
template <typename SignedInt, int NBits>
const std::array<std::size_t,static_cast<std::size_t>(dynamic_kronecker_monomial<SignedInt,NBits>::max_size /
	dynamic_kronecker_monomial<SignedInt,NBits>::ksize)> dynamic_kronecker_monomial<SignedInt,NBits>::hash_mixer =
	dynamic_kronecker_monomial<SignedInt,NBits>::get_hash_mixer();

using dk_monomial = dynamic_kronecker_monomial<>;

}

namespace std
{

template <typename SignedInt, int NBits>
struct hash<piranha::dynamic_kronecker_monomial<SignedInt,NBits>>
{
	/// Result type.
	using result_type = size_t;
	/// Argument type.
	using argument_type = piranha::dynamic_kronecker_monomial<SignedInt,NBits>;
	result_type operator()(const argument_type &s) const
	{
		return s.hash();
	}
};

}

#endif
