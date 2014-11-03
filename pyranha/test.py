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
		from .types import polynomial, k_monomial, double, real
		# NOTE: according to
		# https://docs.python.org/3/library/math.html
		# the CPython math functions are wrappers around the corresponding
		# C functions. The results should thus be the same.
		self.assertEqual(math.cos(3.),pcos(3.))
		self.assertEqual(math.cos(3.1234),pcos(3.1234))
		self.assertEqual(math.sin(3.),psin(3.))
		self.assertEqual(math.sin(3.1234),psin(3.1234))
		pt = polynomial(double,k_monomial)()
		self.assertEqual(math.cos(3),pcos(pt(3)))
		self.assertEqual(math.cos(2.456),pcos(pt(2.456)))
		self.assertEqual(math.sin(3),psin(pt(3)))
		self.assertEqual(math.sin(-2.456),psin(pt(-2.456)))
		self.assertRaises(TypeError,lambda : pcos(""))
		self.assertRaises(TypeError,lambda : psin(""))
		try:
			from mpmath import mpf, workdps
			from mpmath import cos as mpcos, sin as mpsin
			pt = polynomial(real,k_monomial)()
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
		self.binomialTest()
		self.sincosTest()
	def binomialTest(self):
		from fractions import Fraction as F
		from .math import binomial
		# Check the return types of binomial.
		self.assertEqual(type(binomial(5,4)),int)
		self.assertEqual(type(binomial(5,4.)),float)
		self.assertEqual(type(binomial(5,F(4))),float)
		self.assertEqual(type(binomial(5.,4)),float)
		self.assertEqual(type(binomial(5.,4.)),float)
		self.assertEqual(type(binomial(5.,F(4))),float)
		self.assertEqual(type(binomial(F(5),4)),F)
		self.assertEqual(type(binomial(F(5),4.)),float)
		self.assertEqual(type(binomial(F(5),F(4))),float)
		try:
			from mpmath import mpf
		except ImportError:
			return
		self.assertEqual(type(binomial(mpf(5),mpf(4))),mpf)
		self.assertEqual(type(binomial(5,mpf(4))),mpf)
		self.assertEqual(type(binomial(5.,mpf(4))),mpf)
		self.assertEqual(type(binomial(F(5),mpf(4))),mpf)
		self.assertEqual(type(binomial(mpf(5),4)),mpf)
		self.assertEqual(type(binomial(mpf(5),4.)),mpf)
		self.assertEqual(type(binomial(mpf(5),F(4))),mpf)
	def sincosTest(self):
		from fractions import Fraction as F
		from .math import sin, cos
		# Check the return types.
		self.assertEqual(type(cos(0)),int)
		self.assertEqual(type(sin(0)),int)
		self.assertEqual(type(cos(F(0))),F)
		self.assertEqual(type(sin(F(0))),F)
		self.assertEqual(type(cos(1.)),float)
		self.assertEqual(type(sin(1.)),float)
		try:
			from mpmath import mpf
		except ImportError:
			return
		self.assertEqual(type(cos(mpf(1))),mpf)
		self.assertEqual(type(sin(mpf(1))),mpf)

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

