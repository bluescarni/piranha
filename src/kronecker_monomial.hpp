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

#ifndef PIRANHA_KRONECKER_MONOMIAL_HPP
#define PIRANHA_KRONECKER_MONOMIAL_HPP

#include <boost/integer_traits.hpp>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <vector>

#include "config.hpp"
#include "exceptions.hpp"
#include "integer.hpp"
#include "static_vector.hpp"
#include "symbol.hpp"

namespace piranha
{

template <typename SignedInteger = std::make_signed<std::size_t>::type>
class kronecker_monomial
{
	public:
		typedef SignedInteger int_type;
		typedef typename std::make_unsigned<int_type>::type uint_type;
	private:
		static_assert(std::is_signed<int_type>::value,"This class can be used only with signed integers.");
		static_assert(std::numeric_limits<int_type>::digits <= boost::integer_traits<int_type>::const_max,"Incompatible numerical limits.");
		static const int_type nbits = std::numeric_limits<int_type>::digits;
		// This is a 6-tuple of int_type built as follows:
		// 0. n (i.e., degree of the power-of-two),
		// 1. -(2**(n-1)) (lower limit),
		// 2. 2**(n-1) - 1 (upper limit),
		// 3. h_min,
		// 4. h_max,
		// 5. h_max - h_min.
		typedef std::tuple<int_type,int_type,int_type,int_type,int_type,int_type> limit_type;
		// Vector of limits (actual size will depend on the width and limits of int_type, never greater than nbits).
		typedef static_vector<limit_type,nbits> limits_type;
		static_assert(unsigned(nbits) <= boost::integer_traits<typename limits_type::size_type>::const_max,"Incompatible numerical limits.");
		static const limits_type log2_limits;
		// Determine limits for m-dimensional vectors.
		static limit_type determine_limit(const int_type &m)
		{
			auto f_h_min = [&m](const int_type &n) {return ((integer(2).pow(n).pow(m) - 1) * -integer(2).pow(n - int_type(1))) / (integer(2).pow(n) - 1);};
			auto f_h_max = [&m](const int_type &n) {return ((integer(2).pow(n).pow(m) - 1) * (integer(2).pow(n - int_type(1)) - 1)) / (integer(2).pow(n) - 1);};
			int_type n = 1;
			while (true) {
				integer h_min = f_h_min(n);
				integer h_max = f_h_max(n);
				integer diff = h_max - h_min;
				try {
					static_cast<int_type>(h_min);
					static_cast<int_type>(h_max);
					static_cast<int_type>(diff);
				} catch (const std::overflow_error &) {
					const int_type old_n = n - int_type(1);
					return std::make_tuple(
						old_n,
						old_n ? static_cast<int_type>(-(integer(2).pow(old_n - int_type(1)))) : int_type(0),
						old_n ? static_cast<int_type>(integer(2).pow(old_n - int_type(1)) - 1) : int_type(0),
						old_n ? static_cast<int_type>(f_h_min(old_n)) : int_type(0),
						old_n ? static_cast<int_type>(f_h_max(old_n)) : int_type(0),
						old_n ? static_cast<int_type>(f_h_max(old_n) - f_h_min(old_n)) : int_type(0)
					);
				}
				piranha_assert(n < boost::integer_traits<int_type>::const_max);
				++n;
			}
		}
		static limits_type determine_limits()
		{
			limits_type retval;
			retval.push_back(std::make_tuple(int_type(0),int_type(0),int_type(0),int_type(0),int_type(0),int_type(0)));
			for (int_type i = 1; i < nbits; ++i) {
				const auto tmp = determine_limit(i);
				if (std::get<0u>(tmp)) {
					retval.push_back(tmp);
				} else {
					break;
				}
			}
			return retval;
		}
	public:
		typedef typename limits_type::size_type size_type;
		kronecker_monomial():m_size(0u),m_value(0) {}
		kronecker_monomial(const kronecker_monomial &) = default;
		kronecker_monomial(kronecker_monomial &&) = default;
		explicit kronecker_monomial(const std::vector<symbol> &args):m_size(0u),m_value(0)
		{
			if (unlikely(args.size() >= log2_limits.size())) {
				piranha_throw(std::invalid_argument,"excessive number of symbols for Kronecker monomial");
			}
			static_vector<int_type,nbits> tmp(args.size(),int_type(0));
			m_value = encode(tmp);
			m_size = args.size();
		}
		~kronecker_monomial() = default;
		limits_type get_limits() const
		{
			return log2_limits;
		}
		template <typename Vector>
		static int_type encode(const Vector &v)
		{
			static_assert(std::is_same<typename Vector::value_type,int_type>::value,"Cannot encode vectors of different integer types.");
			const auto size = v.size();
			// NOTE: here the check is >= because indices in the limits vector correspond to the sizes of the vectors to be coded.
			if (unlikely(size >= log2_limits.size())) {
				piranha_throw(std::invalid_argument,"size of vector to be encoded is too large");
			}
			if (unlikely(!size)) {
				return int_type(0);
			}
			const auto &limit = log2_limits[size];
			// Check that the vector's components are compatible with the limits.
			for (size_type i = 0u; i < size; ++i) {
				if (unlikely(v[i] < std::get<1u>(limit) || v[i] > std::get<2u>(limit))) {
					piranha_throw(std::invalid_argument,"a component of the vector to be encoded is out of bounds");
				}
			}
			// NOTE: here we are sure this is valid because this quantity is always positive and less than h_max - h_min, which is representable.
			int_type retval = v[0u] - std::get<1u>(limit);
			auto shift = static_cast<uint_type>(std::get<0u>(limit));
			for (decltype(v.size()) i = 1u; i < size; ++i, shift += static_cast<uint_type>(std::get<0u>(limit))) {
				retval += (v[i] - std::get<1u>(limit)) * (int_type(1) << shift);
			}
			return retval + std::get<3u>(limit);
		}
		static static_vector<int_type,nbits> decode(const int_type &n, const size_type &m)
		{
			if (unlikely(m >= log2_limits.size())) {
				piranha_throw(std::invalid_argument,"size of vector to be decoded is too large");
			}
			static_vector<int_type,nbits> retval;
			if (unlikely(!m)) {
				if (unlikely(n)) {
					piranha_throw(std::invalid_argument,"a vector of size 0 must always be encoded as 0");
				}
				return retval;
			}
			const auto &limit = log2_limits[m];
			if (unlikely(n < std::get<3u>(limit) || n > std::get<4u>(limit))) {
				piranha_throw(std::invalid_argument,"the integer to be decoded is out of bounds");
			}
			piranha_assert(n >= std::get<3u>(limit));
			const auto code = static_cast<uint_type>(n - std::get<3u>(limit));
			auto shift = static_cast<uint_type>(std::get<0u>(limit));
			// Do the first value manually.
			const auto mod_arg = (uint_type(1u) << shift) - uint_type(1u);
			retval.push_back(static_cast<int_type>(code & mod_arg) + std::get<1u>(limit));
			for (size_type i = 1u; i < m; ++i, shift += static_cast<uint_type>(std::get<0u>(limit))) {
				const auto mod_arg = (uint_type(1u) << (shift + static_cast<uint_type>(std::get<0u>(limit)))) - uint_type(1u);
				retval.push_back(static_cast<int_type>((code & mod_arg) >> shift) + std::get<1u>(limit));
			}
			return retval;
		}
	private:
		size_type	m_size;
		int_type	m_value;
};

template <typename SignedInteger>
const typename kronecker_monomial<SignedInteger>::limits_type kronecker_monomial<SignedInteger>::log2_limits = kronecker_monomial<SignedInteger>::determine_limits();

}

#endif
