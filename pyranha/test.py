# -*- coding: iso-8859-1 -*-
# Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)
#
# This file is part of the Piranha library.
#
# The Piranha library is free software; you can redistribute it and/or modify
# it under the terms of either:
#
#  * the GNU Lesser General Public License as published by the Free
#    Software Foundation; either version 3 of the License, or (at your
#    option) any later version.
#
# or
#
#  * the GNU General Public License as published by the Free Software
#    Foundation; either version 3 of the License, or (at your option) any
#    later version.
#
# or both in parallel, as here.
#
# The Piranha library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received copies of the GNU General Public License and the
# GNU Lesser General Public License along with the Piranha library.  If not,
# see https://www.gnu.org/licenses/.

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
		from .types import polynomial, integer, rational, short, double, monomial
		# Arithmetic with int and Fraction, len, str and comparisons.
		for s,t in [(integer,int),(rational,Fraction)]:
			tp = polynomial(s,monomial(short))()
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
		tp_int = polynomial(integer,monomial(short))()
		self.assertRaises(ValueError,tp_int,float('inf'))
		self.assertRaises(ZeroDivisionError,lambda : tp_int() ** -1)
		tp_q = polynomial(rational,monomial(short))()
		self.assertRaises(ZeroDivisionError,lambda : tp_q(Fraction(0,1)) ** -1)
		self.assertEqual(tp_q(Fraction(1,3)) ** -2,9)
		self.assertRaises(ZeroDivisionError,lambda : tp_int(Fraction(1,3)) ** -2)
		tp_f = polynomial(double,monomial(short))()
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
		from .math import evaluate
		x = tp_q('x')
		self.assertEqual(evaluate(x,{'x' : 3}),3)
		self.assertEqual(evaluate(2 * x,{'x' : Fraction(3,2)}),Fraction(3))

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
		from .types import polynomial, real, short, monomial
		pt = polynomial(real,monomial(short))()
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
		# C functions. The results should thus be roughly the same.
		self.assertAlmostEqual(math.cos(3.),pcos(3.))
		self.assertAlmostEqual(math.cos(3.1234),pcos(3.1234))
		self.assertAlmostEqual(math.sin(3.),psin(3.))
		self.assertAlmostEqual(math.sin(3.1234),psin(3.1234))
		pt = polynomial(double,k_monomial)()
		self.assertAlmostEqual(math.cos(3),pcos(pt(3)).list[0][0])
		self.assertAlmostEqual(math.cos(2.456),pcos(pt(2.456)).list[0][0])
		self.assertAlmostEqual(math.sin(3),psin(pt(3)).list[0][0])
		self.assertAlmostEqual(math.sin(-2.456),psin(pt(-2.456)).list[0][0])
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
		self.evaluateTest()
		self.subsTest()
		self.invertTest()
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
	def evaluateTest(self):
		from fractions import Fraction as F
		from .math import evaluate
		from .types import polynomial, k_monomial, double, integer, real, rational
		pt = polynomial(double,k_monomial)()
		x,y,z = pt('x'), pt('y'), pt('z')
		self.assertEqual(evaluate(3*x*y,{'x':4,'y':5}),3.*(4.*5.))
		self.assertEqual(type(evaluate(3*x*y,{'x':4,'y':5})),float)
		self.assertEqual(evaluate(x**2*y**3/5,{'x':4,'y':5}),(4.**2*5.**3)/5)
		pt = polynomial(rational,k_monomial)()
		x,y,z = pt('x'), pt('y'), pt('z')
		self.assertEqual(evaluate(3*x*y,{'x':F(4),'y':F(5)}),3*(F(4)*F(5)))
		self.assertEqual(type(evaluate(3*x*y,{'x':F(4),'y':F(5)})),F)
		self.assertEqual(evaluate(x**2*y**3/5,{'x':F(4),'y':F(5)}),(F(4)**2*F(5)**3)/5)
		pt = polynomial(integer,k_monomial)()
		x,y,z = pt('x'), pt('y'), pt('z')
		self.assertEqual(evaluate(3*x*y,{'x':4,'y':5}),3*(4*5))
		self.assertEqual(type(evaluate(3*x*y,{'x':4,'y':5})),int)
		# Integer division truncates.
		self.assertEqual(evaluate(x**2*y**3/5,{'x':2,'y':1}),0)
		# Check errors.
		self.assertRaises(ValueError,lambda : evaluate(pt(0),{}))
		self.assertRaises(TypeError,lambda : evaluate(x**2*y**3/5,{'x':2.,'y':1}))
		self.assertRaises(TypeError,lambda : evaluate(x**2*y**3/5,{2:2,'y':1}))
		self.assertRaises(TypeError,lambda : evaluate(x**2*y**3/5,[1,2]))
		# A missing symbol, thrown from C++.
		self.assertRaises(ValueError,lambda : evaluate(3*x*y,{'x':4,'z':5}))
		try:
			from mpmath import mpf
		except ImportError:
			return
		pt = polynomial(double,k_monomial)()
		x,y,z = pt('x'), pt('y'), pt('z')
		self.assertEqual(evaluate(3*x*y,{'x':mpf(4),'y':mpf(5)}),mpf(3.)*(4.*5.))
		self.assertEqual(type(evaluate(3*x*y,{'x':mpf(4),'y':mpf(5)})),mpf)
		self.assertEqual(evaluate(x**2*y**3/5,{'x':mpf(4),'y':mpf(5)}),(mpf(4)**2*5.**3)/5)
	def subsTest(self):
		from fractions import Fraction as F
		from .math import subs, ipow_subs, t_subs, cos, sin
		from .types import poisson_series, polynomial, k_monomial, rational, double, real
		pt = poisson_series(polynomial(rational,k_monomial))()
		x,y,z = pt('x'), pt('y'), pt('z')
		# Normal subs().
		self.assertEqual(subs(z*cos(x+y),'x',0),z*cos(y))
		# Make sure that substitution with int does not trigger any conversion
		# to floating point.
		self.assertEqual(type(subs(z*cos(x+y),'x',0)),pt)
		# Trigger a floating-point conversion.
		self.assertEqual(type(subs(z*cos(x+y),'x',0.)),poisson_series(polynomial(double,k_monomial))())
		try:
			from mpmath import mpf
			self.assertEqual(type(subs(z*cos(x+y),'x',mpf(1.23))),poisson_series(polynomial(real,k_monomial))())
		except ImportError:
			pass
		# Trig subs.
		self.assertEqual(t_subs(z*sin(x+y),'y',0,1),z*cos(x))
		self.assertEqual(type(t_subs(z*sin(x+y),'y',0,1)),pt)
		n,m = 2,3
		c,s = F(n**2-m**2,n**2+m**2), F(n*m,n**2+m**2)
		self.assertEqual(t_subs(z*sin(x+y),'y',c,s),z*sin(x)*c+z*cos(x)*s)
		self.assertEqual(type(t_subs(z*sin(x+y),'y',c,s)),pt)
		self.assertEqual(type(t_subs(z*sin(x+y),'y',0.,1.)),poisson_series(polynomial(double,k_monomial))())
		try:
			from mpmath import mpf
			self.assertEqual(type(t_subs(z*sin(x+y),'y',mpf(0.),mpf(1.))),poisson_series(polynomial(real,k_monomial))())
		except ImportError:
			pass
		# Ipow subs.
		self.assertEqual(ipow_subs(x**5*y**2*z/5,'x',2,3),9*x*y**2*z/5)
		self.assertEqual(type(ipow_subs(x**5*y**2*z/5,'x',2,3)),pt)
		self.assertEqual(ipow_subs(x**6*y**2*z/5,'x',2,3),27*y**2*z/5)
		self.assertEqual(type(ipow_subs(x**5*y**2*z/5,'x',2,3.)),poisson_series(polynomial(double,k_monomial))())
		try:
			from mpmath import mpf
			self.assertEqual(type(ipow_subs(x**5*y**2*z/5,'x',2,mpf(3.))),poisson_series(polynomial(real,k_monomial))())
		except ImportError:
			pass
	def invertTest(self):
		from fractions import Fraction as F
		from .math import invert
		self.assertEqual(invert(F(3,4)),F(4,3))
		self.assertEqual(invert(F(3,-4)),-F(4,3))
		self.assertEqual(invert(1.234),1.234**-1.)
		self.assertEqual(invert(-3.456),-3.456**-1.)
		try:
			from mpmath import mpf
			self.assertEqual(type(invert(mpf(1.23))),mpf)
		except ImportError:
			pass
		from .types import polynomial, monomial, rational, short, divisor_series, divisor, poisson_series
		t = polynomial(rational,monomial(rational))()
		self.assertEqual(invert(t(F(3,4))),F(4,3))
		self.assertEqual(type(invert(t(F(3,4)))),t)
		self.assertEqual(invert(t('x')),t('x')**-1)
		t = divisor_series(polynomial(rational,monomial(rational)),divisor(short))()
		self.assertEqual(invert(t(F(3,4))),F(4,3))
		self.assertEqual(type(invert(t(F(3,4)))),t)
		self.assertEqual(str(invert(t('x'))),'1/[(x)]')
		self.assertEqual(str(t('x')**-1),'x**-1')
		t = poisson_series(divisor_series(polynomial(rational,monomial(rational)),divisor(short)))()
		self.assertEqual(invert(t(F(3,4))),F(4,3))
		self.assertEqual(type(invert(t(F(3,4)))),t)
		self.assertEqual(str(invert(t('x'))),'1/[(x)]')
		self.assertEqual(str(t('x')**-1),'x**-1')

