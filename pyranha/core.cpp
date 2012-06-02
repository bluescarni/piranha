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

// NOTE: the order of inclusion in the first two items here is forced by these two issues:
// http://mail.python.org/pipermail/python-list/2004-March/907592.html
// http://mail.python.org/pipermail/new-bugs-announce/2011-March/010395.html
#include <Python.h>
#include <cmath>
#include <boost/python/docstring_options.hpp>
#include <boost/python/module.hpp>

using namespace boost::python;

BOOST_PYTHON_MODULE(_core)
{
	docstring_options doc_options(true,true,false);
}
