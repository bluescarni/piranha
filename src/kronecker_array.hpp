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

#ifndef PIRANHA_KRONECKER_ARRAY_HPP
#define PIRANHA_KRONECKER_ARRAY_HPP

#include <boost/integer_traits.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include "config.hpp"
#include "exceptions.hpp"
#include "integer.hpp"
#include "static_vector.hpp"

namespace piranha
{

/// Kronecker array.
/**
 * This class offers static methods to encode (and decode) arrays of integral values as instances of \p SignedInteger type,
 * using a technique known as "Kronecker substitution".
 * 
 * Depending on the bit width and numerical limits of \p SignedInteger, the class will be able to operate on vectors of integers up to a certain
 * dimension and within certain bounds on the vector's components. Such limits can be queried with the get_limits() static method.
 * 
 * \section type_requirements Type requirements
 * 
 * \p SignedInteger must be a C++ signed integral type with finite bounds.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * Unless otherwise specified, this class provides the strong exception safety guarantee for all operations.
 * 
 * \section move_semantics Move semantics
 * 
 * This class does not have any non-static data members, hence it has trivial move semantics.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo confirm experimentally that boost numeric_cast does not influence performance.
 */
template <typename SignedInteger>
class kronecker_array
{
	public:
		/// Signed integer type used for encoding.
		typedef SignedInteger int_type;
	private:
		static_assert(std::is_signed<int_type>::value && std::numeric_limits<int_type>::is_bounded,"This class can be used only with bounded signed integers.");
		static_assert(std::numeric_limits<int_type>::digits <= boost::integer_traits<int_type>::const_max,"Incompatible numerical limits.");
		static const int_type nbits = std::numeric_limits<int_type>::digits;
		// This is a 5-tuple of int_type built as follows:
		// 0. lower limit of each component,
		// 1. upper limit of each component,
		// 2. h_min,
		// 3. h_max,
		// 4. h_max - h_min.
		typedef std::tuple<int_type,int_type,int_type,int_type,int_type> limit_type;
		// Vector of limits (actual size will depend on the width and limits of int_type, never greater than nbits).
		typedef static_vector<limit_type,unsigned(nbits)> limits_type;
		static_assert(unsigned(nbits) <= boost::integer_traits<typename limits_type::size_type>::const_max,"Incompatible numerical limits.");
		// Static vector of limits built at startup.
		static const limits_type m_limits;
		// Determine limits for m-dimensional vectors.
		static limit_type determine_limit(const int_type &m)
		{
			piranha_assert(m >= 1);
			auto f_h_min = [&m](const integer &n) {
				return (-n * (1 - (2 * n).pow(m))) / (1 - 2 * n);
			};
			auto f_h_max = [&m](const integer &n) {
				return (n - 1) * (1 - (2 * n).pow(m)) / (1 - 2 * n);
			};
			integer log2(0), cur_n(1), prev_n(0);
			while (true) {
				integer h_min = f_h_min(cur_n);
				integer h_max = f_h_max(cur_n);
				integer diff = h_max - h_min;
				piranha_assert(diff >= 0);
				try {
					static_cast<int_type>(h_min);
					static_cast<int_type>(h_max);
					// Here it is +1 because h_max - h_min must be strictly less than the maximum value
					// of int_type - see paper.
					static_cast<int_type>(diff + 1);
				} catch (const std::overflow_error &) {
					const int_type n_min = static_cast<int_type>(-prev_n), n_max = static_cast<int_type>(prev_n - 1);
					if (n_min < n_max) {
						// Condition for which m-variate representation is viable.
						return std::make_tuple(
							n_min,
							n_max,
							static_cast<int_type>(f_h_min(prev_n)),
							static_cast<int_type>(f_h_max(prev_n)),
							static_cast<int_type>(f_h_max(prev_n) - f_h_min(prev_n))
						);
					} else {
						// Here it means m variables are too many, and we stopped at the first iteration
						// of the cycle. Return tuple filled with zeroes.
						return std::make_tuple(int_type(0),int_type(0),int_type(0),int_type(0),int_type(0));
					}
				}
				++log2;
				prev_n = cur_n;
				// Take as cur_n the middle point between powers of 2.
				cur_n = (integer(2).pow(log2) + integer(2).pow(log2 + 1)) / 2;
				// If it is not definitely a prime, take the next prime.
				if (cur_n.probab_prime_p() != 2) {
					cur_n = cur_n.nextprime();
				}
			}
		}
		static limits_type determine_limits()
		{
			limits_type retval;
			retval.push_back(std::make_tuple(int_type(0),int_type(0),int_type(0),int_type(0),int_type(0)));
			for (int_type i = 1; i < nbits; ++i) {
				const auto tmp = determine_limit(i);
				if (std::get<0u>(tmp) == 0 && std::get<1u>(tmp) == 0) {
					break;
				} else {
					retval.push_back(tmp);
				}
			}
			return retval;
		}
	public:
		/// Size type.
		/**
		 * Unisigned integer type equivalent to the size type of piranha::static_vector. Used to represent the
		 * dimension of the vectors on which the class can operate.
		 */
		typedef typename limits_type::size_type size_type;
		/// Get the limits of the Kronecker codification.
		/**
		 * Will return a const reference to a piranha::static_vector of tuples describing the limits for the Kronecker
		 * codification of arrays of integer. The indices in this vector correspond to the dimension of the array to be encoded,
		 * so that the object at index \f$i\f$ in the returned vector describes the limits for the codification of \f$i\f$-dimensional arrays
		 * of integers.
		 * 
		 * Each element of the returned vector is an \p std::tuple of 5 \p SignedInteger instances built as follows:
		 * 
		 * - position 0: the lower bound of the array's components,
		 * - position 1: the upper bound of the array's components,
		 * - position 2: \f$h_\textnormal{min}\f$, the minimum value for the integer encoding the array,
		 * - position 3: \f$h_\textnormal{max}\f$, the maximum value for the integer encoding the array,
		 * - position 4: \f$h_\textnormal{max}-h_\textnormal{min}\f$.
		 * 
		 * The tuple at index 0 of the returned vector is filled with zeroes. The size of the returned vector determines the maximum
		 * dimension of the vectors to be encoded.
		 * 
		 * @return const reference to a piranha::static_vector of limits for the kronecker codification of arrays of integers.
		 */
		static const limits_type &get_limits()
		{
			return m_limits;
		}
		/// Encode vector.
		/**
		 * Encode input vector \p v into an instance of \p SignedInteger. \p Vector must be a type with a vector-like interface.
		 * Specifically, it must have a <tt>size()</tt> method and overloaded const index operator. If the value type of \p Vector
		 * is not \p SignedInteger, the values of \p v will be converted to \p SignedInteger using <tt>boost::numeric_cast</tt>.
		 * A vector of size 0 is always encoded as 0.
		 * 
		 * @param[in] v vector to be encoded.
		 * 
		 * @return \p v encoded as a \p SignedInteger using Kronecker substitution.
		 * 
		 * @throws std::invalid_argument if any of these conditions hold:
		 * - the size of \p v is equal to or greater than the size of the output of get_limits(),
		 * - one of the components of \p v is outside the bounds reported by get_limits().
		 * @throws unspecified any exception thrown by <tt>boost::numeric_cast</tt> in case the value type of
		 * \p Vector is not \p SignedInteger. 
		 */
		template <typename Vector>
		static int_type encode(const Vector &v)
		{
			const auto size = v.size();
			// NOTE: here the check is >= because indices in the limits vector correspond to the sizes of the vectors to be coded.
			if (unlikely(size >= m_limits.size())) {
				piranha_throw(std::invalid_argument,"size of vector to be encoded is too large");
			}
			if (unlikely(!size)) {
				return int_type(0);
			}
			// Cache quantities.
			const auto &limit = m_limits[size];
			const int_type emin = std::get<0u>(limit), emax = std::get<1u>(limit), hmin = std::get<2u>(limit), delta = (emax - emin) + 1;
			int_type mult = delta;
			// Check that the vector's components are compatible with the limits.
			for (size_type i = 0u; i < size; ++i) {
				if (unlikely(boost::numeric_cast<int_type>(v[i]) < emin || boost::numeric_cast<int_type>(v[i]) > emax)) {
					piranha_throw(std::invalid_argument,"a component of the vector to be encoded is out of bounds");
				}
			}
			int_type retval = boost::numeric_cast<int_type>(v[0u]) - emin;
			piranha_assert(retval >= 0);
			piranha_assert(delta > 0);
			for (decltype(v.size()) i = 1u; i < size; ++i, mult *= delta) {
				retval += (boost::numeric_cast<int_type>(v[i]) - emin) * mult;
			}
			return retval + hmin;
		}
		/// Decode into vector.
		/**
		 * Decode input code \p n into \p retval. \p Vector must be a type with a vector-like interface.
		 * Specifically, it must have a <tt>size()</tt> method and overloaded mutable index operator.
		 * If the value type of \p Vector
		 * is not \p SignedInteger, the components decoded from \p n will be converted to the value type of \p Vector
		 * using <tt>boost::numeric_cast</tt>.
		 * 
		 * In case of exceptions, \p retval will be left in a valid but undefined state.
		 * 
		 * @param[out] retval object that will store the decoded vector.
		 * @param[in] n code to be decoded.
		 * 
		 * @throws std::invalid_argument if any of these conditions hold:
		 * - the size of \p retval is equal to or greater than the size of the output of get_limits(),
		 * - the size of \p retval is zero and \p n is not zero,
		 * - \p n is out of the allowed bounds reported by get_limits().
		 * @throws unspecified any exception thrown by <tt>boost::numeric_cast</tt> in case the value type of
		 * \p Vector is not \p SignedInteger.
		 */
		template <typename Vector>
		static void decode(Vector &retval, const int_type &n)
		{
			typedef typename Vector::value_type v_type;
			const auto m = retval.size();
			if (unlikely(m >= m_limits.size())) {
				piranha_throw(std::invalid_argument,"size of vector to be decoded is too large");
			}
			if (unlikely(!m)) {
				if (unlikely(n)) {
					piranha_throw(std::invalid_argument,"a vector of size 0 must always be encoded as 0");
				}
				return;
			}
			// Cache values.
			const auto &limit = m_limits[m];
			const int_type emin = std::get<0u>(limit), emax = std::get<1u>(limit), hmin = std::get<2u>(limit),
				hmax = std::get<3u>(limit), delta = (emax - emin) + 1;
			int_type mod_arg = delta;
			if (unlikely(n < hmin || n > hmax)) {
				piranha_throw(std::invalid_argument,"the integer to be decoded is out of bounds");
			}
			const int_type code = n - hmin;
			piranha_assert(code >= 0);
			piranha_assert(delta > 0);
			// Do the first value manually.
			retval[0u] = boost::numeric_cast<v_type>((code % mod_arg) + emin);
			for (size_type i = 1u; i < m; ++i, mod_arg *= delta) {
				retval[i] = boost::numeric_cast<v_type>((code % (mod_arg * delta)) / mod_arg + emin);
			}
		}
};

template <typename SignedInteger>
const typename kronecker_array<SignedInteger>::limits_type kronecker_array<SignedInteger>::m_limits = kronecker_array<SignedInteger>::determine_limits();

}

#endif