class polynomial_test_case(_ut.TestCase):
	""":mod:`polynomial` module test case.
	
	To be used within the :mod:`unittest` framework. Will test the functions implemented in the
	:mod:`polynomial` module.
	
	>>> import unittest as ut
	>>> suite = ut.TestLoader().loadTestsFromTestCase(polynomial_test_case)
	
	"""
	def runTest(self):
		from . import polynomial_gcd_algorithm as pga
		from .types import polynomial, rational, short, integer, double, real, monomial
		from fractions import Fraction
		from .math import integrate, gcd
		self.assertEqual(type(polynomial(rational,monomial(short))()(1).list[0][0]),Fraction)
		self.assertEqual(type(polynomial(integer,monomial(short))()(1).list[0][0]),int)
		self.assertEqual(type(polynomial(double,monomial(short))()(1).list[0][0]),float)
		try:
			from mpmath import mpf
			self.assertEqual(type(polynomial(real,monomial(short))()(1).list[0][0]),mpf)
		except ImportError:
			pass
		# A couple of tests for integration.
		pt = polynomial(rational,monomial(short))()
		x,y,z = [pt(_) for _ in ['x','y','z']]
		self.assertEqual(integrate(x,'x'),x*x/2)
		self.assertEqual(integrate(x,'y'),x*y)
		self.assertEqual(integrate(x,'z'),x*z)
		self.assertEqual(integrate(x+y*z,'x'),x*x/2+x*y*z)
		# Some tests for find_cf().
		self.assertEqual(x.find_cf([0]),0)
		self.assertEqual(x.find_cf([1]),1)
		self.assertEqual((3*x).find_cf([1]),3)
		self.assertEqual((3*x+(4*y**2)/3).find_cf([0,2]),Fraction(4,3))
		self.assertRaises(ValueError,lambda : x.find_cf([1,2]))
		self.assertRaises(TypeError,lambda : x.find_cf([1.]))
		self.assertRaises(TypeError,lambda : x.find_cf([1,'a']))
		self.assertRaises(TypeError,lambda : x.find_cf(1))
		# Split and join.
		pt = polynomial(integer,monomial(short))()
		pt2 = polynomial(double,monomial(short))()
		x,y,z = [pt(_) for _ in ['x','y','z']]
		self.assertEqual((x+3*y-2*z).split().join(),x+3*y-2*z)
		self.assertEqual(type((x+3*y-2*z).split()),polynomial(polynomial(integer,monomial(short)),monomial(short))())
		# Division.
		self.assertEqual(x/x,1)
		self.assertEqual(((x+y)*(x+1))/(x+1),x+y)
		self.assertRaises(TypeError, lambda: pt2() / pt2())
		self.assertRaises(ArithmeticError, lambda: pt(1) / x)
		self.assertEqual(pt.udivrem(x,x)[0],1)
		self.assertEqual(pt.udivrem(x,x)[1],0)
		self.assertEqual(pt.udivrem(x,x+1)[0],1)
		self.assertEqual(pt.udivrem(x,x+1)[1],-1)
		self.assertRaises(ValueError,lambda : pt.udivrem(x+y,x+y))
		# GCD.
		self.assert_(gcd((x**2-y**2)*(x+3),(x-y)*(x**3+y)) == x-y or gcd((x**2-y**2)*(x+3),(x-y)*(x**3+y)) == -x+y)
		self.assertRaises(TypeError, lambda: gcd(x,1))
		self.assertRaises(ValueError, lambda: gcd(x**-1,y))
		# Test the methods to get/set the default GCD algo.
		self.assertEqual(pt.get_default_gcd_algorithm(),pga.automatic)
		pt.set_default_gcd_algorithm(pga.prs_sr)
		self.assertEqual(pt.get_default_gcd_algorithm(),pga.prs_sr)
		pt.reset_default_gcd_algorithm()
		self.assertEqual(pt.get_default_gcd_algorithm(),pga.automatic)
		def gcd_check(a,b,cmp):
			for algo in [pga.automatic,pga.prs_sr,pga.heuristic]:
				res = pt.gcd(a,b,algo)
				self.assert_(res[0] == cmp or res[0] == -cmp)
				res = pt.gcd(a,b,True,algo)
				self.assert_(res[0] == cmp or res[0] == -cmp)
				self.assertEqual(res[1],a / res[0])
				self.assertEqual(res[2],b / res[0])
		gcd_check((x**2-y**2)*(x+3),(x-y)*(x**3+y),x-y)
		# Content.
		self.assertEqual((12*x+20*y).content(),4)
		self.assertEqual((x-x).content(),0)
		self.assertRaises(AttributeError, lambda: pt2().content())
		# Primitive part.
		self.assertEqual((12*x+20*y).primitive_part(),3*x+5*y)
		self.assertRaises(ZeroDivisionError, lambda: (x-x).primitive_part())
		self.assertRaises(AttributeError, lambda: pt2().primitive_part())

