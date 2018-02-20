/* Copyright 2009-2017 Francesco Biscani (bluescarni@gmail.com)

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

#include <piranha/math/ldegree.hpp>

#define BOOST_TEST_MODULE ldegree_test
#include <boost/test/included/unit_test.hpp>

#include <piranha/symbol_utils.hpp>

using namespace piranha;

class foo
{
};

class bar
{
};

namespace piranha
{

template <>
class ldegree_impl<foo>
{
public:
    int operator()(const foo &) const
    {
        return 0;
    }
    int operator()(const foo &, const symbol_fset &) const
    {
        return 1;
    }
};

// bar is missing the partial degree overload.
template <>
class ldegree_impl<bar>
{
public:
    int operator()(const bar &) const
    {
        return 0;
    }
};
}

BOOST_AUTO_TEST_CASE(ldegree_test_00)
{
    BOOST_CHECK(!is_ldegree_type<void>::value);
    BOOST_CHECK(!is_ldegree_type<int>::value);
    BOOST_CHECK(!is_ldegree_type<const int>::value);
    BOOST_CHECK(!is_ldegree_type<const int &&>::value);
    BOOST_CHECK(!is_ldegree_type<int &&>::value);
    BOOST_CHECK(is_ldegree_type<foo>::value);
    BOOST_CHECK(is_ldegree_type<foo &>::value);
    BOOST_CHECK(is_ldegree_type<const foo>::value);
    BOOST_CHECK(is_ldegree_type<const foo &>::value);
    BOOST_CHECK(is_ldegree_type<foo &&>::value);
    BOOST_CHECK_EQUAL(piranha::ldegree(foo{}), 0);
    BOOST_CHECK_EQUAL(piranha::ldegree(foo{}, symbol_fset{}), 1);
    BOOST_CHECK(!is_ldegree_type<bar>::value);
    BOOST_CHECK(!is_ldegree_type<bar &>::value);
    BOOST_CHECK(!is_ldegree_type<const bar>::value);
    BOOST_CHECK(!is_ldegree_type<const bar &>::value);
    BOOST_CHECK(!is_ldegree_type<bar &&>::value);
}
