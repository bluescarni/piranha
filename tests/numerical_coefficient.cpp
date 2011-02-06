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

#include "../src/numerical_coefficient.hpp"

#define BOOST_TEST_MODULE numerical_coefficient_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <type_traits>

#include "../src/integer.hpp"
#include "../src/mf_int.hpp"

using namespace piranha;

typedef boost::mpl::vector<double,mf_int,integer> types;

struct constructor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef numerical_coefficient<T> nc;
		BOOST_CHECK((std::has_trivial_destructor<T>::value && std::has_trivial_destructor<nc>::value) || (!std::has_trivial_destructor<T>::value && !std::has_trivial_destructor<nc>::value));
		BOOST_CHECK((std::has_trivial_copy_constructor<T>::value && std::has_trivial_copy_constructor<nc>::value) || (!std::has_trivial_copy_constructor<T>::value && !std::has_trivial_copy_constructor<nc>::value));
		// Default constructor.
		BOOST_CHECK_EQUAL(nc().get_value(),T());
		// Copy construction from value.
		T value(3);
		BOOST_CHECK_EQUAL(nc(value).get_value(),T(3));
		// Move construction from value.
		BOOST_CHECK_EQUAL(nc(T(3)).get_value(),T(3));
		// Copy construction.
		nc other(value);
		BOOST_CHECK_EQUAL(nc(other).get_value(),T(3));
		// Move construction.
		BOOST_CHECK_EQUAL(nc(nc(value)).get_value(),T(3));
		// Check copy/move construction from coefficients of different types.
		typedef numerical_coefficient<int> nc_other;
		nc_other nco{3};
		BOOST_CHECK_EQUAL(nc(nco).get_value(),T(3));
		BOOST_CHECK_EQUAL(nc(nc_other(3)).get_value(),T(3));
		// Assignment from int.
		nc n;
		n = 10;
		BOOST_CHECK_EQUAL(n.get_value(),T(10));
		n = int(10);
		BOOST_CHECK_EQUAL(n.get_value(),T(10));
		// Assignment from numerical_coefficient of same type.
		n = other;
		BOOST_CHECK_EQUAL(n.get_value(),T(3));
		n = nc(value);
		BOOST_CHECK_EQUAL(n.get_value(),T(3));
		// Assignment from numerical_coefficient of different type.
		n = nco;
		BOOST_CHECK_EQUAL(n.get_value(),T(3));
		n = nc_other(3);
		BOOST_CHECK_EQUAL(n.get_value(),T(3));
	}
};

BOOST_AUTO_TEST_CASE(numerical_coefficient_constructor_test)
{
	boost::mpl::for_each<types>(constructor_tester());
}