class divisor_series_test_case(_ut.TestCase):
	""":mod:`divisor_series` module test case.

	To be used within the :mod:`unittest` framework. Will test the functions implemented in the
	:mod:`divisor_series` module.

	>>> import unittest as ut
	>>> suite = ut.TestLoader().loadTestsFromTestCase(divisor_series_test_case)

	"""
	def runTest(self):
		from .types import divisor_series, rational, short, double, real, divisor, monomial, polynomial
		from fractions import Fraction
		from .math import partial, integrate, invert
		self.assertEqual(type(divisor_series(polynomial(rational,monomial(short)),divisor(short))()(1).list[0][0]),polynomial(rational,monomial(short))())
		self.assertEqual(type(divisor_series(polynomial(double,monomial(short)),divisor(short))()(1).list[0][0]),polynomial(double,monomial(short))())
		self.assertEqual(type(divisor_series(polynomial(real,monomial(short)),divisor(short))()(1).list[0][0]),polynomial(real,monomial(short))())
		# A couple of tests for the invert() functionality.
		dt = divisor_series(polynomial(rational,monomial(short)),divisor(short))()
		self.assertEqual(str(invert(2*dt('x')+4*dt('y'))),"1/2*1/[(x+2*y)]")
		self.assertEqual(str(invert(dt('x')+2*dt('y'))),"1/[(x+2*y)]")
		self.assertRaises(ValueError,lambda : invert(dt('x')+2*dt('y')/3))
		self.assertRaises(ValueError,lambda : invert(dt('x')+1))
		# Check about partial().
		x,y,z = [dt(_) for _ in ['x','y','z']]
		self.assertEqual(partial(z,'x'),0)
		self.assertEqual(partial(z*x,'x'),z)
		self.assertEqual(partial(x**2,'x'),2*x)
		# Variables both in the coefficient and in the divisors.
		self.assertEqual(partial(y*x**-1,'x'),-y*x**-2)
		self.assertEqual(partial(x*invert(x-y),'x'),invert(x-y)-x*invert(x-y)**2)
		# Some integrate() testing.
		self.assertEqual(integrate(x,'x'),x*x/2)
		self.assertEqual(integrate(x*invert(y),'x'),x*x/2*invert(y))
		self.assertEqual(integrate(x*invert(y)+z,'x'),z*x+x*x/2*invert(y))
		self.assertRaises(ValueError,lambda : integrate(invert(x),'x'))

