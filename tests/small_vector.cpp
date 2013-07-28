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

#include "../src/small_vector.hpp"

#define BOOST_TEST_MODULE small_vector_test
#include <boost/test/unit_test.hpp>

#include "../src/environment.hpp"

using namespace piranha;

// Constructors, assignments and element access.
struct constructor_tester
{

};

BOOST_AUTO_TEST_CASE(small_vector_constructor_test)
{
	environment env;
	std::cout << sizeof(small_vector<signed char>) << '\n';
	std::cout << detail::auto_static_size<signed char,std::allocator<short>>::value << '\n';
	//boost::mpl::for_each<value_types>(constructor_tester());
}
