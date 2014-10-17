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

#include "../src/trigonometric_series.hpp"

#define BOOST_TEST_MODULE trigonometric_series_test
#include <boost/test/unit_test.hpp>

#include <cstddef>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "../src/base_term.hpp"
#include "../src/environment.hpp"
#include "../src/forwarding.hpp"
#include "../src/math.hpp"
#include "../src/mp_rational.hpp"
#include "../src/poisson_series.hpp"
#include "../src/polynomial.hpp"
#include "../src/series.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(trigonometric_series_degree_order_test)
{
	environment env;
	using math::sin;
	using math::cos;
	typedef poisson_series<polynomial<rational,short>> p_type1;
	p_type1 x{"x"}, y{"y"};
	BOOST_CHECK_EQUAL(x.t_degree(),0);
	BOOST_CHECK_EQUAL(cos(3 * x).t_degree(),3);
	BOOST_CHECK_EQUAL(cos(3 * x - 4 * y).t_degree(),-1);
	BOOST_CHECK_EQUAL((cos(3 * x - 4 * y) + sin(x + y)).t_degree(),2);
	BOOST_CHECK_EQUAL((cos(-3 * x - 4 * y) + sin(-x - y)).t_degree(),7);
	BOOST_CHECK_EQUAL(math::t_degree(cos(-3 * x - 4 * y) + sin(-x - y)),7);
	BOOST_CHECK_EQUAL((cos(-3 * x - 2 * y) + sin(-x + y)).t_degree(),5);
	BOOST_CHECK_EQUAL(math::t_degree(cos(2 * x),{"x"}),2);
	BOOST_CHECK_EQUAL(math::t_degree(cos(2 * x),{"y"}),0);
	BOOST_CHECK_EQUAL(math::t_degree(cos(2 * x) + cos(3 * x + y),{"x"}),3);
	BOOST_CHECK_EQUAL(math::t_degree(cos(2 * x) + cos(x + y),{"x"}),2);
	BOOST_CHECK_EQUAL(math::t_degree(x * cos(2 * x) - y * cos(x + y),{"y"}),1);
	BOOST_CHECK_EQUAL(math::t_degree(y * cos(x - y),{"y"}),-1);
	BOOST_CHECK_EQUAL(math::t_degree(y * cos(x - y) + x,{"y"}),0);
	BOOST_CHECK_EQUAL(math::t_degree(y * cos(x - y) + x,{"y","x","y"}),0);
	BOOST_CHECK_EQUAL(math::t_degree(y * cos(x - y) + cos(x + y),{"y","x","y","z"}),2);
	BOOST_CHECK_EQUAL(math::t_degree(y * cos(x - y) + cos(x + y),{"x"}),1);
	BOOST_CHECK_EQUAL(math::t_degree(y * sin(x - y) + cos(x + y),std::vector<std::string>{}),0);
	BOOST_CHECK_EQUAL(math::t_degree(p_type1{},std::vector<std::string>{}),0);
	BOOST_CHECK_EQUAL(math::t_degree(p_type1{},{"x"}),0);
	BOOST_CHECK_EQUAL(math::t_degree(p_type1{}),0);
	BOOST_CHECK_EQUAL(math::t_degree(p_type1{2}),0);
	// Low trigonometric degree.
	BOOST_CHECK_EQUAL(math::t_ldegree(x),0);
	BOOST_CHECK_EQUAL(math::t_ldegree(cos(3 * x)),3);
	BOOST_CHECK_EQUAL(math::t_ldegree(cos(3 * x - 4 * y)),-1);
	BOOST_CHECK_EQUAL(math::t_ldegree((cos(3 * x - 4 * y) + sin(x + y))),-1);
	BOOST_CHECK_EQUAL(math::t_ldegree((cos(-3 * x - 4 * y) + sin(-x - y))),2);
	BOOST_CHECK_EQUAL(math::t_ldegree((cos(-3 * x - 2 * y) + sin(-x + y))),0);
	BOOST_CHECK_EQUAL(math::t_ldegree(cos(2 * x),{"x"}),2);
	BOOST_CHECK_EQUAL(math::t_ldegree(cos(2 * x),{"y"}),0);
	BOOST_CHECK_EQUAL(math::t_ldegree((cos(2 * x) + cos(3 * x + y)),{"x"}),2);
	BOOST_CHECK_EQUAL(math::t_ldegree((cos(2 * x) + cos(x + y)),{"x"}),1);
	BOOST_CHECK_EQUAL(math::t_ldegree((x * cos(2 * x) - y * cos(x + y)),{"y"}),0);
	BOOST_CHECK_EQUAL(math::t_ldegree((y * cos(x - y)),{"y"}),-1);
	BOOST_CHECK_EQUAL(math::t_ldegree((y * cos(x - y) + x),{"y"}),-1);
	BOOST_CHECK_EQUAL(math::t_ldegree((y * cos(x - y) + x),{"y","x","y"}),0);
	BOOST_CHECK_EQUAL(math::t_ldegree((y * cos(x - y) + cos(x + y)),{"y","x","y","z"}),0);
	BOOST_CHECK_EQUAL(math::t_ldegree((y * cos(x - y) + cos(x + y)),{"x"}),1);
	BOOST_CHECK_EQUAL(math::t_ldegree((y * sin(x - y) + cos(x + y)),std::vector<std::string>{}),0);
	BOOST_CHECK_EQUAL(math::t_ldegree(p_type1{},std::vector<std::string>{}),0);
	BOOST_CHECK_EQUAL(math::t_ldegree(p_type1{},{"x"}),0);
	BOOST_CHECK_EQUAL(math::t_ldegree(p_type1{}),0);
	BOOST_CHECK_EQUAL(math::t_ldegree(p_type1{2}),0);
	// Order tests.
	BOOST_CHECK_EQUAL(math::t_order(x),0);
	BOOST_CHECK_EQUAL(math::t_order(cos(3 * x)),3);
	BOOST_CHECK_EQUAL(math::t_order(cos(3 * x - 4 * y)),7);
	BOOST_CHECK_EQUAL(math::t_order((cos(3 * x - 4 * y) + sin(x + y))),7);
	BOOST_CHECK_EQUAL(math::t_order((cos(-3 * x - 4 * y) + sin(-x - y))),7);
	BOOST_CHECK_EQUAL(math::t_order((cos(-3 * x - 2 * y) + sin(-x + y))),5);
	BOOST_CHECK_EQUAL(math::t_order(cos(2 * x),{"x"}),2);
	BOOST_CHECK_EQUAL(math::t_order(cos(2 * x),{"y"}),0);
	BOOST_CHECK_EQUAL(math::t_order((cos(2 * x) + cos(3 * x + y)),{"x"}),3);
	BOOST_CHECK_EQUAL(math::t_order((cos(2 * x) + cos(x + y)),{"x"}),2);
	BOOST_CHECK_EQUAL(math::t_order((x * cos(2 * x) - y * cos(x + y)),{"y"}),1);
	BOOST_CHECK_EQUAL(math::t_order((y * cos(x - y)),{"y"}),1);
	BOOST_CHECK_EQUAL(math::t_order((y * cos(x - y) + x),{"y"}),1);
	BOOST_CHECK_EQUAL(math::t_order((y * cos(x - y) + x),{"y","x","y"}),2);
	BOOST_CHECK_EQUAL(math::t_order((y * cos(x - y) + cos(x + y)),{"y","x","y","z"}),2);
	BOOST_CHECK_EQUAL(math::t_order((y * cos(x - y) + cos(x + y)),{"x"}),1);
	BOOST_CHECK_EQUAL(math::t_order((y * sin(x - y) + cos(x + y)),std::vector<std::string>{}),0);
	BOOST_CHECK_EQUAL(math::t_order(p_type1{},std::vector<std::string>{}),0);
	BOOST_CHECK_EQUAL(math::t_order(p_type1{},{"x"}),0);
	BOOST_CHECK_EQUAL(math::t_order(p_type1{}),0);
	BOOST_CHECK_EQUAL(math::t_order(p_type1{2}),0);
	// Low trigonometric order.
	BOOST_CHECK_EQUAL(math::t_lorder(x),0);
	BOOST_CHECK_EQUAL(math::t_lorder(cos(3 * x)),3);
	BOOST_CHECK_EQUAL(math::t_lorder(cos(3 * x - 4 * y)),7);
	BOOST_CHECK_EQUAL(math::t_lorder((cos(3 * x - 4 * y) + sin(x + y))),2);
	BOOST_CHECK_EQUAL(math::t_lorder((cos(-3 * x - 4 * y) + sin(-x - y))),2);
	BOOST_CHECK_EQUAL(math::t_lorder((cos(-3 * x - 2 * y) + sin(-x + y))),2);
	BOOST_CHECK_EQUAL(math::t_lorder(cos(2 * x),{"x"}),2);
	BOOST_CHECK_EQUAL(math::t_lorder(cos(2 * x),{"y"}),0);
	BOOST_CHECK_EQUAL(math::t_lorder((cos(2 * x) + cos(3 * x + y)),{"x"}),2);
	BOOST_CHECK_EQUAL(math::t_lorder((cos(2 * x) + cos(x + y)),{"x"}),1);
	BOOST_CHECK_EQUAL(math::t_lorder((x * cos(2 * x) - y * cos(x + y)),{"y"}),0);
	BOOST_CHECK_EQUAL(math::t_lorder((y * cos(x - y)),{"y"}),1);
	BOOST_CHECK_EQUAL(math::t_lorder((y * cos(x - y) + x),{"y"}),0);
	BOOST_CHECK_EQUAL(math::t_lorder((y * cos(x - y) + x),{"y","x","y"}),0);
	BOOST_CHECK_EQUAL(math::t_lorder((y * cos(x - y) + cos(x + y)),{"y","x","y","z"}),2);
	BOOST_CHECK_EQUAL(math::t_lorder((y * cos(x - y) + cos(x + y)),{"x"}),1);
	BOOST_CHECK_EQUAL(math::t_lorder((y * sin(x - y) + cos(x + y)),std::vector<std::string>{}),0);
	BOOST_CHECK_EQUAL(math::t_lorder(p_type1{},std::vector<std::string>{}),0);
	BOOST_CHECK_EQUAL(math::t_lorder(p_type1{},{"x"}),0);
	BOOST_CHECK_EQUAL(math::t_lorder(p_type1{}),0);
	BOOST_CHECK_EQUAL(math::t_lorder(p_type1{2}),0);
	// Type traits checks.
	BOOST_CHECK(has_t_degree<p_type1>::value);
	BOOST_CHECK(has_t_degree<const p_type1>::value);
	BOOST_CHECK(has_t_degree<p_type1 &>::value);
	BOOST_CHECK(has_t_degree<p_type1 const &>::value);
	BOOST_CHECK(has_t_ldegree<p_type1>::value);
	BOOST_CHECK(has_t_ldegree<const p_type1>::value);
	BOOST_CHECK(has_t_ldegree<p_type1 &>::value);
	BOOST_CHECK(has_t_ldegree<p_type1 const &>::value);
	BOOST_CHECK(has_t_order<p_type1>::value);
	BOOST_CHECK(has_t_order<const p_type1>::value);
	BOOST_CHECK(has_t_order<p_type1 &>::value);
	BOOST_CHECK(has_t_order<p_type1 const &>::value);
	BOOST_CHECK(has_t_lorder<p_type1>::value);
	BOOST_CHECK(has_t_lorder<const p_type1>::value);
	BOOST_CHECK(has_t_lorder<p_type1 &>::value);
	BOOST_CHECK(has_t_lorder<p_type1 const &>::value);
	// Trigonometric properties in the coefficients.
	BOOST_CHECK(!has_t_degree<poisson_series<p_type1>>::value);
	BOOST_CHECK(!has_t_ldegree<poisson_series<p_type1>>::value);
	BOOST_CHECK(!has_t_order<poisson_series<p_type1>>::value);
	BOOST_CHECK(!has_t_lorder<poisson_series<p_type1>>::value);
	typedef polynomial<p_type1,short> p_type2;
	BOOST_CHECK(has_t_degree<p_type2>::value);
	BOOST_CHECK(has_t_ldegree<p_type2>::value);
	BOOST_CHECK(has_t_order<p_type2>::value);
	BOOST_CHECK(has_t_lorder<p_type2>::value);
	BOOST_CHECK_EQUAL(math::t_degree(p_type2{}),0);
	BOOST_CHECK_EQUAL(math::t_degree(p_type2{"x"}),0);
	BOOST_CHECK_EQUAL(math::t_degree(p_type2{p_type1{"x"}}),0);
	BOOST_CHECK_EQUAL(math::t_degree(p_type2{math::cos(p_type1{"x"})}),1);
	BOOST_CHECK_EQUAL(math::t_degree(p_type2{math::cos(p_type1{"x"}-p_type1{"y"})}),0);
	BOOST_CHECK_EQUAL(math::t_ldegree(p_type2{1 + math::cos(p_type1{"x"}+p_type1{"y"})}),0);
	BOOST_CHECK_EQUAL(math::t_order(p_type2{math::cos(p_type1{"x"}-p_type1{"y"})}),2);
	BOOST_CHECK_EQUAL(math::t_lorder(p_type2{math::cos(p_type1{"x"}-p_type1{"y"}) + math::cos(p_type1{"x"}+p_type1{"y"})}),2);
}

