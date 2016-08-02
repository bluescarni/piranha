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

#include "../src/monomial.hpp"

#define BOOST_TEST_MODULE monomial_02_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <sstream>
#include <type_traits>

#include "../src/config.hpp"
#include "../src/init.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/serialization.hpp"

using namespace piranha;

typedef boost::mpl::vector<signed char, int/*, integer, rational*/> expo_types;
typedef boost::mpl::vector<std::integral_constant<std::size_t, 0u>, std::integral_constant<std::size_t, 1u>,
                           std::integral_constant<std::size_t, 5u>, std::integral_constant<std::size_t, 10u>>
    size_types;

BOOST_AUTO_TEST_CASE(monomial_init_test)
{
    // Make sure there's always at least 1 test case to run.
    init();
}

#if defined(PIRANHA_ENABLE_MSGPACK)

// Constructors, assignments and element access.
struct msgpack_tester {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &)
        {
            using monomial_type = monomial<T, U>;
            BOOST_CHECK((key_has_msgpack_pack<msgpack::sbuffer,monomial_type>::value));
            BOOST_CHECK((key_has_msgpack_pack<std::ostringstream,monomial_type>::value));
            BOOST_CHECK((!key_has_msgpack_pack<msgpack::sbuffer &,monomial_type>::value));
            BOOST_CHECK((!key_has_msgpack_pack<const std::ostringstream,monomial_type>::value));
            BOOST_CHECK((!key_has_msgpack_pack<const std::ostringstream &&,monomial_type>::value));
            BOOST_CHECK((!key_has_msgpack_pack<std::ostringstream &&,monomial_type>::value));
        }
    };
    template <typename T>
    void operator()(const T &)
    {
        boost::mpl::for_each<size_types>(runner<T>());
    }
};

BOOST_AUTO_TEST_CASE(monomial_msgpack_test)
{
    boost::mpl::for_each<expo_types>(msgpack_tester());
}

#endif
