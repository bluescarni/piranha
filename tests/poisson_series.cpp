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
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "../src/config.hpp"
#include "../src/environment.hpp"
#include "../src/math.hpp"
#include "../src/poisson_series.hpp"
#include "../src/polynomial.hpp"
#include "../src/power_series.hpp"
#include "../src/rational.hpp"
#include "../src/real.hpp"

using namespace piranha;

typedef boost::mpl::vector<double,rational,real,polynomial<rational>,polynomial<real>> cf_types;

struct constructor_tester
{
	template <typename Cf>
	void poly_ctor_test(typename std::enable_if<std::is_base_of<detail::polynomial_tag,Cf>::value>::type * = piranha_nullptr)
	{
		typedef poisson_series<Cf> p_type;
		// Construction from symbol name.
		p_type p2{"x"};
		BOOST_CHECK(p2.size() == 1u);
		BOOST_CHECK(p2 == p_type{"x"});
		BOOST_CHECK(p2 != p_type{std::string("y")});
		BOOST_CHECK(p2 == p_type{"x"} + p_type{"y"} - p_type{"y"});
		BOOST_CHECK((std::is_constructible<p_type,std::string>::value));
		BOOST_CHECK((std::is_constructible<p_type,char *>::value));
	}
	template <typename Cf>
	void poly_ctor_test(typename std::enable_if<!std::is_base_of<detail::polynomial_tag,Cf>::value>::type * = piranha_nullptr)
	{
		typedef poisson_series<Cf> p_type;
		BOOST_CHECK((!std::is_constructible<p_type,std::string>::value));
		BOOST_CHECK((!std::is_constructible<p_type,char *>::value));
	}
	template <typename Cf>
	void operator()(const Cf &)
	{
		typedef poisson_series<Cf> p_type;
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
		p_type p3a{integer(3)};
		BOOST_CHECK(p3a == p3);
		BOOST_CHECK(p3 == p3a);
		// Construction from poisson series of different type.
		typedef poisson_series<polynomial<rational>> p_type1;
		typedef poisson_series<polynomial<integer>> p_type2;
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
	void poly_assignment_test(typename std::enable_if<std::is_base_of<detail::polynomial_tag,Cf>::value>::type * = piranha_nullptr)
	{
		typedef poisson_series<Cf> p_type;
		p_type p1;
		p1 = "x";
		BOOST_CHECK(p1 == p_type("x"));
	}
	template <typename Cf>
	void poly_assignment_test(typename std::enable_if<!std::is_base_of<detail::polynomial_tag,Cf>::value>::type * = piranha_nullptr)
	{}
	template <typename Cf>
	void operator()(const Cf &)
	{
		typedef poisson_series<Cf> p_type;
		p_type p1;
		p1 = 1;
		BOOST_CHECK(p1 == 1);
		p1 = integer(10);
		BOOST_CHECK(p1 == integer(10));
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
	typedef poisson_series<polynomial<rational>> p_type3;
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_type3{}),"0");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_type3{"x"}),"x");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(rational(3,-2) * p_type3{"x"}),"-3/2x");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(rational(3,-2) * p_type3{"x"}.pow(2)),"-3/2x**2");
}

