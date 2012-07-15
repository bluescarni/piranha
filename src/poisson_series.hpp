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
#include <boost/concept/assert.hpp>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "concepts/series.hpp"
#include "config.hpp"
#include "detail/poisson_series_fwd.hpp"
#include "detail/polynomial_fwd.hpp"
#include "integer.hpp"
#include "math.hpp"
#include "poisson_series_term.hpp"
#include "power_series.hpp"
#include "series.hpp"
#include "symbol.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Poisson series class.
/**
 * This class represents multivariate Poisson series as collections of multivariate Poisson series terms
 * (represented by the piranha::poisson_series_term class). The coefficient
 * type \p Cf represents the ring over which the Poisson series is defined.
 * 
 * This class is a model of the piranha::concept::Series concept.
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
 * 
 * \todo the h_degree methods should probably go in the future in a harmonic_series toolbox (which, contrary to the power_series
 * toolbox, would allow only either cf or key to have harmonic degree). This should all be wrapped up in a type-trait/concept thing
 * similarly to the upcoming power_series type-trait rework. The harmonic order Morbidelli talks about should go in it as well.
 */
template <typename Cf>
class poisson_series:
	public power_series<series<poisson_series_term<Cf>,poisson_series<Cf>>>,
	detail::poisson_series_tag
{
		typedef power_series<series<poisson_series_term<Cf>,poisson_series<Cf>>> base;
		template <typename T, typename... Args>
		struct generic_enabler
		{
			static const bool value = (sizeof...(Args) != 0u) ||
				(!std::is_same<typename std::decay<T>::type,poisson_series>::value &&
				!std::is_same<typename std::decay<T>::type,char *>::value &&
				!std::is_same<typename std::decay<T>::type,std::string>::value);
		};
		template <bool IsCos, typename T>
		poisson_series sin_cos_impl(const T &, typename std::enable_if<
			std::is_same<T,std::true_type>::value>::type * = piranha_nullptr) const
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
			std::is_same<T,std::false_type>::value>::type * = piranha_nullptr) const
		{
			return sin_cos_cf_impl<IsCos>();
		}
		template <bool IsCos>
		poisson_series sin_cos_cf_impl() const
		{
			if (IsCos) {
				// NOTE: here we cast back to the base class, and then we have to move-construct the output
				// Poisson series as the math::cos functor will produce an output of the type of the base class.
				return poisson_series(math::cos(
					*static_cast<series<poisson_series_term<Cf>,poisson_series<Cf>> const *>(this)));
			} else {
				return poisson_series(math::sin(
					*static_cast<series<poisson_series_term<Cf>,poisson_series<Cf>> const *>(this)));
			}
		}
		// Subs typedef.
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
		// Harmonic degree utilities.
		template <typename... Args>
		struct h_degree_type
		{
			typedef typename base::term_type::key_type key_type;
			typedef decltype(std::declval<key_type>().h_degree(
				std::declval<typename std::decay<Args>::type>()...,std::declval<symbol_set>())) type;
		};
		template <typename... Args>
		struct h_ldegree_type
		{
			typedef typename base::term_type::key_type key_type;
			typedef decltype(std::declval<key_type>().h_ldegree(
				std::declval<typename std::decay<Args>::type>()...,std::declval<symbol_set>())) type;
		};
		template <typename... Args>
		typename h_degree_type<Args ...>::type h_degree_impl(Args && ... params) const
		{
			// NOTE: this code is taken from power series, keep it in mind
			// if it gets changed.
			typedef typename h_degree_type<Args ...>::type return_type;
			if (this->empty()) {
				return return_type(0);
			}
			auto it = this->m_container.begin();
			const auto it_f = this->m_container.end();
			return_type retval = it->m_key.h_degree(std::forward<Args>(params)...,this->m_symbol_set);
			++it;
			return_type tmp;
			for (; it != it_f; ++it) {
				tmp = it->m_key.h_degree(std::forward<Args>(params)...,this->m_symbol_set);
				if (tmp > retval) {
					retval = std::move(tmp);
				}
			}
			return retval;
		}
		template <typename... Args>
		typename h_ldegree_type<Args ...>::type h_ldegree_impl(Args && ... params) const
		{
			typedef typename h_ldegree_type<Args ...>::type return_type;
			if (this->empty()) {
				return return_type(0);
			}
			auto it = this->m_container.begin();
			const auto it_f = this->m_container.end();
			return_type retval = it->m_key.h_ldegree(std::forward<Args>(params)...,this->m_symbol_set);
			++it;
			return_type tmp;
			for (; it != it_f; ++it) {
				tmp = it->m_key.h_ldegree(std::forward<Args>(params)...,this->m_symbol_set);
				if (tmp < retval) {
					retval = std::move(tmp);
				}
			}
			return retval;
		}
	public:
		/// Defaulted default constructor.
		/**
		 * Will construct a Poisson series with zero terms.
		 */
		poisson_series() = default;
		/// Defaulted copy constructor.
		poisson_series(const poisson_series &) = default;
		/// Defaulted move constructor.
		poisson_series(poisson_series &&) = default;
		/// Constructor from string.
		/**
		 * This constructor is enabled only if \p Str is a string type (either C or C++) and the coefficient type of the series
		 * is an instance of piranha::polyomial. The string will be forwarded to the constructor of the base series and will result
		 * in the construction of a single-coefficient Poisson series in which the only coefficient is a polynomial representing
		 * the symbolic quantity \p str.
		 * 
		 * @param[in] str name of the symbolic quantity that the constructed series will represent.
		 * 
		 * @throws unspecified any exception thrown by the invoked based constructor.
		 */
		template <typename Str>
		poisson_series(Str &&str, typename std::enable_if<
			(std::is_same<typename std::decay<Str>::type,std::string>::value ||
			std::is_same<typename std::decay<Str>::type,char *>::value) &&
			std::is_base_of<detail::polynomial_tag,Cf>::value>::type * = piranha_nullptr) : base(std::forward<Str>(str))
		{}
		/// Generic constructor.
		/**
		 * This constructor, activated only if the number of arguments is at least 2 or if the only argument is not of type
		 * piranha::poisson_series or string, will perfectly forward its arguments to a constructor in the base class.
		 * 
		 * @param[in] arg1 first argument for construction.
		 * @param[in] argn additional construction arguments.
		 * 
		 * @throws unspecified any exception thrown by the invoked base constructor.
		 */
		template <typename T, typename... Args, typename std::enable_if<generic_enabler<T,Args...>::value>::type*& = enabler>
		explicit poisson_series(T &&arg1, Args && ... argn) : base(std::forward<T>(arg1),std::forward<Args>(argn)...) {}
		/// Trivial destructor.
		~poisson_series() piranha_noexcept_spec(true)
		{
			BOOST_CONCEPT_ASSERT((concept::Series<poisson_series>));
		}
		/// Defaulted copy assignment operator.
		poisson_series &operator=(const poisson_series &) = default;
		/// Move assignment operator.
		/**
		 * @param[in] other assignment argument.
		 * 
		 * @return reference to \p this.
		 */
		poisson_series &operator=(poisson_series &&other) piranha_noexcept_spec(true)
		{
			base::operator=(std::move(other));
			return *this;
		}
		/// Assignment from string.
		/**
		 * This operator is enabled only if \p Str is a string type (either C or C++) and the coefficient type of the series
		 * is an instance of piranha::polyomial. The operation is equivalent to assignment to a series constructed from \p str.
		 * 
		 * @param[in] str name of the symbolic quantity that will be assigned to \p this.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the constructor from string.
		 */
		template <typename Str>
		typename std::enable_if<(std::is_same<typename std::decay<Str>::type,std::string>::value ||
			std::is_same<typename std::decay<Str>::type,char *>::value) &&
			std::is_base_of<detail::polynomial_tag,Cf>::value,poisson_series &>::type operator=(Str &&str)
		{
			operator=(poisson_series(std::forward<Str>(str)));
			return *this;
		}
		/// Generic assignment operator.
		/**
		 * Will forward the assignment to the base class. This assignment operator is activated only when \p T is not
		 * piranha::poisson_series or a string type.
		 * 
		 * @param[in] x assignment argument.
		 * 
		 * @return reference to \p this.
		 * 
		 * @throws unspecified any exception thrown by the assignment operator in the base class.
		 */
		template <typename T>
		typename std::enable_if<generic_enabler<T>::value,poisson_series &>::type operator=(T &&x)
		{
			base::operator=(std::forward<T>(x));
			return *this;
		}
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
		/// Harmonic degree.
		/**
		 * The harmonic degree of a Poisson series is defined in the same way as the degree in a polynomial,
		 * with the exponents replaced by the multipliers. That is, the harmonic degree of a term is the sum
		 * of its trigonometric multipliers, and the harmonic degree of a series is given by the term with the
		 * highest harmonic degree.
		 * 
		 * If the series is empty, zero will be returned.
		 * 
		 * @return the total harmonic degree of the series.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the construction of return type from zero,
		 * - the calculation of the degree of each term,
		 * - the assignment and greater-than operators for the return type.
		 * 
		 * \todo requirement on the degree type (less-than comparable, etc.), probably should fold them in with the new has_degree
		 * type-trait (and do the same for power_series_term).
		 */
		typename h_degree_type<>::type h_degree() const
		{
			return h_degree_impl();
		}
		/// Partial harmonic degree.
		/**
		 * Equivalent to the harmonic degree, but only the symbols in \p s are considered in the computation.
		 * 
		 * If the series is empty, zero will be returned.
		 * 
		 * @param[in] s list of names of the variables that will be considered in the computation.
		 * 
		 * @return the partial harmonic degree of the series.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the construction of return type from zero,
		 * - the calculation of the degree of each term,
		 * - the assignment and greater-than operators for the return type.
		 */
		typename h_degree_type<std::set<std::string>>::type h_degree(const std::set<std::string> &s) const
		{
			return h_degree_impl(s);
		}
		/// Harmonic low degree.
		/**
		 * The harmonic low degree of a Poisson series is defined in the same way as the low degree in a polynomial,
		 * with the exponents replaced by the multipliers. That is, the harmonic degree of a term is the sum
		 * of its trigonometric multipliers, and the harmonic low degree of a series is given by the term with the
		 * lowest harmonic degree.
		 * 
		 * If the series is empty, zero will be returned.
		 * 
		 * @return the total harmonic low degree of the series.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the construction of return type from zero,
		 * - the calculation of the degree of each term,
		 * - the assignment and less-than operators for the return type.
		 */
		typename h_ldegree_type<>::type h_ldegree() const
		{
			return h_ldegree_impl();
		}
		/// Partial harmonic low degree.
		/**
		 * Equivalent to the harmonic low degree, but only the symbols in \p s are considered in the computation.
		 * 
		 * If the series is empty, zero will be returned.
		 * 
		 * @param[in] s list of names of the variables that will be considered in the computation.
		 * 
		 * @return the partial harmonic low degree of the series.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - the construction of return type from zero,
		 * - the calculation of the degree of each term,
		 * - the assignment and less-than operators for the return type.
		 */
		typename h_ldegree_type<std::set<std::string>>::type h_ldegree(const std::set<std::string> &s) const
		{
			return h_ldegree_impl(s);
		}
};

namespace math
{

/// Specialisation of the piranha::math::subs() functor for Poisson series types.
/**
 * This specialisation is activated when \p Series is an instance of piranha::poisson_series.
 */
template <typename Series>
struct subs_impl<Series,typename std::enable_if<std::is_base_of<detail::poisson_series_tag,Series>::value>::type>
{
	private:
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

}

}

#endif
