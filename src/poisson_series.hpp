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
#include <iterator>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "config.hpp"
#include "detail/gcd.hpp"
#include "detail/poisson_series_fwd.hpp"
#include "detail/polynomial_fwd.hpp"
#include "detail/sfinae_types.hpp"
#include "divisor.hpp"
#include "divisor_series.hpp"
#include "exceptions.hpp"
#include "forwarding.hpp"
#include "ipow_substitutable_series.hpp"
#include "is_cf.hpp"
#include "math.hpp"
#include "mp_integer.hpp"
#include "power_series.hpp"
#include "real_trigonometric_kronecker_monomial.hpp"
#include "safe_cast.hpp"
#include "serialization.hpp"
#include "series.hpp"
#include "substitutable_series.hpp"
#include "symbol.hpp"
#include "symbol_set.hpp"
#include "t_substitutable_series.hpp"
#include "term.hpp"
#include "trigonometric_series.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

// Implementation detail to detect a divisor series.
template <typename T>
struct is_divisor_series
{
	static const bool value = false;
};

template <typename Cf, typename Key>
struct is_divisor_series<divisor_series<Cf,Key>>
{
	static const bool value = true;
};

// Detect the presence of the t_integrate method. This needs to go in the future, it is repeated
// in pyranha.
template <typename T>
struct has_t_integrate: detail::sfinae_types
{
	template <typename T1>
	static auto test(const T1 &x) -> decltype(x.t_integrate(),void(),yes());
	static no test(...);
	static const bool value = std::is_same<decltype(test(std::declval<T>())),yes>::value;
};

}

