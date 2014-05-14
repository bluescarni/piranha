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

#ifndef PYRANHA_PYTHON_INCLUDES_HPP
#define PYRANHA_PYTHON_INCLUDES_HPP

// NOTE: the order of inclusion in the first two items here is forced by these two issues:
// http://mail.python.org/pipermail/python-list/2004-March/907592.html
// http://mail.python.org/pipermail/new-bugs-announce/2011-March/010395.html
#if defined(_WIN32)
#include <cmath>
#include <Python.h>
#else
#include <Python.h>
#include <cmath>
#endif

#if PY_MAJOR_VERSION < 2 || (PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION < 6)
	#error Minimum supported Python version is 2.6.
#endif

#endif
