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
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "../src/environment.hpp"
#include "../src/math.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/poisson_series.hpp"
#include "../src/poisson_series_term.hpp"
#include "../src/polynomial.hpp"
#include "../src/polynomial_term.hpp"
#include "../src/real.hpp"
#include "../src/serialization.hpp"
#include "../src/series.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

typedef boost::mpl::vector<double,integer,rational,real> cf_types;
typedef boost::mpl::vector<int,integer> expo_types;

template <typename T>
class null_toolbox: public T
{
		PIRANHA_SERIALIZE_THROUGH_BASE(T)
	public:
		null_toolbox() = default;
		null_toolbox(const null_toolbox &) = default;
		null_toolbox(null_toolbox &&) = default;
		null_toolbox &operator=(const null_toolbox &) = default;
		null_toolbox &operator=(null_toolbox &&) = default;
		PIRANHA_FORWARDING_CTOR(null_toolbox,T)
		PIRANHA_FORWARDING_ASSIGNMENT(null_toolbox,T)
		static bool at_called;
		void auto_truncate()
		{
			at_called = true;
		}
};

template <typename T>
bool null_toolbox<T>::at_called = false;

template <typename Cf, typename Expo>
class g_series_type: public power_series<null_toolbox<series<polynomial_term<Cf,Expo>,g_series_type<Cf,Expo>>>,g_series_type<Cf,Expo>>
{
		typedef power_series<null_toolbox<series<polynomial_term<Cf,Expo>,g_series_type<Cf,Expo>>>,g_series_type<Cf,Expo>> base;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
	public:
		g_series_type() = default;
		g_series_type(const g_series_type &) = default;
		g_series_type(g_series_type &&) = default;
		g_series_type &operator=(const g_series_type &) = default;
		g_series_type &operator=(g_series_type &&) = default;
		PIRANHA_FORWARDING_CTOR(g_series_type,base)
		PIRANHA_FORWARDING_ASSIGNMENT(g_series_type,base)
};

template <typename Cf>
class g_series_type2: public power_series<series<poisson_series_term<Cf>,g_series_type2<Cf>>,g_series_type2<Cf>>
{
		typedef power_series<series<poisson_series_term<Cf>,g_series_type2<Cf>>,g_series_type2<Cf>> base;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
	public:
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
			BOOST_CHECK((std::is_same<integer,decltype(math::degree(p_type1{}))>::value));
			BOOST_CHECK((std::is_same<integer,decltype(math::degree(p_type1{},std::vector<std::string>{}))>::value));
			BOOST_CHECK((std::is_same<integer,decltype(math::ldegree(p_type1{}))>::value));
			BOOST_CHECK((std::is_same<integer,decltype(math::ldegree(p_type1{},std::vector<std::string>{}))>::value));
			BOOST_CHECK(math::degree(p_type1{}) == 0);
			BOOST_CHECK(math::degree(p_type1{},std::vector<std::string>{}) == 0);
			BOOST_CHECK(math::ldegree(p_type1{}) == 0);
			BOOST_CHECK(math::ldegree(p_type1{},std::vector<std::string>{}) == 0);
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
			std::vector<std::string> empty_set;
			BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<const p_type11 &>())),integer>::value));
			BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<const p_type11 &>(),empty_set)),integer>::value));
			BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<const p_type11 &>())),integer>::value));
			BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<const p_type11 &>(),empty_set)),integer>::value));
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
			BOOST_CHECK((std::is_same<integer,decltype(math::degree(pstype1{}))>::value));
			BOOST_CHECK((std::is_same<integer,decltype(math::degree(pstype1{},std::vector<std::string>{}))>::value));
			BOOST_CHECK((std::is_same<integer,decltype(math::ldegree(pstype1{}))>::value));
			BOOST_CHECK((std::is_same<integer,decltype(math::ldegree(pstype1{},std::vector<std::string>{}))>::value));
			// As usual, operations on Poisson series with (polynomial) integer coefficients are not gonna give
			// meaningful mathematical results.
			if (std::is_same<Cf,integer>::value) {
				return;
			}
			BOOST_CHECK(math::degree(pstype1{}) == 0);
			BOOST_CHECK(math::degree(pstype1{},std::vector<std::string>{}) == 0);
			BOOST_CHECK(math::ldegree(pstype1{}) == 0);
			BOOST_CHECK(math::ldegree(pstype1{},std::vector<std::string>{}) == 0);
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
	result_type operator()(const argument_type &) const;
};

}

