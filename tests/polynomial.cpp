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

#include "../src/polynomial.hpp"

#define BOOST_TEST_MODULE polynomial_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>

#include "../src/integer.hpp"
#include "../src/numerical_coefficient.hpp"

using namespace piranha;

typedef boost::mpl::vector<numerical_coefficient<double>,numerical_coefficient<integer>> cf_types;
typedef boost::mpl::vector<unsigned,integer> expo_types;

struct constructor_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef polynomial<Cf,Expo> p_type;
			// Default construction.
			p_type p1;
			BOOST_CHECK(p1 == 0);
			BOOST_CHECK(p1.empty());
			// Construction from symbol name.
			p_type p2{"x"};
			BOOST_CHECK(p2.size() == 1u);
			BOOST_CHECK(p2 == p_type{"x"});
			BOOST_CHECK(p2 != p_type{"y"});
			BOOST_CHECK(p2 == p_type{"x"} + p_type{"y"} - p_type{"y"});
			// Construction from number-like entities.
			p_type p3{3};
			BOOST_CHECK(p3.size() == 1u);
			BOOST_CHECK(p3 == 3);
			BOOST_CHECK(3 == p3);
			BOOST_CHECK(p3 != p2);
			p_type p3a{integer(3)};
			BOOST_CHECK(p3a == p3);
			BOOST_CHECK(p3 == p3a);
			// Construction from polynomial of different type.
			typedef polynomial<numerical_coefficient<long>,int> p_type1;
			typedef polynomial<numerical_coefficient<int>,short> p_type2;
			p_type1 p4(1);
			p_type2 p5(p4);
			BOOST_CHECK(p4 == p5);
			BOOST_CHECK(p5 == p4);
			p_type1 p6("x");
			p_type2 p7("x");
			p_type2 p8("y");
			BOOST_CHECK(p6 == p7);
			BOOST_CHECK(p7 == p6);
			BOOST_CHECK(p6 != p8);
			BOOST_CHECK(p8 != p6);
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(polynomial_constructors_test)
{
	boost::mpl::for_each<cf_types>(constructor_tester());
}

struct assignment_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef polynomial<Cf,Expo> p_type;
			p_type p1;
			p1 = 1;
			BOOST_CHECK(p1 == 1);
			p1 = integer(10);
			BOOST_CHECK(p1 == integer(10));
			p1 = "x";
			BOOST_CHECK(p1 == p_type("x"));
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(polynomial_assignment_test)
{
	boost::mpl::for_each<cf_types>(assignment_tester());
}
