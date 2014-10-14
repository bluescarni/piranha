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

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <limits>
#include <list>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "../src/environment.hpp"
#include "../src/symbol.hpp"

static std::mt19937 rng;
static const int ntries = 1000;

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
	BOOST_CHECK(std::is_nothrow_move_constructible<symbol_set>::value);
	// Constructor from iterators.
	std::vector<std::string> vs1 = {"a","b","c"};
	symbol_set ss5(vs1.begin(),vs1.end());
	BOOST_CHECK_EQUAL(ss5.size(),3u);
	auto cmp = [](const symbol &s, const std::string &str) {
		return s.get_name() == str;
	};
	BOOST_CHECK(std::equal(ss5.begin(),ss5.end(),vs1.begin(),cmp));
	std::vector<std::string> vs2 = {"b","c","a","a","b","c"};
	BOOST_CHECK((std::is_constructible<symbol_set,decltype(vs2.begin()),decltype(vs2.end())>::value));
	symbol_set ss6(vs2.begin(),vs2.end());
	BOOST_CHECK_EQUAL(ss6.size(),3u);
	BOOST_CHECK(std::equal(ss6.begin(),ss6.end(),vs1.begin(),cmp));
	std::vector<symbol> vs3({symbol("b"),symbol("c"),symbol("a"),symbol("b"),symbol("a")});
	symbol_set ss7(vs3.begin(),vs3.end());
	BOOST_CHECK_EQUAL(ss7.size(),3u);
	BOOST_CHECK(std::equal(ss7.begin(),ss7.end(),vs1.begin(),cmp));
	std::list<std::string> ls1 = {"a","b","c"};
	BOOST_CHECK((std::is_constructible<symbol_set,decltype(ls1.begin()),decltype(ls1.end())>::value));
	symbol_set ss8(ls1.begin(),ls1.end());
	BOOST_CHECK_EQUAL(ss8.size(),3u);
	BOOST_CHECK(std::equal(ss8.begin(),ss8.end(),ls1.begin(),cmp));
	std::vector<int> vint;
	BOOST_CHECK((!std::is_constructible<symbol_set,decltype(vint.begin()),decltype(vint.end())>::value));
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

BOOST_AUTO_TEST_CASE(symbol_set_diff_test)
{
	symbol_set ss1;
	ss1.add(symbol("b"));
	ss1.add(symbol("d"));
	symbol_set ss2;
	ss2.add(symbol("a"));
	ss2.add(symbol("b"));
	ss2.add(symbol("c"));
	ss2.add(symbol("d"));
	ss2.add(symbol("e"));
	auto ss3 = ss2.diff(ss1);
	BOOST_CHECK(ss3 == symbol_set({symbol("a"),symbol("c"),symbol("e")}));
	BOOST_CHECK(ss2.diff(ss2) == symbol_set{});
}