BOOST_AUTO_TEST_CASE(power_series_test_02)
{
	// Check the rational degree.
	typedef g_series_type<double,rational> stype0;
	BOOST_CHECK((has_degree<stype0>::value));
	BOOST_CHECK((has_ldegree<stype0>::value));
	BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype0>())),rational>::value));
	BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype0>())),rational>::value));
	BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype0>(),std::vector<std::string>{})),rational>::value));
	BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype0>(),std::vector<std::string>{})),rational>::value));
	typedef g_series_type<double,int> stype1;
	BOOST_CHECK((has_degree<stype1>::value));
	BOOST_CHECK((has_ldegree<stype1>::value));
	BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype1>())),integer>::value));
	BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype1>())),integer>::value));
	BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype1>(),std::vector<std::string>{})),integer>::value));
	BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype1>(),std::vector<std::string>{})),integer>::value));
	typedef g_series_type<stype1,long> stype2;
	BOOST_CHECK((has_degree<stype2>::value));
	BOOST_CHECK((has_ldegree<stype2>::value));
	BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype2>())),integer>::value));
	BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype2>())),integer>::value));
	BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype2>(),std::vector<std::string>{})),integer>::value));
	BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype2>(),std::vector<std::string>{})),integer>::value));
	typedef g_series_type2<double> stype3;
	BOOST_CHECK((!has_degree<stype3>::value));
	BOOST_CHECK((!has_ldegree<stype3>::value));
	typedef g_series_type2<g_series_type<g_series_type<double,int>,integer>> stype4;
	BOOST_CHECK((has_degree<stype4>::value));
	BOOST_CHECK((has_ldegree<stype4>::value));
	BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype4>())),integer>::value));
	BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype4>())),integer>::value));
	BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype4>(),std::vector<std::string>{})),integer>::value));
	BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype4>(),std::vector<std::string>{})),integer>::value));
	// Check actual instantiations as well.
	std::vector<std::string> ss;
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
	BOOST_CHECK((std::is_same<decltype(math::degree(std::declval<stype5>(),std::vector<std::string>{})),fake_int>::value));
	BOOST_CHECK((std::is_same<decltype(math::ldegree(std::declval<stype5>(),std::vector<std::string>{})),fake_int>::value));
	typedef g_series_type<stype5,int> stype6;
	// This does not have a degree type because fake_int cannot be added to integer.
	BOOST_CHECK((!has_degree<stype6>::value));
	BOOST_CHECK((!has_ldegree<stype6>::value));
}

BOOST_AUTO_TEST_CASE(power_series_serialization_test)
{
	typedef g_series_type<polynomial<rational,rational>,rational> stype;
	stype x("x"), y("y"), z = x + y, tmp;
	std::stringstream ss;
	{
	boost::archive::text_oarchive oa(ss);
	oa << z;
	}
	{
	boost::archive::text_iarchive ia(ss);
	ia >> tmp;
	}
	BOOST_CHECK_EQUAL(z,tmp);
}