struct key02
{
	key02() = default;
	key02(const key02 &) = default;
	key02(key02 &&) noexcept;
	key02 &operator=(const key02 &) = default;
	key02 &operator=(key02 &&) noexcept;
	key02(const symbol_set &);
	bool operator==(const key02 &) const;
	bool operator!=(const key02 &) const;
	bool is_compatible(const symbol_set &) const noexcept;
	bool is_ignorable(const symbol_set &) const noexcept;
	key02 merge_args(const symbol_set &, const symbol_set &) const;
	bool is_unitary(const symbol_set &) const;
	void print(std::ostream &, const symbol_set &) const;
	void print_tex(std::ostream &, const symbol_set &) const;
	template <typename ... Args>
	int t_degree(const Args & ...) const;
	template <typename ... Args>
	int t_ldegree(const Args & ...) const;
	template <typename ... Args>
	int t_order(const Args & ...) const;
	template <typename ... Args>
	int t_lorder(const Args & ...) const;
};

struct key03: key02
{
	key03() = default;
	key03(const key03 &) = default;
	key03(key03 &&) noexcept;
	key03 &operator=(const key03 &) = default;
	key03 &operator=(key03 &&) noexcept;
	key03(const symbol_set &);
	key03 merge_args(const symbol_set &, const symbol_set &) const;
	bool operator==(const key03 &) const;
	bool operator!=(const key03 &) const;
	template <typename ... Args>
	int t_lorder(const Args & ...);
};

