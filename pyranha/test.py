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

import unittest as _ut

class basic_test_case(_ut.TestCase):
	"""Basic test case.
	
	To be used within the :mod:`unittest` framework. Will test features common
	to all series types.
	
	>>> import unittest as ut
	>>> suite = ut.TestLoader().loadTestsFromTestCase(basic_test_case)
	
	"""
	def runTest(self):
		from fractions import Fraction
		from copy import copy, deepcopy
		from ._core import _get_big_int
		# Use polynomial for testing.
		from .types import polynomial, integer, rational, short, double
		# Arithmetic with int and Fraction, len, str and comparisons.
		for s,t in [(integer,int),(rational,Fraction)]:
			tp = polynomial(s,short)()
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
		tp_int = polynomial(integer,short)()
		self.assertRaises(ValueError,tp_int,float('inf'))
		self.assertRaises(ZeroDivisionError,lambda : tp_int() ** -1)
		tp_q = polynomial(rational,short)()
		self.assertRaises(ZeroDivisionError,lambda : tp_q(Fraction(0,1)) ** -1)
		self.assertEqual(tp_q(Fraction(1,3)) ** -2,9)
		self.assertRaises(ZeroDivisionError,lambda : tp_int(Fraction(1,3)) ** -2)
		tp_f = polynomial(double,short)()
		# NOTE: here we are going to assume that Python's float implementation uses C++ doubles and
		# the corresponding pow() function.
		self.assertEqual(tp_f(0.1) ** (0.5),0.1**0.5)
		q1 = tp_q(Fraction(1,6))
		self.assertEqual(q1 ** 20,Fraction(2,12) ** 20)
		self.assertEqual(Fraction(1,3) ** 100000,tp_q(Fraction(1,3)) ** 100000)
		# Copy and deepcopy.
		s1 = tp_int(2)
		self.assertNotEqual(id(copy(s1)),s1)
		self.assertEqual(copy(s1),s1)
		self.assertNotEqual(id(deepcopy(s1)),s1)
		self.assertEqual(deepcopy(s1),s1)
		# Latex renderer, if available.
		x = tp_int('x')
		try:
			tmp = x._repr_png_()
			self.assertTrue(len(tmp) != 0)
		except OSError:
			pass
		# Evaluation.
		x = tp_q('x')
		self.assertEqual(x.evaluate({'x' : 3}),3)
		self.assertEqual((2 * x).evaluate({'x' : Fraction(3,2)}),Fraction(3))

class mpmath_test_case(_ut.TestCase):
	""":mod:`mpmath` test case.
	
	To be used within the :mod:`unittest` framework. Will test interoperability between
	the :mod:`mpmath` library and the C++ *real* class. If the :mod:`mpmath` library is not available, the
	test will return immediately.
	
	>>> import unittest as ut
	>>> suite = ut.TestLoader().loadTestsFromTestCase(mpmath_test_case)
	
	"""
	def runTest(self):
		try:
			from mpmath import workdps, mpf, pi
		except ImportError:
			return
		from .types import polynomial, real, short
		pt = polynomial(real,short)()
		self.assertEqual(pt(mpf("4.5667")),mpf("4.5667"))
		self.assertEqual(pt(mpf("4.5667")) ** mpf("1.234567"),mpf("4.5667") ** mpf("1.234567"))
		for n in [11,21,51,101,501]:
			with workdps(n):
				self.assertEqual(pt(mpf("4.5667")),mpf("4.5667"))
				self.assertEqual(pt(mpf("4.5667")) ** mpf("1.234567"),mpf("4.5667") ** mpf("1.234567"))
				self.assertEqual(pt(mpf(pi)),mpf(pi))

