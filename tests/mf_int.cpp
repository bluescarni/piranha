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

#include "../src/mf_int.hpp"

#define BOOST_TEST_MODULE runtime_info_test
#include <boost/test/unit_test.hpp>

#include <boost/integer_traits.hpp>

BOOST_AUTO_TEST_CASE(mf_int_msb_test)
{
	BOOST_CHECK_EQUAL(piranha::mf_int_traits::msb(0),-1);
	piranha::mf_uint tmp(1);
	for (unsigned i = 0; i < piranha::mf_int_traits::nbits; ++i, tmp <<= 1) {
		BOOST_CHECK_EQUAL(piranha::mf_int_traits::msb(tmp),static_cast<int>(i));
		BOOST_CHECK_EQUAL(piranha::mf_int_traits::msb(tmp + i),static_cast<int>(i));
	}
	BOOST_CHECK_EQUAL(static_cast<unsigned>(piranha::mf_int_traits::msb(boost::integer_traits<piranha::mf_uint>::const_max)),
		piranha::mf_int_traits::nbits - static_cast<unsigned>(1));
}
