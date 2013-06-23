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

#include "../src/power_series.hpp"

#define BOOST_TEST_MODULE power_series_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <set>
#include <string>
#include <type_traits>
#include <utility>

#include "../src/environment.hpp"
#include "../src/poisson_series_term.hpp"
#include "../src/polynomial.hpp"
#include "../src/polynomial_term.hpp"
#include "../src/rational.hpp"
#include "../src/real.hpp"
#include "../src/series.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

typedef boost::mpl::vector<double,integer,rational,real> cf_types;
typedef boost::mpl::vector<int,integer> expo_types;

template <typename Cf, typename Expo>
class g_series_type: public power_series<series<polynomial_term<Cf,Expo>,g_series_type<Cf,Expo>>>
{
	public:
		typedef power_series<series<polynomial_term<Cf,Expo>,g_series_type<Cf,Expo>>> base;
		g_series_type() = default;
		g_series_type(const g_series_type &) = default;
		g_series_type(g_series_type &&) = default;
		g_series_type &operator=(const g_series_type &) = default;
		g_series_type &operator=(g_series_type &&) = default;
		PIRANHA_FORWARDING_CTOR(g_series_type,base)
		PIRANHA_FORWARDING_ASSIGNMENT(g_series_type,base)
};

template <typename Cf>
class g_series_type2: public power_series<series<poisson_series_term<Cf>,g_series_type2<Cf>>>
{
	public:
		typedef power_series<series<poisson_series_term<Cf>,g_series_type2<Cf>>> base;
		g_series_type2() = default;
		g_series_type2(const g_series_type2 &) = default;
		g_series_type2(g_series_type2 &&) = default;
		g_series_type2 &operator=(const g_series_type2 &) = default;
		g_series_type2 &operator=(g_series_type2 &&) = default;
		PIRANHA_FORWARDING_CTOR(g_series_type2,base)
		PIRANHA_FORWARDING_ASSIGNMENT(g_series_type2,base)
};