BOOST_AUTO_TEST_CASE(power_series_truncation_test)
{
	// A test with polynomial, degree only in the key.
	{
	typedef polynomial<double,rational> stype0;
	BOOST_CHECK((has_truncate_degree<stype0,int>::value));
	BOOST_CHECK((has_truncate_degree<stype0,rational>::value));
	BOOST_CHECK((has_truncate_degree<stype0,integer>::value));
	BOOST_CHECK((!has_truncate_degree<stype0,std::string>::value));
	stype0 x{"x"}, y{"y"}, z{"z"};
	stype0 s0;
	BOOST_CHECK((std::is_same<stype0,decltype(s0.truncate_degree(5))>::value));
	BOOST_CHECK_EQUAL(s0.truncate_degree(5),s0);
	s0 = x.pow(rational(10,3));
	BOOST_CHECK_EQUAL(s0.truncate_degree(5),s0);
	BOOST_CHECK_EQUAL(s0.truncate_degree(3/2_q),0);
	// x**5*y+1/2*z**-5*x*y+x*y*z/4
	s0 = x.pow(5) * y + z.pow(-5)/2 * x * y + x*y*z/4;
	BOOST_CHECK_EQUAL(s0.truncate_degree(3),z.pow(-5)/2 * x * y + x*y*z/4);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,-1),z.pow(-5)/2 * x * y);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,2,{"x"}),z.pow(-5)/2 * x * y + x*y*z/4);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,5,{"x","y"}),z.pow(-5)/2 * x * y + x*y*z/4);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,5,{"y","x","y"}),z.pow(-5)/2 * x * y + x*y*z/4);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,5,{"z","x"}),s0);
	// Test with non-existing variable.
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,0,{"a","b"}),s0);
	}
	{
	// Poisson series, degree only in the coefficient.
	typedef poisson_series<polynomial<rational,rational>> st;
	BOOST_CHECK((has_truncate_degree<st,int>::value));
	BOOST_CHECK((has_truncate_degree<st,rational>::value));
	BOOST_CHECK((has_truncate_degree<st,integer>::value));
	BOOST_CHECK((!has_truncate_degree<st,std::string>::value));
	st x("x"), y("y"), z("z"), a("a"), b("b");
	// (x + y**2/4 + 3*x*y*z/7) * cos(a) + (x*y+y*z/3+3*z**2*x/8) * sin(a+b)
	st s0 = (x + y*y/4 + 3*z*x*y/7) * math::cos(a) + (x*y + z*y/3 + 3*z*z*x/8) * math::sin(a + b);
	BOOST_CHECK_EQUAL(s0.truncate_degree(2),(x + y*y/4) * math::cos(a) + (x*y + z*y/3) * math::sin(a + b));
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,1l),x * math::cos(a));
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,-1ll),0);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,1l,{"x"}),(x + y*y/4 + 3*z*x*y/7) * math::cos(a) + (x*y + z*y/3 + 3*z*z*x/8) * math::sin(a + b));
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,char(0),{"x"}),y*y/4 * math::cos(a) + z*y/3 * math::sin(a + b));
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,char(1),{"y","x"}),x * math::cos(a) + (z*y/3 + 3*z*z*x/8) * math::sin(a + b));
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,integer(1),{"z"}),(x + y*y/4 + 3*z*x*y/7) * math::cos(a) + (x*y + z*y/3) * math::sin(a + b));
	// Test with non-existing variable.
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,0,{"foo","bar"}),s0);
	}
	{
	// Recursive poly.
	typedef polynomial<rational,rational> st0;
	typedef polynomial<st0,rational> st1;
	BOOST_CHECK((has_truncate_degree<st1,int>::value));
	BOOST_CHECK((has_truncate_degree<st1,rational>::value));
	BOOST_CHECK((has_truncate_degree<st1,integer>::value));
	BOOST_CHECK((!has_truncate_degree<st1,std::string>::value));
	// (x*y+x**2+x+1/4)*z + (x+y**2+x**2*y)*z**2 + 3
	st0 x{"x"}, y{"y"};
	st1 z{"z"};
	auto s0 = (x*y+x*x+x+1_q/4)*z+(x+y*y+x*x*y)*z*z + 3;
	BOOST_CHECK_EQUAL(s0.truncate_degree(1),1/4_q*z + 3);
	BOOST_CHECK_EQUAL(s0.truncate_degree(0),3);
	BOOST_CHECK_EQUAL(s0.truncate_degree(2),(x+1_q/4)*z + 3);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,-3),0);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,3_q),(x*y+x*x+x+1_q/4)*z + x*z*z + 3);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,1,{"x"}),(x*y+x+1_q/4)*z+(x+y*y)*z*z + 3);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,1ll,{"x","y"}),(x+1_q/4)*z+x*z*z + 3);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,1,{"x","z"}),1_q/4*z + 3);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,2,{"x","z"}),(x*y+x+1_q/4)*z + y*y*z*z + 3);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,3,{"x","z"}),(x*y+x*x+x+1_q/4)*z+(x+y*y)*z*z + 3);
	// Test with non-existing variable.
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,0,{"foo","bar"}),s0);
	}
	{
	// Recursive poly, integers and rational exponent mixed, same example as above.
	typedef polynomial<rational,integer> st0;
	typedef polynomial<st0,rational> st1;
	BOOST_CHECK((has_truncate_degree<st1,int>::value));
	BOOST_CHECK((has_truncate_degree<st1,rational>::value));
	BOOST_CHECK((has_truncate_degree<st1,integer>::value));
	BOOST_CHECK((!has_truncate_degree<st1,std::string>::value));
	// (x*y+x**2+x+1/4)*z + (x+y**2+x**2*y)*z**2 + 3
	st0 x{"x"}, y{"y"};
	st1 z{"z"};
	auto s0 = (x*y+x*x+x+1_q/4)*z+(x+y*y+x*x*y)*z*z + 3;
	BOOST_CHECK_EQUAL(s0.truncate_degree(1),1/4_q*z + 3);
	BOOST_CHECK_EQUAL(s0.truncate_degree(0),3);
	BOOST_CHECK_EQUAL(s0.truncate_degree(2),(x+1_q/4)*z + 3);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,-3),0);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,3_q),(x*y+x*x+x+1_q/4)*z + x*z*z + 3);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,1,{"x"}),(x*y+x+1_q/4)*z+(x+y*y)*z*z + 3);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,1ll,{"x","y"}),(x+1_q/4)*z+x*z*z + 3);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,1,{"x","z"}),1_q/4*z + 3);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,2,{"x","z"}),(x*y+x+1_q/4)*z + y*y*z*z + 3);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,3,{"x","z"}),(x*y+x*x+x+1_q/4)*z+(x+y*y)*z*z + 3);
	// Test with non-existing variable.
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,0_q,{"foo","bar"}),s0);
	}
	{
	// Recursive poly, integers and rational exponent mixed, same example as above but switched.
	typedef polynomial<rational,rational> st0;
	typedef polynomial<st0,integer> st1;
	BOOST_CHECK((has_truncate_degree<st1,int>::value));
	BOOST_CHECK((has_truncate_degree<st1,rational>::value));
	BOOST_CHECK((has_truncate_degree<st1,integer>::value));
	BOOST_CHECK((!has_truncate_degree<st1,std::string>::value));
	// (x*y+x**2+x+1/4)*z + (x+y**2+x**2*y)*z**2 + 3
	st0 x{"x"}, y{"y"};
	st1 z{"z"};
	auto s0 = (x*y+x*x+x+1_q/4)*z+(x+y*y+x*x*y)*z*z + 3;
	BOOST_CHECK_EQUAL(s0.truncate_degree(1),1/4_q*z + 3);
	BOOST_CHECK_EQUAL(s0.truncate_degree(0),3);
	BOOST_CHECK_EQUAL(s0.truncate_degree(2),(x+1_q/4)*z + 3);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,-3),0);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,3_q),(x*y+x*x+x+1_q/4)*z + x*z*z + 3);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,1,{"x"}),(x*y+x+1_q/4)*z+(x+y*y)*z*z + 3);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,1ll,{"x","y"}),(x+1_q/4)*z+x*z*z + 3);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,1,{"x","z"}),1_q/4*z + 3);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,2,{"x","z"}),(x*y+x+1_q/4)*z + y*y*z*z + 3);
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,3,{"x","z"}),(x*y+x*x+x+1_q/4)*z+(x+y*y)*z*z + 3);
	// Test with non-existing variable.
	BOOST_CHECK_EQUAL(math::truncate_degree(s0,0_z,{"foo","bar"}),s0);
	}
}

