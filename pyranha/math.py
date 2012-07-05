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

# Use absolute imports to avoid issues with the main math module.
from __future__ import absolute_import as _ai

def cos(arg):
	"""Cosine.
	
	This function is a wrapper around a lower level function. If the argument is a standard *float* or *int*,
	the function from the builtin :mod:`math` module will be used. If the argument is an :mod:`mpmath`
	float, the corresponding multiprecision function, if available, will be used. Otherwise, the argument is assumed
	to be a series type and a function from the piranha C++ library is used.
	
	:param arg: cosine argument
	:rtype: cosine of *arg*
	:raises: :exc:`TypeError` if the type of *arg* is not supported, or any other exception raised by the invoked
		low-level function
	
	>>> from poisson_series import get_type
	>>> t = get_type("polynomial_rational")
	>>> cos(2 * t("x"))
	cos(2x)
	
	"""
	if isinstance(arg,float) or isinstance(arg,int):
		import math
		return math.cos(arg)
	try:
		from mpmath import mpf, cos
		if isinstance(arg,mpf):
			return cos(arg)
	except ImportError:
		pass
	from ._core import _cos
	try:
		return _cos(arg)
	except TypeError:
		raise TypeError("Invalid argument type.")

def sin(arg):
	"""Sin.
	
	This function is a wrapper around a lower level function. If the argument is a standard *float* or *int*,
	the function from the builtin :mod:`math` module will be used. If the argument is an :mod:`mpmath`
	float, the corresponding multiprecision function, if available, will be used. Otherwise, the argument is assumed
	to be a series type and a function from the piranha C++ library is used.
	
	:param arg: sine argument
	:rtype: sine of *arg*
	:raises: :exc:`TypeError` if the type of *arg* is not supported, or any other exception raised by the invoked
		low-level function
	
	>>> from poisson_series import get_type
	>>> t = get_type("polynomial_rational")
	>>> sin(2 * t("x"))
	sin(2x)
	
	"""
	if isinstance(arg,float) or isinstance(arg,int):
		import math
		return math.sin(arg)
	try:
		from mpmath import mpf, sin
		if isinstance(arg,mpf):
			return sin(arg)
	except ImportError:
		pass
	from ._core import _sin
	try:
		return _sin(arg)
	except TypeError:
		raise TypeError("Invalid argument type.")

def partial(arg,name):
	"""Partial derivative.
	
	Compute the partial derivative of *arg* with respect to the variable *name*. *arg* must be a series type and
	*name* a string.
	
	:param arg: argument for the partial derivative
	:type arg: a series type
	:param name: name of the variable with respect to which the derivative will be calculated
	:type name: string
	:rtype: partial derivative of *arg* with respect to *name*
	:raises: :exc:`TypeError` if the types of *arg* and/or *name* are not supported, or any other exception raised by the invoked
		low-level function
	
	>>> from polynomial import get_type
	>>> pt = get_type(int)
	>>> x,y = pt("x"), pt("y")
	>>> partial(x + 2*x*y,"y")
	2x
	
	"""
	from ._core import _partial
	return _partial(arg,name)
