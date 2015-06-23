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

#include "../src/dynamic_kronecker_monomial.hpp"

#define BOOST_TEST_MODULE dynamic_kronecker_monomial_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>

#include "../src/environment.hpp"
#include "../src/symbol_set.hpp"

#include <iostream>

using namespace piranha;

typedef boost::mpl::vector<signed char,int,long,long long> int_types;

// Constructors, assignments, getters, setters, etc.
struct constructor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using k_type = dynamic_kronecker_monomial<T>;
		std::cout << sizeof(k_type) << '\n';
		k_type k{1,2,3};
		std::cout << k << '\n';
		symbol_set ss;
		ss.add("x");
		ss.add("y");
		ss.add("z");
		std::cout << k.unpack(ss) << '\n';
	}
};

BOOST_AUTO_TEST_CASE(dynamic_kronecker_monomial_constructor_test)
{
	environment env;
	boost::mpl::for_each<int_types>(constructor_tester());
}
