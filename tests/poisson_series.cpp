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

#include "../src/poisson_series.hpp"

#define BOOST_TEST_MODULE poisson_series_test
#include <boost/test/unit_test.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "../src/environment.hpp"
#include "../src/integer.hpp"
#include "../src/math.hpp"
#include "../src/poisson_series.hpp"
#include "../src/polynomial.hpp"
#include "../src/power_series.hpp"
#include "../src/rational.hpp"
#include "../src/real.hpp"
#include "../src/series.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

typedef boost::mpl::vector<double,rational,real,polynomial<rational,int>,polynomial<real,signed char>> cf_types;

struct constructor_tester
{
	template <typename Cf>
	void poly_ctor_test(typename std::enable_if<std::is_base_of<detail::polynomial_tag,Cf>::value>::type * = nullptr)
	{
		typedef poisson_series<Cf> p_type;
		// Construction from symbol name.
		p_type p2{"x"};
		BOOST_CHECK(p2.size() == 1u);
		BOOST_CHECK(p2 == p_type{"x"});
		BOOST_CHECK(p2 != p_type{std::string("y")});
		BOOST_CHECK(p2 == p_type{"x"} + p_type{"y"} - p_type{"y"});
		BOOST_CHECK((std::is_constructible<p_type,std::string>::value));
		BOOST_CHECK((std::is_constructible<p_type,const char *>::value));
		BOOST_CHECK((!std::is_constructible<p_type,environment>::value));
		BOOST_CHECK((std::is_assignable<p_type,std::string>::value));
		BOOST_CHECK((!std::is_assignable<p_type,environment>::value));
	}
	template <typename Cf>
	void poly_ctor_test(typename std::enable_if<!std::is_base_of<detail::polynomial_tag,Cf>::value>::type * = nullptr)
	{
		typedef poisson_series<Cf> p_type;
		if (!std::is_constructible<Cf,std::string>::value) {
			BOOST_CHECK((!std::is_constructible<p_type,std::string>::value));
			BOOST_CHECK((!std::is_constructible<p_type,const char *>::value));
		}
		BOOST_CHECK((!std::is_constructible<p_type,environment>::value));
		BOOST_CHECK((!std::is_assignable<p_type,environment>::value));
		BOOST_CHECK((std::is_assignable<p_type,int>::value));
	}
	template <typename Cf>
	void operator()(const Cf &)
	{
		typedef poisson_series<Cf> p_type;
		BOOST_CHECK(is_series<p_type>::value);
		// Default construction.
		p_type p1;
		BOOST_CHECK(p1 == 0);
		BOOST_CHECK(p1.empty());
		poly_ctor_test<Cf>();
		// Construction from number-like entities.
		p_type p3{3};
		BOOST_CHECK(p3.size() == 1u);
		BOOST_CHECK(p3 == 3);
		BOOST_CHECK(3 == p3);
// NOTE: one instance of this fails to compile because at the present time clang/libc++ reports false
// for std::is_constructible<double,integer>::value. It seems like the explicit cast operator is not
// considered by the type trait. We need to check with the latest versions
// and, if this is still a problem, submit a bugreport.
#if !defined(__clang__)
		p_type p3a{integer(3)};
		BOOST_CHECK(p3a == p3);
		BOOST_CHECK(p3 == p3a);
#endif
		// Construction from poisson series of different type.
		typedef poisson_series<polynomial<rational,short>> p_type1;
		typedef poisson_series<polynomial<integer,short>> p_type2;
		p_type1 p4(1);
		p_type2 p5(p4);
		BOOST_CHECK(p4 == p5);
		BOOST_CHECK(p5 == p4);
		p_type1 p6("x");
		p_type2 p7(std::string("x"));
		p_type2 p8("y");
		BOOST_CHECK(p6 == p7);
		BOOST_CHECK(p7 == p6);
		BOOST_CHECK(p6 != p8);
		BOOST_CHECK(p8 != p6);
	}
};

BOOST_AUTO_TEST_CASE(poisson_series_constructors_test)
{
	environment env;
	boost::mpl::for_each<cf_types>(constructor_tester());
}

struct assignment_tester
{
	template <typename Cf>
	void poly_assignment_test(typename std::enable_if<std::is_base_of<detail::polynomial_tag,Cf>::value>::type * = nullptr)
	{
		typedef poisson_series<Cf> p_type;
		p_type p1;
		p1 = "x";
		BOOST_CHECK(p1 == p_type("x"));
	}
	template <typename Cf>
	void poly_assignment_test(typename std::enable_if<!std::is_base_of<detail::polynomial_tag,Cf>::value>::type * = nullptr)
	{}
	template <typename Cf>
	void operator()(const Cf &)
	{
		typedef poisson_series<Cf> p_type;
		p_type p1;
		p1 = 1;
		BOOST_CHECK(p1 == 1);
#if !defined(__clang__)
		p1 = integer(10);
		BOOST_CHECK(p1 == integer(10));
#endif
		poly_assignment_test<Cf>();
	}
};

