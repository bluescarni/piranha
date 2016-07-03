/* Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)

This file is part of the Piranha library.

The Piranha library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The Piranha library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the Piranha library.  If not,
see https://www.gnu.org/licenses/. */

#include "../src/symbol.hpp"

#define BOOST_TEST_MODULE symbol_test
#include <boost/test/unit_test.hpp>

#include <boost/lexical_cast.hpp>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <unordered_set>

#include "../src/init.hpp"
#include "../src/serialization.hpp"
#include "../src/type_traits.hpp"

static const int ntries = 1000;
static std::mt19937 rng;

using namespace piranha;

BOOST_AUTO_TEST_CASE(symbol_constructor_test)
{
	init();
	symbol x("x");
	BOOST_CHECK_EQUAL(x.get_name(),"x");
	symbol y("y");
	BOOST_CHECK_EQUAL(y.get_name(),"y");
	symbol y2("y");
	BOOST_CHECK_EQUAL(y2.get_name(),"y");
	BOOST_CHECK_EQUAL(std::addressof(y2.get_name()),std::addressof(y.get_name()));
	symbol x2("x");
	BOOST_CHECK_EQUAL(x2.get_name(),"x");
	BOOST_CHECK_EQUAL(std::addressof(x2.get_name()),std::addressof(x.get_name()));
	symbol x3("x");
	BOOST_CHECK_EQUAL(x3.get_name(),"x");
	BOOST_CHECK_EQUAL(std::addressof(x3.get_name()),std::addressof(x.get_name()));
	// Copy ctor.
	symbol x4(x3);
	BOOST_CHECK_EQUAL(x4.get_name(),"x");
	BOOST_CHECK_EQUAL(std::addressof(x4.get_name()),std::addressof(x.get_name()));
	// Move ctor - equivalent to copy.
	symbol x5(std::move(x4));
	BOOST_CHECK_EQUAL(x4.get_name(),"x");
	BOOST_CHECK_EQUAL(x5.get_name(),"x");
	BOOST_CHECK_EQUAL(std::addressof(x5.get_name()),std::addressof(x.get_name()));
	// Copy assignment.
	x5 = y;
	BOOST_CHECK_EQUAL(x5.get_name(),"y");
	BOOST_CHECK_EQUAL(std::addressof(x5.get_name()),std::addressof(y.get_name()));
	// Move assignment - equivalent to copy assignment.
	x5 = std::move(y);
	BOOST_CHECK_EQUAL(x5.get_name(),"y");
	BOOST_CHECK_EQUAL(y.get_name(),"y");
	BOOST_CHECK_EQUAL(std::addressof(x5.get_name()),std::addressof(y.get_name()));
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
	BOOST_CHECK_EQUAL(symbol("x").hash(),std::hash<std::string const *>()(std::addressof(symbol("x").get_name())));
	BOOST_CHECK(is_hashable<symbol>::value);
	BOOST_CHECK(is_hashable<symbol &>::value);
	BOOST_CHECK(is_hashable<symbol &&>::value);
}

BOOST_AUTO_TEST_CASE(symbol_streaming_test)
{
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(symbol("x")),"name = \'x\'");
}

BOOST_AUTO_TEST_CASE(symbol_serialization_test)
{
	std::uniform_int_distribution<int> int_dist(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
	symbol tmp("foo");
	for (int i = 0; i < ntries; ++i) {
		symbol s(boost::lexical_cast<std::string>(int_dist(rng)));
		std::stringstream ss;
		{
		boost::archive::text_oarchive oa(ss);
		oa << s;
		}
		{
		boost::archive::text_iarchive ia(ss);
		ia >> tmp;
		}
		BOOST_CHECK_EQUAL(tmp,s);
	}
}
