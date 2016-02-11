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

#include "../src/detail/safe_integral_adder.hpp"

#define BOOST_TEST_MODULE safe_integral_adder_test
#include <boost/test/unit_test.hpp>

#include <limits>
#include <stdexcept>

#include "../src/environment.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(sia_test_00)
{
	environment env;
	{
	// Short.
	using int_type = short;
	int_type a;
	a = 0;
	detail::safe_integral_adder(a,int_type(1));
	BOOST_CHECK_EQUAL(a,1);
	detail::safe_integral_adder(a,int_type(-1));
	BOOST_CHECK_EQUAL(a,0);
	detail::safe_integral_adder(a,std::numeric_limits<int_type>::max());
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::max());
	a = 0;
	detail::safe_integral_adder(a,std::numeric_limits<int_type>::min());
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::min());
	a = 1;
	BOOST_CHECK_THROW(detail::safe_integral_adder(a,std::numeric_limits<int_type>::max()),std::overflow_error);
	a = -1;
	BOOST_CHECK_THROW(detail::safe_integral_adder(a,std::numeric_limits<int_type>::min()),std::overflow_error);
	a = std::numeric_limits<int_type>::max();
	BOOST_CHECK_THROW(detail::safe_integral_adder(a,std::numeric_limits<int_type>::max()),std::overflow_error);
	a = std::numeric_limits<int_type>::min();
	BOOST_CHECK_THROW(detail::safe_integral_adder(a,std::numeric_limits<int_type>::min()),std::overflow_error);
	}
	{
	// Int.
	using int_type = int;
	int_type a;
	a = 0;
	detail::safe_integral_adder(a,int_type(1));
	BOOST_CHECK_EQUAL(a,1);
	detail::safe_integral_adder(a,int_type(-1));
	BOOST_CHECK_EQUAL(a,0);
	detail::safe_integral_adder(a,std::numeric_limits<int_type>::max());
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::max());
	a = 0;
	detail::safe_integral_adder(a,std::numeric_limits<int_type>::min());
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::min());
	a = 1;
	BOOST_CHECK_THROW(detail::safe_integral_adder(a,std::numeric_limits<int_type>::max()),std::overflow_error);
	a = -1;
	BOOST_CHECK_THROW(detail::safe_integral_adder(a,std::numeric_limits<int_type>::min()),std::overflow_error);
	a = std::numeric_limits<int_type>::max();
	BOOST_CHECK_THROW(detail::safe_integral_adder(a,std::numeric_limits<int_type>::max()),std::overflow_error);
	a = std::numeric_limits<int_type>::min();
	BOOST_CHECK_THROW(detail::safe_integral_adder(a,std::numeric_limits<int_type>::min()),std::overflow_error);
	}
	{
	// Long long.
	using int_type = long long;
	int_type a;
	a = 0;
	detail::safe_integral_adder(a,int_type(1));
	BOOST_CHECK_EQUAL(a,1);
	detail::safe_integral_adder(a,int_type(-1));
	BOOST_CHECK_EQUAL(a,0);
	detail::safe_integral_adder(a,std::numeric_limits<int_type>::max());
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::max());
	a = 0;
	detail::safe_integral_adder(a,std::numeric_limits<int_type>::min());
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::min());
	a = 1;
	BOOST_CHECK_THROW(detail::safe_integral_adder(a,std::numeric_limits<int_type>::max()),std::overflow_error);
	a = -1;
	BOOST_CHECK_THROW(detail::safe_integral_adder(a,std::numeric_limits<int_type>::min()),std::overflow_error);
	a = std::numeric_limits<int_type>::max();
	BOOST_CHECK_THROW(detail::safe_integral_adder(a,std::numeric_limits<int_type>::max()),std::overflow_error);
	a = std::numeric_limits<int_type>::min();
	BOOST_CHECK_THROW(detail::safe_integral_adder(a,std::numeric_limits<int_type>::min()),std::overflow_error);
	}
	{
	// Unsigned char.
	using int_type = unsigned char;
	int_type a;
	a = 0;
	detail::safe_integral_adder(a,int_type(1));
	BOOST_CHECK_EQUAL(a,1);
	a = 0;
	detail::safe_integral_adder(a,std::numeric_limits<int_type>::max());
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::max());
	a = 1;
	BOOST_CHECK_THROW(detail::safe_integral_adder(a,std::numeric_limits<int_type>::max()),std::overflow_error);
	a = std::numeric_limits<int_type>::max();
	BOOST_CHECK_THROW(detail::safe_integral_adder(a,std::numeric_limits<int_type>::max()),std::overflow_error);
	a = static_cast<int_type>(std::numeric_limits<int_type>::max() - 1u);
	detail::safe_integral_adder(a,int_type(1));
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::max());
	a = 1u;
	detail::safe_integral_adder(a,int_type(std::numeric_limits<int_type>::max() - int_type(1)));
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::max());
	}
	{
	// Unsigned.
	using int_type = unsigned;
	int_type a;
	a = 0;
	detail::safe_integral_adder(a,int_type(1));
	BOOST_CHECK_EQUAL(a,1);
	a = 0;
	detail::safe_integral_adder(a,std::numeric_limits<int_type>::max());
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::max());
	a = 1;
	BOOST_CHECK_THROW(detail::safe_integral_adder(a,std::numeric_limits<int_type>::max()),std::overflow_error);
	a = std::numeric_limits<int_type>::max();
	BOOST_CHECK_THROW(detail::safe_integral_adder(a,std::numeric_limits<int_type>::max()),std::overflow_error);
	a = std::numeric_limits<int_type>::max() - 1u;
	detail::safe_integral_adder(a,int_type(1));
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::max());
	a = 1u;
	detail::safe_integral_adder(a,int_type(std::numeric_limits<int_type>::max() - int_type(1)));
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::max());
	}
	{
	// Unsigned long.
	using int_type = unsigned long;
	int_type a;
	a = 0;
	detail::safe_integral_adder(a,int_type(1));
	BOOST_CHECK_EQUAL(a,1);
	a = 0;
	detail::safe_integral_adder(a,std::numeric_limits<int_type>::max());
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::max());
	a = 1;
	BOOST_CHECK_THROW(detail::safe_integral_adder(a,std::numeric_limits<int_type>::max()),std::overflow_error);
	a = std::numeric_limits<int_type>::max();
	BOOST_CHECK_THROW(detail::safe_integral_adder(a,std::numeric_limits<int_type>::max()),std::overflow_error);
	a = std::numeric_limits<int_type>::max() - 1u;
	detail::safe_integral_adder(a,int_type(1));
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::max());
	a = 1u;
	detail::safe_integral_adder(a,int_type(std::numeric_limits<int_type>::max() - int_type(1)));
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::max());
	}
}