BOOST_AUTO_TEST_CASE(symbol_set_positions_test)
{
	using positions = symbol_set::positions;
	// Checker for the consistency of the positions class.
	auto checker = [](const symbol_set &a, const symbol_set &b, const positions &p) {
		// The position vector cannot have a size larger than b or a.
		BOOST_CHECK(p.size() <= b.size());
		BOOST_CHECK(p.size() <= a.size());
		for (const auto &i: p) {
			// Any index in positions must be an index valid for a.
			BOOST_CHECK(i < a.size());
			// The element of a at index i must be in b somewhere.
			BOOST_CHECK(std::find(b.begin(),b.end(),a[i]) != b.end());
		}
		// Number of elements of b that are in a.
		const auto nb = std::count_if(b.begin(),b.end(),[&a](const symbol &s) {
			return std::find(a.begin(),a.end(),s) != a.end();
		});
		BOOST_CHECK_EQUAL(unsigned(nb),p.size());
	};
	// Some simple cases.
	{
	symbol_set a({symbol("b"),symbol("c"),symbol("d"),symbol("e")});
	symbol_set b({symbol("a"),symbol("b"),symbol("f"),symbol("d")});
	positions p(a,b);
	checker(a,b,p);
	BOOST_CHECK_EQUAL(p.size(),2u);
	BOOST_CHECK_EQUAL(*p.begin(),0u);
	BOOST_CHECK_EQUAL(p.begin()[1],2u);
	BOOST_CHECK_EQUAL(p.back(),2u);
	}
	{
	symbol_set a({symbol("a"),symbol("b"),symbol("c")});
	symbol_set b({symbol("d"),symbol("e"),symbol("f")});
	positions p(a,b);
	checker(a,b,p);
	BOOST_CHECK_EQUAL(p.size(),0u);
	}
	{
	symbol_set b({symbol("a"),symbol("b"),symbol("c")});
	symbol_set a({symbol("d"),symbol("e"),symbol("f")});
	positions p(a,b);
	checker(a,b,p);
	BOOST_CHECK_EQUAL(p.size(),0u);
	}
	{
	symbol_set a({symbol("a"),symbol("b"),symbol("c")});
	symbol_set b({symbol("c"),symbol("e"),symbol("f")});
	positions p(a,b);
	checker(a,b,p);
	BOOST_CHECK_EQUAL(p.size(),1u);
	BOOST_CHECK_EQUAL(p.begin()[0],2u);
	BOOST_CHECK_EQUAL(p.back(),2u);
	}
	{
	// Interleaved with no overlapping.
	symbol_set a({symbol("b"),symbol("f"),symbol("q")});
	symbol_set b({symbol("a"),symbol("e"),symbol("g"),symbol("r")});
	positions p(a,b);
	checker(a,b,p);
	BOOST_CHECK_EQUAL(p.size(),0u);
	}
	{
	// Interleaved with some overlapping.
	symbol_set a({symbol("b"),symbol("f"),symbol("q")});
	symbol_set b({symbol("a"),symbol("b"),symbol("f"),symbol("g"),symbol("q"),symbol("r")});
	positions p(a,b);
	checker(a,b,p);
	BOOST_CHECK_EQUAL(p.size(),3u);
	BOOST_CHECK_EQUAL(p.begin()[0],0u);
	BOOST_CHECK_EQUAL(p.begin()[1],1u);
	BOOST_CHECK_EQUAL(p.begin()[2],2u);
	BOOST_CHECK_EQUAL(p.back(),2u);
	}
	// Random testing.
	std::uniform_int_distribution<int> dist(std::numeric_limits<int>::min(),std::numeric_limits<int>::max() - 1);
	for (int i = 0; i < ntries; ++i) {
		{
		// First check with completely different symbols.
		symbol_set a, b;
		for (int j = 0; j < 6; ++j) {
			try {
				int tmp = dist(rng);
				a.add(boost::lexical_cast<std::string>(tmp));
				b.add(boost::lexical_cast<std::string>(tmp + 1));
			} catch (...) {
				// Just ignore any existing symbol (unlikely).
			}
		}
		positions p(a,b);
		checker(a,b,p);
		}
		{
		// All equal
		symbol_set a, b;
		for (int j = 0; j < 6; ++j) {
			try {
				int tmp = dist(rng);
				a.add(boost::lexical_cast<std::string>(tmp));
				b.add(boost::lexical_cast<std::string>(tmp));
			} catch (...) {}
		}
		positions p(a,b);
		checker(a,b,p);
		}
		{
		// a larger than b, some elements shared.
		symbol_set a, b;
		for (int j = 0; j < 6; ++j) {
			try {
				int tmp = dist(rng);
				a.add(boost::lexical_cast<std::string>(tmp));
				b.add(boost::lexical_cast<std::string>(tmp));
			} catch (...) {}
		}
		for (int j = 0; j < 6; ++j) {
			try {
				int tmp = dist(rng);
				a.add(boost::lexical_cast<std::string>(tmp));
			} catch (...) {}
		}
		positions p(a,b);
		checker(a,b,p);
		}
		{
		// b larger than a, some elements shared.
		symbol_set a, b;
		for (int j = 0; j < 6; ++j) {
			try {
				int tmp = dist(rng);
				a.add(boost::lexical_cast<std::string>(tmp));
				b.add(boost::lexical_cast<std::string>(tmp));
			} catch (...) {}
		}
		for (int j = 0; j < 6; ++j) {
			try {
				int tmp = dist(rng);
				b.add(boost::lexical_cast<std::string>(tmp));
			} catch (...) {}
		}
		positions p(a,b);
		checker(a,b,p);
		}
	}
}
