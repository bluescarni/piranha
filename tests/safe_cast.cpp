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

#include "../src/safe_cast.hpp"

#define BOOST_TEST_MODULE safe_cast_test
#include <boost/test/unit_test.hpp>

#include <limits>
#include <stdexcept>
#include <string>

#include "../src/environment.hpp"
#include "../src/mp_integer.hpp"

using namespace piranha;

struct foo {};

struct foo_nc
{
	foo_nc(const foo_nc &) = delete;
};

BOOST_AUTO_TEST_CASE(safe_cast_main_test)
{
	environment env;
	BOOST_CHECK((has_safe_cast<int,int>::value));
	BOOST_CHECK((has_safe_cast<int,char>::value));
	BOOST_CHECK((has_safe_cast<char,unsigned long long>::value));
	BOOST_CHECK((!has_safe_cast<int,std::string>::value));
	BOOST_CHECK((has_safe_cast<std::string,std::string>::value));
	BOOST_CHECK((has_safe_cast<foo,foo>::value));
	BOOST_CHECK((!has_safe_cast<foo_nc,foo_nc>::value));
	BOOST_CHECK((!has_safe_cast<double,int>::value));
	BOOST_CHECK((has_safe_cast<int,double>::value));
	BOOST_CHECK_EQUAL(safe_cast<int>(2.),2);
	BOOST_CHECK_EQUAL(safe_cast<int>(-2.),-2);
	BOOST_CHECK_THROW(safe_cast<int>(1. / std::numeric_limits<double>::radix),std::invalid_argument);
}

