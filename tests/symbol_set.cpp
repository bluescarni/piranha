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

#include "../src/symbol_set.hpp"

#define BOOST_TEST_MODULE symbol_set_test
#include <boost/test/unit_test.hpp>

#include <stdexcept>

#include "../src/environment.hpp"
#include "../src/symbol.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(symbol_set_constructor_test)
{
	environment env;
	symbol_set ss;
	BOOST_CHECK(ss.size() == 0u);
	BOOST_CHECK(ss.begin() == ss.end());
	ss.add(symbol("a"));
	BOOST_CHECK(ss.size() == 1u);
	auto ss2(ss);
	BOOST_CHECK(ss2.size() == 1u);
	auto ss3(std::move(ss2));
	BOOST_CHECK(ss3.size() == 1u);
	BOOST_CHECK(ss2.size() == 0u);
	ss2 = ss3;
	BOOST_CHECK(ss2.size() == 1u);
	ss3 = std::move(ss2);
	BOOST_CHECK(ss2.size() == 0u);
	BOOST_CHECK(ss3[0u] == symbol("a"));
	symbol_set ss4({symbol("a"),symbol("c"),symbol("b")});
	BOOST_CHECK(ss4 == symbol_set({symbol("a"),symbol("b"),symbol("c")}));
	BOOST_CHECK(ss4 == symbol_set({symbol("c"),symbol("b"),symbol("a")}));
	// Self assignment.
	ss4 = ss4;
	BOOST_CHECK(ss4 == symbol_set({symbol("c"),symbol("b"),symbol("a")}));
	ss4 = std::move(ss4);
	BOOST_CHECK(ss4 == symbol_set({symbol("c"),symbol("b"),symbol("a")}));
}

BOOST_AUTO_TEST_CASE(symbol_set_add_test)
{
	symbol_set ss;
	ss.add(symbol("b"));
	BOOST_CHECK(ss.size() == 1u);
	BOOST_CHECK(ss[0u] == symbol("b"));
	ss.add(symbol("a"));
	BOOST_CHECK(ss[0u] == symbol("a"));
	BOOST_CHECK(ss[1u] == symbol("b"));
	ss.add("c");
	BOOST_CHECK(ss[0u] == symbol("a"));
	BOOST_CHECK(ss[1u] == symbol("b"));
	BOOST_CHECK(ss[2u] == symbol("c"));
	BOOST_CHECK_THROW(ss.add(symbol("b")),std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(symbol_set_equality_test)
{
	symbol_set ss1, ss2;
	BOOST_CHECK(ss1 == ss2);
	BOOST_CHECK(!(ss1 != ss2));
	ss1.add(symbol("c"));
	BOOST_CHECK(ss1 != ss2);
	BOOST_CHECK(!(ss1 == ss2));
	ss2.add(symbol("c"));
	BOOST_CHECK(ss1 == ss2);
	BOOST_CHECK(!(ss1 != ss2));
	ss1.add(symbol("a"));
	ss2.add(symbol("b"));
	BOOST_CHECK(ss1 != ss2);
	BOOST_CHECK(!(ss1 == ss2));
}

BOOST_AUTO_TEST_CASE(symbol_set_merge_test)
{
	symbol_set ss1;
	ss1.add(symbol("c"));
	ss1.add(symbol("b"));
	ss1.add(symbol("d"));
	symbol_set ss2;
	ss2.add(symbol("a"));
	ss2.add(symbol("b"));
	ss2.add(symbol("e"));
	ss2.add(symbol("f"));
	ss2.add(symbol("s"));
	auto ss3a = ss1.merge(ss2);
	auto ss3b = ss2.merge(ss1);
	BOOST_CHECK(ss3a == ss3b);
	symbol_set un;
	un.add(symbol("c"));
	un.add(symbol("b"));
	un.add(symbol("d"));
	un.add(symbol("a"));
	un.add(symbol("e"));
	un.add(symbol("f"));
	un.add(symbol("s"));
	BOOST_CHECK(un == ss3a);
}

BOOST_AUTO_TEST_CASE(symbol_set_remove_test)
{
	symbol_set ss;
	ss.add(symbol("c"));
	ss.add(symbol("b"));
	ss.add(symbol("d"));
	BOOST_CHECK_THROW(ss.remove(symbol("a")),std::invalid_argument);
	BOOST_CHECK_EQUAL(ss.size(),3u);
	BOOST_CHECK_THROW(ss.remove("a"),std::invalid_argument);
	BOOST_CHECK_EQUAL(ss.size(),3u);
	BOOST_CHECK_NO_THROW(ss.remove(symbol("b")));
	BOOST_CHECK_EQUAL(ss.size(),2u);
	BOOST_CHECK_NO_THROW(ss.remove("c"));
	BOOST_CHECK_EQUAL(ss.size(),1u);
	BOOST_CHECK_NO_THROW(ss.remove("d"));
	BOOST_CHECK_EQUAL(ss.size(),0u);
	BOOST_CHECK_THROW(ss.remove("a"),std::invalid_argument);
	BOOST_CHECK_EQUAL(ss.size(),0u);
}