struct fake_int_01
{
	fake_int_01();
	explicit fake_int_01(int);
	fake_int_01(const fake_int_01 &);
	fake_int_01(fake_int_01 &&) noexcept;
	fake_int_01 &operator=(const fake_int_01 &);
	fake_int_01 &operator=(fake_int_01 &&) noexcept;
	~fake_int_01();
	bool operator==(const fake_int_01 &) const;
	bool operator!=(const fake_int_01 &) const;
	bool operator<(const fake_int_01 &) const;
	fake_int_01 operator+(const fake_int_01 &) const;
	fake_int_01 &operator+=(const fake_int_01 &);
	fake_int_01 operator-(const fake_int_01 &) const;
	fake_int_01 &operator-=(const fake_int_01 &);
	friend std::ostream &operator<<(std::ostream &, const fake_int_01 &);
};

struct fake_int_02
{
	fake_int_02();
	explicit fake_int_02(int);
	fake_int_02(const fake_int_02 &);
	fake_int_02(fake_int_02 &&) noexcept;
	fake_int_02 &operator=(const fake_int_02 &);
	fake_int_02 &operator=(fake_int_02 &&) noexcept;
	~fake_int_02();
	bool operator==(const fake_int_02 &) const;
	bool operator!=(const fake_int_02 &) const;
	//bool operator<(const fake_int_02 &) const;
	fake_int_02 operator+(const fake_int_02 &) const;
	fake_int_02 &operator+=(const fake_int_02 &);
	fake_int_02 operator-(const fake_int_02 &) const;
	fake_int_02 &operator-=(const fake_int_02 &);
	friend std::ostream &operator<<(std::ostream &, const fake_int_02 &);
};