class poisson_series_test_case(_ut.TestCase):
	""":mod:`poisson_series` module test case.
	
	To be used within the :mod:`unittest` framework. Will test the functions implemented in the
	:mod:`poisson_series` module.
	
	>>> import unittest as ut
	>>> suite = ut.TestLoader().loadTestsFromTestCase(poisson_series_test_case)
	
	"""
	def runTest(self):
		from .types import poisson_series, rational, monomial, short, divisor_series, divisor, polynomial
		from .math import partial, integrate, sin, cos, invert
		# A couple of tests with eps.
		eps = poisson_series(divisor_series(polynomial(rational,monomial(short)),divisor(short)))()
		x,y,z = [eps(_) for _ in ['x','y','z']]
		self.assertEqual(str(cos(x)),'cos(x)')
		self.assertEqual(str(sin(x)),'sin(x)')
		self.assertRaises(ValueError,lambda : sin(x**2))
		self.assertEqual(str(invert(x-y)),'1/[(x-y)]')
		self.assertEqual(str(invert(-2*x+4*y)**2),'1/4*1/[(x-2*y)**2]')
		self.assertEqual(str(invert(-2*x+4*y)**3),'-1/8*1/[(x-2*y)**3]')
		# Partial.
		self.assertEqual(partial(invert(x-y)*cos(z),'z'),-invert(x-y)*sin(z))
		self.assertEqual(partial(x*invert(x-y)*cos(x),'x'),invert(x-y)*cos(x)+x*(-invert(x-y)**2*cos(x)-sin(x)*invert(x-y)))
		# Integrate.
		self.assertEqual(integrate(invert(x-y)*cos(z),'z'),invert(x-y)*sin(z))
		self.assertEqual(integrate(invert(x-y)*cos(2*z),'z'),invert(x-y)*sin(2*z)/2)
		self.assertEqual(integrate(y*invert(x)*cos(2*z),'y'),y*y/2*invert(x)*cos(2*z))
		self.assertRaises(ValueError,lambda : integrate(y*invert(x)*cos(2*z),'x'))
		self.assertRaises(ValueError,lambda : integrate(z*invert(x)*cos(2*z),'z'))