BOOST_AUTO_TEST_CASE(power_series_auto_truncate_test)
{
	typedef polynomial<double,rational> stype0;
	typedef polynomial<rational,int> stype1;
	// Check the initial setup.
	auto tup0 = stype0::get_auto_truncate_degree();
	BOOST_CHECK_EQUAL(std::get<0>(tup0),0);
	BOOST_CHECK_EQUAL(std::get<1>(tup0),0);
	BOOST_CHECK(std::get<2>(tup0).empty());
	BOOST_CHECK((std::is_same<rational,std::decay<decltype(std::get<1>(tup0))>::type>::value));
	// With no truncation enabled first.
	auto x = stype0{"x"};
	x.auto_truncate();
	BOOST_CHECK_EQUAL(x,stype0{"x"});
	// Activate total truncation.
	x.set_auto_truncate_degree(2);
	x.auto_truncate();
	BOOST_CHECK_EQUAL(x,stype0{"x"});
	x.set_auto_truncate_degree(0);
	x.auto_truncate();
	BOOST_CHECK_EQUAL(x,stype0{});
	tup0 = stype0::get_auto_truncate_degree();
	BOOST_CHECK_EQUAL(std::get<0>(tup0),1);
	BOOST_CHECK_EQUAL(std::get<1>(tup0),0);
	BOOST_CHECK(std::get<2>(tup0).empty());
	// Check the resetting.
	stype0::unset_auto_truncate_degree();
	tup0 = stype0::get_auto_truncate_degree();
	BOOST_CHECK_EQUAL(std::get<0>(tup0),0);
	BOOST_CHECK_EQUAL(std::get<1>(tup0),0);
	BOOST_CHECK(std::get<2>(tup0).empty());
	// Revive x and check partial.
	x = stype0{"x"};
	stype0::set_auto_truncate_degree(0,{"y"});
	x.auto_truncate();
	BOOST_CHECK_EQUAL(x,stype0{"x"});
	stype0::set_auto_truncate_degree(0,{"x","y"});
	x.auto_truncate();
	BOOST_CHECK_EQUAL(x,stype0{});
	tup0 = stype0::get_auto_truncate_degree();
	BOOST_CHECK_EQUAL(std::get<0>(tup0),2);
	BOOST_CHECK_EQUAL(std::get<1>(tup0),0);
	BOOST_CHECK((std::get<2>(tup0) == std::vector<std::string>{"x","y"}));
	// Check that for another series type the truncation settings are untouched.
	auto tup1 = stype1::get_auto_truncate_degree();
	BOOST_CHECK_EQUAL(std::get<0>(tup1),0);
	BOOST_CHECK_EQUAL(std::get<1>(tup1),0);
	BOOST_CHECK(std::get<2>(tup1).empty());
	BOOST_CHECK((std::is_same<integer,std::decay<decltype(std::get<1>(tup1))>::type>::value));
	// Final unset.
	stype0::unset_auto_truncate_degree();
	tup0 = stype0::get_auto_truncate_degree();
	BOOST_CHECK_EQUAL(std::get<0>(tup0),0);
	BOOST_CHECK_EQUAL(std::get<1>(tup0),0);
	BOOST_CHECK(std::get<2>(tup0).empty());
	// Check that the auto truncation from the null toolbox is called.
	using stype2 = g_series_type<rational,rational>;
	BOOST_CHECK(!stype2::at_called);
	stype2::set_auto_truncate_degree(-1);
	auto x2 = stype2{1};
	x2.auto_truncate();
	BOOST_CHECK(x2.empty());
	BOOST_CHECK(stype2::at_called);
	// Check the auto truncation when doing multiplications.
	x = stype0{"x"};
	x.set_auto_truncate_degree(3);
	BOOST_CHECK_EQUAL((x + 1).pow(4),6*x*x+1+4*x+4*x*x*x);
	auto y = stype0{"y"};
	x.unset_auto_truncate_degree();
	BOOST_CHECK_EQUAL((y + 1).pow(4),6*y*y+1+4*y+4*y*y*y+y*y*y*y);
	y.clear_pow_cache();
	x.set_auto_truncate_degree(3,{"x","y"});
	BOOST_CHECK_EQUAL((y + 1).pow(4),6*y*y+1+4*y+4*y*y*y);
	// Test the type traits.
	BOOST_CHECK((detail::has_set_auto_truncate_degree<stype0,int>::value));
	BOOST_CHECK((detail::has_set_auto_truncate_degree<stype0,integer>::value));
	BOOST_CHECK((detail::has_set_auto_truncate_degree<stype0,rational>::value));
	BOOST_CHECK((detail::has_set_auto_truncate_degree<stype0,double>::value));
	BOOST_CHECK((!detail::has_set_auto_truncate_degree<stype0,std::string>::value));
	BOOST_CHECK((!detail::has_set_auto_truncate_degree<stype0,std::vector<int>>::value));
	BOOST_CHECK((detail::has_get_auto_truncate_degree<stype0>::value));
	BOOST_CHECK((detail::has_get_auto_truncate_degree<stype0>::value));
	BOOST_CHECK((detail::has_get_auto_truncate_degree<stype0>::value));
	BOOST_CHECK((detail::has_get_auto_truncate_degree<stype0>::value));
	using ps_type0 = poisson_series<double>;
	BOOST_CHECK((!detail::has_get_auto_truncate_degree<ps_type0>::value));
}
