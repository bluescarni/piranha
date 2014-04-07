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

from __future__ import absolute_import as _ai

__all__ = ['celmec', 'math', 'test', 'settings', 'types']

import threading as _thr
from ._common import _cpp_type_catcher, _register_wrappers, _cleanup_custom_derivatives, _replace_gtg_call

# Register common wrappers.
_register_wrappers()
# Monkey patch the generic type getter.
_replace_gtg_call()

class _settings(object):
	# Main lock for protecting reads/writes from multiple threads.
	__lock = _thr.RLock()
	# Wrapper to get/set max term output.
	@property
	def max_term_output(self):
		from ._core import _settings as _s
		return _s._get_max_term_output()
	@max_term_output.setter
	def max_term_output(self,n):
		from ._core import _settings as _s
		return _cpp_type_catcher(_s._set_max_term_output,n)
	@property
	def n_threads(self):
		from ._core import _settings as _s
		return _s._get_n_threads()
	@n_threads.setter
	def n_threads(self,n):
		from ._core import _settings as _s
		return _cpp_type_catcher(_s._set_n_threads,n)
	# Wrapper method to enable/disable latex representation.
	@property
	def latex_repr(self):
		from ._core import _get_series_list as gsl
		s_type = gsl()[0]
		with self.__lock:
			return hasattr(s_type,'_repr_latex_')
	@latex_repr.setter
	def latex_repr(self,flag):
		from . import _core
		from ._core import _get_series_list as gsl
		from ._common import _register_repr_latex
		f = bool(flag)
		with self.__lock:
			# NOTE: reentrant lock in action.
			if f == self.latex_repr:
				return
			if f:
				_register_repr_latex()
			else:
				for s_type in gsl():
					assert(hasattr(s_type,'_repr_latex_'))
					delattr(s_type,'_repr_latex_')

settings = _settings()

import atexit as _atexit
_atexit.register(lambda : _cleanup_custom_derivatives())