class converters_test_case(_ut.TestCase):
	"""Test case for the automatic conversion to/from Python from/to C++.

	To be used within the :mod:`unittest` framework.

	>>> import unittest as ut
	>>> suite = ut.TestLoader().loadTestsFromTestCase(converters_test_case)

	"""
	def runTest(self):
		import sys
		from .types import polynomial, short, integer, rational, real, monomial
		from fractions import Fraction as F
		from .math import evaluate
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
		pt = polynomial(integer,monomial(short))()
		# Use small integers to make it work both on Python 2 and Python 3.
		self.assertEqual(int,type(pt(4).list[0][0]))
		self.assertEqual(pt(4).list[0][0],4)
		self.assertEqual(int,type(evaluate(pt("x"),{"x":5})))
		self.assertEqual(evaluate(pt("x"),{"x":5}),5)
		# Specific for Python 2.
		if sys.version_info[0] == 2:
			self.assertEqual(int,type(pt(sys.maxint).list[0][0]))
			self.assertEqual(pt(sys.maxint).list[0][0],sys.maxint)
			self.assertEqual(long,type((pt(sys.maxint) + 1).list[0][0]))
			self.assertEqual((pt(sys.maxint) + 1).list[0][0],sys.maxint+1)
			self.assertEqual(int,type(evaluate(pt("x"),{"x":sys.maxint})))
			self.assertEqual(evaluate(pt("x"),{"x":sys.maxint}),sys.maxint)
			self.assertEqual(long,type(evaluate(pt("x"),{"x":sys.maxint + 1})))
			self.assertEqual(evaluate(pt("x"),{"x":sys.maxint + 1}),sys.maxint + 1)
		# Now rationals.
		pt = polynomial(rational,monomial(short))()
		self.assertEqual(F,type((pt(4)/3).list[0][0]))
		self.assertEqual((pt(4)/3).list[0][0],F(4,3))
		self.assertEqual(F,type(evaluate(pt("x"),{"x":F(5,6)})))
		self.assertEqual(evaluate(pt("x"),{"x":F(5,6)}),F(5,6))
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
		pt = polynomial(real,monomial(short))()
		self.assertEqual(mpf,type(pt(4).list[0][0]))
		self.assertEqual(pt(4).list[0][0],mpf(4))
		self.assertEqual(mpf,type(evaluate(pt("x"),{"x":mpf(5)})))
		self.assertEqual(evaluate(pt("x"),{"x":mpf(5)}),mpf(5))
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
		self.assertEqual(evaluate(tmp,{'x':mpf(1)}).context.dps,orig_dps)
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
		import pickle, random, tempfile, os
		from .types import polynomial, short, rational, poisson_series, monomial
		from .math import sin, cos
		from . import file_format, file_compression
		# Set the seed for deterministic output.
		random.seed(0)
		rand = lambda: random.randint(-10,10)
		# Start with some random polynomial tests.
		pt = polynomial(rational,monomial(short))()
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
			# Check load/save methods..
			f = tempfile.NamedTemporaryFile(delete=False)
			f.close()
			try:
				pt.save(res,f.name)
				self.assertEqual(pt.load(f.name),res)
				pt.save(res,f.name,file_format.binary)
				self.assertEqual(pt.load(f.name,file_format.binary),res)
				pt.save(res,f.name,file_compression.bzip2)
				self.assertEqual(pt.load(f.name,file_compression.bzip2),res)
				pt.save(res,f.name,file_format.binary,file_compression.bzip2)
				self.assertEqual(pt.load(f.name,file_format.binary,file_compression.bzip2),res)
			finally:
				# Remove the temp file in any case.
				os.remove(f.name)
		# Poisson series.
		pt = poisson_series(polynomial(rational,monomial(short)))()
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
			f = tempfile.NamedTemporaryFile(delete=False)
			f.close()
			try:
				pt.save(res,f.name)
				self.assertEqual(pt.load(f.name),res)
				pt.save(res,f.name,file_format.binary)
				self.assertEqual(pt.load(f.name,file_format.binary),res)
				pt.save(res,f.name,file_compression.bzip2)
				self.assertEqual(pt.load(f.name,file_compression.bzip2),res)
				pt.save(res,f.name,file_format.binary,file_compression.bzip2)
				self.assertEqual(pt.load(f.name,file_format.binary,file_compression.bzip2),res)
			finally:
				os.remove(f.name)

class truncate_degree_test_case(_ut.TestCase):
	"""Test case for the degree-based truncation of series.

	To be used within the :mod:`unittest` framework.

	>>> import unittest as ut
	>>> suite = ut.TestLoader().loadTestsFromTestCase(truncate_degree_test_case)

	"""
	def runTest(self):
		from fractions import Fraction as F
		from .math import truncate_degree, cos, sin, degree
		from .types import polynomial, short, rational, poisson_series, monomial
		pt = polynomial(rational,monomial(short))()
		x,y,z = pt('x'), pt('y'), pt('z')
		s = x**5*y+F(1,2)*z**-5*x*y+x*y*z/4
		self.assertEqual(s.truncate_degree(3),z**-5/2 * x * y + x*y*z/4)
		self.assertEqual(truncate_degree(s,3),z**-5/2 * x * y + x*y*z/4)
		self.assertEqual(s.truncate_degree(2,["x"]),z**-5/2 * x * y + x*y*z/4)
		self.assertEqual(truncate_degree(s,2,["x"]),z**-5/2 * x * y + x*y*z/4)
		self.assertRaises(TypeError,lambda: truncate_degree(s,2,["x",1]))
		self.assertRaises(TypeError,lambda: truncate_degree(s,2,"x"))
		pt = poisson_series(polynomial(rational,monomial(short)))()
		x,y,z,a,b = pt("x"), pt("y"), pt("z"), pt("a"), pt("b")
		s = (x + y**2/4 + 3*x*y*z/7) * cos(a) + (x*y+y*z/3+3*z**2*x/8) * sin(a+b)
		self.assertEqual(s.truncate_degree(2),(x + y*y/4) * cos(a) + (x*y + z*y/3) * sin(a + b))
		self.assertEqual(truncate_degree(s,2),(x + y*y/4) * cos(a) + (x*y + z*y/3) * sin(a + b))
		self.assertEqual(s.truncate_degree(1,["y","x"]),x * cos(a) + (z*y/3 + 3*z*z*x/8) * sin(a + b))
		self.assertEqual(truncate_degree(s,1,["y","x"]),x * cos(a) + (z*y/3 + 3*z*z*x/8) * sin(a + b))
		self.assertRaises(TypeError,lambda: truncate_degree(s,2,["x",1]))
		self.assertRaises(TypeError,lambda: truncate_degree(s,2,"x"))
		# Automatic truncation.
		# Just make sure we do not re-use any previous result.
		pt = polynomial(rational,monomial(short))()
		x,y = pt('x'), pt('y')
		pt.clear_pow_cache()
		self.assertEqual(pt.get_auto_truncate_degree(),(0,0,[]))
		pt.set_auto_truncate_degree(4)
		self.assertEqual(pt.get_auto_truncate_degree(),(1,4,[]))
		self.assertEqual(degree(x*x*x),3)
		self.assertEqual(x*x*x*x*x,0)
		pt.set_auto_truncate_degree(3,['x','y'])
		self.assertEqual(pt.get_auto_truncate_degree(),(2,3,['x','y']))
		self.assertEqual(1+12*x*y+4*x+12*x**2*y+6*x**2+4*x**3+4*y**3+6*y**2+4*y+12*x*y**2,(x+y+1)**4)
		# Check throwing conversion.
		self.assertRaises(ValueError,lambda: pt.set_auto_truncate_degree(F(4,3),['x','y']))
		self.assertEqual(pt.get_auto_truncate_degree(),(2,3,['x','y']))
		# Check multiplication.
		self.assertEqual(degree(x*x*y),3)
		self.assertEqual((x*x*y*y),0)
		# Check we cannot use float for auto truncation.
		self.assertRaises(TypeError,lambda: pt.set_auto_truncate_degree(1.23))
		# Reset before finishing.
		pt.unset_auto_truncate_degree()
		pt.clear_pow_cache()
		self.assertEqual(pt.get_auto_truncate_degree(),(0,0,[]))

