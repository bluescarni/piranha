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

#ifndef PIRANHA_POISSON_SERIES_HPP
#define PIRANHA_POISSON_SERIES_HPP

#include <algorithm>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "config.hpp"
#include "detail/poisson_series_fwd.hpp"
#include "detail/polynomial_fwd.hpp"
#include "forwarding.hpp"
#include "integer.hpp"
#include "math.hpp"
#include "poisson_series_term.hpp"
#include "power_series.hpp"
#include "series.hpp"
#include "symbol.hpp"
#include "symbol_set.hpp"
#include "t_substitutable_series.hpp"
#include "trigonometric_series.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Poisson series class.
/**
 * This class represents multivariate Poisson series as collections of multivariate Poisson series terms
 * (represented by the piranha::poisson_series_term class). The coefficient
 * type \p Cf represents the ring over which the Poisson series is defined.
 * 
 * This class satisfies the piranha::is_series type trait.
 * 
 * \section type_requirements Type requirements
 * 
 * \p Cf must be suitable for use in piranha::poisson_series_term.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the same guarantee as piranha::power_series.
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics is equivalent to piranha::power_series's move semantics.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename Cf>
class poisson_series:
	public power_series<t_substitutable_series<trigonometric_series<series<poisson_series_term<Cf>,poisson_series<Cf>>>,poisson_series<Cf>>>
{
		typedef power_series<t_substitutable_series<trigonometric_series<series<poisson_series_term<Cf>,poisson_series<Cf>>>,poisson_series<Cf>>> base;
		template <bool IsCos, typename T>
		poisson_series sin_cos_impl(const T &, typename std::enable_if<
			std::is_same<T,std::true_type>::value>::type * = nullptr) const
		{
			// Do something only if the series is equivalent to a polynomial.
			if (this->is_single_coefficient() && !this->empty()) {
				try {
					// Shortcuts.
					typedef poisson_series_term<Cf> term_type;
					typedef typename term_type::key_type key_type;
					typedef typename key_type::value_type value_type;
					// Try to get the integral combination.
					auto lc = this->m_container.begin()->m_cf.integral_combination();
					// Change sign if needed.
					bool sign_change = false;
					if (!lc.empty() && lc.begin()->second.sign() < 0) {
						std::for_each(lc.begin(),lc.end(),[](std::pair<const std::string,integer> &p) {p.second.negate();});
						sign_change = true;
					}
					// Return value.
					poisson_series retval;
					// Build vector of integral multipliers.
					std::vector<value_type> v;
					for (auto it = lc.begin(); it != lc.end(); ++it) {
						retval.m_symbol_set.add(it->first);
						v.push_back(static_cast<value_type>(it->second));
					}
					// Build term, fix signs and flavour and move-insert it.
					term_type term(Cf(1),key_type(v.begin(),v.end()));
					if (!IsCos) {
						term.m_key.set_flavour(false);
						if (sign_change) {
							math::negate(term.m_cf);
						}
					}
					retval.insert(std::move(term));
					return retval;
				} catch (const std::invalid_argument &) {
					// Interpret invalid_argument as a failure in extracting integral combination,
					// and move on.
				}
			}
			return sin_cos_cf_impl<IsCos>();
		}
		template <bool IsCos, typename T>
		poisson_series sin_cos_impl(const T &, typename std::enable_if<
			std::is_same<T,std::false_type>::value>::type * = nullptr) const
		{
			return sin_cos_cf_impl<IsCos>();
		}
		template <bool IsCos>
		poisson_series sin_cos_cf_impl() const
		{
			// NOTE: here we cast back to the base class, and then we have to move-construct the output
			// Poisson series as the math::cos functor will produce an output of the type of the base class.
			return ((IsCos) ? poisson_series(math::cos(*static_cast<series<poisson_series_term<Cf>,poisson_series<Cf>> const *>(this))) :
				poisson_series(math::sin(*static_cast<series<poisson_series_term<Cf>,poisson_series<Cf>> const *>(this))));
		}
		// Subs typedefs.
		// TODO: fix declval usage. -> remove it everywhere, remove <utility>.
		template <typename T>
		struct subs_type
		{
			typedef typename base::term_type::cf_type cf_type;
			typedef typename base::term_type::key_type key_type;
			typedef decltype(
				(math::subs(std::declval<cf_type>(),std::declval<std::string>(),std::declval<T>()) * std::declval<poisson_series>()) *
				std::declval<key_type>().subs(std::declval<symbol>(),std::declval<T>(),std::declval<symbol_set>()).first.first
			) type;
		};
		template <typename T>
		struct ipow_subs_type
		{
			typedef typename base::term_type::cf_type cf_type;
			typedef decltype(
				math::ipow_subs(std::declval<cf_type>(),std::declval<std::string>(),std::declval<integer>(),std::declval<T>()) *
				std::declval<poisson_series>()
			) type;
		};
		// Implementation details for integration.
		template <typename T>
		static auto integrate_cf(const T &cf, const std::string &name,
			typename std::enable_if<is_integrable<T>::value>::type * = nullptr) -> decltype(math::integrate(cf,name))
		{
			return math::integrate(cf,name);
		}
		template <typename T>
		static T integrate_cf(const T &, const std::string &,
			typename std::enable_if<!is_integrable<T>::value>::type * = nullptr)
		{
			piranha_throw(std::invalid_argument,"unable to perform Poisson series integration: coefficient type is not integrable");
		}
		poisson_series integrate_impl(const symbol &s, const typename base::term_type &term,
			const std::true_type &) const
		{
			typedef typename base::term_type term_type;
			typedef typename term_type::cf_type cf_type;
			integer degree;
			try {
				degree = math::integral_cast(math::degree(term.m_cf,{s.get_name()}));
			} catch (const std::invalid_argument &) {
				piranha_throw(std::invalid_argument,
					"unable to perform Poisson series integration: cannot extract the integral form of a polynomial degree");
			}
			// If the variable is in both cf and key, and the cf degree is negative, we cannot integrate.
			if (degree.sign() < 0) {
				piranha_throw(std::invalid_argument,
					"unable to perform Poisson series integration: polynomial coefficient has negative integral degree");
			}
			// Init retval and auxiliary quantities for the iteration.
			poisson_series retval;
			retval.m_symbol_set = this->m_symbol_set;
			auto key_int = term.m_key.integrate(s,this->m_symbol_set);
			// NOTE: here we are sure that the variable is contained in the monomial.
			piranha_assert(key_int.first.sign() != 0);
			cf_type p_cf(term.m_cf / key_int.first);
			retval.insert(term_type(p_cf,key_int.second));
			for (integer i(1); i <= degree; ++i) {
				key_int = key_int.second.integrate(s,this->m_symbol_set);
				piranha_assert(key_int.first.sign() != 0);
				p_cf = math::partial(p_cf / key_int.first,s.get_name());
				// Sign change due to the second portion of integration by part.
				math::negate(p_cf);
				retval.insert(term_type(p_cf,key_int.second));
			}
			return retval;
		}
		poisson_series integrate_impl(const symbol &, const typename base::term_type &,
			const std::false_type &) const
		{
			piranha_throw(std::invalid_argument,"unable to perform Poisson series integration: coefficient type is not a polynomial");
		}
	public:
		/// Inherited constructors.
		using base::base;
		PIRANHA_FORWARDING_ASSIGNMENT(poisson_series,base)
		/// Override sine implementation.
		/**
		 * This method will override the default math::sin() implementation in case the coefficient type is an instance of
		 * piranha::polynomial. If the series is single-coefficient and not empty, and the coefficient represents a linear combination
		 * of variables with integral coefficients, then the return value will be a Poisson series consisting of a single term with
		 * coefficient constructed from "1" and trigonometric key containing the linear combination of variables.
		 * 
		 * In any other case, the default algorithm to calculate the sine of a series will take place.
		 * 
		 * @return sine of \p this.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::series::is_single_coefficient(), piranha::series::insert(),
		 *   piranha::series::apply_cf_functor(),
		 * - memory allocation errors in standard containers,
		 * - the <tt>linear_argument()</tt> method of the key type,
		 * - piranha::math::integral_cast(), piranha::math::sin(),
		 * - the cast operator of piranha::integer,
		 * - the constructors of coefficient, key and term types.
		 *
		 * \todo type requirements: integral_cast for use in linear combination.
		 */
		poisson_series sin() const
		{
			return sin_cos_impl<false>(std::integral_constant<bool,std::is_base_of<detail::polynomial_tag,Cf>::value>());
		}
		/// Override cosine implementation.
		/**
		 * The procedure is the same as explained in sin().
		 * 
		 * @return cosine of \p this.
		 * 
		 * @throws unspecified any exception thrown by sin().
		 *
		 * \todo type requirements: integral_cast for use in linear combination.
		 */
		poisson_series cos() const
		{
			return sin_cos_impl<true>(std::integral_constant<bool,std::is_base_of<detail::polynomial_tag,Cf>::value>());
		}
		/// Substitution.
		/**
		 * Substitute the symbolic quantity \p name with the generic value \p x. The result for each term is computed
		 * via piranha::math::subs() for the coefficients and via the substitution method for the monomials, and
		 * then assembled into the final return value via multiplications and additions.
		 * 
		 * @param[in] name name of the symbolic variable that will be subject to substitution.
		 * @param[in] x quantity that will be substituted for \p name.
		 * 
		 * @return result of the substitution.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - symbol construction,
		 * - piranha::symbol_set::remove() and assignment operator,
		 * - piranha::math::subs(),
		 * - the substitution method of the monomial type,
		 * - piranha::series::insert(),
		 * - construction, addition and multiplication of the types involved in the computation.
		 * 
		 * \todo type requirements.
		 */
		template <typename T>
		typename subs_type<T>::type subs(const std::string &name, const T &x) const
		{
			typedef typename subs_type<T>::type return_type;
			typedef typename base::term_type term_type;
			typedef typename term_type::cf_type cf_type;
			typedef typename term_type::key_type key_type;
			// Turn name into symbol.
			const symbol s(name);
			// Init return value.
			return_type retval = return_type();
			// Remove the symbol from the current symbol set, if present.
			symbol_set sset(this->m_symbol_set);
			if (std::binary_search(sset.begin(),sset.end(),s)) {
				sset.remove(s);
			}
			const auto it_f = this->m_container.end();
			for (auto it = this->m_container.begin(); it != it_f; ++it) {
				auto cf_sub = math::subs(it->m_cf,name,x);
				auto key_sub = it->m_key.subs(s,x,this->m_symbol_set);
				poisson_series tmp_series1;
				tmp_series1.m_symbol_set = sset;
				tmp_series1.insert(term_type(cf_type(1),key_type(key_sub.first.second)));
				poisson_series tmp_series2;
				tmp_series2.m_symbol_set = sset;
				tmp_series2.insert(term_type(cf_type(1),key_type(key_sub.second.second)));
				retval += (cf_sub * tmp_series1) * key_sub.first.first;
				retval += (cf_sub * tmp_series2) * key_sub.second.first;
			}
			return retval;
		}
		/// Substitution of integral power.
		/**
		 * This method will substitute occurrences of \p name to the power of \p n with \p x.
		 * The result for each term is computed by calling piranha::math::ipow_subs() on the coefficients, with
		 * the final return value assembled via multiplications and additions.
		 * 
		 * @param[in] name name of the symbolic variable that will be subject to substitution.
		 * @param[in] n power of \p name that will be substituted.
		 * @param[in] x quantity that will be substituted for \p name to the power of \p n.
		 * 
		 * @return result of the substitution.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the assignment operator of piranha::symbol_set,
		 * - piranha::math::ipow_subs(),
		 * - piranha::series::insert(),
		 * - construction, addition and multiplication of the types involved in the computation.
		 * 
		 * \todo type requirements.
		 */
		template <typename T>
		typename ipow_subs_type<T>::type ipow_subs(const std::string &name, const integer &n, const T &x) const
		{
			typedef typename ipow_subs_type<T>::type return_type;
			typedef typename base::term_type term_type;
			typedef typename term_type::cf_type cf_type;
			// Init return value.
			return_type retval = return_type();
			const auto it_f = this->m_container.end();
			for (auto it = this->m_container.begin(); it != it_f; ++it) {
				auto cf_sub = math::ipow_subs(it->m_cf,name,n,x);
				poisson_series tmp_series;
				tmp_series.m_symbol_set = this->m_symbol_set;
				tmp_series.insert(term_type(cf_type(1),it->m_key));
				// NOTE: use series multadd if it becomes available.
				retval += cf_sub * tmp_series;
			}
			return retval;
		}
		/// Integration.
		/**
		 * This method will attempt to compute the antiderivative of the Poisson series term by term using the
		 * following procedure:
		 * - if the term's monomial does not depend on the integration variable, the integration will be deferred to the coefficient;
		 * - otherwise:
		 *   - if the coefficient does not depend on the integration variable, the monomial is integrated;
		 *   - if the coefficient is a polynomial, a strategy of integration by parts is attempted, its success depending on whether
		 *     the degree of the polynomial is a non-negative integral value;
		 *   - otherwise, an error will be produced.
		 * 
		 * This method requires the coefficient type to be differentiable.
		 * 
		 * @param[in] name integration variable.
		 * 
		 * @return the antiderivative of \p this with respect to \p name.
		 * 
		 * @throws std::invalid_argument if the integration procedure fails.
		 * @throws unspecified any exception thrown by:
		 * - piranha::symbol construction,
		 * - piranha::math::partial(), piranha::math::is_zero(), piranha::math::integrate(), piranha::math::integral_cast() and
		 *   piranha::math::negate(),
		 * - the assignment operator of piranha::symbol_set,
		 * - term construction,
		 * - coefficient construction, assignment and arithmetics,
		 * - integration, construction and assignment of the key type,
		 * - insert(),
		 * - piranha::polynomial::degree(),
		 * - series arithmetics.
		 * 
		 * \todo requirements on dividability by multiplier type (or integer), integral_cast, etc.
		 */
		poisson_series integrate(const std::string &name) const
		{
			typedef typename base::term_type term_type;
			typedef typename term_type::cf_type cf_type;
			PIRANHA_TT_CHECK(is_differentiable,cf_type);
			// Turn name into symbol.
			const symbol s(name);
			poisson_series retval;
			retval.m_symbol_set = this->m_symbol_set;
			const auto it_f = this->m_container.end();
			for (auto it = this->m_container.begin(); it != it_f; ++it) {
				// Try to integrate the key first.
				const auto key_int = it->m_key.integrate(s,this->m_symbol_set);
				if (key_int.first.sign() == 0) {
					// The variable does not appear in the monomial, try deferring the integration
					// to the coefficient.
					retval.insert(term_type(cf_type(integrate_cf(it->m_cf,name)),it->m_key));
					continue;
				}
				// The variable is in the monomial, let's check if the variable is also in the coefficient.
				if (math::is_zero(math::partial(it->m_cf,name))) {
					// No variable in the coefficient, proceed with the integrated key and divide by multiplier.
					retval.insert(term_type(it->m_cf / key_int.first,std::move(key_int.second)));
				} else {
					// With the variable both in the coefficient and the key, we only know how to proceed with polynomials.
					retval += integrate_impl(s,*it,std::integral_constant<bool,std::is_base_of<detail::polynomial_tag,Cf>::value>());
				}
			}
			return retval;
		}
};

