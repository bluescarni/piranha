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

#include "../src/polynomial.hpp"

using namespace piranha;

typedef boost::mpl::vector<double,integer> cf_types;
typedef boost::mpl::vector<int,integer> expo_types;

// TODO missing tests with degree only in the coefficient -> implement them once we have poisson series.

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
			BOOST_CHECK((std::is_same<Expo,decltype(p_type1{}.degree({}))>::value));
			BOOST_CHECK((std::is_same<Expo,decltype(p_type1{}.ldegree())>::value));
			BOOST_CHECK((std::is_same<Expo,decltype(p_type1{}.ldegree({}))>::value));
			BOOST_CHECK(p_type1{}.degree() == 0);
			BOOST_CHECK(p_type1{}.degree({}) == 0);
			BOOST_CHECK(p_type1{}.ldegree() == 0);
			BOOST_CHECK(p_type1{}.ldegree({}) == 0);
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
			BOOST_CHECK((p_type1{"x"} + p_type1{"y"} + p_type1{1}).degree({"y"}) == 1);
			BOOST_CHECK((p_type1{"x"} + p_type1{"y"} + p_type1{1}).degree({"z"}) == 0);
			BOOST_CHECK((p_type1{"x"} + p_type1{"y"} + p_type1{1}).ldegree() == 0);
			BOOST_CHECK((p_type1{"x"} + p_type1{"y"} + p_type1{1}).ldegree({"x"}) == 0);
			BOOST_CHECK((p_type1{"x"} + p_type1{"y"} + p_type1{1}).ldegree({"y"}) == 0);
			BOOST_CHECK((p_type1{"x"} + p_type1{"y"} + p_type1{1}).ldegree({"z"}) == 0);
			BOOST_CHECK((p_type1{"x"} * p_type1{"x"} + p_type1{"y"} + p_type1{"x"}).ldegree() == 1);
			BOOST_CHECK((p_type1{"x"} * p_type1{"x"} + p_type1{"y"} + p_type1{"x"}).ldegree({"x"}) == 0);
			BOOST_CHECK((p_type1{"x"} * p_type1{"x"} + 2 * p_type1{"x"}).ldegree({"x"}) == 1);
			BOOST_CHECK((p_type1{"x"} * p_type1{"y"} + 2 * p_type1{"x"}).ldegree({"x"}) == 1);
			BOOST_CHECK((p_type1{"x"} * p_type1{"y"} + 2 * p_type1{"x"}).ldegree({"y"}) == 0);
			std::set<std::string> empty_set;
			BOOST_CHECK((std::is_same<decltype(std::declval<p_type11>().degree()),decltype(std::declval<Expo>() + std::declval<int>())>::value));
			BOOST_CHECK((std::is_same<decltype(std::declval<p_type11>().degree(empty_set)),decltype(std::declval<Expo>() + std::declval<int>())>::value));
			BOOST_CHECK((std::is_same<decltype(std::declval<p_type11>().ldegree()),decltype(std::declval<Expo>() + std::declval<int>())>::value));
			BOOST_CHECK((std::is_same<decltype(std::declval<p_type11>().ldegree(empty_set)),decltype(std::declval<Expo>() + std::declval<int>())>::value));
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"}).degree() == 2);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"}).degree({"x"}) == 1);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"}).degree({"y"}) == 1);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"}).ldegree() == 1);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"}).ldegree({"y"}) == 1);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"}).ldegree({"z"}) == 0);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"} + 1).ldegree() == 0);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"} + 1).ldegree({"x"}) == 0);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"} + 1).ldegree({"y"}) == 0);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"} + 1).degree() == 3);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"} + 1).degree({"x"}) == 1);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"} + 1).degree({"y"}) == 2);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"} + 1).ldegree() == 0);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"}).ldegree({"x"}) == 0);
			BOOST_CHECK((p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"}).ldegree({"y"}) == 1);
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
	boost::mpl::for_each<cf_types>(degree_tester());
}