class integrate_test_case(_ut.TestCase):
	"""Test case for the integration functionality.

	To be used within the :mod:`unittest` framework.

	>>> import unittest as ut
	>>> suite = ut.TestLoader().loadTestsFromTestCase(integrate_test_case)

	"""
	def runTest(self):
		from fractions import Fraction as F
		from .math import sin, cos
		from .types import polynomial, short, rational, poisson_series, monomial, integer
		pt = poisson_series(polynomial(rational,monomial(short)))()
		x,y,z = pt('x'), pt('y'), pt('z')
		s = F(1,3)*z*sin(4*x-2*y)
		self.assertEqual(s.integrate('y'),F(1,6)*z*cos(4*x-2*y))
		self.assertEqual(s.integrate('z'),z**2/6*sin(4*x-2*y))
		pt = polynomial(integer,monomial(rational))()
		z = pt('z')
		s = z**F(3,4)
		self.assertEqual(type(s),pt)
		self.assertEqual(s.integrate('z'),F(4,7)*z**F(7,4))
		self.assertEqual(type(s.integrate('z')),polynomial(rational,monomial(rational))())

class t_integrate_test_case(_ut.TestCase):
	"""Test case for the time integration functionality.

	To be used within the :mod:`unittest` framework.

	>>> import unittest as ut
	>>> suite = ut.TestLoader().loadTestsFromTestCase(t_integrate_test_case)

	"""
	def runTest(self):
		from fractions import Fraction as F
		from .math import sin
		from .types import polynomial, short, rational, poisson_series, monomial, divisor, divisor_series
		pt = poisson_series(divisor_series(polynomial(rational,monomial(short)),divisor(short)))()
		x,y,z = pt('x'), pt('y'), pt('z')
		s = F(1,3)*z*sin(4*x-2*y)
		st = s.t_integrate()
		self.assertEqual(type(st.list[0][0]),divisor_series(polynomial(rational,monomial(short)),divisor(short))())
		self.assertEqual(str(st),'-1/6*z*1/[(2*\\nu_{x}-\\nu_{y})]*cos(4*x-2*y)')
		st = s.t_integrate(['a','b'])
		self.assertEqual(type(st.list[0][0]),divisor_series(polynomial(rational,monomial(short)),divisor(short))())
		self.assertEqual(str(st),'-1/6*z*1/[(2*a-b)]*cos(4*x-2*y)')
		self.assertRaises(ValueError,lambda : s.t_integrate([]))
		self.assertRaises(ValueError,lambda : s.t_integrate(['a','b','c']))
		self.assertRaises(ValueError,lambda : s.t_integrate(['b','a']))
		st = s.t_integrate(['a','b','b'])
		self.assertEqual(str(st),'-1/6*z*1/[(2*a-b)]*cos(4*x-2*y)')
		st = s.t_integrate(['a','a','b','b'])
		self.assertEqual(str(st),'-1/6*z*1/[(2*a-b)]*cos(4*x-2*y)')
		st = s.t_integrate(['a','a','b'])
		self.assertEqual(str(st),'-1/6*z*1/[(2*a-b)]*cos(4*x-2*y)')

class doctests_test_case(_ut.TestCase):
	"""Test case that will run all the doctests.

	To be used within the :mod:`unittest` framework.

	>>> import unittest as ut
	>>> suite = ut.TestLoader().loadTestsFromTestCase(doctests_test_case)

	"""
	def runTest(self):
		import doctest
		import pyranha
		from . import celmec, math, test
		doctest.testmod(pyranha)
		doctest.testmod(celmec)
		doctest.testmod(math)
		doctest.testmod(test)

class tutorial_test_case(_ut.TestCase):
	"""Test case that will check the tutorial files.

	To be used within the :mod:`unittest` framework.

	>>> import unittest as ut
	>>> suite = ut.TestLoader().loadTestsFromTestCase(tutorial_test_case)

	"""
	def runTest(self):
		from . import _tutorial