// TODO missing tests with degree only in the coefficient -> implement them once we have poisson series.
// Also, implement tests for power series that do not satisfy the requirements.
struct degree_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef polynomial<Cf,Expo> p_type1;
			typedef polynomial<polynomial<Cf,int>,Expo> p_type11;
			BOOST_CHECK((std::is_same<Expo,decltype(p_type1{}.degree())>::value));
			BOOST_CHECK((std::is_same<Expo,decltype(p_type1{}.degree(std::set<std::string>{}))>::value));
			BOOST_CHECK((std::is_same<Expo,decltype(p_type1{}.ldegree())>::value));
			BOOST_CHECK((std::is_same<Expo,decltype(p_type1{}.ldegree(std::set<std::string>{}))>::value));
			BOOST_CHECK(p_type1{}.degree() == 0);
			BOOST_CHECK(p_type1{}.degree(std::set<std::string>{}) == 0);
			BOOST_CHECK(p_type1{}.ldegree() == 0);
			BOOST_CHECK(p_type1{}.ldegree(std::set<std::string>{}) == 0);
			BOOST_CHECK((std::is_same<Expo,decltype(p_type1{}.degree())>::value));
			BOOST_CHECK(p_type1{"x"}.degree() == 1);
			BOOST_CHECK(p_type1{"x"}.degree({"x"}) == 1);
			BOOST_CHECK(p_type1{"x"}.degree({"y"}) == 0);
			BOOST_CHECK(p_type1{"x"}.ldegree() == 1);
			BOOST_CHECK(p_type1{"x"}.ldegree({"x"}) == 1);
			BOOST_CHECK(p_type1{"x"}.ldegree({"y"}) == 0);
			BOOST_CHECK((p_type1{"x"} * p_type1{"x"}).degree() == 2);
			BOOST_CHECK((p_type1{"x"} * p_type1{"x"}).degree({"x"}) == 2);
			BOOST_CHECK((p_type1{"x"} * p_type1{"y"}).degree({"y"}) == 1);
			BOOST_CHECK((p_type1{"x"} * p_type1{"x"}).ldegree() == 2);
			BOOST_CHECK((p_type1{"x"} * p_type1{"x"}).ldegree({"x"}) == 2);
			BOOST_CHECK((p_type1{"x"} * p_type1{"y"}).ldegree({"y"}) == 1);
			BOOST_CHECK((p_type1{"x"} + p_type1{"y"} + p_type1{1}).degree() == 1);
			BOOST_CHECK((p_type1{"x"} + p_type1{"y"} + p_type1{1}).degree({"x"}) == 1);
			BOOST_CHECK((p_type1{"x"} + p_type1{"y"} + p_type1{1}).degree("x") == 1);
			BOOST_CHECK((p_type1{"x"} + p_type1{"y"} + p_type1{1}).degree({"y"}) == 1);
			BOOST_CHECK((p_type1{"x"} + p_type1{"y"} + p_type1{1}).degree("y") == 1);
			BOOST_CHECK((p_type1{"x"} + p_type1{"y"} + p_type1{1}).degree({"z"}) == 0);
			BOOST_CHECK((p_type1{"x"} + p_type1{"y"} + p_type1{1}).degree("z") == 0);
			BOOST_CHECK((p_type1{"x"} + p_type1{"y"} + p_type1{1}).ldegree() == 0);
			BOOST_CHECK((p_type1{"x"} + p_type1{"y"} + p_type1{1}).ldegree({"x"}) == 0);
			BOOST_CHECK((p_type1{"x"} + p_type1{"y"} + p_type1{1}).ldegree("x") == 0);
			BOOST_CHECK((p_type1{"x"} + p_type1{"y"} + p_type1{1}).ldegree({"y"}) == 0);
			BOOST_CHECK((p_type1{"x"} + p_type1{"y"} + p_type1{1}).ldegree("y") == 0);
			BOOST_CHECK((p_type1{"x"} + p_type1{"y"} + p_type1{1}).ldegree({"z"}) == 0);
			BOOST_CHECK((p_type1{"x"} + p_type1{"y"} + p_type1{1}).ldegree("z") == 0);
			BOOST_CHECK((p_type1{"x"} * p_type1{"x"} + p_type1{"y"} + p_type1{"x"}).ldegree() == 1);
			BOOST_CHECK((p_type1{"x"} * p_type1{"x"} + p_type1{"y"} + p_type1{"x"}).ldegree({"x"}) == 0);
			BOOST_CHECK((p_type1{"x"} * p_type1{"x"} + p_type1{"y"} + p_type1{"x"}).ldegree("x") == 0);
			BOOST_CHECK((p_type1{"x"} * p_type1{"x"} + 2 * p_type1{"x"}).ldegree({"x"}) == 1);
			BOOST_CHECK((p_type1{"x"} * p_type1{"y"} + 2 * p_type1{"x"}).ldegree({"x"}) == 1);
			BOOST_CHECK((p_type1{"x"} * p_type1{"y"} + 2 * p_type1{"x"}).ldegree({"y"}) == 0);
			std::set<std::string> empty_set;
			BOOST_CHECK((std::is_same<decltype(std::declval<const p_type11 &>().degree()),decltype(std::declval<Expo>() + std::declval<int>())>::value));
			BOOST_CHECK((std::is_same<decltype(std::declval<const p_type11 &>().degree(empty_set)),decltype(std::declval<Expo>() + std::declval<int>())>::value));
			BOOST_CHECK((std::is_same<decltype(std::declval<const p_type11 &>().ldegree()),decltype(std::declval<Expo>() + std::declval<int>())>::value));
			BOOST_CHECK((std::is_same<decltype(std::declval<const p_type11 &>().ldegree(empty_set)),decltype(std::declval<Expo>() + std::declval<int>())>::value));
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"}).degree() == 2);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"}).degree({"x"}) == 1);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"}).degree("x") == 1);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"}).degree({"y"}) == 1);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"}).ldegree() == 1);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"}).ldegree({"y"}) == 1);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"}).ldegree("y") == 1);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"}).ldegree({"z"}) == 0);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"}).ldegree("z") == 0);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"} + 1).ldegree() == 0);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"} + 1).ldegree({"x"}) == 0);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"} + 1).ldegree({"y"}) == 0);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"} + 1).degree() == 3);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"} + 1).degree({"x"}) == 1);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"} + 1).degree({"y"}) == 2);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"} + 1).degree("y") == 2);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"} + 1).ldegree() == 0);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"}).ldegree({"x"}) == 0);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"}).ldegree({"y"}) == 1);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"}).ldegree("y") == 1);
			// Test the type traits.
			BOOST_CHECK(has_degree<p_type1>::value);
			BOOST_CHECK(has_degree<p_type11>::value);
			BOOST_CHECK(has_ldegree<p_type1>::value);
			BOOST_CHECK(has_ldegree<p_type11>::value);
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(power_series_degree_test)
{
	environment env;
	//boost::mpl::for_each<cf_types>(degree_tester());
}

BOOST_AUTO_TEST_CASE(power_series_tester)
{
	typedef g_series_type<double,int> stype1;
	BOOST_CHECK(has_degree<stype1>::value);
	BOOST_CHECK(has_ldegree<stype1>::value);
	stype1 s;
	s.degree();
	s.degree({"x"});
/*
	typedef g_series_type2<double> stype2;
	stype2 s2;
	//s2.degree();
*/
}
