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

#include <algorithm>
#include <boost/numeric/conversion/cast.hpp>
#include <cstddef>
#include <iterator>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <vector>

#include "config.hpp"
#include "exceptions.hpp"
#include "mp_integer.hpp"
#include "type_traits.hpp"

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
 */
template <typename SignedInteger>
class kronecker_array
{
	public:
		/// Signed integer type used for encoding.
		typedef SignedInteger int_type;
	private:
		static_assert(std::is_integral<int_type>::value && std::is_signed<int_type>::value &&
			std::numeric_limits<int_type>::is_bounded,"This class can be used only with bounded signed integers.");
		// This is a 4-tuple of int_type built as follows:
		// 0. vector of absolute values of the upper/lower limit for each component,
		// 1. h_min,
		// 2. h_max,
		// 3. h_max - h_min.
		typedef std::tuple<std::vector<int_type>,int_type,int_type,int_type> limit_type;
		// Vector of limits.
		typedef std::vector<limit_type> limits_type;
	public:
		/// Size type.
		/**
		 * Equivalent to \p std::size_t, used to represent the
		 * dimension of the vectors on which the class can operate.
		 */
		typedef std::size_t size_type;
	private:
		// Static vector of limits built at startup.
		// NOTE: here we should not have problems when interoperating with libraries that modify the GMP allocation functions,
		// as we do not store any static piranha::integer: the creation and destruction of integer objects is confined to the determine_limit()
		// function.
		static const limits_type m_limits;
		// Determine limits for m-dimensional vectors.
		// NOTE: when reasoning about this, keep in mind that this is not a completely generic
		// codification: min/max vectors are negative/positive and symmetric. This makes it easy
		// to reason about overflows during (de)codification of vectors, representability of the
		// quantities involved, etc.
		static limit_type determine_limit(const size_type &m)
		{
			piranha_assert(m >= 1u);
			std::mt19937 engine(static_cast<unsigned long>(m));
			std::uniform_int_distribution<int> dist(-5,5);
			// Perturb integer value: add random quantity and then take next prime.
			auto perturb = [&engine,&dist] (integer &arg) {
				arg += (dist(engine) * (arg)) / 100;
				arg = arg.nextprime();
			};
			// Build initial minmax and coding vectors: all elements in the [-1,1] range.
			std::vector<integer> m_vec, M_vec, c_vec, prev_c_vec, prev_m_vec, prev_M_vec;
			c_vec.push_back(integer(1));
			m_vec.push_back(integer(-1));
			M_vec.push_back(integer(1));
			for (size_type i = 1u; i < m; ++i) {
				m_vec.push_back(integer(-1));
				M_vec.push_back(integer(1));
				c_vec.push_back(c_vec.back() * integer(3));
			}
			// Functor for scalar product of two vectors.
			auto dot_prod = [](const std::vector<integer> &v1, const std::vector<integer> &v2) -> integer {
				piranha_assert(v1.size() && v1.size() == v2.size());
				return std::inner_product(v1.begin(),v1.end(),v2.begin(),integer(0));
			};
			while (true) {
				// Compute the current h_min/max and diff.
				integer h_min = dot_prod(c_vec,m_vec);
				integer h_max = dot_prod(c_vec,M_vec);
				integer diff = h_max - h_min;
				piranha_assert(diff >= 0);
				try {
					// Try to cast everything to hardware integers.
					(void)static_cast<int_type>(h_min);
					(void)static_cast<int_type>(h_max);
					// Here it is +1 because h_max - h_min must be strictly less than the maximum value
					// of int_type. In the paper, in eq. (7), the Delta_i product appearing in the
					// decoding of the last component of a vector is equal to (h_max - h_min + 1) so we need
					// to be able to represent it.
					(void)static_cast<int_type>(diff + 1);
					// NOTE: we do not need to cast the individual elements of m/M vecs, as the representability
					// of h_min/max ensures the representability of m/M as well.
				} catch (const std::overflow_error &) {
					std::vector<int_type> tmp;
					// Check if we are at the first iteration.
					if (prev_c_vec.size()) {
						const auto h_min = dot_prod(prev_c_vec,prev_m_vec), h_max = dot_prod(prev_c_vec,prev_M_vec);
						std::transform(prev_M_vec.begin(),prev_M_vec.end(),std::back_inserter(tmp),[](const integer &n) {
							return static_cast<int_type>(n);
						});
						return std::make_tuple(
							tmp,
							static_cast<int_type>(h_min),
							static_cast<int_type>(h_max),
							static_cast<int_type>(h_max - h_min)
						);
					} else {
						// Here it means m variables are too many, and we stopped at the first iteration
						// of the cycle. Return tuple filled with zeroes.
						return std::make_tuple(tmp,int_type(0),int_type(0),int_type(0));
					}
				}
				// Store old vectors.
				prev_c_vec = c_vec;
				prev_m_vec = m_vec;
				prev_M_vec = M_vec;
				// Generate new coding vector for next iteration.
				auto it = c_vec.begin() + 1, prev_it = prev_c_vec.begin();
				for (; it != c_vec.end(); ++it, ++prev_it) {
					// Recover original delta.
					*it /= *prev_it;
					// Multiply by two and perturb.
					*it *= 2;
					perturb(*it);
					// Multiply by the new accumulated delta product.
					*it *= *(it - 1);
				}
				// Fill in the minmax vectors, apart from the last component.
				it = c_vec.begin() + 1;
				piranha_assert(M_vec.size() && M_vec.size() == m_vec.size());
				for (size_type i = 0u; i < M_vec.size() - 1u; ++i, ++it) {
					M_vec[i] = ((*it) / *(it - 1) - 1) / 2;
					m_vec[i] = -M_vec[i];
				}
				// We need to generate the last interval, which does not appear in the coding vector.
				// Take the previous interval and enlarge it so that the corresponding delta is increased by a
				// perturbed factor of 2.
				M_vec.back() = (4 * M_vec.back() + 1) / 2;
				perturb(M_vec.back());
				m_vec.back() = -M_vec.back();
			}
		}
		static limits_type determine_limits()
		{
			limits_type retval;
			retval.push_back(std::make_tuple(std::vector<int_type>{},int_type(0),int_type(0),int_type(0)));
			for (size_type i = 1u; ; ++i) {
				const auto tmp = determine_limit(i);
				if (std::get<0u>(tmp).empty()) {
					break;
				} else {
					retval.push_back(tmp);
				}
			}
			return retval;
		}
	public:
		/// Get the limits of the Kronecker codification.
		/**
		 * Will return a const reference to an \p std::vector of tuples describing the limits for the Kronecker
		 * codification of arrays of integer. The indices in this vector correspond to the dimension of the array to be encoded,
		 * so that the object at index \f$i\f$ in the returned vector describes the limits for the codification of \f$i\f$-dimensional arrays
		 * of integers.
		 * 
		 * Each element of the returned vector is an \p std::tuple of 4 elements built as follows:
		 * 
		 * - position 0: a vector containing the absolute value of the lower/upper bounds for each component,
		 * - position 1: \f$h_\textnormal{min}\f$, the minimum value for the integer encoding the array,
		 * - position 2: \f$h_\textnormal{max}\f$, the maximum value for the integer encoding the array,
		 * - position 3: \f$h_\textnormal{max}-h_\textnormal{min}\f$.
		 * 
		 * The tuple at index 0 of the returned vector is filled with zeroes. The size of the returned vector determines the maximum
		 * dimension of the vectors to be encoded.
		 * 
		 * @return const reference to an \p std::vector of limits for the Kronecker codification of arrays of integers.
		 */
		static const limits_type &get_limits()
		{
			return m_limits;
		}
		/// Encode vector.
		/**
		 * \note
		 * This method can be called only if \p Vector is a type with a vector-like interface.
		 * Specifically, it must have a <tt>size()</tt> method and overloaded const index operator.
		 *
		 * Encode input vector \p v into an instance of \p SignedInteger. If the value type of \p Vector
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
			// NOTE: here the check is >= because indices in the limits vector correspond to the sizes of the vectors to be encoded.
			if (unlikely(size >= m_limits.size())) {
				piranha_throw(std::invalid_argument,"size of vector to be encoded is too large");
			}
			if (unlikely(!size)) {
				return int_type(0);
			}
			// Cache quantities.
			const auto &limit = m_limits[size];
			const auto &minmax_vec = std::get<0u>(limit);
			// Check that the vector's components are compatible with the limits.
			// NOTE: here size is not greater than m_limits.size(), which in turn is compatible with the minmax vectors.
			for (min_int<decltype(v.size()),decltype(minmax_vec.size())> i = 0u; i < size; ++i) {
				if (unlikely(boost::numeric_cast<int_type>(v[i]) < -minmax_vec[i] || boost::numeric_cast<int_type>(v[i]) > minmax_vec[i])) {
					piranha_throw(std::invalid_argument,"a component of the vector to be encoded is out of bounds");
				}
			}
			piranha_assert(minmax_vec[0u] > 0);
			int_type retval = static_cast<int_type>(boost::numeric_cast<int_type>(v[0u]) + minmax_vec[0u]),
				cur_c = static_cast<int_type>(2 * minmax_vec[0u] + 1);
			piranha_assert(retval >= 0);
			for (decltype(v.size()) i = 1u; i < size; ++i) {
				retval = static_cast<int_type>(retval + ((boost::numeric_cast<int_type>(v[i]) + minmax_vec[i]) * cur_c));
				piranha_assert(minmax_vec[i] > 0);
				cur_c = static_cast<int_type>(cur_c * (2 * minmax_vec[i] + 1));
			}
			return static_cast<int_type>(retval + std::get<1u>(limit));
		}
		/// Decode into vector.
		/**
		 * \note
		 * This method can be called only if \p Vector is a type with a vector-like interface.
		 * Specifically, it must have a <tt>size()</tt> method and overloaded const index operator.
		 *
		 * Decode input code \p n into \p retval. If the value type of \p Vector
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
				if (unlikely(n != 0)) {
					piranha_throw(std::invalid_argument,"a vector of size 0 must always be encoded as 0");
				}
				return;
			}
			// Cache values.
			const auto &limit = m_limits[m];
			const auto &minmax_vec = std::get<0u>(limit);
			const auto hmin = std::get<1u>(limit), hmax = std::get<2u>(limit);
			if (unlikely(n < hmin || n > hmax)) {
				piranha_throw(std::invalid_argument,"the integer to be decoded is out of bounds");
			}
			// NOTE: the static_cast here is useful when working with int_type == char. In that case,
			// the binary operation on the RHS produces an int (due to integer promotion rules), which gets
			// assigned back to char causing the compiler to complain about potentially lossy conversion.
			const int_type code = static_cast<int_type>(n - hmin);
			piranha_assert(code >= 0);
			piranha_assert(minmax_vec[0u] > 0);
			int_type mod_arg = static_cast<int_type>(2 * minmax_vec[0u] + 1);
			// Do the first value manually.
			retval[0u] = boost::numeric_cast<v_type>((code % mod_arg) - minmax_vec[0u]);
			for (min_int<typename Vector::size_type,decltype(minmax_vec.size())> i = 1u; i < m; ++i) {
				piranha_assert(minmax_vec[i] > 0);
				retval[i] = boost::numeric_cast<v_type>((code % (mod_arg * (2 * minmax_vec[i] + 1))) / mod_arg - minmax_vec[i]);
				mod_arg = static_cast<int_type>(mod_arg * (2 * minmax_vec[i] + 1));
			}
		}
};

// Static initialization.
template <typename SignedInteger>
const typename kronecker_array<SignedInteger>::limits_type kronecker_array<SignedInteger>::m_limits = kronecker_array<SignedInteger>::determine_limits();

}

#endif