class math_test_case(_ut.TestCase):
	""":mod:`math` module test case.
	
	To be used within the :mod:`unittest` framework. Will test the functions implemented in the
	:mod:`math` module.
	
	>>> import unittest as ut
	>>> suite = ut.TestLoader().loadTestsFromTestCase(math_test_case)
	
	"""
	def runTest(self):
		import math
		from .math import cos as pcos, sin as psin
		from .types import polynomial, kronecker_monomial, double, real
		self.assertEqual(math.cos(3),pcos(3))
		self.assertEqual(math.cos(3.1234),pcos(3.1234))
		self.assertEqual(math.sin(3),psin(3))
		self.assertEqual(math.sin(3.1234),psin(3.1234))
		pt = polynomial(double,kronecker_monomial())()
		self.assertEqual(math.cos(3),pcos(pt(3)))
		self.assertEqual(math.cos(-2.456),pcos(pt(2.456)))
		self.assertEqual(math.sin(3),psin(pt(3)))
		self.assertEqual(math.sin(-2.456),-psin(pt(2.456)))
		self.assertRaises(TypeError,lambda : pcos(""))
		self.assertRaises(TypeError,lambda : psin(""))
		try:
			from mpmath import mpf, workdps
			from mpmath import cos as mpcos, sin as mpsin
			pt = polynomial(real,kronecker_monomial())()
			self.assertEqual(mpcos(mpf("1.2345")),pcos(mpf("1.2345")))
			self.assertEqual(mpcos(mpf("3")),pcos(pt(mpf("3"))))
			self.assertEqual(mpcos(mpf("-2.456")),pcos(pt(mpf("-2.456"))))
			self.assertEqual(mpsin(mpf("1.2345")),psin(mpf("1.2345")))
			self.assertEqual(mpsin(mpf("3")),psin(pt(mpf("3"))))
			self.assertEqual(mpsin(mpf("-2.456")),psin(pt(mpf("-2.456"))))
			with workdps(500):
				self.assertEqual(mpcos(mpf("1.2345")),pcos(mpf("1.2345")))
				self.assertEqual(mpcos(mpf("3")),pcos(pt(mpf("3"))))
				self.assertEqual(mpcos(mpf("-2.456")),pcos(pt(mpf("-2.456"))))
				self.assertEqual(mpsin(mpf("1.2345")),psin(mpf("1.2345")))
				self.assertEqual(mpsin(mpf("3")),psin(pt(mpf("3"))))
				self.assertEqual(mpsin(mpf("-2.456")),psin(pt(mpf("-2.456"))))
		except ImportError:
			pass

class polynomial_test_case(_ut.TestCase):
	""":mod:`polynomial` module test case.
	
	To be used within the :mod:`unittest` framework. Will test the functions implemented in the
	:mod:`polynomial` module.
	
	>>> import unittest as ut
	>>> suite = ut.TestLoader().loadTestsFromTestCase(polynomial_test_case)
	
	"""
	def runTest(self):
		from .types import polynomial, rational, short, integer, double, real
		from fractions import Fraction
		self.assertEqual(type(polynomial(rational,short)()(1).list[0][0]),Fraction)
		self.assertEqual(type(polynomial(integer,short)()(1).list[0][0]),int)
		self.assertEqual(type(polynomial(double,short)()(1).list[0][0]),float)
		try:
			from mpmath import mpf
			self.assertEqual(type(polynomial(real,short)()(1).list[0][0]),mpf)
		except ImportError:
			pass

class poisson_series_test_case(_ut.TestCase):
	""":mod:`poisson_series` module test case.
	
	To be used within the :mod:`unittest` framework. Will test the functions implemented in the
	:mod:`poisson_series` module.
	
	>>> import unittest as ut
	>>> suite = ut.TestLoader().loadTestsFromTestCase(poisson_series_test_case)
	
	"""
	def runTest(self):
		from .types import poisson_series, rational, double, real
		from fractions import Fraction
		self.assertEqual(type(poisson_series(rational)()(1).list[0][0]),Fraction)
		self.assertEqual(type(poisson_series(double)()(1).list[0][0]),float)
		try:
			from mpmath import mpf
			self.assertEqual(type(poisson_series(real)()(1).list[0][0]),mpf)
		except ImportError:
			pass

def run_test_suite():
	"""Run the full test suite.
	
	"""
	suite = _ut.TestLoader().loadTestsFromTestCase(basic_test_case)
	suite.addTest(mpmath_test_case())
	suite.addTest(math_test_case())
	suite.addTest(polynomial_test_case())
	suite.addTest(poisson_series_test_case())
	_ut.TextTestRunner(verbosity=2).run(suite)
	# Run the doctests.
	import doctest
	import pyranha
	from . import celmec, math, test
	doctest.testmod(pyranha)
	doctest.testmod(celmec)
	doctest.testmod(math)
	doctest.testmod(test)
