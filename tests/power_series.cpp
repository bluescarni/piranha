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
#include <cstddef>
#include <functional>
#include <iostream>
#include <set>
#include <string>
#include <type_traits>
#include <utility>

#include "../src/environment.hpp"
#include "../src/math.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/poisson_series.hpp"
#include "../src/poisson_series_term.hpp"
#include "../src/polynomial.hpp"
#include "../src/polynomial_term.hpp"
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

struct degree_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef polynomial<Cf,Expo> p_type1;
			typedef polynomial<polynomial<Cf,Expo>,Expo> p_type11;
			BOOST_CHECK((std::is_same<Expo,decltype(math::degree(p_type1{}))>::value));
			BOOST_CHECK((std::is_same<Expo,decltype(math::degree(p_type1{},std::set<std::string>{}))>::value));
			BOOST_CHECK((std::is_same<Expo,decltype(math::ldegree(p_type1{}))>::value));
			BOOST_CHECK((std::is_same<Expo,decltype(math::ldegree(p_type1{},std::set<std::string>{}))>::value));
			BOOST_CHECK(math::degree(p_type1{}) == 0);
			BOOST_CHECK(math::degree(p_type1{},std::set<std::string>{}) == 0);
			BOOST_CHECK(math::ldegree(p_type1{}) == 0);
			BOOST_CHECK(math::ldegree(p_type1{},std::set<std::string>{}) == 0);
			BOOST_CHECK(math::degree(p_type1{"x"}) == 1);
			BOOST_CHECK(math::degree(p_type1{"x"},{"x"}) == 1);
			BOOST_CHECK(math::degree(p_type1{"x"},{"y"}) == 0);
			BOOST_CHECK(math::ldegree(p_type1{"x"}) == 1);
			BOOST_CHECK(math::ldegree(p_type1{"x"},{"x"}) == 1);
			BOOST_CHECK(math::ldegree(p_type1{"x"},{"y"}) == 0);
			BOOST_CHECK(math::degree(p_type1{"x"} * p_type1{"x"}) == 2);
			BOOST_CHECK(math::degree(p_type1{"x"} * p_type1{"x"},{"x"}) == 2);
			BOOST_CHECK(math::degree(p_type1{"x"} * p_type1{"y"},{"y"}) == 1);
			BOOST_CHECK(math::ldegree(p_type1{"x"} * p_type1{"x"}) == 2);
			BOOST_CHECK(math::ldegree(p_type1{"x"} * p_type1{"x"},{"x"}) == 2);
			BOOST_CHECK(math::ldegree(p_type1{"x"} * p_type1{"y"},{"y"}) == 1);
			BOOST_CHECK(math::degree(p_type1{"x"} + p_type1{"y"} + p_type1{1}) == 1);
			BOOST_CHECK(math::degree(p_type1{"x"} + p_type1{"y"} + p_type1{1},{"x"}) == 1);
			BOOST_CHECK(math::degree(p_type1{"x"} + p_type1{"y"} + p_type1{1},{"x"}) == 1);
			BOOST_CHECK(math::degree(p_type1{"x"} + p_type1{"y"} + p_type1{1},{"y"}) == 1);
			BOOST_CHECK(math::degree(p_type1{"x"} + p_type1{"y"} + p_type1{1},{"y"}) == 1);
			BOOST_CHECK(math::degree(p_type1{"x"} + p_type1{"y"} + p_type1{1},{"z"}) == 0);
			BOOST_CHECK(math::degree(p_type1{"x"} + p_type1{"y"} + p_type1{1},{"z"}) == 0);
			BOOST_CHECK(math::ldegree(p_type1{"x"} + p_type1{"y"} + p_type1{1}) == 0);
			BOOST_CHECK(math::ldegree(p_type1{"x"} + p_type1{"y"} + p_type1{1},{"x"}) == 0);
			BOOST_CHECK(math::ldegree(p_type1{"x"} + p_type1{"y"} + p_type1{1},{"x"}) == 0);
			BOOST_CHECK(math::ldegree(p_type1{"x"} + p_type1{"y"} + p_type1{1},{"y"}) == 0);
			BOOST_CHECK(math::ldegree(p_type1{"x"} + p_type1{"y"} + p_type1{1},{"y"}) == 0);
			BOOST_CHECK(math::ldegree(p_type1{"x"} + p_type1{"y"} + p_type1{1},{"z"}) == 0);
			BOOST_CHECK(math::ldegree(p_type1{"x"} + p_type1{"y"} + p_type1{1},{"z"}) == 0);
			BOOST_CHECK(math::ldegree(p_type1{"x"} * p_type1{"x"} + p_type1{"y"} + p_type1{"x"}) == 1);
			BOOST_CHECK(math::ldegree(p_type1{"x"} * p_type1{"x"} + p_type1{"y"} + p_type1{"x"},{"x"}) == 0);
			BOOST_CHECK(math::ldegree(p_type1{"x"} * p_type1{"x"} + p_type1{"y"} + p_type1{"x"},{"x"}) == 0);
			BOOST_CHECK(math::ldegree(p_type1{"x"} * p_type1{"x"} + 2 * p_type1{"x"},{"x"}) == 1);
			BOOST_CHECK(math::ldegree(p_type1{"x"} * p_type1{"y"} + 2 * p_type1{"x"},{"x"}) == 1);
			BOOST_CHECK(math::ldegree(p_type1{"x"} * p_type1{"y"} + 2 * p_type1{"x"},{"y"}) == 0);
			std::set<std::string> empty_set;
			BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<const p_type11 &>())),decltype(std::declval<Expo>() + std::declval<int>())>::value));
			BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<const p_type11 &>(),empty_set)),decltype(std::declval<Expo>() + std::declval<int>())>::value));
			BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<const p_type11 &>())),decltype(std::declval<Expo>() + std::declval<int>())>::value));
			BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<const p_type11 &>(),empty_set)),decltype(std::declval<Expo>() + std::declval<int>())>::value));
			BOOST_CHECK(math::degree(p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"}) == 2);
			BOOST_CHECK(math::degree(p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"},{"x"}) == 1);
			BOOST_CHECK(math::degree(p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"},{"x"}) == 1);
			BOOST_CHECK(math::degree(p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"},{"y"}) == 1);
			BOOST_CHECK(math::ldegree(p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"}) == 1);
			BOOST_CHECK(math::ldegree(p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"},{"y"}) == 1);
			BOOST_CHECK(math::ldegree(p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"},{"y"}) == 1);
			BOOST_CHECK(math::ldegree(p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"},{"z"}) == 0);
			BOOST_CHECK(math::ldegree(p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"},{"z"}) == 0);
			BOOST_CHECK(math::ldegree(p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"} + 1) == 0);
			BOOST_CHECK(math::ldegree(p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"} + 1,{"x"}) == 0);
			BOOST_CHECK(math::ldegree(p_type11{"x"} * p_type1{"y"} + 2 * p_type1{"y"} + 1,{"y"}) == 0);
			BOOST_CHECK(math::degree(p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"} + 1) == 3);
			BOOST_CHECK(math::degree(p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"} + 1,{"x"}) == 1);
			BOOST_CHECK(math::degree(p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"} + 1,{"y"}) == 2);
			BOOST_CHECK(math::degree(p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"} + 1,{"y"}) == 2);
			BOOST_CHECK(math::ldegree(p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"} + 1) == 0);
			BOOST_CHECK(math::ldegree(p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"},{"x"}) == 0);
			BOOST_CHECK(math::ldegree(p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"},{"y"}) == 1);
			BOOST_CHECK(math::ldegree(p_type11{"x"} * p_type1{"y"} * p_type1{"y"} + 2 * p_type1{"y"},{"y"}) == 1);
			// Test the type traits.
			BOOST_CHECK(has_degree<p_type1>::value);
			BOOST_CHECK(has_degree<p_type11>::value);
			BOOST_CHECK(has_ldegree<p_type1>::value);
			BOOST_CHECK(has_ldegree<p_type11>::value);
			// Poisson series tests.
			typedef poisson_series<p_type1> pstype1;
			BOOST_CHECK(has_degree<pstype1>::value);
			BOOST_CHECK(has_ldegree<pstype1>::value);
			typedef poisson_series<Cf> pstype2;
			BOOST_CHECK(!has_degree<pstype2>::value);
			BOOST_CHECK(!has_ldegree<pstype2>::value);
			BOOST_CHECK((std::is_same<Expo,decltype(math::degree(pstype1{}))>::value));
			BOOST_CHECK((std::is_same<Expo,decltype(math::degree(pstype1{},std::set<std::string>{}))>::value));
			BOOST_CHECK((std::is_same<Expo,decltype(math::ldegree(pstype1{}))>::value));
			BOOST_CHECK((std::is_same<Expo,decltype(math::ldegree(pstype1{},std::set<std::string>{}))>::value));
			// As usual, operations on Poisson series with (polynomial) integer coefficients are not gonna give
			// meaningful mathematical results.
			if (std::is_same<Cf,integer>::value) {
				return;
			}
			BOOST_CHECK(math::degree(pstype1{}) == 0);
			BOOST_CHECK(math::degree(pstype1{},std::set<std::string>{}) == 0);
			BOOST_CHECK(math::ldegree(pstype1{}) == 0);
			BOOST_CHECK(math::ldegree(pstype1{},std::set<std::string>{}) == 0);
			BOOST_CHECK(math::degree(pstype1{"x"}) == 1);
			BOOST_CHECK(math::degree(pstype1{"x"},{"x"}) == 1);
			BOOST_CHECK(math::degree(pstype1{"x"},{"y"}) == 0);
			BOOST_CHECK(math::ldegree(pstype1{"x"}) == 1);
			BOOST_CHECK(math::ldegree(pstype1{"x"},{"x"}) == 1);
			BOOST_CHECK(math::ldegree(pstype1{"x"},{"y"}) == 0);
			BOOST_CHECK(math::degree(pstype1{"x"} * pstype1{"x"}) == 2);
			BOOST_CHECK(math::degree(pstype1{"x"} * pstype1{"x"},{"x"}) == 2);
			BOOST_CHECK(math::degree(pstype1{"x"} * pstype1{"y"},{"y"}) == 1);
			BOOST_CHECK(math::ldegree(pstype1{"x"} * pstype1{"x"}) == 2);
			BOOST_CHECK(math::ldegree(pstype1{"x"} * pstype1{"x"},{"x"}) == 2);
			BOOST_CHECK(math::ldegree(pstype1{"x"} * pstype1{"y"},{"y"}) == 1);
			BOOST_CHECK(math::degree(pstype1{"x"} + pstype1{"y"} + pstype1{1}) == 1);
			BOOST_CHECK(math::degree(pstype1{"x"} + pstype1{"y"} + pstype1{1},{"x"}) == 1);
			BOOST_CHECK(math::degree(pstype1{"x"} + pstype1{"y"} + pstype1{1},{"x"}) == 1);
			BOOST_CHECK(math::degree(pstype1{"x"} + pstype1{"y"} + pstype1{1},{"y"}) == 1);
			BOOST_CHECK(math::degree(pstype1{"x"} + pstype1{"y"} + pstype1{1},{"y"}) == 1);
			BOOST_CHECK(math::degree(pstype1{"x"} + pstype1{"y"} + pstype1{1},{"z"}) == 0);
			BOOST_CHECK(math::degree(pstype1{"x"} + pstype1{"y"} + pstype1{1},{"z"}) == 0);
			BOOST_CHECK(math::ldegree(pstype1{"x"} + pstype1{"y"} + pstype1{1}) == 0);
			BOOST_CHECK(math::ldegree(pstype1{"x"} + pstype1{"y"} + pstype1{1},{"x"}) == 0);
			BOOST_CHECK(math::ldegree(pstype1{"x"} + pstype1{"y"} + pstype1{1},{"x"}) == 0);
			BOOST_CHECK(math::ldegree(pstype1{"x"} + pstype1{"y"} + pstype1{1},{"y"}) == 0);
			BOOST_CHECK(math::ldegree(pstype1{"x"} + pstype1{"y"} + pstype1{1},{"y"}) == 0);
			BOOST_CHECK(math::ldegree(pstype1{"x"} + pstype1{"y"} + pstype1{1},{"z"}) == 0);
			BOOST_CHECK(math::ldegree(pstype1{"x"} + pstype1{"y"} + pstype1{1},{"z"}) == 0);
			BOOST_CHECK(math::ldegree(pstype1{"x"} * pstype1{"x"} + pstype1{"y"} + pstype1{"x"}) == 1);
			BOOST_CHECK(math::ldegree(pstype1{"x"} * pstype1{"x"} + pstype1{"y"} + pstype1{"x"},{"x"}) == 0);
			BOOST_CHECK(math::ldegree(pstype1{"x"} * pstype1{"x"} + pstype1{"y"} + pstype1{"x"},{"x"}) == 0);
			BOOST_CHECK(math::ldegree(pstype1{"x"} * pstype1{"x"} + 2 * pstype1{"x"},{"x"}) == 1);
			BOOST_CHECK(math::ldegree(pstype1{"x"} * pstype1{"y"} + 2 * pstype1{"x"},{"x"}) == 1);
			BOOST_CHECK(math::ldegree(pstype1{"x"} * pstype1{"y"} + 2 * pstype1{"x"},{"y"}) == 0);
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(power_series_test_01)
{
	environment env;
	boost::mpl::for_each<cf_types>(degree_tester());
}

struct fake_int
{
	fake_int();
	explicit fake_int(int);
	fake_int(const fake_int &);
	fake_int(fake_int &&) noexcept;
	fake_int &operator=(const fake_int &);
	fake_int &operator=(fake_int &&) noexcept;
	~fake_int();
	bool operator==(const fake_int &) const;
	bool operator!=(const fake_int &) const;
	bool operator<(const fake_int &) const;
	fake_int operator+(const fake_int &) const;
	fake_int &operator+=(const fake_int &);
	fake_int operator-(const fake_int &) const;
	fake_int &operator-=(const fake_int &);
	friend std::ostream &operator<<(std::ostream &, const fake_int &);
};

namespace std
{

template <>
struct hash<fake_int>
{
	typedef size_t result_type;
	typedef fake_int argument_type;
	result_type operator()(const argument_type &) const noexcept;
};

}

BOOST_AUTO_TEST_CASE(power_series_test_02)
{
	typedef g_series_type<double,int> stype1;
	BOOST_CHECK((has_degree<stype1>::value));
	BOOST_CHECK((has_ldegree<stype1>::value));
	BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype1>())),int>::value));
	BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype1>())),int>::value));
	BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype1>(),std::set<std::string>{})),int>::value));
	BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype1>(),std::set<std::string>{})),int>::value));
	typedef g_series_type<stype1,long> stype2;
	BOOST_CHECK((has_degree<stype2>::value));
	BOOST_CHECK((has_ldegree<stype2>::value));
	BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype2>())),long>::value));
	BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype2>())),long>::value));
	BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype2>(),std::set<std::string>{})),long>::value));
	BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype2>(),std::set<std::string>{})),long>::value));
	typedef g_series_type2<double> stype3;
	BOOST_CHECK((!has_degree<stype3>::value));
	BOOST_CHECK((!has_ldegree<stype3>::value));
	typedef g_series_type2<g_series_type<g_series_type<double,int>,integer>> stype4;
	BOOST_CHECK((has_degree<stype4>::value));
	BOOST_CHECK((has_ldegree<stype4>::value));
	BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype4>())),integer>::value));
	BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype4>())),integer>::value));
	BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype4>(),std::set<std::string>{})),integer>::value));
	BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype4>(),std::set<std::string>{})),integer>::value));
	// Check actual instantiations as well.
	std::set<std::string> ss;
	BOOST_CHECK_EQUAL(math::degree(stype1{}),0);
	BOOST_CHECK_EQUAL(math::ldegree(stype1{}),0);
	BOOST_CHECK_EQUAL(math::degree(stype1{},ss),0);
	BOOST_CHECK_EQUAL(math::ldegree(stype1{},ss),0);
	BOOST_CHECK_EQUAL(math::degree(stype2{}),0);
	BOOST_CHECK_EQUAL(math::ldegree(stype2{}),0);
	BOOST_CHECK_EQUAL(math::degree(stype2{},ss),0);
	BOOST_CHECK_EQUAL(math::ldegree(stype2{},ss),0);
	BOOST_CHECK_EQUAL(math::degree(stype4{}),0);
	BOOST_CHECK_EQUAL(math::ldegree(stype4{}),0);
	BOOST_CHECK_EQUAL(math::degree(stype4{},ss),0);
	BOOST_CHECK_EQUAL(math::ldegree(stype4{},ss),0);
	// Tests with fake int.
	typedef g_series_type<double,fake_int> stype5;
	BOOST_CHECK((has_degree<stype5>::value));
	BOOST_CHECK((has_ldegree<stype5>::value));
	BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype5>())),fake_int>::value));
	BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype5>())),fake_int>::value));
	BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype5>(),std::set<std::string>{})),fake_int>::value));
	BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype5>(),std::set<std::string>{})),fake_int>::value));
	typedef g_series_type<stype5,int> stype6;
	BOOST_CHECK((!has_degree<stype6>::value));
	BOOST_CHECK((!has_ldegree<stype6>::value));
}
