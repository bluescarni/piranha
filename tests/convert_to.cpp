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

#include "../src/convert_to.hpp"

#define BOOST_TEST_MODULE convert_to_test
#include <boost/test/unit_test.hpp>

#include <string>
#include <type_traits>
#include <utility>

#include "../src/environment.hpp"

using namespace piranha;

static void test_func() {}
static auto l1 = [](){};

BOOST_AUTO_TEST_CASE(convert_to_main_test)
{
	environment env;
	BOOST_CHECK_EQUAL(convert_to<int>(3.5),3);
	BOOST_CHECK((std::is_same<int,decltype(convert_to<int>(3.5))>::value));
	BOOST_CHECK_EQUAL(convert_to<std::string>("asdasd"),"asdasd");
	BOOST_CHECK_NO_THROW(convert_to<std::function<void()>>(test_func));
	BOOST_CHECK_NO_THROW(convert_to<std::function<void()>>(l1));
	BOOST_CHECK_NO_THROW(convert_to<void(*)()>(l1));
	BOOST_CHECK((!has_convert_to<int,std::string>::value));
	BOOST_CHECK((has_convert_to<std::string,const char *>::value));
	BOOST_CHECK((has_convert_to<std::string,char *>::value));
	BOOST_CHECK((has_convert_to<double,long double>::value));
	BOOST_CHECK((has_convert_to<long double,double>::value));
	BOOST_CHECK((has_convert_to<long double,int>::value));
	BOOST_CHECK((has_convert_to<std::function<void()>,void(*)()>::value));
	BOOST_CHECK((has_convert_to<std::function<void()>,decltype(l1)>::value));
	BOOST_CHECK((has_convert_to<void(*)(),decltype(l1)>::value));
}
