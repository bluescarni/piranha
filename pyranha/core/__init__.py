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

__doc__ = 'Pyranha core module.'

from _core import *

# Decorator to change input arguments of type int/long into integer.
# TODO: put in check for Python version.
# TODO: generalize to other types, e.g., the new Python rationals. How can we generalize this for arbitrary
# Python types? Thinking, e.g., of gmpy mpf types and the likes.
def switch_int_to_integer(f):
	def _inner(*args,**kwargs):
		def change_arg(x):
			if isinstance(x,int) or isinstance(x,long):
				return integer(str(x))
			else:
				return x
		new_args = [change_arg(arg) for arg in args]
		for k in kwargs:
			kwargs[k] = change_arg(kwargs[k])
		return f(*tuple(new_args),**kwargs)
	# Retain the original documentation.
	_inner.__doc__ = f.__doc__
	return _inner

integer.__init__ = switch_int_to_integer(integer.__init__)
integer.__add__ = switch_int_to_integer(integer.__add__)
integer.__radd__ = switch_int_to_integer(integer.__radd__)


# Implement conversion to (long) int.
integer.__int__ = lambda self: int(str(self))
# NOTE: this is not needed any more in Python 3.
integer.__long__ = lambda self: long(str(self))
