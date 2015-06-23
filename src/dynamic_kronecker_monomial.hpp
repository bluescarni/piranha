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
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <ostream>
#include <limits>
#include <tuple>
#include <type_traits>

#include "config.hpp"
#include "detail/prepare_for_print.hpp"
#include "exceptions.hpp"
#include "kronecker_array.hpp"
#include "small_vector.hpp"
#include "safe_cast.hpp"
#include "static_vector.hpp"
#include "symbol_set.hpp"

namespace piranha
{

// TODO: document that here NBits includes the sign bit as well.
// NOTE: this class kind of looks like kronecker_monomial, but in many ways it is a different beast.
// The major difference is that here we know exactly how many values are packed in a big int. As a
// consequence, we don't need the reference symbol set as much as in kronecker_monomial, and we generally
// use it only as a consistency check (similarly to monomial.hpp).
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
		static const std::size_t ksize = static_cast<std::size_t>(ksize_);
	public:
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
			// NOTE: candidate for begin_end() function from small_vector.
			for (size_type i = 0u; i < m_vec.size(); ++i) {
				ka::decode(tmp,m_vec[i]);
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
					return 0u;
				case 1u:
					return static_cast<std::size_t>(*std::get<1u>(t));
			}
			std::size_t retval = static_cast<std::size_t>(*std::get<1u>(t));
			++std::get<1u>(t);
			for (; std::get<1u>(t) != std::get<2u>(t); ++std::get<1u>(t)) {
				retval = static_cast<std::size_t>(retval + static_cast<std::size_t>(*std::get<1u>(t)));
			}
			return retval;
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

using dk_monomial = dynamic_kronecker_monomial<>;

}

#endif
