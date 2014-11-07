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

from __future__ import absolute_import as _ai

#: List of Pyranha modules.
__all__ = ['celmec', 'math', 'test', 'types']

import threading as _thr
from ._common import _cpp_type_catcher, _monkey_patching, _cleanup

# Run the monkey patching.
_monkey_patching()

class settings(object):
	"""Settings class.

	This class is used to configure global Pyranha settings via static methods.
	The methods are thread-safe.

	"""
	# Main lock for protecting reads/writes from multiple threads.
	__lock = _thr.RLock()
	@staticmethod
	def get_max_term_output():
		"""Get the maximum number of series terms to print.

		:returns: the maximum number of series terms to print
		:raises: any exception raised by the invoked low-level function

		>>> settings.get_max_term_output()
		20

		"""
		from ._core import _settings as _s
		return _s._get_max_term_output()
	@staticmethod
	def set_max_term_output(n):
		"""Set the maximum number of series terms to print.

		:param n: number of series terms to print
		:type n: ``int``
		:raises: any exception raised by the invoked low-level function

		>>> settings.set_max_term_output(10)
		>>> settings.get_max_term_output()
		10
		>>> settings.set_max_term_output(-1) # doctest: +IGNORE_EXCEPTION_DETAIL
		Traceback (most recent call last):
		  ...
		OverflowError: invalid value
		>>> settings.set_max_term_output("hello") # doctest: +IGNORE_EXCEPTION_DETAIL
		Traceback (most recent call last):
		  ...
		TypeError: invalid type
		>>> settings.reset_max_term_output()

		"""
		from ._core import _settings as _s
		return _cpp_type_catcher(_s._set_max_term_output,n)
	@staticmethod
	def reset_max_term_output():
		"""Reset the maximum number of series terms to print to the default value.

		:raises: any exception raised by the invoked low-level function

		>>> settings.set_max_term_output(10)
		>>> settings.get_max_term_output()
		10
		>>> settings.reset_max_term_output()
		>>> settings.get_max_term_output()
		20

		"""
		from ._core import _settings as _s
		return _s._reset_max_term_output()
	@staticmethod
	def get_n_threads():
		"""Get the number of threads that can be used by Piranha.

		The initial value is auto-detected on program startup.

		:raises: any exception raised by the invoked low-level function

		>>> settings.get_n_threads() # doctest: +SKIP
		16 # This will be a platform-dependent value

		"""
		from ._core import _settings as _s
		return _s._get_n_threads()
	@staticmethod
	def set_n_threads(n):
		"""Set the number of threads that can be used by Piranha.

		:param n: desired number of threads
		:type n: ``int``
		:raises: any exception raised by the invoked low-level function

		>>> settings.set_n_threads(2)
		>>> settings.get_n_threads()
		2
		>>> settings.set_n_threads(0) # doctest: +IGNORE_EXCEPTION_DETAIL
		Traceback (most recent call last):
		  ...
		ValueError: invalid value
		>>> settings.set_n_threads(-1) # doctest: +IGNORE_EXCEPTION_DETAIL
		Traceback (most recent call last):
		  ...
		OverflowError: invalid value
		>>> settings.reset_n_threads()

		"""
		from ._core import _settings as _s
		return _cpp_type_catcher(_s._set_n_threads,n)
	@staticmethod
	def reset_n_threads():
		"""Reset the number of threads that can be used by Piranha to the default value.

		:raises: any exception raised by the invoked low-level function

		>>> n = settings.get_n_threads()
		>>> settings.set_n_threads(10)
		>>> settings.get_n_threads()
		10
		>>> settings.reset_n_threads()
		>>> settings.get_n_threads() == n
		True

		"""
		from ._core import _settings as _s
		return _s._reset_n_threads()
	@staticmethod
	def get_latex_repr():
		r"""Check if series types have a ``_repr_latex_()`` method.

		The ``_repr_latex_()`` method, if present, is used by the IPython notebook to display the TeX
		representation of a series via a Javascript library. If a series is very large, it might be preferrable
		to disable the TeX representation in order to improve the performance of the IPython UI. When the TeX
		representation is disabled, IPython will fall back to the PNG-based representation of the series, which
		leverages - if available - a local installation of TeX for increased performance via a static rendering
		of the TeX representation of the series to a PNG bitmap.

		By default, the TeX representation of series is enabled.

		>>> settings.get_latex_repr()
		True
		>>> from .types import polynomial, rational, k_monomial
		>>> pt = polynomial(rational,k_monomial)()
		>>> x = pt('x')
		>>> (x**2/2)._repr_latex_()
		'\\[ \\frac{1}{2}{x}^{2} \\]'

		"""
		from ._core import _get_series_list as gsl
		s_type = gsl()[0]
		with settings.__lock:
			return hasattr(s_type,'_repr_latex_')
	@staticmethod
	def set_latex_repr(flag):
		r"""Set the availability of the ``_repr_latex_()`` method in series.

		If *flag* is ``True``, the ``_repr_latex_()`` method of series types will be enabled. Otherwise,
		the method will be disabled. See the documentation for :py:meth:`pyranha.settings.get_latex_repr` for a
		description of how the method is used.

		:param flag: availability flag for the ``_repr_latex_()`` method
		:type flag: ``bool``
		:raises: :exc:`TypeError` if *flag* is not a ``bool``

		>>> settings.set_latex_repr(False)
		>>> settings.get_latex_repr()
		False
		>>> from .types import polynomial, rational, k_monomial
		>>> pt = polynomial(rational,k_monomial)()
		>>> x = pt('x')
		>>> (x**2/2)._repr_latex_() # doctest: +IGNORE_EXCEPTION_DETAIL
		Traceback (most recent call last):
		  ...
		AttributeError: object has no attribute '_latex_repr_'
		>>> settings.set_latex_repr(True)
		>>> (x**2/2)._repr_latex_()
		'\\[ \\frac{1}{2}{x}^{2} \\]'
		>>> settings.set_latex_repr("hello") # doctest: +IGNORE_EXCEPTION_DETAIL
		Traceback (most recent call last):
		  ...
		TypeError: the 'flag' parameter must be a bool

		"""
		from . import _core
		from ._core import _get_series_list as gsl
		from ._common import _register_repr_latex
		if not isinstance(flag,bool):
			raise TypeError("the 'flag' parameter must be a bool")
		with settings.__lock:
			# NOTE: reentrant lock in action.
			if flag == settings.get_latex_repr():
				return
			if flag:
				_register_repr_latex()
			else:
				for s_type in gsl():
					assert(hasattr(s_type,'_repr_latex_'))
					delattr(s_type,'_repr_latex_')

import atexit as _atexit
_atexit.register(lambda : _cleanup())
