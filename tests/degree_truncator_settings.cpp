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

#include "../src/degree_truncator_settings.hpp"

#define BOOST_TEST_MODULE degree_truncator_settings_test
#include <boost/test/unit_test.hpp>

#include "../src/integer.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(degree_truncator_settings_get_set_test)
{
	typedef degree_truncator_settings dts_type;
	dts_type dts;
	BOOST_CHECK(dts.get_mode() == dts_type::inactive);
	BOOST_CHECK(dts.get_limit() == 0);
	BOOST_CHECK(dts.get_args().empty());
	dts.set(5);
	BOOST_CHECK(dts.get_mode() == dts_type::total);
	BOOST_CHECK(dts.get_limit() == 5);
	BOOST_CHECK(dts.get_args().empty());
	dts.set(integer(5));
	BOOST_CHECK(dts.get_mode() == dts_type::total);
	BOOST_CHECK(dts.get_limit() == 5);
	BOOST_CHECK(dts.get_args().empty());
	dts.set("x",5);
	BOOST_CHECK(dts.get_mode() == dts_type::partial);
	BOOST_CHECK(dts.get_limit() == 5);
	BOOST_CHECK(dts.get_args().size() == 1u && *dts.get_args().begin() == "x");
	dts.set("y",integer(5));
	BOOST_CHECK(dts.get_mode() == dts_type::partial);
	BOOST_CHECK(dts.get_limit() == 5);
	BOOST_CHECK(dts.get_args().size() == 1u && *dts.get_args().begin() == "y");
	dts.set({"x","y"},5);
	BOOST_CHECK(dts.get_mode() == dts_type::partial);
	BOOST_CHECK(dts.get_limit() == 5);
	BOOST_CHECK(dts.get_args().size() == 2u && *dts.get_args().begin() == "x");
	dts.set({"a","b"},integer(5));
	BOOST_CHECK(dts.get_mode() == dts_type::partial);
	BOOST_CHECK(dts.get_limit() == 5);
	BOOST_CHECK(dts.get_args().size() == 2u && *dts.get_args().begin() == "a");
	dts.unset();
	BOOST_CHECK(dts.get_mode() == dts_type::inactive);
	BOOST_CHECK(dts.get_limit() == 0);
	BOOST_CHECK(dts.get_args().empty());
}