class degree_test_case(_ut.TestCase):
	"""Test case for the degree/ldegree functionality.

	To be used within the :mod:`unittest` framework.

	>>> import unittest as ut
	>>> suite = ut.TestLoader().loadTestsFromTestCase(degree_test_case)

	"""
	def runTest(self):
		from .types import polynomial, short, rational, poisson_series, monomial
		from .math import degree, ldegree
		from fractions import Fraction as F
		pt = polynomial(rational,monomial(short))()
		x,y,z = [pt(_) for _ in 'xyz']
		self.assertEqual(degree(x),1)
		self.assertEqual(degree(x**2*y**-3*z**-4),-5)
		self.assertEqual(degree(x**2*y**-3*z**-4 + 1),0)
		self.assertEqual(degree(x**2*y**-3*z**-4,['x']),2)
		self.assertEqual(degree(x**2*y**-3*z**-4,['x','y']),-1)
		self.assertEqual(degree(x**2*y**-3*z**-4+1,['x','y']),0)
		self.assertRaises(TypeError,lambda : degree(x**2*y**-3*z**-4+1,[11]))
		self.assertEqual(ldegree(x**2*y**-3*z**-4 + 1),-5)
		self.assertEqual(ldegree(x**2*y**-3*z**-4 + 1,['y']),-3)
		self.assertEqual(ldegree(x**2*y**-3*z**-4 + 1,['y','z']),-7)
		self.assertRaises(TypeError,lambda : ldegree(x**2*y**-3*z**-4+1,[11]))
		pt = poisson_series(polynomial(rational,monomial(rational)))()
		x,y,z = [pt(_) for _ in 'xyz']
		self.assertEqual(degree(x**F(4,5)*y**-3*z**-4),F(4,5)-3-4)
		self.assertEqual(degree(x**F(4,5)*y**-3*z**-4+1),0)
		self.assertEqual(degree(x**F(4,5)*y**-3*z**-4,['x']),F(4,5))
		self.assertEqual(degree(x**F(4,5)*y**-3*z**-4,['x','z']),F(4,5)-4)
		self.assertRaises(TypeError,lambda : degree(x**2*y**-3*z**-4+1,[11]))
		self.assertEqual(ldegree(x**F(4,5)*y**-3*z**-4+1),F(4,5)-3-4)
		self.assertEqual(ldegree(x**F(4,5)*y**-3*z**-4+1,['y']),-3)
		self.assertEqual(ldegree(x**F(4,5)*y**-3*z**-4+1,['y','z']),-7)
		self.assertRaises(TypeError,lambda : ldegree(x**2*y**-3*z**-4+1,[11]))

class t_degree_order_test_case(_ut.TestCase):
	"""Test case for the t_degree/t_ldegree/t_order/t_lorder functionality.

	To be used within the :mod:`unittest` framework.

	>>> import unittest as ut
	>>> suite = ut.TestLoader().loadTestsFromTestCase(t_degree_order_test_case)

	"""
	def runTest(self):
		from .types import polynomial, short, rational, poisson_series, monomial
		from .math import t_degree, t_ldegree, t_order, t_lorder, cos, sin
		pt = poisson_series(polynomial(rational,monomial(short)))()
		x,y,z = [pt(_) for _ in 'xyz']
		self.assertEqual(t_degree(cos(x)),1)
		self.assertEqual(t_degree(cos(3*x-y)),2)
		self.assertEqual(t_degree(cos(3*x-y),['x']),3)
		self.assertRaises(TypeError,lambda: t_degree(cos(3*x-y),[11]))
		self.assertEqual(t_ldegree(cos(x)),1)
		self.assertEqual(t_ldegree(cos(3*x-y)+cos(x)),1)
		self.assertEqual(t_ldegree(cos(3*x-y)+sin(y),['x']),0)
		self.assertRaises(TypeError,lambda :t_ldegree(cos(3*x-y),[11]))
		self.assertEqual(t_order(cos(x)),1)
		self.assertEqual(t_order(cos(3*x-y)),4)
		self.assertEqual(t_order(cos(3*x-y),['x']),3)
		self.assertRaises(TypeError,lambda: t_order(cos(3*x-y),[11]))
		self.assertEqual(t_lorder(cos(x)),1)
		self.assertEqual(t_lorder(cos(3*x-y)+cos(x-y)),2)
		self.assertEqual(t_lorder(cos(3*x-y)+sin(y),['x']),0)
		self.assertRaises(TypeError,lambda :t_lorder(cos(3*x-y),[11]))