struct key04: key02
{
	key04() = default;
	key04(const key04 &) = default;
	key04(key04 &&) noexcept;
	key04 &operator=(const key04 &) = default;
	key04 &operator=(key04 &&) noexcept;
	key04(const symbol_set &);
	key04 merge_args(const symbol_set &, const symbol_set &) const;
	bool operator==(const key04 &) const;
	bool operator!=(const key04 &) const;
	template <typename ... Args>
	fake_int_01 t_lorder(const Args & ...) const;
};

struct key05: key02
{
	key05() = default;
	key05(const key05 &) = default;
	key05(key05 &&) noexcept;
	key05 &operator=(const key05 &) = default;
	key05 &operator=(key05 &&) noexcept;
	key05(const symbol_set &);
	key05 merge_args(const symbol_set &, const symbol_set &) const;
	bool operator==(const key05 &) const;
	bool operator!=(const key05 &) const;
	template <typename ... Args>
	fake_int_02 t_lorder(const Args & ...) const;
};

namespace std
{

template <>
struct hash<key02>
{
	std::size_t operator()(const key02 &) const noexcept;
};

template <>
struct hash<key03>
{
	std::size_t operator()(const key03 &) const noexcept;
};

template <>
struct hash<key04>
{
	std::size_t operator()(const key04 &) const noexcept;
};

template <>
struct hash<key05>
{
	std::size_t operator()(const key05 &) const noexcept;
};

}

