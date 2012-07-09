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

#include "../src/print_tex_coefficient.hpp"

#define BOOST_TEST_MODULE print_tex_coefficient_test
#include <boost/test/unit_test.hpp>

#include <sstream>

#include "../src/environment.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(print_tex_coefficient_main_test)
{
	environment env;
	std::ostringstream oss;
	print_tex_coefficient(oss,-10);
	BOOST_CHECK_EQUAL(oss.str(),"-10");
	oss.str("");
	print_tex_coefficient(oss,11ull);
	BOOST_CHECK_EQUAL(oss.str(),"11");
	oss.str("");
}
