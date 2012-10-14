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

#include "../src/power_series_term.hpp"

#define BOOST_TEST_MODULE power_series_term_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <type_traits>

#include "../src/environment.hpp"
#include "../src/integer.hpp"
#include "../src/polynomial.hpp"
#include "../src/polynomial_term.hpp"
#include "../src/symbol_set.hpp"
#include "../src/symbol.hpp"

using namespace piranha;

typedef boost::mpl::vector<double,integer> cf_types;
typedef boost::mpl::vector<int,integer> expo_types;

// TODO: missing tests when only coefficient has degree. Implement when Poisson series are available.
// Also, implement tests for power series terms that do not satisfy the requirements.

struct degree_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef polynomial_term<Cf,int> term_type1;
			typedef typename term_type1::key_type key_type1;
			BOOST_CHECK((std::is_same<int,decltype(term_type1{}.degree(std::declval<symbol_set>()))>::value));
			BOOST_CHECK((std::is_same<int,decltype(term_type1{}.degree({},std::declval<symbol_set>()))>::value));
			BOOST_CHECK((std::is_same<int,decltype(term_type1{}.ldegree(std::declval<symbol_set>()))>::value));
			BOOST_CHECK((std::is_same<int,decltype(term_type1{}.ldegree({},std::declval<symbol_set>()))>::value));
			BOOST_CHECK(term_type1{}.degree(symbol_set{}) == 0);
			BOOST_CHECK((term_type1{1,key_type1{1}}.degree(symbol_set{symbol("a")}) == 1));
			BOOST_CHECK((term_type1{1,key_type1{1,2}}.degree(symbol_set{symbol("a"),symbol("b")}) == 3));
			BOOST_CHECK((std::is_same<int,decltype(term_type1{}.degree(symbol_set{}))>::value));
			BOOST_CHECK((term_type1{1,key_type1{1,2}}.degree({"b"},symbol_set{symbol("a"),symbol("b")}) == 2));
			BOOST_CHECK((term_type1{1,key_type1{1,2}}.degree({},symbol_set{symbol("a"),symbol("b")}) == 0));
			BOOST_CHECK(term_type1{}.ldegree(symbol_set{}) == 0);
			BOOST_CHECK((term_type1{1,key_type1{1}}.ldegree(symbol_set{symbol("a")}) == 1));
			BOOST_CHECK((term_type1{1,key_type1{1,2}}.ldegree(symbol_set{symbol("a"),symbol("b")}) == 3));
			BOOST_CHECK((std::is_same<int,decltype(term_type1{}.ldegree(symbol_set{}))>::value));
			BOOST_CHECK((term_type1{1,key_type1{1,2}}.ldegree({"b"},symbol_set{symbol("a"),symbol("b")}) == 2));
			BOOST_CHECK((term_type1{1,key_type1{1,2}}.ldegree({},symbol_set{symbol("a"),symbol("b")}) == 0));
			typedef polynomial_term<polynomial<Cf,Expo>,int> term_type2;
			typedef typename term_type2::cf_type cf_type2;
			typedef typename term_type2::key_type key_type2;
			BOOST_CHECK((std::is_same<Expo,decltype(term_type2{}.degree(std::declval<symbol_set>()))>::value));
			BOOST_CHECK((std::is_same<Expo,decltype(term_type2{}.degree({},std::declval<symbol_set>()))>::value));
			BOOST_CHECK((std::is_same<Expo,decltype(term_type2{}.ldegree(std::declval<symbol_set>()))>::value));
			BOOST_CHECK((std::is_same<Expo,decltype(term_type2{}.ldegree({},std::declval<symbol_set>()))>::value));
			BOOST_CHECK(term_type2{}.degree(symbol_set{}) == 0);
			BOOST_CHECK((term_type2{cf_type2(1),key_type2{1}}.degree(symbol_set{symbol("a")}) == 1));
			BOOST_CHECK((term_type2{cf_type2(1),key_type2{1,2}}.degree(symbol_set{symbol("a"),symbol("b")}) == 3));
			BOOST_CHECK((std::is_same<decltype(std::declval<Expo>() + std::declval<int>()),decltype(term_type2{}.degree(symbol_set{}))>::value));
			BOOST_CHECK((term_type2{cf_type2("a"),key_type2{1,2}}.degree(symbol_set{symbol("a"),symbol("b")}) == 4));
			BOOST_CHECK((term_type2{cf_type2(1),key_type2{1,2}}.degree({"b"},symbol_set{symbol("a"),symbol("b")}) == 2));
			BOOST_CHECK((term_type2{cf_type2("b"),key_type2{1,2}}.degree({"b"},symbol_set{symbol("a"),symbol("b")}) == 3));
			BOOST_CHECK((term_type2{cf_type2("a"),key_type2{1,2}}.degree({"b"},symbol_set{symbol("a"),symbol("b")}) == 2));
			BOOST_CHECK((term_type2{cf_type2(1),key_type2{1,2}}.degree({},symbol_set{symbol("a"),symbol("b")}) == 0));
			BOOST_CHECK(term_type2{}.ldegree(symbol_set{}) == 0);
			BOOST_CHECK((term_type2{cf_type2(1),key_type2{1}}.ldegree(symbol_set{symbol("a")}) == 1));
			BOOST_CHECK((term_type2{cf_type2(1),key_type2{1,2}}.ldegree(symbol_set{symbol("a"),symbol("b")}) == 3));
			BOOST_CHECK((std::is_same<decltype(std::declval<Expo>() + std::declval<int>()),decltype(term_type2{}.ldegree(symbol_set{}))>::value));
			BOOST_CHECK((term_type2{cf_type2(1),key_type2{1,2}}.ldegree({"b"},symbol_set{symbol("a"),symbol("b")}) == 2));
			BOOST_CHECK((term_type2{cf_type2(1),key_type2{1,2}}.ldegree({},symbol_set{symbol("a"),symbol("b")}) == 0));
			BOOST_CHECK((term_type2{cf_type2("a") + cf_type2("b"),key_type2{1,2}}.ldegree({"a"},symbol_set{symbol("a"),symbol("b")}) == 1));
			BOOST_CHECK((term_type2{cf_type2("a") + cf_type2("b") * cf_type2("a"),key_type2{1,2}}.ldegree({"a"},symbol_set{symbol("a"),symbol("b")}) == 2));
			BOOST_CHECK((term_type2{cf_type2("a") + cf_type2("b") * cf_type2("a") + 1,key_type2{1,2}}.ldegree({"a"},symbol_set{symbol("a"),symbol("b")}) == 1));
			// Check type traits.
			BOOST_CHECK(is_power_series_term<term_type1>::value);
			BOOST_CHECK(is_power_series_term<term_type2>::value);
			BOOST_CHECK((std::is_constructible<term_type1,Cf,key_type1>::value));
			BOOST_CHECK((!std::is_constructible<term_type1,Cf,std::string>::value));
			BOOST_CHECK((!std::is_constructible<term_type1,Cf,std::string,int>::value));
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(power_series_term_degree_test)
{
	environment env;
	boost::mpl::for_each<cf_types>(degree_tester());
}
