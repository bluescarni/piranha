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

// NOTE: we need to include cmath here because of this issue with pyconfig.h and hypot:
// http://mail.python.org/pipermail/new-bugs-announce/2011-March/010395.html
#include <cmath>
#include <boost/python/class.hpp>
#include <boost/python/module.hpp>
#include <boost/python/operators.hpp>
#include <string>

#include "../../src/integer.hpp"
#include "../../src/runtime_info.hpp"
#include "../../src/settings.hpp"

using namespace boost::python;
using namespace piranha;

BOOST_PYTHON_MODULE(_core)
{
	class_<integer>("integer", "Arbitrary precision integer class.", init<>())
		.def(init<integer>())
		.def(init<double>())
		.def(init<std::string>())
		.def(float_(self))
		.def(str(self))
		.def(repr(self))
		.def(self + self)
		.def(self + double())
		.def(double() + self);

	class_<settings>("settings", "Global piranha settings.", init<>())
		.add_static_property("n_threads",&settings::get_n_threads,&settings::set_n_threads)
		.def("reset_n_threads",&settings::reset_n_threads,"Reset the number of threads to the default value determined on startup.")
		.staticmethod("reset_n_threads");

	class_<runtime_info>("runtime_info", "Runtime information.", init<>())
		.add_static_property("hardware_concurrency",&runtime_info::get_hardware_concurrency)
		.add_static_property("cache_line_size",&runtime_info::get_cache_line_size);
}
