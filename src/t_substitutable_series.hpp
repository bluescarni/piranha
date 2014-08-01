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

#ifndef PIRANHA_T_SUBSTITUTABLE_SERIES_HPP
#define PIRANHA_T_SUBSTITUTABLE_SERIES_HPP

#include <string>
#include <type_traits>
#include <utility>

#include "forwarding.hpp"
#include "math.hpp"
#include "series.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"

namespace piranha
{

/// Toolbox for series that support trigonometric substitution.
/**
 * This toolbox extends a series class with methods to perform trigonometric substitution (i.e., substitution of cosine
 * and sine of a symbolic variable). These methods are enabled only if either the coefficient or the key supports trigonometric
 * substitution, as established by the piranha::has_t_subs and piranha::key_has_t_subs type traits, and if
 * the types involved in the computation of the substitution are constructible from \p int and support basic arithmetics.
 * 
 * This class satisfies the piranha::is_series type trait.
 * 
 * \section type_requirements Type requirements
 *
 * \p Series must be an instance of piranha::series, and \p Derived must satisfy the piranha::is_series type trait.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the same guarantee as \p Series.
 * 
 * \section move_semantics Move semantics
 * 
 * Move semantics is equivalent to the move semantics of \p Series.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename Series, typename Derived>
class t_substitutable_series: public Series
{
		typedef Series base;
		PIRANHA_TT_CHECK(is_instance_of,base,series);
		// Detect t_subs term.
		template <typename Term, typename T, typename U>
		struct t_subs_term_score
		{
			static const unsigned value = static_cast<unsigned>(has_t_subs<typename Term::cf_type,T,U>::value) +
				(static_cast<unsigned>(key_has_t_subs<typename Term::key_type,T,U>::value) << 1u);
		};
		template <typename T, typename U, typename Term = typename Series::term_type, typename = void>
		struct t_subs_utils
		{
			static_assert(t_subs_term_score<Term,T,U>::value == 0u,"Wrong t_subs_term_score value.");
		};
		// Case 1: t_subs on cf only.
		template <typename T, typename U, typename Term>
		struct t_subs_utils<T,U,Term,typename std::enable_if<t_subs_term_score<Term,T,U>::value == 1u>::type>
		{
			#define PIRANHA_TMP_RETURN_TYPE decltype(math::t_subs(std::declval<typename Term1::cf_type const &>(),std::declval<std::string const &>(), \
				std::declval<T1 const &>(),std::declval<U1 const &>()) * std::declval<Derived const &>())
			template <typename T1, typename U1, typename Term1>
			using return_type = typename std::enable_if<std::is_constructible<PIRANHA_TMP_RETURN_TYPE,int>::value &&
				is_addable_in_place<PIRANHA_TMP_RETURN_TYPE>::value,PIRANHA_TMP_RETURN_TYPE>::type;
			template <typename T1, typename U1, typename Term1>
			static return_type<T1,U1,Term1> subs(const Term1 &t, const std::string &name, const T1 &c, const U1 &s, const symbol_set &s_set)
			{
				Derived tmp;
				tmp.m_symbol_set = s_set;
				tmp.insert(Term1(typename Term1::cf_type(1),t.m_key));
				return math::t_subs(t.m_cf,name,c,s) * tmp;
			}
			#undef PIRANHA_TMP_RETURN_TYPE
		};
		// Case 2: t_subs on key only.
		template <typename T, typename U, typename Term>
		struct t_subs_utils<T,U,Term,typename std::enable_if<t_subs_term_score<Term,T,U>::value == 2u>::type>
		{
			#define PIRANHA_TMP_RETURN_TYPE decltype(std::declval<typename Term1::key_type const &>().t_subs(std::declval<std::string const &>(), \
				std::declval<T1 const &>(),std::declval<U1 const &>(),std::declval<symbol_set const &>())[0u].first * std::declval<Derived const &>())
			template <typename T1, typename U1, typename Term1>
			using return_type = typename std::enable_if<std::is_constructible<PIRANHA_TMP_RETURN_TYPE,int>::value &&
				is_addable_in_place<PIRANHA_TMP_RETURN_TYPE>::value,PIRANHA_TMP_RETURN_TYPE>::type;
			template <typename T1, typename U1, typename Term1>
			static return_type<T1,U1,Term1> subs(const Term1 &t, const std::string &name, const T1 &c, const U1 &s, const symbol_set &s_set)
			{
				return_type<T1,U1,Term1> retval(0);
				const auto key_subs = t.m_key.t_subs(name,c,s,s_set);
				for (const auto &x: key_subs) {
					Derived tmp;
					tmp.m_symbol_set = s_set;
					tmp.insert(Term1(t.m_cf,x.second));
					retval += x.first * tmp;
				}
				return retval;
			}
			#undef PIRANHA_TMP_RETURN_TYPE
		};
		// NOTE: we have no way of testing this at the moment, so better leave it out at present time.
#if 0
		// Case 3: t_subs on cf and key.
		template <typename T, typename U, typename Term>
		struct t_subs_utils<T,U,Term,typename std::enable_if<t_subs_term_score<Term,T,U>::value == 3u>::type>
		{
			typedef decltype(std::declval<typename Term::key_type>().t_subs(std::declval<std::string>(),std::declval<T>(),
				std::declval<U>(),std::declval<symbol_set>())) key_t_subs_type;
			typedef decltype(math::t_subs(std::declval<Series>(),std::declval<std::string>(),std::declval<T>(),std::declval<U>()))
				cf_t_subs_type;
			typedef decltype(std::declval<typename key_t_subs_type::value_type::first_type>() * std::declval<Series>() *
				std::declval<cf_t_subs_type>()) type;
		};
#endif
	public:
		/// Defaulted default constructor.
		t_substitutable_series() = default;
		/// Defaulted copy constructor.
		t_substitutable_series(const t_substitutable_series &) = default;
		/// Defaulted move constructor.
		t_substitutable_series(t_substitutable_series &&) = default;
		PIRANHA_FORWARDING_CTOR(t_substitutable_series,base)
		/// Defaulted copy assignment operator.
		t_substitutable_series &operator=(const t_substitutable_series &) = default;
		/// Defaulted move assignment operator.
		t_substitutable_series &operator=(t_substitutable_series &&) = default;
		/// Trivial destructor.
		~t_substitutable_series()
		{
			PIRANHA_TT_CHECK(is_series,t_substitutable_series);
			PIRANHA_TT_CHECK(is_series,Derived);
		}
		PIRANHA_FORWARDING_ASSIGNMENT(t_substitutable_series,base)
		/// Trigonometric substitution.
		/**
		 * \note
		 * This method is available only if the requirements outlined in piranha::t_substitutable_series are satisfied.
		 * 
		 * Trigonometric substitution is the substitution of the cosine and sine of \p name for \p c and \p s.
		 * 
		 * @param[in] name name of the symbol that will be subject to substitution.
		 * @param[in] c cosine of \p name.
		 * @param[in] s sine of \p name.
		 * 
		 * @return the result of the trigonometric substitution.
		 * 
		 * @throws unspecified any exception resulting from:
		 * - construction of the return type,
		 * - the assignment operator of piranha::symbol_set,
		 * - term construction,
		 * - arithmetics on the intermediary values needed to compute the return value,
		 * - piranha::series::insert(),
		 * - the substitution methods of coefficient and key.
		 */
		template <typename T, typename U>
		auto t_subs(const std::string &name, const T &c, const U &s) const ->
			decltype(t_subs_utils<T,U>::subs(std::declval<typename Series::term_type const &>(),name,c,s,std::declval<symbol_set const &>()))
		{
			typedef decltype(this->t_subs(name,c,s)) ret_type;
			ret_type retval(0);
			for (const auto &t: this->m_container) {
				retval += t_subs_utils<T,U>::subs(t,name,c,s,this->m_symbol_set);
			}
			return retval;
		}
};

namespace math
{

/// Specialisation of the piranha::math::t_subs() functor for instances of piranha::t_substitutable_series.
/**
 * This specialisation is activated if \p Series is an instance of piranha::t_substitutable_series.
 */
template <typename Series>
struct t_subs_impl<Series,typename std::enable_if<is_instance_of<Series,t_substitutable_series>::value>::type>
{
	/// Call operator.
	/**
	 * \note
	 * This call operator is available only if \p Series satisfies the requirements outlined in piranha::t_substitutable_series.
	 *
	 * Will use piranha::t_substitutable_series::t_subs().
	 * 
	 * @param[in] series argument for the substitution.
	 * @param[in] name name of the symbol that will be subject to substitution.
	 * @param[in] c cosine of \p name.
	 * @param[in] s sine of \p name.
	 * 
	 * @return the result of the trigonometric substitution.
	 * 
	 * @throws unspecified any exception thrown by piranha::t_substitutable_series::t_subs().
	 */
	template <typename T, typename U, typename V>
	auto operator()(const T &series, const std::string &name, const U &c, const V &s) const ->
		decltype(series.t_subs(name,c,s))
	{
		return series.t_subs(name,c,s);
	}
};

}

}

#endif