BOOST_AUTO_TEST_CASE(poisson_series_assignment_test)
{
	boost::mpl::for_each<cf_types>(assignment_tester());
}

BOOST_AUTO_TEST_CASE(poisson_series_stream_test)
{
	typedef poisson_series<integer> p_type1;
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_type1{}),"0");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_type1{1}),"1");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_type1{1} - 3),"-2");
	typedef poisson_series<rational> p_type2;
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_type2{}),"0");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_type2{rational(1,2)}),"1/2");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_type2{real("-0.5")}),"-1/2");
	typedef poisson_series<polynomial<rational,short>> p_type3;
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_type3{}),"0");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_type3{"x"}),"x");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(rational(3,-2) * p_type3{"x"}),"-3/2*x");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(rational(3,-2) * p_type3{"x"}.pow(2)),"-3/2*x**2");
}

BOOST_AUTO_TEST_CASE(poisson_series_sin_cos_test)
{
	typedef poisson_series<polynomial<rational,short>> p_type1;
	p_type1 p1{"x"};
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::sin(-p1)),"-sin(x)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::cos(p1)),"cos(x)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p1.sin()),"sin(x)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>((-p1).cos()),"cos(x)");
	p1 = 0;
	BOOST_CHECK_EQUAL(math::sin(-p1),0);
	BOOST_CHECK_EQUAL(math::cos(p1),1);
	p1 = p_type1{"x"} - 2 * p_type1{"y"};
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::sin(-p1)),"-sin(x-2*y)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::cos(-p1)),"cos(x-2*y)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(3 * p1.sin()),"3*sin(x-2*y)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p1.cos()),"cos(x-2*y)");
	p1 = p_type1{"x"} * p_type1{"y"};
	BOOST_CHECK_THROW(math::sin(p1),std::invalid_argument);
	BOOST_CHECK_THROW(math::cos(p1),std::invalid_argument);
	BOOST_CHECK_THROW(math::sin(p_type1{"x"} + 1),std::invalid_argument);
	BOOST_CHECK_THROW(math::cos(p_type1{"x"} - 1),std::invalid_argument);
	BOOST_CHECK_THROW(math::sin(p_type1{"x"} * rational(1,2)),std::invalid_argument);
	BOOST_CHECK_THROW(math::cos(p_type1{"x"} * rational(1,2)),std::invalid_argument);
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::sin(p_type1{"x"} * rational(4,-2))),"-sin(2*x)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(-math::cos(p_type1{"x"} * rational(4,2))),"-cos(2*x)");
	typedef poisson_series<polynomial<real,short>> p_type2;
	BOOST_CHECK_EQUAL(math::sin(p_type2{3}),math::sin(real(3)));
	BOOST_CHECK_EQUAL(math::cos(p_type2{3}),math::cos(real(3)));
	p_type2 p2 = p_type2{"x"} - 2 * p_type2{"y"};
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::sin(-p2)),"-1.00000000000000000000000000000000000*sin(x-2*y)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::cos(-p2)),"1.00000000000000000000000000000000000*cos(x-2*y)");
	BOOST_CHECK_THROW(math::sin(p_type2{"x"} * real(rational(1,2))),std::invalid_argument);
	BOOST_CHECK_THROW(math::cos(p_type2{"x"} * real(rational(1,2))),std::invalid_argument);
	typedef poisson_series<real> p_type3;
	BOOST_CHECK_EQUAL(math::sin(p_type3{3}),math::sin(real(3)));
	BOOST_CHECK_EQUAL(math::cos(p_type3{3}),math::cos(real(3)));
	typedef poisson_series<rational> p_type4;
	BOOST_CHECK_EQUAL(math::sin(p_type4{0}),0);
	BOOST_CHECK_EQUAL(math::cos(p_type4{0}),1);
	BOOST_CHECK_THROW(math::cos(p_type4{1}),std::invalid_argument);
	BOOST_CHECK_THROW(math::sin(p_type4{1}),std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(poisson_series_arithmetic_test)
{
	// Just some random arithmetic tests using known trigonometric identities.
	typedef poisson_series<polynomial<rational,short>> p_type1;
	p_type1 x{"x"}, y{"y"};
	BOOST_CHECK_EQUAL(math::cos(x) * math::cos(y),(math::cos(x - y) + math::cos(x + y)) / 2);
	BOOST_CHECK_EQUAL(math::cos(-x) * math::cos(y),(math::cos(x - y) + math::cos(x + y)) / 2);
	BOOST_CHECK_EQUAL(math::cos(x) * math::cos(-y),(math::cos(x - y) + math::cos(x + y)) / 2);
	BOOST_CHECK_EQUAL(math::cos(-x) * math::cos(-y),(math::cos(x - y) + math::cos(x + y)) / 2);
	BOOST_CHECK_EQUAL(math::sin(x) * math::sin(y),(math::cos(x - y) - math::cos(x + y)) / 2);
	BOOST_CHECK_EQUAL(math::sin(-x) * math::sin(y),-(math::cos(x - y) - math::cos(x + y)) / 2);
	BOOST_CHECK_EQUAL(math::sin(x) * math::sin(-y),-(math::cos(x - y) - math::cos(x + y)) / 2);
	BOOST_CHECK_EQUAL(math::sin(-x) * math::sin(-y),(math::cos(x - y) - math::cos(x + y)) / 2);
	BOOST_CHECK_EQUAL(math::sin(x) * math::cos(y),(math::sin(x + y) + math::sin(x - y)) / 2);
	BOOST_CHECK_EQUAL(math::sin(-x) * math::cos(y),-(math::sin(x + y) + math::sin(x - y)) / 2);
	BOOST_CHECK_EQUAL(math::sin(x) * math::cos(-y),(math::sin(x + y) + math::sin(x - y)) / 2);
	BOOST_CHECK_EQUAL(math::sin(-x) * math::cos(-y),-(math::sin(x + y) + math::sin(x - y)) / 2);
	BOOST_CHECK_EQUAL(math::cos(x) * math::sin(y),(math::sin(x + y) - math::sin(x - y)) / 2);
	BOOST_CHECK_EQUAL(math::cos(-x) * math::sin(y),(math::sin(x + y) - math::sin(x - y)) / 2);
	BOOST_CHECK_EQUAL(math::cos(x) * math::sin(-y),-(math::sin(x + y) - math::sin(x - y)) / 2);
	BOOST_CHECK_EQUAL(math::cos(-x) * math::sin(-y),-(math::sin(x + y) - math::sin(x - y)) / 2);
	using math::sin;
	using math::cos;
	using math::pow;
	BOOST_CHECK_EQUAL(pow(sin(x),5),(10 * sin(x) - 5 * sin(3 * x) + sin(5 * x)) / 16);
	BOOST_CHECK_EQUAL(pow(cos(x),5),(10 * cos(x) + 5 * cos(3 * x) + cos(5 * x)) / 16);
	BOOST_CHECK_EQUAL(pow(cos(x),5) * pow(sin(x),5),(10 * sin(2 * x) - 5 * sin(6 * x) + sin(10 * x)) / 512);
	BOOST_CHECK_EQUAL(pow(p_type1{rational(1,2)},5),pow(rational(1,2),5));
	typedef poisson_series<polynomial<real,short>> p_type2;
	BOOST_CHECK_EQUAL(pow(p_type2(real("1.234")),real("-5.678")),pow(real("1.234"),real("-5.678")));
	BOOST_CHECK_EQUAL(sin(p_type2(real("1.234"))),sin(real("1.234")));
	BOOST_CHECK_EQUAL(cos(p_type2(real("1.234"))),cos(real("1.234")));
	typedef poisson_series<real> p_type3;
	BOOST_CHECK_EQUAL(sin(p_type3(real("1.234"))),sin(real("1.234")));
	BOOST_CHECK_EQUAL(cos(p_type3(real("1.234"))),cos(real("1.234")));
}

BOOST_AUTO_TEST_CASE(poisson_series_degree_test)
{
	using math::sin;
	using math::cos;
	using math::pow;
	typedef poisson_series<polynomial<rational,short>> p_type1;
	BOOST_CHECK(has_degree<p_type1>::value);
	BOOST_CHECK(has_ldegree<p_type1>::value);
	BOOST_CHECK(math::degree(p_type1{}) == 0);
	BOOST_CHECK(math::degree(p_type1{"x"}) == 1);
	BOOST_CHECK(math::degree(p_type1{"x"} + 1) == 1);
	BOOST_CHECK(math::degree(p_type1{"x"}.pow(2) + 1) == 2);
	BOOST_CHECK(math::degree(p_type1{"x"} * p_type1{"y"} + 1) == 2);
	BOOST_CHECK(math::degree(p_type1{"x"} * p_type1{"y"} + 1,{"x"}) == 1);
	BOOST_CHECK(math::degree(p_type1{"x"} * p_type1{"y"} + 1,{"x","y"}) == 2);
	BOOST_CHECK(math::degree(p_type1{"x"} * p_type1{"y"} + 1,{"z"}) == 0);
	BOOST_CHECK(math::ldegree(p_type1{"x"} + 1) == 0);
	BOOST_CHECK(math::ldegree(p_type1{"x"} * p_type1{"y"} + p_type1{"x"},{"x","y"}) == 1);
	BOOST_CHECK(math::ldegree(p_type1{"x"} * p_type1{"y"} + p_type1{"x"},{"x"}) == 1);
	BOOST_CHECK(math::ldegree(p_type1{"x"} * p_type1{"y"} + p_type1{"x"},{"y"}) == 0);
	p_type1 x{"x"}, y{"y"};
	BOOST_CHECK(math::degree(pow(x,2) * cos(y) + 1) == 2);
	BOOST_CHECK(math::ldegree(pow(x,2) * cos(y) + 1) == 0);
	BOOST_CHECK(math::ldegree((x * y + y) * cos(y) + 1,{"x"}) == 0);
	BOOST_CHECK(math::ldegree((x * y + y) * cos(y) + 1,{"y"}) == 0);
	BOOST_CHECK(math::ldegree((x * y + y) * cos(y) + y,{"y"}) == 1);
	BOOST_CHECK(math::ldegree((x * y + y) * cos(y) + y,{"x"}) == 0);
	BOOST_CHECK(math::ldegree((x * y + y) * cos(y) + y) == 1);
	BOOST_CHECK(math::ldegree((x * y + y) * cos(y) + y,{"x","y"}) == 1);
	BOOST_CHECK(math::ldegree((x * y + y) * cos(y) + 1,{"x","y"}) == 0);
	typedef poisson_series<rational> p_type2;
	BOOST_CHECK(!has_degree<p_type2>::value);
	BOOST_CHECK(!has_ldegree<p_type2>::value);
}

// Mock coefficient.
struct mock_cf
{
	mock_cf();
	mock_cf(const int &);
	mock_cf(const mock_cf &);
	mock_cf(mock_cf &&) noexcept(true);
	mock_cf &operator=(const mock_cf &);
	mock_cf &operator=(mock_cf &&) noexcept(true);
	friend std::ostream &operator<<(std::ostream &, const mock_cf &);
	mock_cf operator-() const;
	bool operator==(const mock_cf &) const;
	bool operator!=(const mock_cf &) const;
	mock_cf &operator+=(const mock_cf &);
	mock_cf &operator-=(const mock_cf &);
	mock_cf operator+(const mock_cf &) const;
	mock_cf operator-(const mock_cf &) const;
	mock_cf &operator*=(const mock_cf &);
	mock_cf operator*(const mock_cf &) const;
	mock_cf &operator/=(int);
};

BOOST_AUTO_TEST_CASE(poisson_series_partial_test)
{
	using math::sin;
	using math::cos;
	using math::pow;
	using math::partial;
	typedef poisson_series<polynomial<rational,short>> p_type1;
	BOOST_CHECK(is_differentiable<p_type1>::value);
	BOOST_CHECK(has_pbracket<p_type1>::value);
	BOOST_CHECK(has_transformation_is_canonical<p_type1>::value);
	p_type1 x{"x"}, y{"y"};
	BOOST_CHECK_EQUAL(partial(x * cos(y),"x"),cos(y));
	BOOST_CHECK_EQUAL(partial(x * cos(2 * x),"x"),cos(2 * x) - 2 * x * sin(2 * x));
	BOOST_CHECK_EQUAL(partial(x * cos(2 * x + y),"y"),-x * sin(2 * x + y));
	BOOST_CHECK_EQUAL(partial(rational(3,2) * cos(2 * x + y),"x"),-3 * sin(2 * x + y));
	BOOST_CHECK_EQUAL(partial(rational(3,2) * x * cos(y),"y"),-rational(3,2) * x * sin(+y));
	BOOST_CHECK_EQUAL(partial(pow(x * cos(y),5),"y"),5 * sin(-y) * x * pow(x * cos(y),4));
	BOOST_CHECK_EQUAL(partial(pow(x * cos(y),5),"z"),0);
	// y as implicit function of x: y = cos(x).
	p_type1::register_custom_derivative("x",[x](const p_type1 &p) {
		return p.partial("x") - partial(p,"y") * sin(x);
	});
	BOOST_CHECK_EQUAL(partial(x + cos(y),"x"),1 + sin(y) * sin(x));
	BOOST_CHECK_EQUAL(partial(x + x * cos(y),"x"),1 + cos(y) + x * sin(y) * sin(x));
	BOOST_CHECK((!is_differentiable<poisson_series<polynomial<mock_cf,short>>>::value));
	BOOST_CHECK((!has_pbracket<poisson_series<polynomial<mock_cf,short>>>::value));
	BOOST_CHECK((!has_transformation_is_canonical<poisson_series<polynomial<mock_cf,short>>>::value));
}

BOOST_AUTO_TEST_CASE(poisson_series_transform_filter_test)
{
	using math::sin;
	using math::cos;
	using math::pow;
	typedef poisson_series<polynomial<rational,short>> p_type1;
	typedef std::decay<decltype(*(p_type1{}.begin()))>::type pair_type;
	typedef std::decay<decltype(*(p_type1{}.begin()->first.begin()))>::type pair_type2;
	p_type1 x{"x"}, y{"y"};
	auto s = pow(1 + x + y,3) * cos(x) + pow(y,3) * sin(x);
	auto s_t = s.transform([](const pair_type &p) {
		return std::make_pair(p.first.filter([](const pair_type2 &p2) {return math::degree(p2.second) < 2;}),p.second);
	});
	BOOST_CHECK_EQUAL(s_t,(3*x + 3*y + 1) * cos(x));
}

BOOST_AUTO_TEST_CASE(poisson_series_evaluate_test)
{
	using math::sin;
	using math::cos;
	using math::pow;
	typedef poisson_series<polynomial<rational,short>> p_type1;
	std::unordered_map<std::string,real> dict{{"x",real(1.234)},{"y",real(5.678)}};
	p_type1 x{"x"}, y{"y"};
	auto s1 = (x + y) * cos(x + y);
	auto tmp1 = (real(0) + real(1) * pow(real(1.234),1) * pow(real(5.678),0) + real(1) * pow(real(1.234),0) * pow(real(5.678),1)) *
		cos(real(0) + real(1) * real(1.234) + real(1) * real(5.678));
	BOOST_CHECK_EQUAL(tmp1,s1.evaluate(dict));
	BOOST_CHECK((std::is_same<real,decltype(s1.evaluate(dict))>::value));
	auto s2 = pow(y,3) * sin(x + y);
	auto tmp2 = (real(0) + real(1) * pow(real(1.234),0) * pow(real(5.678),3)) *
		sin(real(0) + real(1) * real(1.234) + real(1) * real(5.678));
	BOOST_CHECK_EQUAL(tmp2,s2.evaluate(dict));
	BOOST_CHECK((std::is_same<real,decltype(s2.evaluate(dict))>::value));
	// NOTE: here it seems to be quite a brittle test: if one changes the order of the operands s1 and s2,
	// the test fails on my test machine due to differences of order epsilon. Most likely it's a matter
	// of ordering of the floating-point operations and it will depend on a ton of factors. Better just disable it,
	// and keep this in mind if other tests start failing similarly.
	// BOOST_CHECK_EQUAL(tmp1 + tmp2,(s2 + s1).evaluate(dict));
}

BOOST_AUTO_TEST_CASE(poisson_series_subs_test)
{
	using math::sin;
	using math::cos;
	using math::pow;
	using math::subs;
	typedef poisson_series<polynomial<real,short>> p_type1;
	BOOST_CHECK(p_type1{}.subs("x",integer(4)).empty());
	p_type1 x{"x"}, y{"y"};
	auto s = (x + y) * cos(x) + pow(y,3) * sin(x);
	BOOST_CHECK_EQUAL(s.subs("x",real(1.234)),(real(1.234) + y) * cos(real(1.234)) + pow(y,3) * sin(real(1.234)));
	BOOST_CHECK((std::is_same<decltype(s.subs("x",real(1.234))),p_type1>::value));
	BOOST_CHECK((std::is_same<decltype(s.subs("x",rational(1.234))),p_type1>::value));
	s = (x + y) * cos(x + y) + pow(y,3) * sin(x + y);
	real r(1.234);
	BOOST_CHECK_EQUAL(s.subs("x",r),(r + y) * (cos(r) * cos(y) - sin(r) * sin(y)) + pow(y,3) * (sin(r) * cos(y) + cos(r) * sin(y)));
	BOOST_CHECK_EQUAL(subs(s,"x",r),(r + y) * (cos(r) * cos(y) - sin(r) * sin(y)) + pow(y,3) * (sin(r) * cos(y) + cos(r) * sin(y)));
	BOOST_CHECK_EQUAL(subs(s,"z",r),s);
	s = (x + y) * cos(-x + y) + pow(y,3) * sin(-x + y);
	BOOST_CHECK_EQUAL(s.subs("x",r),(r + y) * (cos(r) * cos(y) + sin(r) * sin(y)) + pow(y,3) * (-sin(r) * cos(y) + cos(r) * sin(y)));
	s = (x + y) * cos(-2 * x + y) + pow(y,3) * sin(-5 * x + y);
	BOOST_CHECK_EQUAL(s.subs("x",r),(r + y) * (cos(r * 2) * cos(y) + sin(r * 2) * sin(y)) + pow(y,3) * (-sin(r * 5) * cos(y) + cos(r * 5) * sin(y)));
	s = (x + y) * cos(-2 * x + y) + pow(x,3) * sin(-5 * x + y);
	BOOST_CHECK_EQUAL(s.subs("x",r),(r + y) * (cos(r * 2) * cos(y) + sin(r * 2) * sin(y)) + pow(r,3) * (-sin(r * 5) * cos(y) + cos(r * 5) * sin(y)));
	typedef poisson_series<polynomial<rational,short>> p_type2;
	p_type2 a{"a"}, b{"b"};
	auto t = a * cos(a + b) + b * sin(a);
	BOOST_CHECK_EQUAL(t.subs("a",b),b * cos(b + b) + b * sin(b));
	BOOST_CHECK_EQUAL(subs(t,"a",a + b),(a + b) * cos(a + b + b) + b * sin(a + b));
	t = a * cos(-3 * a + b) + b * sin(-5 * a - b);
	BOOST_CHECK_EQUAL(subs(t,"a",a + b),(a + b) * cos(-3 * (a + b) + b) + b * sin(-5 * (a + b) - b));
	BOOST_CHECK_EQUAL(subs(t,"a",2 * (a + b)),2 * (a + b) * cos(-6 * (a + b) + b) + b * sin(-10 * (a + b) - b));
	BOOST_CHECK_EQUAL(subs(t,"b",-5 * a),a * cos(-3 * a - 5 * a));
	BOOST_CHECK(t.subs("b",5 * a).subs("a",rational(0)).empty());
	BOOST_CHECK_EQUAL((a * cos(b)).subs("b",rational(0)),a);
	BOOST_CHECK_EQUAL((a * sin(b)).subs("b",rational(0)),rational(0));
	BOOST_CHECK((std::is_same<decltype(subs(t,"a",a + b)),p_type2>::value));
}

BOOST_AUTO_TEST_CASE(poisson_series_print_tex_test)
{
	using math::sin;
	using math::cos;
	typedef poisson_series<polynomial<rational,short>> p_type1;
	p_type1 x{"x"}, y{"y"};
	std::ostringstream oss;
	std::string s1 = "3\\frac{{x}}{{y}}\\cos{\\left({x}+{y}\\right)}";
	std::string s2 = "2\\frac{{x}^{2}}{{y}^{2}}\\cos{\\left(3{x}\\right)}";
	((3*x*y.pow(-1)) * cos(x + y)).print_tex(oss);
	BOOST_CHECK_EQUAL(oss.str(),s1);
	oss.str("");
	((3*x*y.pow(-1)) * cos(x + y) - (2*x.pow(2)*y.pow(-2)) * cos(-3 * x)).print_tex(oss);
	BOOST_CHECK(oss.str() == s1 + "-" + s2 || oss.str() == std::string("-") + s2 + "+" + s1);
	std::string s3 = "\\left({x}+{y}\\right)";
	std::string s4 = "\\left({y}+{x}\\right)";
	oss.str("");
	((x+y)*cos(x)).print_tex(oss);
	BOOST_CHECK(oss.str() == s3 + "\\cos{\\left({x}\\right)}" || oss.str() == s4 + "\\cos{\\left({x}\\right)}");
}

BOOST_AUTO_TEST_CASE(poisson_series_integrate_test)
{
	using math::sin;
	using math::cos;
	typedef poisson_series<polynomial<rational,short>> p_type1;
	p_type1 x{"x"}, y{"y"}, z{"z"};
	BOOST_CHECK_EQUAL(p_type1{}.integrate("x"),p_type1{});
	BOOST_CHECK_EQUAL(x.integrate("x"),x*x/2);
	BOOST_CHECK_EQUAL(x.pow(-2).integrate("x"),-x.pow(-1));
	BOOST_CHECK_EQUAL(math::integrate((x+y)*cos(x) + cos(y),"x"),(x+y)*sin(x) + x*cos(y) + cos(x));
	BOOST_CHECK_EQUAL(math::integrate((x+y)*cos(x) + cos(y),"y"),y/2*(2*x + y)*cos(x)+sin(y));
	BOOST_CHECK_EQUAL(math::integrate((x+y)*cos(x) + cos(x),"x"),(x+y+1)*sin(x)+cos(x));
	BOOST_CHECK_THROW(math::integrate(x.pow(-1)*cos(x),"x"),std::invalid_argument);
	BOOST_CHECK_THROW(math::integrate(x.pow(-2)*cos(x+y) + x,"x"),std::invalid_argument);
	// Some examples computed with Wolfram alpha for checking.
	BOOST_CHECK_EQUAL(math::integrate(x.pow(-2)*cos(x+y) + x,"y"),sin(x+y)*x.pow(-2)+x*y);
	BOOST_CHECK_EQUAL(math::integrate(x.pow(5)*y.pow(4)*z.pow(3)*cos(5*x+4*y+3*z),"x"),
		y.pow(4)*z.pow(3)/3125*(5*x*(125*x.pow(4)-100*x*x+24)*sin(5*x+4*y+3*z)+(625*x.pow(4)-
		300*x*x+24)*cos(5*x+4*y+3*z)));
	BOOST_CHECK_EQUAL(math::integrate(x.pow(5)/37*y.pow(4)*z.pow(3)*cos(5*x-4*y+3*z),"y"),
		x.pow(5)*z.pow(3)/4736*(4*y*(8*y*y-3)*cos(5*x-4*y+3*z)+(-32*y.pow(4)+24*y*y-3)*sin(5*x-4*y+3*z)));
	BOOST_CHECK_EQUAL(math::partial(math::integrate(x.pow(5)/37*y.pow(4)*z.pow(3)*cos(5*x-4*y+3*z),"y"),"y"),
		x.pow(5)/37*y.pow(4)*z.pow(3)*cos(5*x-4*y+3*z));
	BOOST_CHECK_EQUAL(math::partial(math::partial(math::integrate(math::integrate(x.pow(5)/37*y.pow(4)*z.pow(3)*cos(5*x-4*y+3*z),"y"),"y"),"y"),"y"),
		x.pow(5)/37*y.pow(4)*z.pow(3)*cos(5*x-4*y+3*z));
	BOOST_CHECK_EQUAL(math::integrate(rational(1,37)*y.pow(4)*z.pow(3)*cos(5*x-4*y+3*z),"x"),rational(1,185)*y.pow(4)*z.pow(3)*sin(5*x-4*y+3*z));
	BOOST_CHECK_EQUAL(math::integrate(rational(1,37)*x.pow(4)*z.pow(3)*cos(4*y-3*z),"x"),rational(1,185)*x.pow(5)*z.pow(3)*cos(4*y-3*z));
	BOOST_CHECK_EQUAL(math::integrate(y.pow(-5)*cos(4*x-3*z)-x*y*y*sin(y).pow(4),"x"),
		(sin(4*x-3*z)-2*x*x*y.pow(7)*sin(y).pow(4))*(4*y.pow(5)).pow(-1));
	BOOST_CHECK_EQUAL((x * x * cos(x)).integrate("x"),(x*x - 2) * sin(x) + 2*x*cos(x));
	BOOST_CHECK_EQUAL(((x * x + y) * cos(x) - y * cos(x)).integrate("x"),(x*x - 2) * sin(x) + 2*x*cos(x));
	BOOST_CHECK_EQUAL(((x * x + y) * cos(x) + y * cos(x) - x * sin(y)).integrate("x"),
		-(x*x)/2 * sin(y) + (x*x+2*y-2)*sin(x) + 2*x*cos(x));
	BOOST_CHECK_EQUAL(((x*x*x + y*x)*cos(2*x - 3*y) + y*x.pow(4)*cos(x) - (x.pow(-5) * sin(y))).integrate("x"),
		x.pow(-4)/8*(32*(x*x-6)*x.pow(5)*y*cos(x)+x.pow(4)*(6*x*x+2*y-3)*cos(2*x-3*y)+2*(x.pow(5)*(2*x*x+2*y-3)*sin(2*x-3*y)+
		4*(x.pow(4)-12*x*x+24)*x.pow(4)*y*sin(x)+sin(y))));
	BOOST_CHECK_EQUAL(math::integrate((x.pow(-1)*cos(y)+x*y*cos(x)).pow(2),"x"),x.pow(-1)/24*(4*x.pow(4)*y*y+
		6*x.pow(3)*y*y*sin(2*x)+6*x*x*y*y*cos(2*x)-3*x*y*y*sin(2*x)+24*x*y*sin(x-y)+24*x*y*sin(x+y)-
		12*cos(2*y)-12));
	BOOST_CHECK_EQUAL(math::integrate((cos(y)*x.pow(-2)+x*x*y*cos(x)).pow(2),"x"),x.pow(5)*y*y/10-(cos(y).pow(2))*x.pow(-3)/3+
		rational(1,4)*(2*x*x-3)*x*y*y*cos(2*x)+rational(1,8)*(2*x.pow(4)-6*x*x+3)*y*y*sin(2*x)+
		2*y*sin(x)*cos(y));
	BOOST_CHECK_EQUAL(math::integrate((x*cos(y)+y*cos(x)).pow(2),"x"),rational(1,6)*x*(x*x*cos(2*y)+x*x+3*y*y)+
		rational(1,4)*y*y*sin(2*x)+2*y*cos(x)*cos(y)+2*x*y*sin(x)*cos(y));
	BOOST_CHECK_EQUAL(math::integrate((x*y*cos(y)+y*cos(x)).pow(2),"x"),rational(1,12)*y*y*(2*x*(x*x*cos(2*y)+x*x+3)
		+24*cos(x)*cos(y)+24*x*sin(x)*cos(y)+3*sin(2*x)));
	BOOST_CHECK_EQUAL(math::integrate((x*y*cos(y)+y*cos(x)+x*x*cos(x)).pow(2),"x"),rational(1,60)*(15*x*(2*x*x+2*y-3)*
		cos(x).pow(2)+x*(6*x.pow(4)+5*x*x*y*y+10*x*x*y*y*cos(y).pow(2)+5*x*x*y*y*cos(2*y)+20*x*x*y-15*
		(2*x*x+2*y-3)*sin(x).pow(2)+120*y*(x*x+y-6)*sin(x)*cos(y)+30*y*y)+15*cos(x)*(8*y*(3*x*x+y-6)*cos(y)+
		(2*x.pow(4)+x*x*(4*y-6)+2*y*y-2*y+3)*sin(x))));
	// This would require sine/cosine integral special functions.
	BOOST_CHECK_THROW(math::integrate((x*y.pow(-1)*cos(y)+y*cos(x)+x*x*cos(x)).pow(2),"y"),std::invalid_argument);
	// Check type trait.
	BOOST_CHECK(is_integrable<p_type1>::value);
	BOOST_CHECK(is_integrable<p_type1 &>::value);
	BOOST_CHECK(is_integrable<const p_type1>::value);
	BOOST_CHECK(is_integrable<p_type1 const &>::value);
	typedef poisson_series<rational> p_type2;
	BOOST_CHECK_EQUAL(p_type2{}.integrate("x"),p_type2{});
	BOOST_CHECK_THROW(p_type2{1}.integrate("x"),std::invalid_argument);
	BOOST_CHECK(is_integrable<p_type2>::value);
	BOOST_CHECK(is_integrable<p_type2 &>::value);
	BOOST_CHECK(is_integrable<const p_type2>::value);
	BOOST_CHECK(is_integrable<p_type2 const &>::value);
}

BOOST_AUTO_TEST_CASE(poisson_series_ipow_subs_test)
{
	typedef poisson_series<polynomial<rational,short>> p_type1;
	BOOST_CHECK(has_ipow_subs<p_type1>::value);
	BOOST_CHECK((has_ipow_subs<p_type1,integer>::value));
	BOOST_CHECK((has_ipow_subs<p_type1,typename p_type1::term_type::cf_type>::value));
	{
	BOOST_CHECK_EQUAL(p_type1{"x"}.ipow_subs("x",integer(4),integer(1)),p_type1{"x"});
	BOOST_CHECK_EQUAL(p_type1{"x"}.ipow_subs("x",integer(1),p_type1{"x"}),p_type1{"x"});
	p_type1 x{"x"}, y{"y"}, z{"z"};
	BOOST_CHECK_EQUAL((x.pow(2) + x * y + z).ipow_subs("x",integer(2),integer(3)),3 + x * y + z);
	BOOST_CHECK_EQUAL((x.pow(2) + x * y + z).ipow_subs("y",integer(1),rational(3,2)),x * x + x * rational(3,2) + z);
	BOOST_CHECK_EQUAL((x.pow(7) + x.pow(2) * y + z).ipow_subs("x",integer(3),x),x.pow(3) + x.pow(2) * y + z);
	BOOST_CHECK_EQUAL((x.pow(6) + x.pow(2) * y + z).ipow_subs("x",integer(3),p_type1{}),x.pow(2) * y + z);
	}
	{
	typedef poisson_series<polynomial<real,short>> p_type2;
	BOOST_CHECK(has_ipow_subs<p_type2>::value);
	BOOST_CHECK((has_ipow_subs<p_type2,integer>::value));
	BOOST_CHECK((has_ipow_subs<p_type2,typename p_type2::term_type::cf_type>::value));
	p_type2 x{"x"}, y{"y"};
	BOOST_CHECK_EQUAL((x*x*x + y*y).ipow_subs("x",integer(1),real(1.234)),y*y + math::pow(real(1.234),3));
	BOOST_CHECK_EQUAL((x*x*x + y*y).ipow_subs("x",integer(3),real(1.234)),y*y + real(1.234));
	BOOST_CHECK_EQUAL((x*x*x + y*y).ipow_subs("x",integer(2),real(1.234)).ipow_subs("y",integer(2),real(-5.678)),real(-5.678) +
		real(1.234) * x);
	BOOST_CHECK_EQUAL(math::ipow_subs(x*x*x + y*y,"x",integer(1),real(1.234)).ipow_subs("y",integer(1),real(-5.678)),math::pow(real(-5.678),2) +
		math::pow(real(1.234),3));
	}
	p_type1 x{"x"}, y{"y"}, z{"z"};
	BOOST_CHECK_EQUAL(math::ipow_subs(x.pow(-7) + y + z,"x",integer(2),y),x.pow(-7) + y + z);
	BOOST_CHECK_EQUAL(math::ipow_subs(x.pow(-7) + y + z,"x",integer(-2),y),x.pow(-1) * y.pow(3) + y + z);
	BOOST_CHECK_EQUAL(math::ipow_subs(x.pow(-7) + y + z,"x",integer(-7),z),y + 2*z);
	BOOST_CHECK_EQUAL(math::ipow_subs(x.pow(-7) * math::cos(x) + y + z,"x",integer(-4),z),(z * x.pow(-3)) * math::cos(x) + y + z);
	BOOST_CHECK_EQUAL(math::ipow_subs(x.pow(-7) * math::cos(x) + y + z,"x",integer(4),z),x.pow(-7) * math::cos(x) + y + z);
}

BOOST_AUTO_TEST_CASE(poisson_series_is_evaluable_test)
{
	typedef poisson_series<polynomial<rational,short>> p_type1;
	BOOST_CHECK((is_evaluable<p_type1,double>::value));
	BOOST_CHECK((is_evaluable<p_type1,float>::value));
	BOOST_CHECK((is_evaluable<p_type1,real>::value));
	BOOST_CHECK((is_evaluable<p_type1,rational>::value));
	BOOST_CHECK((!is_evaluable<p_type1,std::string>::value));
	BOOST_CHECK((is_evaluable<p_type1,integer>::value));
	BOOST_CHECK((!is_evaluable<p_type1,int>::value));
	BOOST_CHECK((!is_evaluable<p_type1,long>::value));
	BOOST_CHECK((!is_evaluable<p_type1,long long>::value));
	BOOST_CHECK((!is_evaluable<poisson_series<polynomial<mock_cf,short>>,double>::value));
	BOOST_CHECK((!is_evaluable<poisson_series<mock_cf>,double>::value));
}
