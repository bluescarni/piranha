# -*- coding: iso-8859-1 -*-
# Copyright (C) 2009-2011 by Francesco Biscani
# bluescarni@gmail.com
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the
# Free Software Foundation, Inc.,
# 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

""".. moduleauthor:: Francesco Biscani <bluescarni@gmail.com>"""

from _common import _get_cf_types, _get_series_type, _cleanup_custom_derivatives

def get_cf_types():
	"""Get the list of implemented coefficient types.
	
	The returned value is a list of pairs consisting of a string and either a type or *None*.
	The string is a descriptor which can be used in :func:`get_type` to request a polynomial
	type with a specific coefficient. The type can be used in exactly the same way, but it will be
	present (i.e., not *None*) only if a direct conversion to/from the underlying C++ type is available.
	
	:rtype: list of types that can be used as polynomial coefficients
	
	>>> l = get_cf_types()
	>>> all([isinstance(t[1],type) if not t[1] is None else True for t in l])
	True
	>>> all([isinstance(t[0],str) for t in l])
	True
	>>> t1 = get_type('integer')
	>>> t2 = get_type(int)
	>>> t1 == t2
	True
	
	"""
	return _get_cf_types('polynomial')

def get_type(cf_type):
	"""Get a polynomial type.
	
	The argument must be either a string or a type, and it represents the requested coefficient type
	for the output polynomial type. *cf_type*, either in string form or in type form, must be
	present in the output of :func:`get_cf_types`, otherwise an error will be produced.
	
	:param cf_type: coefficient type
	:type cf_type: type or string
	:rtype: polynomial type
	:raises: :exc:`TypeError` if the polynomial type could not be determined
	
	>>> tz = get_type(int)
	>>> print(tz('x') * 2)
	2*x
	>>> from fractions import Fraction
	>>> tq = get_type('rational')
	>>> print(Fraction(1,2) * tq('x')**2)
	1/2*x**2
	
	"""
	return _get_series_type('polynomial',cf_type)

import atexit as _atexit
_atexit.register(lambda : _cleanup_custom_derivatives('polynomial'))
