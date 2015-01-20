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

#include "../src/detail/gcd.hpp"

#define BOOST_TEST_MODULE gcd_test
#include <boost/test/unit_test.hpp>

#include "../src/environment.hpp"
#include "../src/mp_integer.hpp"

using namespace piranha;
using namespace piranha::detail;

BOOST_AUTO_TEST_CASE(gcd_test_00)
{
	environment env;
	BOOST_CHECK_EQUAL(gcd(4,3),1);
	BOOST_CHECK_EQUAL(gcd(3,4),1);
	BOOST_CHECK_EQUAL(gcd(4,6),2);
	BOOST_CHECK_EQUAL(gcd(6,4),2);
	BOOST_CHECK_EQUAL(gcd(4,25),1);
	BOOST_CHECK_EQUAL(gcd(25,4),1);
	BOOST_CHECK_EQUAL(gcd(27,54),27);
	BOOST_CHECK_EQUAL(gcd(54,27),27);
	BOOST_CHECK_EQUAL(gcd(1,54),1);
	BOOST_CHECK_EQUAL(gcd(54,1),1);
	BOOST_CHECK_EQUAL(gcd(36,24),12);
	BOOST_CHECK_EQUAL(gcd(24,36),12);
	// Check compiler warnings with short ints.
	BOOST_CHECK_EQUAL(gcd(short(54),short(27)),short(27));
	BOOST_CHECK_EQUAL(gcd(short(27),short(53)),short(1));
	BOOST_CHECK(gcd(short(27),short(-54)) == short(27) || gcd(short(27),short(-54)) == short(-27));
	BOOST_CHECK(gcd(short(-54),short(27)) == short(27) || gcd(short(-54),short(27)) == short(-27));
	// Check with different signs.
	BOOST_CHECK(gcd(27,-54) == 27 || gcd(27,-54) == -27);
	BOOST_CHECK(gcd(-54,27) == 27 || gcd(-54,27) == -27);
	BOOST_CHECK(gcd(4,-25) == 1 || gcd(4,-25) == -1);
	BOOST_CHECK(gcd(-25,4) == 1 || gcd(-25,4) == -1);
	BOOST_CHECK(gcd(-25,1) == -1 || gcd(-25,1) == 1);
	BOOST_CHECK(gcd(25,-1) == -1 || gcd(25,-1) == 1);
	BOOST_CHECK(gcd(-24,36) == -12 || gcd(-24,36) == 12);
	BOOST_CHECK(gcd(24,-36) == -12 || gcd(24,-36) == 12);
	// Check with integer.
	BOOST_CHECK(gcd(27_z,-54_z) == 27_z || gcd(27_z,-54_z) == -27_z);
	BOOST_CHECK(gcd(-54_z,27_z) == 27_z || gcd(-54_z,27_z) == -27_z);
	BOOST_CHECK(gcd(4_z,-25_z) == 1_z || gcd(4_z,-25_z) == -1_z);
	BOOST_CHECK(gcd(-25_z,4_z) == 1_z || gcd(-25_z,4_z) == -1_z);
	BOOST_CHECK(gcd(-24_z,36_z) == -12_z || gcd(-24_z,36_z) == 12_z);
	BOOST_CHECK(gcd(24_z,-36_z) == -12_z || gcd(24_z,-36_z) == 12_z);
	// Check with zeroes.
	BOOST_CHECK_EQUAL(gcd(54,0),54);
	BOOST_CHECK_EQUAL(gcd(0,54),54);
	BOOST_CHECK(gcd(-25,0) == -25 || gcd(-25,0) == 25);
	BOOST_CHECK(gcd(-25_z,0_z) == -25_z || gcd(-25_z,0_z) == 25_z);
	BOOST_CHECK_EQUAL(gcd(0,0),0);
	BOOST_CHECK_EQUAL(gcd(0_z,0_z),0_z);
}