template <typename Cf, typename Key>
class g_term_type: public base_term<Cf,Key,g_term_type<Cf,Key>>
{
		typedef base_term<Cf,Key,g_term_type> base;
	public:
		g_term_type() = default;
		g_term_type(const g_term_type &) = default;
		g_term_type(g_term_type &&) = default;
		g_term_type &operator=(const g_term_type &) = default;
		g_term_type &operator=(g_term_type &&) = default;
		PIRANHA_FORWARDING_CTOR(g_term_type,base)
};

template <typename Cf, typename Key>
class g_series_type: public trigonometric_series<series<g_term_type<Cf,Key>,g_series_type<Cf,Key>>>
{
	public:
		typedef trigonometric_series<series<g_term_type<Cf,Key>,g_series_type<Cf,Key>>> base;
		g_series_type() = default;
		g_series_type(const g_series_type &) = default;
		g_series_type(g_series_type &&) = default;
		g_series_type &operator=(const g_series_type &) = default;
		g_series_type &operator=(g_series_type &&) = default;
		PIRANHA_FORWARDING_CTOR(g_series_type,base)
		PIRANHA_FORWARDING_ASSIGNMENT(g_series_type,base)
};

BOOST_AUTO_TEST_CASE(trigonometric_series_failures_test)
{
	BOOST_CHECK((has_t_degree<g_series_type<double,key02>>::value));
	BOOST_CHECK((has_t_ldegree<g_series_type<double,key02>>::value));
	BOOST_CHECK((has_t_order<g_series_type<double,key02>>::value));
	BOOST_CHECK((has_t_lorder<g_series_type<double,key02>>::value));
	BOOST_CHECK((!has_t_degree<g_series_type<double,key03>>::value));
	BOOST_CHECK((!has_t_ldegree<g_series_type<double,key03>>::value));
	BOOST_CHECK((!has_t_order<g_series_type<double,key03>>::value));
	BOOST_CHECK((!has_t_lorder<g_series_type<double,key03>>::value));
	BOOST_CHECK((has_t_lorder<g_series_type<double,key04>>::value));
	BOOST_CHECK((!has_t_lorder<g_series_type<double,key05>>::value));
}
