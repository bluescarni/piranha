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

#include "../src/settings.hpp"
#include "../src/tracing.hpp"

#define BOOST_TEST_MODULE tracing_test
#include <boost/test/unit_test.hpp>

#include <boost/any.hpp>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace piranha;

BOOST_AUTO_TEST_CASE(tracing_trace_test)
{
	settings::set_tracing(true);
	tracing::trace("event1",[](boost::any &){});
	auto f2 = [](boost::any &x) -> void {
		if (x.empty()) {
			x = 0;
		} else {
			auto tmp = boost::any_cast<int>(x);
			x = tmp + 1;
		}
	};
	tracing::trace("event2",f2);
	tracing::trace(std::string("event2"),f2);
	tracing::trace("event2",f2);
	auto f3 = [](boost::any &x) -> void {
		if (x.empty()) {
			x = 0;
		} else {
			// This will throw.
			auto tmp = boost::any_cast<std::string>(x);
		}
	};
	tracing::trace("event3",f3);
	BOOST_CHECK_THROW(tracing::trace("event3",f3),boost::bad_any_cast);
	BOOST_CHECK_THROW(tracing::trace("event4",
		[](boost::any &) -> void {throw std::runtime_error("");}),std::runtime_error);
	settings::set_tracing(false);
	BOOST_CHECK_NO_THROW(tracing::trace("event4",
		[](boost::any &) -> void {throw std::runtime_error("");}));
}

BOOST_AUTO_TEST_CASE(tracing_dump_test)
{
	settings::set_tracing(true);
	std::ostringstream oss;
	tracing::dump(oss);
	BOOST_CHECK(!oss.str().empty());
	tracing::dump();
}

BOOST_AUTO_TEST_CASE(tracing_get_test)
{
	settings::set_tracing(true);
	BOOST_CHECK(tracing::get("event1").empty());
	BOOST_CHECK(tracing::get(std::string("event1")).empty());
	BOOST_CHECK(!tracing::get("event2").empty());
	BOOST_CHECK(!tracing::get(std::string("event2")).empty());
	BOOST_CHECK(tracing::get("event_n").empty());
	BOOST_CHECK(tracing::get(std::string("event_n")).empty());
	BOOST_CHECK((bool)boost::any_cast<int>(tracing::get("event2")));
}
