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

#include "../src/echelon_descriptor.hpp"

#define BOOST_TEST_MODULE echelon_descriptor_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>

#include "../src/base_term.hpp"
#include "../src/monomial.hpp"
#include "../src/numerical_coefficient.hpp"
#include "../src/symbol.hpp"

// TODO: test with larger echelon sizes.

using namespace piranha;

class term_type1: public base_term<numerical_coefficient<double>,monomial<int>,term_type1> {};

typedef boost::mpl::vector<term_type1> term_types;

struct constructor_tester
{
	template <typename T>
	void operator()(const T &) const
	{
		// Default constructor.
		typedef echelon_descriptor<T> ed_type;
		// Copy constructor.
		ed_type a, b = a;
		// Move constructor.
		ed_type ed1;
		ed_type ed2(std::move(ed1));
		// Copy assignment.
		ed1 = b;
		// Move assignment.
		ed2 = std::move(a);
	}
};

BOOST_AUTO_TEST_CASE(echelon_descriptor_constructor_test)
{
	boost::mpl::for_each<term_types>(constructor_tester());
}

struct getters_tester
{
	template <typename T>
	void operator()(const T &) const
	{
		// Default constructor.
		typedef echelon_descriptor<T> ed_type;
		ed_type a;
		a.get_args_tuple();
	}
};

BOOST_AUTO_TEST_CASE(echelon_descriptor_getters_test)
{
	boost::mpl::for_each<term_types>(getters_tester());
}
