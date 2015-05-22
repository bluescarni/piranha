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

#ifndef PIRANHA_DIVISOR_SERIES_HPP
#define PIRANHA_DIVISOR_SERIES_HPP

#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "config.hpp"
#include "detail/divisor_series_fwd.hpp"
#include "detail/polynomial_fwd.hpp"
#include "detail/gcd.hpp"
#include "divisor.hpp"
#include "exceptions.hpp"
#include "forwarding.hpp"
#include "ipow_substitutable_series.hpp"
#include "is_cf.hpp"
#include "math.hpp"
#include "mp_integer.hpp"
#include "power_series.hpp"
#include "serialization.hpp"
#include "series.hpp"
#include "substitutable_series.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

// Type trait to check the key type in divisor_series.
template <typename T>
struct is_divisor_series_key
{
	static const bool value = false;
};

template <typename T>
struct is_divisor_series_key<divisor<T>>
{
	static const bool value = true;
};

}

/// Divisor series.
/**
 * This class represents series in which the keys are divisor (see piranha::divisor) and the coefficient type
 * is generic. This class satisfies the piranha::is_series and piranha::is_cf type traits.
 *
 * ## Type requirements ##
 *
 * \p Cf must be suitable for use in piranha::series as first template argument, \p Key must be an instance
 * of piranha::divisor.
 *
 * ## Exception safety guarantee ##
 *
 * This class provides the same guarantee as the base series type it derives from.
 *
 * ## Move semantics ##
 *
 * Move semantics is equivalent to the move semantics of the base series type it derives from.
 *
 * ## Serialization ##
 *
 * This class supports serialization if the underlying coefficient type does.
 */