BOOST_AUTO_TEST_CASE(sia_test_01)
{
	{
	// Short.
	using int_type = short;
	int_type a;
	a = 0;
	detail::safe_integral_subber(a,int_type(1));
	BOOST_CHECK_EQUAL(a,-1);
	detail::safe_integral_subber(a,int_type(-1));
	BOOST_CHECK_EQUAL(a,0);
	a = std::numeric_limits<int_type>::max();
	BOOST_CHECK_THROW(detail::safe_integral_subber(a,int_type(-1)),std::overflow_error);
	detail::safe_integral_subber(a,int_type(1));
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::max() - 1);
	a = std::numeric_limits<int_type>::min();
	BOOST_CHECK_THROW(detail::safe_integral_subber(a,int_type(1)),std::overflow_error);
	detail::safe_integral_subber(a,int_type(-1));
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::min() + 1);
	a = std::numeric_limits<int_type>::max();
	detail::safe_integral_subber(a,int_type(std::numeric_limits<int_type>::max() / 2));
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::max() - std::numeric_limits<int_type>::max() / 2);
	a = std::numeric_limits<int_type>::min();
	detail::safe_integral_subber(a,int_type(std::numeric_limits<int_type>::min() / 2));
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::min() - std::numeric_limits<int_type>::min() / 2);
	}
	{
	// Int.
	using int_type = int;
	int_type a;
	a = 0;
	detail::safe_integral_subber(a,int_type(1));
	BOOST_CHECK_EQUAL(a,-1);
	detail::safe_integral_subber(a,int_type(-1));
	BOOST_CHECK_EQUAL(a,0);
	a = std::numeric_limits<int_type>::max();
	BOOST_CHECK_THROW(detail::safe_integral_subber(a,int_type(-1)),std::overflow_error);
	detail::safe_integral_subber(a,int_type(1));
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::max() - 1);
	a = std::numeric_limits<int_type>::min();
	BOOST_CHECK_THROW(detail::safe_integral_subber(a,int_type(1)),std::overflow_error);
	detail::safe_integral_subber(a,int_type(-1));
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::min() + 1);
	a = std::numeric_limits<int_type>::max();
	detail::safe_integral_subber(a,int_type(std::numeric_limits<int_type>::max() / 2));
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::max() - std::numeric_limits<int_type>::max() / 2);
	a = std::numeric_limits<int_type>::min();
	detail::safe_integral_subber(a,int_type(std::numeric_limits<int_type>::min() / 2));
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::min() - std::numeric_limits<int_type>::min() / 2);
	}
	{
	// Long long.
	using int_type = long long;
	int_type a;
	a = 0;
	detail::safe_integral_subber(a,int_type(1));
	BOOST_CHECK_EQUAL(a,-1);
	detail::safe_integral_subber(a,int_type(-1));
	BOOST_CHECK_EQUAL(a,0);
	a = std::numeric_limits<int_type>::max();
	BOOST_CHECK_THROW(detail::safe_integral_subber(a,int_type(-1)),std::overflow_error);
	detail::safe_integral_subber(a,int_type(1));
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::max() - 1);
	a = std::numeric_limits<int_type>::min();
	BOOST_CHECK_THROW(detail::safe_integral_subber(a,int_type(1)),std::overflow_error);
	detail::safe_integral_subber(a,int_type(-1));
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::min() + 1);
	a = std::numeric_limits<int_type>::max();
	detail::safe_integral_subber(a,int_type(std::numeric_limits<int_type>::max() / 2));
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::max() - std::numeric_limits<int_type>::max() / 2);
	a = std::numeric_limits<int_type>::min();
	detail::safe_integral_subber(a,int_type(std::numeric_limits<int_type>::min() / 2));
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::min() - std::numeric_limits<int_type>::min() / 2);
	}
	{
	// Unsigned char.
	using int_type = unsigned char;
	int_type a;
	a = 1;
	detail::safe_integral_subber(a,int_type(1));
	BOOST_CHECK_EQUAL(a,0);
	a = std::numeric_limits<int_type>::max();
	detail::safe_integral_subber(a,int_type(1));
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::max() - 1);
	a = 1;
	BOOST_CHECK_THROW(detail::safe_integral_subber(a,std::numeric_limits<int_type>::max()),std::overflow_error);
	a = 0;
	BOOST_CHECK_THROW(detail::safe_integral_subber(a,int_type(1)),std::overflow_error);
	BOOST_CHECK_EQUAL(a,0);
	a = std::numeric_limits<int_type>::max();
	detail::safe_integral_subber(a,std::numeric_limits<int_type>::max());
	BOOST_CHECK_EQUAL(a,0);
	a = std::numeric_limits<int_type>::max();
	detail::safe_integral_subber(a,int_type(std::numeric_limits<int_type>::max() - 1));
	BOOST_CHECK_EQUAL(a,1);
	}
	{
	// Unsigned.
	using int_type = unsigned;
	int_type a;
	a = 1;
	detail::safe_integral_subber(a,int_type(1));
	BOOST_CHECK_EQUAL(a,0);
	a = std::numeric_limits<int_type>::max();
	detail::safe_integral_subber(a,int_type(1));
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::max() - 1);
	a = 1;
	BOOST_CHECK_THROW(detail::safe_integral_subber(a,std::numeric_limits<int_type>::max()),std::overflow_error);
	a = 0;
	BOOST_CHECK_THROW(detail::safe_integral_subber(a,int_type(1)),std::overflow_error);
	BOOST_CHECK_EQUAL(a,0);
	a = std::numeric_limits<int_type>::max();
	detail::safe_integral_subber(a,std::numeric_limits<int_type>::max());
	BOOST_CHECK_EQUAL(a,0);
	a = std::numeric_limits<int_type>::max();
	detail::safe_integral_subber(a,int_type(std::numeric_limits<int_type>::max() - 1));
	BOOST_CHECK_EQUAL(a,1);
	}
	{
	// Unsigned long.
	using int_type = unsigned long;
	int_type a;
	a = 1;
	detail::safe_integral_subber(a,int_type(1));
	BOOST_CHECK_EQUAL(a,0);
	a = std::numeric_limits<int_type>::max();
	detail::safe_integral_subber(a,int_type(1));
	BOOST_CHECK_EQUAL(a,std::numeric_limits<int_type>::max() - 1);
	a = 1;
	BOOST_CHECK_THROW(detail::safe_integral_subber(a,std::numeric_limits<int_type>::max()),std::overflow_error);
	a = 0;
	BOOST_CHECK_THROW(detail::safe_integral_subber(a,int_type(1)),std::overflow_error);
	BOOST_CHECK_EQUAL(a,0);
	a = std::numeric_limits<int_type>::max();
	detail::safe_integral_subber(a,std::numeric_limits<int_type>::max());
	BOOST_CHECK_EQUAL(a,0);
	a = std::numeric_limits<int_type>::max();
	detail::safe_integral_subber(a,int_type(std::numeric_limits<int_type>::max() - 1));
	BOOST_CHECK_EQUAL(a,1);
	}
}
