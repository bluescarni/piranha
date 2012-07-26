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

"""Root Pyranha module.

.. moduleauthor:: Francesco Biscani <bluescarni@gmail.com>

"""

_series_types = ['polynomial', 'poisson_series']

__all__ = _series_types + ['math', 'test', 'settings']

from _common import _register_evaluate_wrapper, _register_repr_png

for n in _series_types:
	_register_evaluate_wrapper(n)
	_register_repr_png(n)
	_register_repr_latex(n)

# Cleanup.
del n
del _series_types

class _settings(object):
	@property
	def max_term_output(self):
		from _core import _settings as _s
		return _s._get_max_term_output()
	@max_term_output.setter
	def max_term_output(self,n):
		from _core import _settings as _s
		_s._set_max_term_output(n)

settings = _settings()
