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

from _common import _get_cf_types, _get_series_type
import unittest as _ut

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
	>>> t1 = get_type("integer")
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
	2x
	>>> from fractions import Fraction
	>>> tq = get_type('rational')
	>>> print(Fraction(1,2) * tq('x')**2)
	1/2x**2
	
	"""
	return _get_series_type('polynomial',cf_type)

class main_test_case(_ut.TestCase):
	"""Main test case.
	
	To be used within the :mod:`unittest` framework. Will test construction, arithmetic
	and comparison operators, and exceptions.
	
	>>> import unittest as ut
	>>> suite = ut.TestLoader().loadTestsFromTestCase(main_test_case)
	
	"""
	def runTest(self):
		from fractions import Fraction
		from _core import _get_big_int
		# Arithmetic with int and Fraction, len, str and comparisons.
		for t in [int,Fraction]:
			tp = get_type(t)
			self.assertEqual(tp('x') - tp('x'),t(1) - t(1))
			self.assertEqual(tp(tp('x')) - tp('x'),t(1) - t(1))
			self.assertEqual(tp(),t(0))
			self.assertEqual(tp(t(2)),t(2))
			self.assertEqual(tp('x'),+tp('x'))
			foo = tp(t(1))
			foo += t(2)
			self.assertEqual(foo,t(1) + t(2))
			self.assertEqual(tp(1) + tp(2),t(1) + t(2))
			self.assertEqual(tp() + t(2),t(0) + t(2))
			self.assertEqual(t(2) + tp(),t(0) + t(2))
			self.assertEqual(tp('x') * t(-1),-tp('x'))
			foo -= t(4)
			self.assertEqual(foo,t(3) - t(4))
			self.assertEqual(tp(1) - tp(2),t(1) - t(2))
			self.assertEqual(tp() - t(2),t(0) - t(2))
			self.assertEqual(t(2) - tp(),t(2) - t(0))
			self.assertEqual(tp('x'),-(-tp('x')))
			foo *= t(-5)
			self.assertEqual(foo,t(-1) * t(-5))
			self.assertEqual(tp(1) * tp(2),t(1) * t(2))
			self.assertEqual(tp(1) * t(2),t(1) * t(2))
			self.assertEqual(t(2) * tp(1),t(2) * t(1))
			self.assertNotEqual(repr(tp('x')),'')
			self.assertNotEqual(str(tp('x')),'')
			self.assertEqual(len(tp('x') + 1),2)
			self.assertTrue(tp('x') != tp('y'))
			self.assertTrue(tp('x') != t(1))
			self.assertTrue(t(1) != tp('x'))
			self.assertTrue(tp('x') == tp('x'))
			self.assertTrue(tp(t(1)) == t(1))
			self.assertTrue(t(1) == tp(t(1)))
			self.assertTrue(tp('x') ** 3 == tp('x') * tp('x') * tp('x'))
		tp_int = get_type(int)
		self.assertRaises(ValueError,tp_int,float('inf'))
		self.assertRaises(ZeroDivisionError,lambda : tp_int() ** -1)
		tp_q = get_type(Fraction)
		self.assertRaises(ZeroDivisionError,lambda : tp_q(Fraction(0,1)) ** -1)
		self.assertEqual(tp_q(Fraction(1,3)) ** -2,9)
		self.assertRaises(ZeroDivisionError,lambda : tp_int(Fraction(1,3)) ** -2)
		tp_f = get_type(float)
		# NOTE: here we are going to assume that Python's float implementation uses C++ doubles and
		# the corresponding pow() function.
		self.assertEqual(tp_f(0.1) ** (0.5),0.1**0.5)
		# Test integer exponentiation of double triggering bad numeric cast.
		self.assertRaises(OverflowError,lambda : tp_f(1) ** _get_big_int())

def run_test_suite():
	"""Run the full test suite for the module.
	
	>>> run_test_suite()
	
	"""
	suite = _ut.TestLoader().loadTestsFromTestCase(main_test_case)
	_ut.TextTestRunner(verbosity=2).run(suite)