template <typename Cf, typename Key>
class divisor_series: public power_series<ipow_substitutable_series<substitutable_series<series<Cf,Key,divisor_series<Cf,Key>>,
	divisor_series<Cf,Key>>,divisor_series<Cf,Key>>,divisor_series<Cf,Key>>
{
		PIRANHA_TT_CHECK(detail::is_divisor_series_key,Key);
		using base = power_series<ipow_substitutable_series<substitutable_series<series<Cf,Key,divisor_series<Cf,Key>>,
			divisor_series<Cf,Key>>,divisor_series<Cf,Key>>,divisor_series<Cf,Key>>;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
		// Value type of the divisor.
		using dv_type = typename Key::value_type;
		// Return type for from_polynomial.
		template <typename T, typename U>
		using fp_type = typename std::enable_if<std::is_base_of<detail::polynomial_tag,T>::value,
			decltype(std::declval<const U &>() / std::declval<const integer &>())>::type;
		// Partial utils.
		// Handle exponent increase in a safe way.
		template <typename T, typename std::enable_if<std::is_integral<T>::value,int>::type = 0>
		static void expo_increase(T &e)
		{
			if (unlikely(e == std::numeric_limits<T>::max())) {
				piranha_throw(std::overflow_error,"overflow in the computation of the partial derivative "
					"of a divisor series");
			}
			e = static_cast<T>(e + T(1));
		}
		template <typename T, typename std::enable_if<!std::is_integral<T>::value,int>::type = 0>
		static void expo_increase(T &e)
		{
			++e;
		}
		// Safe computation of the integral multiplier.
		template <typename T, typename std::enable_if<std::is_integral<T>::value,int>::type = 0>
		static auto safe_mult(const T &n, const T &m) -> decltype(n*m)
		{
			auto ret(integer(n) * m);
			ret.negate();
			return static_cast<decltype(n * m)>(ret);
		}
		template <typename T, typename std::enable_if<!std::is_integral<T>::value,int>::type = 0>
		static auto safe_mult(const T &n, const T &m) -> decltype(-(n*m))
		{
			return -(n*m);
		}
		// This is the first stage - the second part of the chain rule for each term.
		template <typename T>
		using d_partial_type_0 = decltype(std::declval<const dv_type &>() * std::declval<const dv_type &>() * std::declval<const T &>());
		template <typename T>
		using d_partial_type_1 = decltype(std::declval<const T &>() * std::declval<const d_partial_type_0<T> &>());
		// Requirements on the final type for the first stage.
		template <typename T>
		using d_partial_type = typename std::enable_if<std::is_constructible<d_partial_type_1<T>,d_partial_type_0<T>>::value &&
			std::is_constructible<d_partial_type_1<T>,int>::value &&
			is_addable_in_place<d_partial_type_1<T>>::value,
			d_partial_type_1<T>>::type;
		template <typename T = divisor_series>
		d_partial_type<T> d_partial_impl(typename T::term_type::key_type &key, const symbol_set::positions &pos) const
		{
			using term_type = typename base::term_type;
			using cf_type = typename term_type::cf_type;
			using key_type = typename term_type::key_type;
			piranha_assert(key.size() != 0u);
			// Construct the first part of the derivative.
			key_type tmp_div;
			// Insert all the terms that depend on the variable, apart from the
			// first one.
			const auto it_f = key.m_container.end();
			auto it = key.m_container.begin(), it_b(it);
			auto first(*it), first_copy(*it);
			// Size type of the multipliers' vector.
			using vs_type = decltype(first.v.size());
			++it;
			for (; it != it_f; ++it) {
				tmp_div.m_container.insert(*it);
			}
			// Remove the first term from the original key.
			key.m_container.erase(it_b);
			// Extract from the first dependent term the aij and the exponent, and multiply+negate them.
			const auto mult = safe_mult(first.e,first.v[static_cast<vs_type>(pos.back())]);
			// Increase by one the exponent of the first dep. term.
			expo_increase(first.e);
			// Insert the modified first term. Don't move, as we need first below.
			tmp_div.m_container.insert(first);
			// Now build the first part of the derivative.
			divisor_series tmp_ds;
			tmp_ds.set_symbol_set(this->m_symbol_set);
			tmp_ds.insert(term_type(cf_type(1),std::move(tmp_div)));
			// Init the retval.
			d_partial_type<T> retval(mult * tmp_ds);
			// Now the second part of the derivative, if appropriate.
			if (!key.m_container.empty()) {
				// Build a series with only the first dependent term and unitary coefficient.
				key_type tmp_div_01;
				tmp_div_01.m_container.insert(std::move(first_copy));
				divisor_series tmp_ds_01;
				tmp_ds_01.set_symbol_set(this->m_symbol_set);
				tmp_ds_01.insert(term_type(cf_type(1),std::move(tmp_div_01)));
				// Recurse.
				retval += tmp_ds_01 * d_partial_impl(key,pos);
			}
			return retval;
		}
		template <typename T = divisor_series>
		d_partial_type<T> divisor_partial(const typename T::term_type &term, const symbol_set::positions &pos) const
		{
			using term_type = typename base::term_type;
			// Return zero if the variable is not in the series.
			if (pos.size() == 0u) {
				return d_partial_type<T>(0);
			}
			// Initial split of the divisor.
			auto sd = term.m_key.split(pos,this->m_symbol_set);
			// If the variable is not in the divisor, just return zero.
			if (sd.first.size() == 0u) {
				return d_partial_type<T>(0);
			}
			// Init the constant part of the derivative: the coefficient and the part of the divisor
			// which does not depend on the variable.
			divisor_series tmp_ds;
			tmp_ds.set_symbol_set(this->m_symbol_set);
			tmp_ds.insert(term_type(term.m_cf,std::move(sd.second)));
			// Construct and return the result.
			return tmp_ds * d_partial_impl(sd.first,pos);
		}
		// The final type.
		template <typename T>
		using partial_type_ = decltype(
			math::partial(std::declval<const typename T::term_type::cf_type &>(),std::string{}) * std::declval<const T &>()
			+ std::declval<const d_partial_type<T> &>()
		);
		template <typename T>
		using partial_type = typename std::enable_if<std::is_constructible<partial_type_<T>,int>::value &&
			is_addable_in_place<partial_type_<T>>::value,partial_type_<T>>::type;
	public:
		/// Series rebind alias.
		template <typename Cf2>
		using rebind = divisor_series<Cf2,Key>;
		/// Defaulted default constructor.
		divisor_series() = default;
		/// Defaulted copy constructor.
		divisor_series(const divisor_series &) = default;
		/// Defaulted move constructor.
		divisor_series(divisor_series &&) = default;
		PIRANHA_FORWARDING_CTOR(divisor_series,base)
		/// Trivial destructor.
		~divisor_series()
		{
			PIRANHA_TT_CHECK(is_series,divisor_series);
			PIRANHA_TT_CHECK(is_cf,divisor_series);
		}
		/// Defaulted copy assignment operator.
		divisor_series &operator=(const divisor_series &) = default;
		/// Defaulted move assignment operator.
		divisor_series &operator=(divisor_series &&) = default;
		PIRANHA_FORWARDING_ASSIGNMENT(divisor_series,base)
		/// Construct a divisor series from a polynomial.
		/**
		 * \note
		 * This method is enabled only if \p T is an instance of piranha::polynomial
		 * and the calling piranha::divisor_series type can be divided by integer.
		 *
		 * This method will construct a piranha::divisor_series consisting of a single term with unitary
		 * coefficient and with the key containing a single divisor built from the input polynomial,
		 * which must be equivalent to an integral linear combination of symbols with no constant terms.
		 *
		 * For example, when this method is called from a divisor series with rational coefficients, the input polynomial
		 * \f[
		 * x+y
		 * \f]
		 * will result in the construction of the divisor series
		 * \f[
		 * \frac{1}{x+y}.
		 * \f]
		 * The input polynomial
		 * \f[
		 * -2x+4y
		 * \f]
		 * will result in the construction of the divisor series
		 * \f[
		 * -\frac{1}{2}\frac{1}{x-2y}.
		 * \f]
		 *
		 * @param[in] p the input polynomial.
		 *
		 * @return a piranha::divisor_series constructed from the input polynomial.
		 *
		 * @throws unspecified any exception thrown by:
		 * - the extraction of an integral linear combination of symbols from \p p,
		 * - memory errors in standard containers,
		 * - the public interface of piranha::symbol_set,
		 * - piranha::math::is_zero(), piranha::math::negate(),
		 * - the construction of terms, coefficients and keys,
		 * - piranha::divisor::insert(),
		 * - piranha::series::insert(), piranha::series::set_symbol_set(),
		 * - arithmetics on piranha::divisor_series.
		 */
		template <typename T, typename U = divisor_series>
		static fp_type<T,U> from_polynomial(const T &p)
		{
			using term_type = typename base::term_type;
			using cf_type = typename term_type::cf_type;
			using key_type = typename term_type::key_type;
			auto lc = p.integral_combination();
			if (unlikely(lc.empty())) {
				piranha_throw(std::invalid_argument,"empty polynomial input");
			}
			std::vector<integer> v_int;
			symbol_set ss;
			for (auto it = lc.begin(); it != lc.end(); ++it) {
				ss.add(symbol(it->first));
				v_int.push_back(it->second);
			}
			// We need to canonicalise the term: switch the sign if the first
			// nonzero element is negative, and divide by the common denom.
			bool first_nonzero_found = false, need_negate = false;
			integer cd(0);
			for (auto &n: v_int) {
				if (!first_nonzero_found && !math::is_zero(n)) {
					if (n < 0) {
						need_negate = true;
					}
					first_nonzero_found = true;
				}
				if (need_negate) {
					math::negate(n);
				}
				// NOTE: gcd(0,n) == n (or +-n, in our case) for all n, zero included.
				// NOTE: the gcd computation here is safe as we are operating on integers.
				cd = detail::gcd(cd,n);
			}
			// Common denominator could be negative.
			if (cd.sign() < 0) {
				math::negate(cd);
			}
			// It should never be zero: if all elements in v_int are zero, we would not have been
			// able to extract the linear combination.
			piranha_assert(!math::is_zero(cd));
			// Divide by the cd.
			for (auto &n: v_int) {
				n /= cd;
			}
			// Now build the key.
			key_type tmp_key;
			integer exponent(1);
			tmp_key.insert(v_int.begin(),v_int.end(),exponent);
			// The return value.
			divisor_series retval;
			retval.set_symbol_set(ss);
			retval.insert(term_type(cf_type(1),std::move(tmp_key)));
			// If we negated in the canonicalisation, we need to re-negate
			// the common divisor before the final division.
			if (need_negate) {
				math::negate(cd);
			}
			return retval / cd;
		}
		/// Partial derivative.
		/**
		 * \note
		 * This method is enabled only if the algorithm described below is supported by all the
		 * involved types.
		 *
		 * This method will compute the partial derivative of \p this with respect to the symbol called \p name.
		 * The derivative is computed via differentiation of the coefficients and the application of the product
		 * rule.
		 *
		 * @param[in] name name of the variable with respect to which the differentiation will be computed.
		 *
		 * @return the partial derivative of \p this with respect to \p name.
		 *
		 * @throws std::overflow_error if the manipulation of integral exponents and multipliers in the divisors
		 * results in an overflow.
		 * @throws unspecified any exception thrown by:
		 * - the construction of the return type,
		 * - the arithmetics operations needed to compute the result,
		 * - the public interface of piranha::symbol_set,
		 * - piranha::math::partial(), piranha::math::negate(),
		 * - construction and insertion of series terms,
		 * - piranha::series::set_symbol_set(),
		 * - piranha::divisor::split(),
		 * - piranha::hash_set::insert().
		 */
		template <typename T = divisor_series>
		partial_type<T> partial(const std::string &name) const
		{
			using term_type = typename base::term_type;
			using cf_type = typename term_type::cf_type;
			partial_type<T> retval(0);
			const auto it_f = this->m_container.end();
			const symbol_set::positions pos(this->m_symbol_set,symbol_set{symbol(name)});
			for (auto it = this->m_container.begin(); it != it_f; ++it) {
				divisor_series tmp;
				tmp.set_symbol_set(this->m_symbol_set);
				tmp.insert(term_type(cf_type(1),it->m_key));
				retval += math::partial(it->m_cf,name) * tmp + divisor_partial(*it,pos);
			}
			return retval;
		}
};

}

#endif
