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

#include "../src/config.hpp"
#include "../src/math.hpp"
#include "../src/poisson_series.hpp"
#include "../src/polynomial.hpp"
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
	}
	template <typename Cf>
	void poly_ctor_test(typename std::enable_if<!std::is_base_of<detail::polynomial_tag,Cf>::value>::type * = piranha_nullptr)
	{}
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
	typedef poisson_series<polynomial<integer>> p_type1;
	p_type1 p1{"x"};
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::sin(p1)),"sin(x)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::cos(p1)),"cos(x)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p1.sin()),"sin(x)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p1.cos()),"cos(x)");
	p1 = 0;
	BOOST_CHECK_EQUAL(math::sin(p1),0);
	BOOST_CHECK_EQUAL(math::cos(p1),1);
	p1 = p_type1{"x"} - 2 * p_type1{"y"};
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::sin(p1)),"sin(x-2y)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(math::cos(p1)),"cos(x-2y)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p1.sin()),"sin(x-2y)");
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p1.cos()),"cos(x-2y)");
}