/// Poisson series class.
/**
 * This class represents multivariate Poisson series as collections of multivariate Poisson series terms,
 * in which the trigonometric monomials are represented by piranha::rtk_monomial.
 * \p Cf represents the ring over which the Poisson series is defined.
 * 
 * This class satisfies the piranha::is_series type trait.
 * 
 * ## Type requirements ##
 * 
 * \p Cf must be suitable for use in piranha::series as first template argument.
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
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
// TODO:
// - make this more general, make the key type selectable;
// - once the above is done, remeber to fix the rebind alias.
// - once we have a selectable key type, we must take care that in a few places we assume that the value type
//   of the key is a C++ integral, but this might not be the case any more (e.g., in the sin/cos implementation
//   we will need a safe cast).
template <typename Cf>
class poisson_series:
	public power_series<ipow_substitutable_series<substitutable_series<t_substitutable_series<trigonometric_series<series<Cf,rtk_monomial,poisson_series<Cf>>>,poisson_series<Cf>>,
		poisson_series<Cf>>,poisson_series<Cf>>,poisson_series<Cf>>
{
		using base = power_series<ipow_substitutable_series<substitutable_series<t_substitutable_series<trigonometric_series<series<Cf,rtk_monomial,poisson_series<Cf>>>,poisson_series<Cf>>,
			poisson_series<Cf>>,poisson_series<Cf>>,poisson_series<Cf>>;
		// TMP for enabling sin and cos overrides.
		// Detect if T's coefficient is a polynomial whose coefficient has integral cast.
		template <typename T, typename = void>
		struct cf_poly_has_icast
		{
			static const bool value = false;
		};
		template <typename T>
		struct cf_poly_has_icast<T,typename std::enable_if<std::is_base_of<detail::polynomial_tag,typename T::term_type::cf_type>::value>::type>
		{
			static const bool value = has_safe_cast<integer,typename T::term_type::cf_type::term_type::cf_type>::value;
		};
		// Detect if T's coefficient has suitable sin/cos implementations.
		template <typename T, typename = void>
		struct cf_has_sin_cos
		{
			static const bool value = false;
		};
		template <typename T>
		struct cf_has_sin_cos<T,typename std::enable_if<
			std::is_same<decltype(math::sin(std::declval<typename T::term_type::cf_type>())),typename T::term_type::cf_type>::value &&
			std::is_same<decltype(math::cos(std::declval<typename T::term_type::cf_type>())),typename T::term_type::cf_type>::value
			>::type>
		{
			static const bool value = true;
		};
		template <typename T>
		using sin_cos_enabler = typename std::enable_if<
			cf_poly_has_icast<T>::value || cf_has_sin_cos<T>::value,
			int>::type;
		// Sin/cos overrides implementation.
		// If cf is a suitable polynomial, attempt the symbolic sin/cos with the integral linear combination.
		template <bool IsCos, typename T, typename std::enable_if<cf_poly_has_icast<T>::value,int>::type = 0>
		poisson_series sin_cos_impl() const
		{
			// Do something only if the series is equivalent to a polynomial.
			if (this->is_single_coefficient() && !this->empty()) {
				try {
					// Shortcuts.
					typedef term<Cf,rtk_monomial> term_type;
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
						// NOTE: this should probably be a safe_cast, if we ever implement it.
						// The value type here could be anything, and not guaranteed to be castable,
						// even if in the current implementation this is guaranteed to be a signed
						// int of some kind.
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
			return sin_cos_cf_impl<IsCos,T>();
		}
		// If cf is not a suitable poly, go for the standard implementation.
		template <bool IsCos, typename T, typename std::enable_if<!cf_poly_has_icast<T>::value,int>::type = 0>
		poisson_series sin_cos_impl() const
		{
			return sin_cos_cf_impl<IsCos,T>();
		}
		template <bool IsCos, typename T, typename std::enable_if<cf_has_sin_cos<T>::value,int>::type = 0>
		poisson_series sin_cos_cf_impl() const
		{
			// NOTE: here we cast back to the base class, and then we have to move-construct the output
			// Poisson series as the math::cos functor will produce an output of the type of the base class.
			if (IsCos) {
				return poisson_series{math::cos(*static_cast<series<Cf,rtk_monomial,poisson_series<Cf>> const *>(this))};
			}
			return poisson_series{math::sin(*static_cast<series<Cf,rtk_monomial,poisson_series<Cf>> const *>(this))};
		}
		template <bool IsCos, typename T, typename std::enable_if<!cf_has_sin_cos<T>::value,int>::type = 0>
		poisson_series sin_cos_cf_impl() const
		{
			// TODO: this will need to be reworked eventually when we fix subs(), there's no need to arrive here
			// with a runtime error when we can exclude this from happening via TMP.
			piranha_throw(std::invalid_argument,"Poisson series is unsuitable for the calculation of sine/cosine");
		}
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
				degree = safe_cast<integer>(math::degree(term.m_cf,{s.get_name()}));
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
			piranha_assert(key_int.first != 0);
			cf_type p_cf(term.m_cf / key_int.first);
			retval.insert(term_type(p_cf,key_int.second));
			for (integer i(1); i <= degree; ++i) {
				key_int = key_int.second.integrate(s,this->m_symbol_set);
				piranha_assert(key_int.first != 0);
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
		// Time integration.
		// Definition of the divisor type. NOTE: this is temporary and should be changed in the future.
		template <typename T>
		using t_int_div_key_type = divisor<short>;
		template <typename T>
		using t_int_div_cf_type = decltype((std::declval<const typename T::term_type::cf_type &>() * 1) /
			std::declval<const typename t_int_div_key_type<T>::value_type &>());
		template <typename T>
		using ti_type_ = typename std::enable_if<is_cf<t_int_div_cf_type<T>>::value,
			piranha::poisson_series<divisor_series<t_int_div_cf_type<T>,t_int_div_key_type<T>>>>::type;
		// Overload if cf is not a divisor series already. The result will be a Poisson series with the same key type, in which the coefficient
		// is a divisor series whose coefficient is calculated from the operations needed in the integration, and the key type is a divisor whose
		// value type is deduced from the trigonometric key.
		template <typename T = poisson_series, typename std::enable_if<!detail::is_divisor_series<typename T::term_type::cf_type>::value,int>::type = 0>
		ti_type_<T> t_integrate_impl() const
		{
			using return_type = ti_type_<T>;
			// The value type of the trigonometric key.
			using k_value_type = typename base::term_type::key_type::value_type;
			// The divisor type in the return type.
			using div_type = typename return_type::term_type::cf_type::term_type::key_type;
			// Initialise the return value. It has the same set of trig arguments as this.
			return_type retval;
			retval.set_symbol_set(this->m_symbol_set);
			// The symbol set for the divisor series - built from the trigonometric arguments.
			symbol_set div_symbols;
			for (auto it = retval.get_symbol_set().begin(); it != retval.get_symbol_set().end(); ++it) {
				div_symbols.add(std::string("\\nu_{") + it->get_name() + "}");
			}
			// A temp vector of integers used to normalise the divisors coming
			// out of the integration operation from the trig keys.
			std::vector<integer> tmp_int;
			// Build the return value.
			const auto it_f = this->m_container.end();
			for (auto it = this->m_container.begin(); it != it_f; ++it) {
				// Clear the tmp integer vector.
				tmp_int.clear();
				// Get the vector of trigonometric multipliers.
				const auto trig_vector = it->m_key.unpack(this->m_symbol_set);
				// Copy it over to the tmp_int as integer values.
				std::transform(trig_vector.begin(),trig_vector.end(),std::back_inserter(tmp_int),
					[](const k_value_type &n) {return integer(n);});
				// Determine the common divisor and if a sign flip is needed.
				integer cd(0);
				bool need_sign_flip = false, first_nonzero_found = false;
				for (auto it2 = tmp_int.begin(); it2 != tmp_int.end(); ++it2) {
					cd = detail::gcd(cd,*it2);
					if (!first_nonzero_found && !math::is_zero(*it2)) {
						if (*it2 < 0) {
							need_sign_flip = true;
						}
						first_nonzero_found = true;
					}
				}
				if (unlikely(math::is_zero(cd))) {
					piranha_throw(std::invalid_argument,"an invalid trigonometric term was encountered while "
						"attempting a time integration");
				}
				// Take the abs of the cd.
				cd = cd.abs();
				// Divide the vector by the common divisor, and flip the sign if needed.
				for (auto it2 = tmp_int.begin(); it2 != tmp_int.end(); ++it2) {
					*it2 /= cd;
					if (need_sign_flip) {
						it2->negate();
					}
				}
				// Build first the divisor series - the coefficient of the term to be inserted
				// into retval.
				typename return_type::term_type::cf_type div_series;
				// Set the arguments of the divisor series.
				div_series.set_symbol_set(div_symbols);
				// The coefficient of the only term of the divisor series is the original coefficient
				// multiplied by any sign change from the integration or the change in sign in the divisors,
				// and divided by the common divisor (cast to the appropriate type).
				// NOTE: probably the sign flip is never needed, as the canonical form of the trigonometric keys
				// also enforces a first nonzero multiplier.
				typename return_type::term_type::cf_type::term_type::cf_type div_cf = (it->m_cf *
					((it->m_key.get_flavour() ? 1 : -1) * (need_sign_flip ? -1 : 1))) /
					static_cast<typename div_type::value_type>(cd);
				// Build the divisor.
				typename div_type::value_type exponent(1);
				typename return_type::term_type::cf_type::term_type::key_type div_key;
				div_key.insert(tmp_int.begin(),tmp_int.end(),exponent);
				// Insert the term into the divisor series.
				div_series.insert(typename return_type::term_type::cf_type::term_type{std::move(div_cf),std::move(div_key)});
				// Insert into the return value.
				auto tmp_key = it->m_key;
				// Switch the flavour for integration.
				tmp_key.set_flavour(!tmp_key.get_flavour());
				retval.insert(typename return_type::term_type{std::move(div_series),std::move(tmp_key)});
			}
			return retval;
		}
		// Final type definition.
		template <typename T>
		using ti_type = decltype(std::declval<const T &>().t_integrate_impl());
		// Enabler for the integration method - thhis will have to be modified once we have proper enabling.
		template <typename T>
		using integrate_enabler = typename std::enable_if<detail::has_t_integrate<T>::value,int>::type;
		// Serialization.
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
	public:
		/// Series rebind alias.
		template <typename Cf2>
		using rebind = poisson_series<Cf2>;
		/// Defaulted default constructor.
		/**
		 * Will construct a Poisson series with zero terms.
		 */
		poisson_series() = default;
		/// Defaulted copy constructor.
		poisson_series(const poisson_series &) = default;
		/// Defaulted move constructor.
		poisson_series(poisson_series &&) = default;
		PIRANHA_FORWARDING_CTOR(poisson_series,base)
		/// Trivial destructor.
		~poisson_series()
		{
			PIRANHA_TT_CHECK(is_series,poisson_series);
		}
		/// Defaulted copy assignment operator.
		poisson_series &operator=(const poisson_series &) = default;
		/// Defaulted move assignment operator.
		poisson_series &operator=(poisson_series &&) = default;
		PIRANHA_FORWARDING_ASSIGNMENT(poisson_series,base)
		/// Override sine implementation.
		/**
		 * \note
		 * This template method is enabled only if either:
		 * - the coefficient type is a piranha::polynomial whose coefficient type supports
		 *   piranha::safe_cast(), or
		 * - the coefficient type has a math::sin() implementation whose return type is the coefficient type.
		 *
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
		 * - piranha::symbol_set::add(),
		 * - memory allocation errors in standard containers,
		 * - the constructors of coefficient, key and term types,
		 * - the cast operator of piranha::integer,
		 * - piranha::math::negate(),
		 * - piranha::math::sin(),
		 * - the extraction of a linear combination of integral arguments from the polynomial coefficient.
		 */
		template <typename T = poisson_series, sin_cos_enabler<T> = 0>
		poisson_series sin() const
		{
			return sin_cos_impl<false,T>();
		}
		/// Override cosine implementation.
		/**
		 * \note
		 * This template method is enabled only if either:
		 * - the coefficient type is a piranha::polynomial whose coefficient type supports
		 *   piranha::safe_cast() to piranha::integer, or
		 * - the coefficient type has a math::cos() implementation whose return type is the coefficient type.
		 *
		 * The procedure is the same as explained in sin().
		 * 
		 * @return cosine of \p this.
		 * 
		 * @throws unspecified any exception thrown by sin().
		 */
		template <typename T = poisson_series, sin_cos_enabler<T> = 0>
		poisson_series cos() const
		{
			return sin_cos_impl<true,T>();
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
		 * - piranha::math::partial(), piranha::math::is_zero(), piranha::math::integrate(), piranha::safe_cast() and
		 *   piranha::math::negate(),
		 * - the assignment operator of piranha::symbol_set,
		 * - term construction,
		 * - coefficient construction, assignment and arithmetics,
		 * - integration, construction and assignment of the key type,
		 * - insert(),
		 * - piranha::polynomial::degree(),
		 * - series arithmetics.
		 * 
		 * \todo requirements on dividability by multiplier type (or integer), safe_cast, etc.
		 */
		// \todo this also needs to be able to deduce the integration type. When that is done, we need to make sure the math::integrate
		// overload and the exposition in pyranha are correct too (as we did at the time for partial()). Test for instance that integration
		// of a polynomial/ps with integer coefficients and rational exponent generates rational coefficients.
		template <typename T = poisson_series, integrate_enabler<T> = 0>
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
				if (key_int.first == 0) {
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
		/// Time integration.
		/**
		 * \note
		 * This method is enabled only if:
		 * - the calling series is not already an echeloned Poisson series,
		 * - the operations required by the computation of the time integration are supported by all
		 *   the involved types.
		 *
		 * This is a special type of integration in which the trigonometric arguments are considered as linear functions
		 * of time, and in which the integration variable is time itself. The result of the operation is a so-called echeloned
		 * Poisson series, that it, a Poisson series in which the coefficient is a piranha::divisor_series whose coefficient type
		 * is the original coefficient type of the Poisson series.
		 *
		 * For instance, if the original series is
		 * \f[
		 * \frac{1}{5}z\cos\left( x - y \right),
		 * \f]
		 * the result of the time integration is
		 * \f[
		 * \frac{1}{5}{z}\frac{1}{\left(\nu_{x}-\nu_{y}\right)}\sin{\left({x}-{y}\right)},
		 * \f]
		 * where \f$ \nu_{x} \f$ and \f$ \nu_{y} \f$ are the frequencies associated to \f$ x \f$ and \f$ y \f$ (that is,
		 * \f$ x = \nu_{x}t \f$ and \f$ x = \nu_{y}t \f$).
		 *
		 * This method will throw an error if any term of the calling series has a unitary key (e.g., in the Poisson series
		 * \f$ \frac{1}{5}z \f$ the only trigonometric key is \f$ \cos\left( 0 \right) \f$ and would thus result in a division by zero
		 * during a time integration).
		 *
		 * @return the result of the time integration.
		 *
		 * @throws unspecified any exception thrown by:
		 * - memory errors in standard containers,
		 * - the public interfaces of piranha::symbol_set, piranha::mp_integer and piranha::series,
		 * - piranha::math::is_zero(),
		 * - the mathematical operations needed to compute the result,
		 * - piranha::divisor::insert(),
		 * - construction of the involved types.
		 */
		template <typename T = poisson_series>
		ti_type<T> t_integrate() const
		{
			return t_integrate_impl();
		}
};

namespace math
{

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
	template <typename T>
	auto operator()(const T &s, const std::string &name) -> decltype(s.integrate(name))
	{
		return s.integrate(name);
	}
};

}

}

#endif