class rational_function_test_case(_ut.TestCase):
	"""Test case for rational functions.

	To be used within the :mod:`unittest` framework.

	>>> import unittest as ut
	>>> suite = ut.TestLoader().loadTestsFromTestCase(rational_function_test_case)

	"""
	def runTest(self):
		from .types import rational_function, k_monomial, monomial, short, polynomial, integer, rational
		from fractions import Fraction as F
		rt = rational_function(k_monomial)()
		pt = polynomial(integer,k_monomial)()
		qt = polynomial(rational,k_monomial)()
		x,y,z = [rt(_) for _ in 'xyz']
		self.assertEqual(rt._is_exposed_type,True)
		# Unary construction.
		self.assertEqual(rt(-3),-3)
		self.assertEqual(rt(F(1,2)),F(1,2))
		self.assertEqual(rt(pt('x')),x)
		self.assertEqual(rt(x),x)
		self.assertEqual(rt(qt('y')),y)
		self.assertRaises(TypeError,lambda: rt(1.23))
		# Binary construction.
		self.assertEqual(rt(-3,-2),F(3,2))
		self.assertEqual(rt(F(-3,-2),2),F(3,4))
		self.assertEqual(rt(F(-3,-2),x),3 / (2*x))
		self.assertEqual(rt("x",2),x / 2)
		self.assertEqual(rt("x","y"),x / y)
		self.assertEqual(rt(pt("x"),qt("y")),x / y)
		self.assertEqual(rt(qt("x"),pt("y")),x / y)
		self.assertRaises(TypeError,lambda: rt(1.23,1))
		# Copy/deepcopy.
		from copy import copy, deepcopy
		self.assert_(id(x) != id(copy(x)))
		self.assert_(id(x) != id(deepcopy(x)))
		# Repr.
		self.assertEqual(repr(3*x),"3*x")
		# Arithmetics.
		tmp = copy(x)
		tmp += 1
		self.assertEqual(tmp,x+1)
		self.assertEqual(tmp,1+x)
		tmp -= 2
		self.assertEqual(tmp,+(x-1))
		self.assertEqual(tmp,-(1-x))
		tmp += 1
		tmp *= 2
		self.assertEqual(tmp,x*2)
		self.assertEqual(tmp,2*x)
		tmp /= 2
		tmp /= 3
		self.assertEqual(tmp,x/3)
		self.assertEqual(tmp,1/(3/x))
		self.assert_(x/3 == tmp)
		self.assert_(x/4 != tmp)
		self.assert_(tmp != x/4)
		self.assert_(x/4 != 1)
		self.assert_(1 != x/4)
		self.assertRaises(TypeError,lambda: x + 1.2)
		# Exponentiation.
		self.assertEqual(((x+y)/(x-y))**2,(x+y)*(x+y)/((x-y)*(x-y)))
		self.assertEqual(((x+y)/(x-y))**-2,(x-y)*(x-y)/((x+y)*(x+y)))
		self.assertRaises(TypeError,lambda: x ** F(3,4))
		# Evaluation.
		from .math import evaluate
		self.assertEqual(evaluate(x,{"x":2}),2)
		self.assertEqual(evaluate(x,{"x":F(3,4)}),F(3,4))
		self.assertEqual(evaluate(x,{"x":rt(3)}),pt(3))
		self.assertEqual(evaluate(x,{"x":3.}),3.)
		# Subs.
		from .math import subs
		self.assertEqual(subs(x,"x",4),4)
		self.assertEqual(subs(x,"x",F(4,5)),F(4,5))
		self.assertEqual(subs(x,"x",pt(5)),5)
		self.assertEqual(subs(x,"x",qt(F(5,4))),F(5,4))
		self.assertEqual(subs(x,"x",y),y)
		# Integration.
		from .math import integrate
		self.assertEqual(integrate(x,"x"),x*x/2)
		self.assertEqual(integrate(x/y,"x"),x*x/(2*y))
		self.assertRaises(ValueError,lambda: integrate(x/y,"y"))
		# Partial.
		from .math import partial
		self.assertEqual(partial(x,"x"),1)
		self.assertEqual(partial(x,"y"),0)

def run_test_suite():
	"""Run the full test suite.
	
	This function will raise an exception if at least one test fails.
	
	"""
	retval = 0
	suite = _ut.TestLoader().loadTestsFromTestCase(basic_test_case)
	suite.addTest(mpmath_test_case())
	suite.addTest(math_test_case())
	suite.addTest(polynomial_test_case())
	suite.addTest(divisor_series_test_case())
	suite.addTest(poisson_series_test_case())
	suite.addTest(converters_test_case())
	suite.addTest(serialization_test_case())
	suite.addTest(integrate_test_case())
	suite.addTest(t_integrate_test_case())
	suite.addTest(truncate_degree_test_case())
	suite.addTest(degree_test_case())
	suite.addTest(t_degree_order_test_case())
	suite.addTest(rational_function_test_case())
	suite.addTest(doctests_test_case())
	test_result = _ut.TextTestRunner(verbosity=2).run(suite)
	if len(test_result.failures) > 0:
		retval = 1
	suite = _ut.TestLoader().loadTestsFromTestCase(tutorial_test_case)
	# Context for the suppression of output while running the tutorials. Inspired by:
	# http://stackoverflow.com/questions/8522689/how-to-temporary-hide-stdout-or-stderr-while-running-a-unittest-in-python
	# This will temporarily replace sys.stdout with the null device.
	class suppress_stdout(object):
		def __init__(self):
			pass
		def __enter__(self):
			import sys, os
			self._stdout = sys.stdout
			# NOTE: originally here it was 'wb', but apparently this will create problems
			# in Python 3 due to the string changes. With 'wt' it seems to work ok in both Python 2
			# and 3.
			null = open(os.devnull,'wt')
			sys.stdout = null
		def __exit__(self, type, value, traceback):
			import sys
			sys.stdout = self._stdout
	with suppress_stdout():
		test_result = _ut.TextTestRunner(verbosity=2).run(suite)
		if len(test_result.failures) > 0:
			retval = 1
	if retval != 0:
		raise RuntimeError('One or more tests failed.')
