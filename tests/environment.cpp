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

#include "../src/environment.hpp"

#define BOOST_TEST_MODULE environment_test
#include <boost/test/unit_test.hpp>

#include <iostream>

using namespace piranha;

struct dummy
{
	~dummy()
	{
		std::cout << "Shutdown flag is: " << environment::shutdown() << '\n';
	}
};

dummy d;

BOOST_AUTO_TEST_CASE(environment_main_test)
{
	// Multiple constructions.
	environment env1;
	environment env2;
	environment env3;
	BOOST_CHECK(!environment::shutdown());
}