namespace math
{

/// Specialisation of the piranha::math::subs() functor for Poisson series types.
/**
 * This specialisation is activated when \p Series is an instance of piranha::poisson_series.
 */
template <typename Series>
struct subs_impl<Series,typename std::enable_if<is_instance_of<Series,poisson_series>::value>::type>
{
	private:
		// TODO: fix declval usage.
		template <typename T>
		struct subs_type
		{
			typedef decltype(std::declval<Series>().subs(std::declval<std::string>(),std::declval<T>())) type;
		};
	public:
		/// Call operator.
		/**
		 * The implementation will use piranha::poisson_series::subs().
		 * 
		 * @param[in] s input Poisson series.
		 * @param[in] name name of the symbolic variable that will be substituted.
		 * @param[in] x object that will replace \p name.
		 * 
		 * @return output of piranha::poisson_series::subs().
		 * 
		 * @throws unspecified any exception thrown by piranha::poisson_series::subs().
		 */
		template <typename T>
		typename subs_type<T>::type operator()(const Series &s, const std::string &name, const T &x) const
		{
			return s.subs(name,x);
		}
};

/// Specialisation of the piranha::math::ipow_subs() functor for Poisson series.
/**
 * This specialisation is activated when \p Series is an instance of piranha::poisson_series.
 */
template <typename Series>
struct ipow_subs_impl<Series,typename std::enable_if<is_instance_of<Series,poisson_series>::value>::type>
{
	private:
		// TODO: fix declval usage.
		template <typename T>
		struct ipow_subs_type
		{
			typedef decltype(std::declval<Series>().ipow_subs(std::declval<std::string>(),std::declval<integer>(),std::declval<T>())) type;
		};
	public:
		/// Call operator.
		/**
		 * The implementation will use piranha::poisson_series::ipow_subs().
		 * 
		 * @param[in] s input Poisson series.
		 * @param[in] name name of the symbolic variable that will be substituted.
		 * @param[in] n power of \p name that will be substituted.
		 * @param[in] x object that will replace \p name.
		 * 
		 * @return output of piranha::poisson_series::ipow_subs().
		 * 
		 * @throws unspecified any exception thrown by piranha::poisson_series::ipow_subs().
		 */
		template <typename T>
		typename ipow_subs_type<T>::type operator()(const Series &s, const std::string &name, const integer &n, const T &x) const
		{
			return s.ipow_subs(name,n,x);
		}
};

/// Specialisation of the piranha::math::integrate() functor for Poisson series.
/**
 * This specialisation is activated when \p Series is an instance of piranha::poisson_series.
 */
template <typename Series>
struct integrate_impl<Series,typename std::enable_if<is_instance_of<Series,poisson_series>::value>::type>
{
	/// Call operator.
	/**
	 * The implementation will use piranha::poisson_series::integrate().
	 * 
	 * @param[in] s input Poisson series.
	 * @param[in] name integration variable.
	 * 
	 * @return antiderivative of \p s with respect to \p name.
	 * 
	 * @throws unspecified any exception thrown by piranha::poisson_series::integrate().
	 */
	Series operator()(const Series &s, const std::string &name) const
	{
		return s.integrate(name);
	}
};

}

}

#endif