class converters_test_case(_ut.TestCase):
	"""Test case for the automatic conversion to/from Python from/to C++.

	To be used within the :mod:`unittest` framework.

	>>> import unittest as ut
	>>> suite = ut.TestLoader().loadTestsFromTestCase(converters_test_case)

	"""
	def runTest(self):
		import sys
		from .types import polynomial, short, integer, rational, real
		from fractions import Fraction as F
		# Context for the temporary monkey-patching of type t to return bogus
		# string bad_str for representation via str.
		class patch_str(object):
			def __init__(self,t,bad_str):
				self.t = t
				self.bad_str = bad_str
			def __enter__(self):
				self.old_str = self.t.__str__
				self.t.__str__ = lambda s: self.bad_str
			def __exit__(self, type, value, traceback):
				self.t.__str__ = self.old_str
		# Same for repr.
		class patch_repr(object):
			def __init__(self,t,bad_repr):
				self.t = t
				self.bad_repr = bad_repr
			def __enter__(self):
				self.old_repr = self.t.__repr__
				self.t.__repr__ = lambda s: self.bad_repr
			def __exit__(self, type, value, traceback):
				self.t.__repr__ = self.old_repr
		# Start with plain integers.
		pt = polynomial(integer,short)()
		# Use small integers to make it work both on Python 2 and Python 3.
		self.assertEqual(int,type(pt(4).list[0][0]))
		self.assertEqual(pt(4).list[0][0],4)
		self.assertEqual(int,type(pt("x").evaluate({"x":5})))
		self.assertEqual(pt("x").evaluate({"x":5}),5)
		# Specific for Python 2.
		if sys.version_info[0] == 2:
			self.assertEqual(int,type(pt(sys.maxint).list[0][0]))
			self.assertEqual(pt(sys.maxint).list[0][0],sys.maxint)
			self.assertEqual(long,type((pt(sys.maxint) + 1).list[0][0]))
			self.assertEqual((pt(sys.maxint) + 1).list[0][0],sys.maxint+1)
			self.assertEqual(int,type(pt("x").evaluate({"x":sys.maxint})))
			self.assertEqual(pt("x").evaluate({"x":sys.maxint}),sys.maxint)
			self.assertEqual(long,type(pt("x").evaluate({"x":sys.maxint + 1})))
			self.assertEqual(pt("x").evaluate({"x":sys.maxint + 1}),sys.maxint + 1)
		# Now rationals.
		pt = polynomial(rational,short)()
		self.assertEqual(F,type((pt(4)/3).list[0][0]))
		self.assertEqual((pt(4)/3).list[0][0],F(4,3))
		self.assertEqual(F,type(pt("x").evaluate({"x":F(5,6)})))
		self.assertEqual(pt("x").evaluate({"x":F(5,6)}),F(5,6))
		# NOTE: if we don't go any more through str for the conversion, these types
		# of tests can go away.
		with patch_str(F,"boo"):
			self.assertRaises(ValueError,lambda: pt(F(5,6)))
		with patch_str(F,"5/0"):
			self.assertRaises(ZeroDivisionError,lambda: pt(F(5,6)))
		# Reals, if possible.
		try:
			from mpmath import mpf, mp, workdps, binomial as bin, fabs
		except ImportError:
			return
		pt = polynomial(real,short)()
		self.assertEqual(mpf,type(pt(4).list[0][0]))
		self.assertEqual(pt(4).list[0][0],mpf(4))
		self.assertEqual(mpf,type(pt("x").evaluate({"x":mpf(5)})))
		self.assertEqual(pt("x").evaluate({"x":mpf(5)}),mpf(5))
		# Test various types of bad repr.
		with patch_repr(mpf,"foo"):
			self.assertRaises(RuntimeError,lambda: pt(mpf(5)))
		with patch_repr(mpf,"'1.23445"):
			self.assertRaises(RuntimeError,lambda: pt(mpf(5)))
		with patch_repr(mpf,"'foobar'"):
			self.assertRaises(ValueError,lambda: pt(mpf(5)))
		# Check the handling of the precision.
		from .math import binomial
		# Original number of decimal digits.
		orig_dps = mp.dps
		tmp = binomial(mpf(5.1),mpf(3.2))
		self.assertEqual(tmp.context.dps,orig_dps)
		with workdps(100):
			# Check that the precision is maintained on output
			# and that the values computed with our implementation and
			# mpmath are close.
			tmp = binomial(mpf(5.1),mpf(3.2))
			self.assertEqual(tmp.context.dps,100)
			self.assert_(fabs(tmp - bin(mpf(5.1),mpf(3.2))) < mpf('5e-100'))
		# This will create a coefficient with dps equal to the current value.
		tmp = 3*pt('x')
		self.assertEqual(tmp.evaluate({'x':mpf(1)}).context.dps,orig_dps)
		self.assertEqual(tmp.list[0][0].context.dps,orig_dps)
		with workdps(100):
			# When we extract the coefficient with increased temporary precision, we get
			# an object with increased precision.
			self.assertEqual(tmp.list[0][0].context.dps,100)

class serialization_test_case(_ut.TestCase):
	"""Test case for the serialization of series.

	To be used within the :mod:`unittest` framework.

	>>> import unittest as ut
	>>> suite = ut.TestLoader().loadTestsFromTestCase(serialization_test_case)

	"""
	def runTest(self):
		import pickle, random
		from .types import polynomial, short, rational, poisson_series
		from .math import sin, cos
		# Set the seed for deterministic output.
		random.seed(0)
		rand = lambda: random.randint(-10,10)
		# Start with some random polynomial tests.
		pt = polynomial(rational,short)()
		x,y,z = pt('x'), pt('y'), pt('z')
		for _ in range(0,100):
			res = pt(0)
			for _ in range(0,20):
				tmp = x**rand() * y**rand() * z**rand()
				tmp *= rand()
				n = rand()
				if n:
					tmp /= n
				res += tmp
			str_rep = pickle.dumps(res)
			self.assertEqual(res,pickle.loads(str_rep))
		# Poisson series.
		pt = poisson_series(polynomial(rational,short))()
		x,y,z,a,b = pt('x'), pt('y'), pt('z'), pt('a'), pt('b')
		for _ in range(0,100):
			res = pt(0)
			for _ in range(0,20):
				tmp = x**rand() * y**rand() * z**rand()
				tmp *= rand()
				n = rand()
				if n:
					tmp /= n
				if (n > 0):
					tmp *= cos(a*rand() + b*rand())
				else:
					tmp *= sin(a*rand() + b*rand())
				res += tmp
			str_rep = pickle.dumps(res)
			self.assertEqual(res,pickle.loads(str_rep))

def run_test_suite():
	"""Run the full test suite.
	
	"""
	suite = _ut.TestLoader().loadTestsFromTestCase(basic_test_case)
	suite.addTest(mpmath_test_case())
	suite.addTest(math_test_case())
	suite.addTest(polynomial_test_case())
	suite.addTest(poisson_series_test_case())
	suite.addTest(converters_test_case())
	suite.addTest(serialization_test_case())
	_ut.TextTestRunner(verbosity=2).run(suite)
	# Run the doctests.
	import doctest
	import pyranha
	from . import celmec, math, test
	doctest.testmod(pyranha)
	doctest.testmod(celmec)
	doctest.testmod(math)
	doctest.testmod(test)
