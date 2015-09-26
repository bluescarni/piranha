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
	}
}
