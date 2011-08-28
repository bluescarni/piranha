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

#include <algorithm>
// #include <boost/concept/assert.hpp>
#include <boost/integer_traits.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <vector>
// 
// #include "array_key.hpp"
// #include "concepts/key.hpp"
#include "config.hpp"
#include "kronecker_array.hpp"
// #include "math.hpp"
#include "static_vector.hpp"
#include "symbol.hpp"
// #include "type_traits.hpp"

namespace piranha
{

/// Kronecker monomial class.
template <typename T = std::make_signed<std::size_t>::type>
class kronecker_monomial
{
		typedef T int_type;
		typedef kronecker_array<int_type> ka;
		typedef typename std::decay<decltype(ka::get_limits())>::type _v_type;
		// Vector type used for temporary packing/unpacking.
		typedef static_vector<int_type,_v_type::max_size> v_type;
	public:
		typedef typename ka::size_type size_type;
		kronecker_monomial():m_value(0) {}
		kronecker_monomial(const kronecker_monomial &) = default;
		kronecker_monomial(kronecker_monomial &&) = default;
		template <typename U>
		explicit kronecker_monomial(std::initializer_list<U> list):m_value(0)
		{
			v_type tmp;
			for (auto it = list.begin(); it != list.end(); ++it) {
				tmp.push_back(boost::numeric_cast<int_type>(*it));
			}
			m_value = ka::encode(tmp);
		}
		explicit kronecker_monomial(const std::vector<symbol> &):m_value(0) {}
		~kronecker_monomial() = default;
		kronecker_monomial &operator=(const kronecker_monomial &) = default;
		kronecker_monomial &operator=(kronecker_monomial &&other) piranha_noexcept_spec(true)
		{
			m_value = std::move(other.m_value);
			return *this;
		}
		/// Compatibility check
		bool is_compatible(const std::vector<symbol> &args) const piranha_noexcept_spec(true)
		{
			const auto s = args.size();
			// No args means the value must also be zero.
			if (s == 0u) {
				return !m_value;
			}
			const auto &limits = ka::get_limits();
			// If we overflow the maximum size available, we cannot use this object as key in series.
			if (s >= limits.size()) {
				return false;
			}
			const auto &l = limits[s];
			// Value is compatible if it is within the bounds for the given size.
			return (m_value >= std::get<3u>(l) && m_value <= std::get<4u>(l));
		}
		/// Ignorability check.
		bool is_ignorable(const std::vector<symbol> &args) const piranha_noexcept_spec(true)
		{
			(void)args;
			piranha_assert(is_compatible(args));
			return false;
		}
		/// Merge arguments.
		kronecker_monomial merge_args(const std::vector<symbol> &orig_args, const std::vector<symbol> &new_args) const
		{
			piranha_assert(new_args.size() > orig_args.size());
			piranha_assert(std::is_sorted(orig_args.begin(),orig_args.end()));
			piranha_assert(std::is_sorted(new_args.begin(),new_args.end()));
			const auto old_vector = unpack(orig_args);
			v_type new_vector;
			auto it_new = new_args.begin();
			for (size_type i = 0u; i < old_vector.size(); ++i, ++it_new) {
				while (*it_new != orig_args[i]) {
					new_vector.push_back(int_type(0));
					++it_new;
					piranha_assert(it_new != new_args.end());
				}
				new_vector.push_back(old_vector[i]);
			}
			// Fill up arguments at the tail of new_args but not in orig_args.
			for (; it_new != new_args.end(); ++it_new) {
				new_vector.push_back(int_type(0));
			}
			piranha_assert(new_vector.size() == new_args.size());
			// Return monomial with the new encoded vector.
			kronecker_monomial retval;
			retval.m_value = ka::encode(new_vector);
			return retval;
		}
		/// Check if monomial is unitary.
		bool is_unitary(const std::vector<symbol> &args) const
		{
			(void)args;
			piranha_assert(args.size() || !m_value);
			// A kronecker code will be zero if all components are zero.
			return !m_value;
		}
		/// Degree.
		int_type degree(const std::vector<symbol> &args) const
		{
			const auto tmp = unpack(args);
			return std::accumulate(tmp.begin(),tmp.end(),int_type(0),safe_adder);
		}
		/// Multiply monomial.
		void multiply(kronecker_monomial &retval, const kronecker_monomial &other, const std::vector<symbol> &args) const
		{
			(void)args;
			retval.m_value = m_value + other.m_value;
		}
		std::size_t hash() const
		{
			return std::hash<int_type>()(m_value);
		}
		bool operator==(const kronecker_monomial &other) const
		{
			return m_value == other.m_value;
		}
		bool operator!=(const kronecker_monomial &other) const
		{
			return m_value != other.m_value;
		}
		friend std::ostream &operator<<(std::ostream &os, const kronecker_monomial &k)
		{
			os << k.m_value;
			return os;
		}
	private:
		static int_type safe_adder(const int_type &result, const int_type &n)
		{
			if (n >= 0) {
				if (unlikely(result > boost::integer_traits<int_type>::const_max - n)) {
					piranha_throw(std::overflow_error,"overflow in the addition of two exponents in a Kronecker monomial");
				}
			} else {
				if (unlikely(result < boost::integer_traits<int_type>::const_min - n)) {
					piranha_throw(std::overflow_error,"overflow in the addition of two exponents in a Kronecker monomial");
				}
			}
			return result + n;
		}
		v_type unpack(const std::vector<symbol> &args) const
		{
			// NOTE: here we should be sure that args size is never greater than the maximum possible value,
			// as this object has supposedly been created with an args vector of admittable size.
			piranha_assert(args.size() <= boost::integer_traits<size_type>::const_max);
			v_type retval(args.size(),0);
			ka::decode(retval,m_value);
			return retval;
		}
	private:
		int_type m_value;
};

}

namespace std
{

/// Specialisation of \p std::hash for piranha::kronecker_monomial.
template <typename T>
struct hash<piranha::kronecker_monomial<T>>
{
	/// Result type.
	typedef size_t result_type;
	/// Argument type.
	typedef piranha::kronecker_monomial<T> argument_type;
	/// Hash operator.
	result_type operator()(const argument_type &a) const
	{
		return a.hash();
	}
};

}

#endif