BOOST_AUTO_TEST_CASE(poisson_series_sin_cos_test)
{
	typedef poisson_series<polynomial<rational>> p_type1;
	p_type1 p1{"x"};
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::sin(-p1)),"-sin(x)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::cos(p1)),"cos(x)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p1.sin()),"sin(x)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>((-p1).cos()),"cos(x)");
	p1 = 0;
	BOOST_CHECK_EQUAL(math::sin(-p1),0);
	BOOST_CHECK_EQUAL(math::cos(p1),1);
	p1 = p_type1{"x"} - 2 * p_type1{"y"};
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::sin(-p1)),"-sin(x-2y)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::cos(-p1)),"cos(x-2y)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(3 * p1.sin()),"3sin(x-2y)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p1.cos()),"cos(x-2y)");
	p1 = p_type1{"x"} * p_type1{"y"};
	BOOST_CHECK_THROW(math::sin(p1),std::invalid_argument);
	BOOST_CHECK_THROW(math::cos(p1),std::invalid_argument);
	BOOST_CHECK_THROW(math::sin(p_type1{"x"} + 1),std::invalid_argument);
	BOOST_CHECK_THROW(math::cos(p_type1{"x"} - 1),std::invalid_argument);
	BOOST_CHECK_THROW(math::sin(p_type1{"x"} * rational(1,2)),std::invalid_argument);
	BOOST_CHECK_THROW(math::cos(p_type1{"x"} * rational(1,2)),std::invalid_argument);
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::sin(p_type1{"x"} * rational(4,-2))),"-sin(2x)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(-math::cos(p_type1{"x"} * rational(4,2))),"-cos(2x)");
	typedef poisson_series<polynomial<real>> p_type2;
	BOOST_CHECK_EQUAL(math::sin(p_type2{3}),math::sin(real(3)));
	BOOST_CHECK_EQUAL(math::cos(p_type2{3}),math::cos(real(3)));
	p_type2 p2 = p_type2{"x"} - 2 * p_type2{"y"};
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::sin(-p2)),"-1.00000000000000000000000000000000000sin(x-2y)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::cos(-p2)),"1.00000000000000000000000000000000000cos(x-2y)");
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
	typedef poisson_series<polynomial<rational>> p_type1;
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
	typedef poisson_series<polynomial<real>> p_type2;
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
	typedef poisson_series<polynomial<rational>> p_type1;
	BOOST_CHECK(is_power_series<p_type1>::value);
	BOOST_CHECK(p_type1{}.degree() == 0);
	BOOST_CHECK(p_type1{"x"}.degree() == 1);
	BOOST_CHECK((p_type1{"x"} + 1).degree() == 1);
	BOOST_CHECK((p_type1{"x"}.pow(2) + 1).degree() == 2);
	BOOST_CHECK((p_type1{"x"} * p_type1{"y"} + 1).degree() == 2);
	BOOST_CHECK((p_type1{"x"} * p_type1{"y"} + 1).degree({"x"}) == 1);
	BOOST_CHECK((p_type1{"x"} * p_type1{"y"} + 1).degree({"x","y"}) == 2);
	BOOST_CHECK((p_type1{"x"} * p_type1{"y"} + 1).degree({"z"}) == 0);
	BOOST_CHECK((p_type1{"x"} + 1).ldegree() == 0);
	BOOST_CHECK((p_type1{"x"} * p_type1{"y"} + p_type1{"x"}).ldegree({"x","y"}) == 1);
	BOOST_CHECK((p_type1{"x"} * p_type1{"y"} + p_type1{"x"}).ldegree({"x"}) == 1);
	BOOST_CHECK((p_type1{"x"} * p_type1{"y"} + p_type1{"x"}).ldegree({"y"}) == 0);
	p_type1 x{"x"}, y{"y"};
	BOOST_CHECK((pow(x,2) * cos(y) + 1).degree() == 2);
	BOOST_CHECK((pow(x,2) * cos(y) + 1).ldegree() == 0);
	BOOST_CHECK(((x * y + y) * cos(y) + 1).ldegree({"x"}) == 0);
	BOOST_CHECK(((x * y + y) * cos(y) + 1).ldegree({"y"}) == 0);
	BOOST_CHECK(((x * y + y) * cos(y) + y).ldegree({"y"}) == 1);
	BOOST_CHECK(((x * y + y) * cos(y) + y).ldegree({"x"}) == 0);
	BOOST_CHECK(((x * y + y) * cos(y) + y).ldegree() == 1);
	BOOST_CHECK(((x * y + y) * cos(y) + y).ldegree({"x","y"}) == 1);
	BOOST_CHECK(((x * y + y) * cos(y) + 1).ldegree({"x","y"}) == 0);
	typedef poisson_series<rational> p_type2;
	BOOST_CHECK(!is_power_series<p_type2>::value);
}

BOOST_AUTO_TEST_CASE(poisson_series_partial_test)
{
	using math::sin;
	using math::cos;
	using math::pow;
	using math::partial;
	typedef poisson_series<polynomial<rational>> p_type1;
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
}

BOOST_AUTO_TEST_CASE(poisson_series_transform_filter_test)
{
	using math::sin;
	using math::cos;
	using math::pow;
	typedef poisson_series<polynomial<rational>> p_type1;
	typedef decltype(*(p_type1{}.begin())) pair_type;
	typedef decltype(*(p_type1{}.begin()->first.begin())) pair_type2;
	p_type1 x{"x"}, y{"y"};
	auto s = pow(1 + x + y,3) * cos(x) + pow(y,3) * sin(x);
	auto s_t = s.transform([](const pair_type &p) {
		return std::make_pair(p.first.filter([](const pair_type2 &p2) {return p2.second.degree() < 2;}),p.second);
	});
	BOOST_CHECK_EQUAL(s_t,(3*x + 3*y + 1) * cos(x));
}

BOOST_AUTO_TEST_CASE(poisson_series_evaluate_test)
{
	using math::sin;
	using math::cos;
	using math::pow;
	typedef poisson_series<polynomial<rational>> p_type1;
	p_type1 x{"x"}, y{"y"};
	auto s = (x + y) * cos(x + y) + pow(y,3) * sin(x + y);
	auto eval = s.evaluate(std::unordered_map<std::string,double>{{"x",1.234},{"y",5.678}});
	BOOST_CHECK_EQUAL(eval,(1.234 + 5.678) * cos(1.234 + 5.678) + pow(5.678,3) * sin(1.234 + 5.678));
	BOOST_CHECK_EQUAL(eval,math::evaluate(s,std::unordered_map<std::string,double>{{"x",1.234},{"y",5.678}}));
	BOOST_CHECK((std::is_same<double,decltype(eval)>::value));
}
