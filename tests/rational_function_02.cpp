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

#include "../src/rational_function.hpp"

#define BOOST_TEST_MODULE rational_function_02_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <type_traits>

#include "../src/environment.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"

using namespace piranha;

using key_types = boost::mpl::vector<k_monomial,monomial<unsigned char>,monomial<integer>>;

struct hash_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using r_type = rational_function<Key>;
		using p_type = typename r_type::p_type;
		r_type x{"x"}, y{"y"};
		BOOST_CHECK_EQUAL(r_type{}.hash(),p_type{1}.hash());
		BOOST_CHECK_EQUAL((x/y).hash(),
			static_cast<std::size_t>((x + y - y).hash() + (y + x -x).hash()));
		BOOST_CHECK_EQUAL((x/y).hash(),(y/x).hash());
		BOOST_CHECK((std::is_same<std::size_t,decltype(x.hash())>::value));
	}
};

BOOST_AUTO_TEST_CASE(rational_function_ctor_test)
{
	environment env;
	boost::mpl::for_each<key_types>(hash_tester());
}

struct is_identical_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using r_type = rational_function<Key>;
		r_type x{"x"}, y{"y"}, z{"z"};
		BOOST_CHECK((std::is_same<bool,decltype(x.is_identical(y))>::value));
		BOOST_CHECK((!x.is_identical(y)));
		BOOST_CHECK((!y.is_identical(x)));
		BOOST_CHECK((x.is_identical(x)));
		BOOST_CHECK(((x / z).is_identical(x / z)));
		BOOST_CHECK((!(x + y - y).is_identical(x)));
		BOOST_CHECK((!(x / y).is_identical((x + z - z) / y)));
		BOOST_CHECK((!(x / y).is_identical(x / (y + z - z))));
		BOOST_CHECK((!(x / y).is_identical((x + z - z) / (y + z - z))));
	}
};

BOOST_AUTO_TEST_CASE(rational_function_is_identical_test)
{
	boost::mpl::for_each<key_types>(is_identical_tester());
}
