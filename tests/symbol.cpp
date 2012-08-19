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

#include "../src/symbol.hpp"

#define BOOST_TEST_MODULE symbol_test
#include <boost/test/unit_test.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/utility.hpp>
#include <string>
#include <unordered_set>

#include "../src/environment.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(symbol_constructor_test)
{
	environment env;
	symbol x("x");
	BOOST_CHECK_EQUAL(x.get_name(),"x");
	symbol y("y");
	BOOST_CHECK_EQUAL(y.get_name(),"y");
	symbol y2("y");
	BOOST_CHECK_EQUAL(y2.get_name(),"y");
	BOOST_CHECK_EQUAL(boost::addressof(y2.get_name()),boost::addressof(y.get_name()));
	symbol x2("x");
	BOOST_CHECK_EQUAL(x2.get_name(),"x");
	BOOST_CHECK_EQUAL(boost::addressof(x2.get_name()),boost::addressof(x.get_name()));
	symbol x3("x");
	BOOST_CHECK_EQUAL(x3.get_name(),"x");
	BOOST_CHECK_EQUAL(boost::addressof(x3.get_name()),boost::addressof(x.get_name()));
	// Copy ctor.
	symbol x4(x3);
	BOOST_CHECK_EQUAL(x4.get_name(),"x");
	BOOST_CHECK_EQUAL(boost::addressof(x4.get_name()),boost::addressof(x.get_name()));
	// Move ctor - equivalent to copy.
	symbol x5(std::move(x4));
	BOOST_CHECK_EQUAL(x4.get_name(),"x");
	BOOST_CHECK_EQUAL(x5.get_name(),"x");
	BOOST_CHECK_EQUAL(boost::addressof(x5.get_name()),boost::addressof(x.get_name()));
	// Copy assignment.
	x5 = y;
	BOOST_CHECK_EQUAL(x5.get_name(),"y");
	BOOST_CHECK_EQUAL(boost::addressof(x5.get_name()),boost::addressof(y.get_name()));
	// Move assignment - equivalent to copy assignment.
	x5 = std::move(y);
	BOOST_CHECK_EQUAL(x5.get_name(),"y");
	BOOST_CHECK_EQUAL(y.get_name(),"y");
	BOOST_CHECK_EQUAL(boost::addressof(x5.get_name()),boost::addressof(y.get_name()));
}

BOOST_AUTO_TEST_CASE(symbol_operators_test)
{
	BOOST_CHECK_EQUAL(symbol("x"),symbol("x"));
	BOOST_CHECK_EQUAL(symbol("x").get_name(),symbol("x").get_name());
	BOOST_CHECK(symbol("y") != symbol("x"));
	BOOST_CHECK(symbol("y").get_name() != symbol("x").get_name());
	// Lexicographic comparison.
	BOOST_CHECK(symbol("a") < symbol("b"));
	BOOST_CHECK(!(symbol("a") < symbol("a")));
	BOOST_CHECK(symbol("abc") < symbol("abd"));
}

BOOST_AUTO_TEST_CASE(symbol_hash_test)
{
	BOOST_CHECK_NO_THROW(symbol("x").hash());
	BOOST_CHECK_EQUAL(symbol("x").hash(),std::hash<symbol>()(symbol("x")));
	BOOST_CHECK_EQUAL(symbol("x").hash(),std::hash<std::string const *>()(boost::addressof(symbol("x").get_name())));
}

BOOST_AUTO_TEST_CASE(symbol_streaming_test)
{
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(symbol("x")),"name = \'x\'");
}
