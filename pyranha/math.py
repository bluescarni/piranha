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

from ._common import _cpp_type_catcher

# NOTE: some ideas to improve the extensibility of these functions:
# http://stackoverflow.com/questions/18957424/proper-way-to-make-functions-extensible-by-the-user

def cos(arg):
	"""Cosine.
	
	This function is a wrapper around a lower level function. If the argument is a standard *float* or *int*,
	the function from the builtin :mod:`math` module will be used. If the argument is an :mod:`mpmath`
	float, the corresponding multiprecision function, if available, will be used. Otherwise, the argument is assumed
	to be a series type and a function from the piranha C++ library is used.
	
	:param arg: cosine argument
	:returns: cosine of *arg*
	:raises: :exc:`TypeError` if the type of *arg* is not supported, or any other exception raised by the invoked
		low-level function
	
	>>> from .types import poisson_series, polynomial, rational, short
	>>> t = poisson_series(polynomial(rational,short))()
	>>> cos(2 * t('x'))
	cos(2*x)
	>>> cos('y') # doctest: +IGNORE_EXCEPTION_DETAIL
	Traceback (most recent call last):
	   ...
	TypeError: invalid argument type(s)
	
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
	return _cpp_type_catcher(_cos,arg)

def sin(arg):
	"""Sine.
	
	This function is a wrapper around a lower level function. If the argument is a standard *float* or *int*,
	the function from the builtin :mod:`math` module will be used. If the argument is an :mod:`mpmath`
	float, the corresponding multiprecision function, if available, will be used. Otherwise, the argument is assumed
	to be a series type and a function from the piranha C++ library is used.
	
	:param arg: sine argument
	:returns: sine of *arg*
	:raises: :exc:`TypeError` if the type of *arg* is not supported, or any other exception raised by the invoked
		low-level function
	
	>>> from .types import poisson_series, polynomial, rational, short
	>>> t = poisson_series(polynomial(rational,short))()
	>>> sin(2 * t('x'))
	sin(2*x)
	>>> sin('y') # doctest: +IGNORE_EXCEPTION_DETAIL
	Traceback (most recent call last):
	   ...
	TypeError: invalid argument type(s)
	
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
	return _cpp_type_catcher(_sin,arg)

def binomial(x,k):
	"""Binomial coefficient.
	
	This function is a wrapper around a lower level function. It will calculate the generalised binomial coefficient,
	supporting integers and rationals as first argument, and integers as second argument.
	
	:param x: top argument for the binomial coefficient
	:type x: *int* or *Fraction*
	:param k: bottom argument for the binomial coefficient
	:type k: *int*
	:returns: *x* choose *k*
	:raises: :exc:`TypeError` if the types of *x* and/or *k* are not supported
	:raises: :exc:`ValueError` if the absolute value of input values is too large
	:raises: any exception raised by the invoked low-level function
	
	>>> binomial(3,2)
	3
	>>> binomial(-6,2)
	21
	>>> from fractions import Fraction
	>>> binomial(Fraction(-4,5),2)
	Fraction(18, 25)
	>>> binomial(1.3,2) # doctest: +IGNORE_EXCEPTION_DETAIL
	Traceback (most recent call last):
	   ...
	TypeError: invalid argument type(s)
	>>> binomial(10,2.4) # doctest: +IGNORE_EXCEPTION_DETAIL
	Traceback (most recent call last):
	   ...
	TypeError: invalid argument type(s)
	>>> binomial(10001,2) # doctest: +IGNORE_EXCEPTION_DETAIL
	Traceback (most recent call last):
	   ...
	ValueError: input value is too large
	
	"""
	from ._core import _binomial
	return _cpp_type_catcher(_binomial,x,k)

def partial(arg,name):
	"""Partial derivative.
	
	Compute the partial derivative of *arg* with respect to the variable *name*. *arg* must be a series type and
	*name* a string.
	
	:param arg: argument for the partial derivative
	:type arg: a series type
	:param name: name of the variable with respect to which the derivative will be calculated
	:type name: string
	:returns: partial derivative of *arg* with respect to *name*
	:raises: :exc:`TypeError` if the types of *arg* and/or *name* are not supported, or any other exception raised by the invoked
		low-level function
	
	>>> from .types import polynomial, integer, short
	>>> pt = polynomial(integer,short)()
	>>> x,y = pt('x'), pt('y')
	>>> partial(x + 2*x*y,'y')
	2*x
	>>> partial(x + 2*x*y,1) # doctest: +IGNORE_EXCEPTION_DETAIL
	Traceback (most recent call last):
	   ...
	TypeError: invalid argument type(s)
	
	"""
	from ._core import _partial
	return _cpp_type_catcher(_partial,arg,name)

def integrate(arg,name):
	"""Integration.
	
	Compute the antiderivative of *arg* with respect to the variable *name*. *arg* must be a series type and
	*name* a string. The success of the operation is not guaranteed, and depends both on the type and value of
	*arg*.
	
	:param arg: argument for the integration
	:type arg: a series type
	:param name: name of the variable with respect to which the integration will be calculated
	:type name: string
	:returns: antiderivative of *arg* with respect to *name*
	:raises: :exc:`TypeError` if the types of *arg* and/or *name* are not supported, or any other exception raised by the invoked
		low-level function
	
	>>> from .types import polynomial, rational, kronecker_monomial
	>>> pt = polynomial(rational,kronecker_monomial())()
	>>> x,y = pt('x'), pt('y')
	>>> integrate(x + 2*x*y,'x') == x**2/2 + x**2*y
	True
	>>> integrate(x**-1,'x') # doctest: +IGNORE_EXCEPTION_DETAIL
	Traceback (most recent call last):
	   ...
	ValueError: negative unitary exponent
	>>> integrate(x**-1,1) # doctest: +IGNORE_EXCEPTION_DETAIL
	Traceback (most recent call last):
	   ...
	TypeError: invalid argument type(s)
	
	"""
	from ._core import _integrate
	return _cpp_type_catcher(_integrate,arg,name)

def factorial(n):
	"""Factorial.
	
	Will compute the factorial of *n*, which must be a non-negative instance of *int*.
	
	:param n: argument for the factorial
	:type n: *int*
	:returns: factorial of *n*
	:raises: :exc:`TypeError` if *n* is not an *int*
	:raises: :exc:`ValueError` if *n* is negative or too large
	
	>>> factorial(0)
	1
	>>> factorial(6)
	720
	>>> factorial(-1)
	Traceback (most recent call last):
	   ...
	ValueError: invalid argument value
	>>> factorial(1.5)
	Traceback (most recent call last):
	   ...
	TypeError: factorial argument must be an integer
	
	"""
	from  ._core import _factorial
	if not isinstance(n,int):
		raise TypeError("factorial argument must be an integer")
	try:
		return _factorial(n)
	except ValueError:
		raise ValueError("invalid argument value")

def pbracket(f,g,p_list,q_list):
	"""Poisson bracket.
	
	Compute the Poisson bracket of *f* and *g* with respect to the momenta with names in *p_list*
	and coordinates with names in *q_list*. *f* and *g* must be series of the same type, and
	*p_list* and *q_list* lists of strings with the same size and no duplicate entries.
	
	:param f: first argument
	:type f: a series type
	:param g: second argument
	:type g: a series type
	:param p_list: list of momenta names
	:type p_list: list of strings
	:param q_list: list of coordinates names
	:type q_list: list of strings
	:returns: Poisson bracket of *f* and *g* with respect to momenta *p_list* and coordinates *q_list*
	:raises: :exc:`ValueError` if *p_list* and *q_list* have different sizes or duplicate entries
	:raises: :exc:`TypeError` if the types of the arguments are invalid
	:raises: any exception raised by the invoked low-level function
	
	>>> from .types import polynomial, rational, short
	>>> pt = polynomial(rational,short)()
	>>> x,v = pt('x'), pt('v')
	>>> pbracket(x+v,x+v,['v'],['x']) == 0
	True
	>>> pbracket(x+v,x+v,[],['x']) # doctest: +IGNORE_EXCEPTION_DETAIL
	Traceback (most recent call last):
	   ...
	ValueError: the number of coordinates is different from the number of momenta
	>>> pbracket(x+v,x+v,['v','v'],['x']) # doctest: +IGNORE_EXCEPTION_DETAIL
	Traceback (most recent call last):
	   ...
	ValueError: the list of momenta contains duplicate entries
	>>> pbracket(v,x,['v'],[1]) # doctest: +IGNORE_EXCEPTION_DETAIL
	Traceback (most recent call last):
	   ...
	TypeError: invalid argument type(s)
	
	"""
	from ._core import _pbracket
	return _cpp_type_catcher(_pbracket,f,g,p_list,q_list)

def transformation_is_canonical(new_p,new_q,p_list,q_list):
	"""Test if transformation is canonical.
	
	This function will check if a transformation of Hamiltonian momenta and coordinates is canonical using the Poisson bracket test.
	The transformation is expressed as two separate list of objects, *new_p* and *new_q*, representing the new momenta
	and coordinates as functions of the old momenta *p_list* and *q_list*.
	
	The function requires *new_p* and *new_q* to be lists of series of the same type, and
	*p_list* and *q_list* lists of strings with the same size and no duplicate entries.
	
	:param new_p: list of objects representing the new momenta
	:type new_p: list of series instances
	:param new_q: list of objects representing the new coordinates
	:type new_q: list of series instances
	:param p_list: list of momenta names
	:type p_list: list of strings
	:param q_list: list of coordinates names
	:type q_list: list of strings
	:returns: ``True`` if the transformation defined by *new_p* and *new_q* is canonical, ``False`` otherwise.
	:raises: :exc:`ValueError` if the size of all input lists is not the same
	:raises: :exc:`TypeError` if the types of the arguments are invalid
	:raises: any exception raised by the invoked low-level function
	
	>>> from .types import polynomial, rational, kronecker_monomial
	>>> pt = polynomial(rational,kronecker_monomial())()
	>>> L,G,H,l,g,h = [pt(_) for _ in 'LGHlgh']
	>>> transformation_is_canonical([-l],[L],['L'],['l'])
	True
	>>> transformation_is_canonical([l],[L],['L'],['l'])
	False
	>>> transformation_is_canonical([2*L+3*G+2*H,4*L+2*G+3*H,9*L+6*G+7*H],[-4*l-g+6*h,-9*l-4*g+15*h,5*l+2*g-8*h],['L','G','H'],['l','g','h'])
	True
	>>> transformation_is_canonical([2*L+3*G+2*H,4*L+2*G+3*H,9*L+6*G+7*H],[-4*l-g+6*h,-9*l-4*g+15*h,5*l+2*g-7*h],['L','G','H'],['l','g','h'])
	False
	>>> transformation_is_canonical(L,l,'L','l') # doctest: +IGNORE_EXCEPTION_DETAIL
	Traceback (most recent call last):
	   ...
	TypeError: non-list input type
	>>> transformation_is_canonical([L,G],[l],['L'],['l']) # doctest: +IGNORE_EXCEPTION_DETAIL
	Traceback (most recent call last):
	   ...
	ValueError: the number of coordinates is different from the number of momenta
	>>> transformation_is_canonical([],[],[],[]) # doctest: +IGNORE_EXCEPTION_DETAIL
	Traceback (most recent call last):
	   ...
	ValueError: empty input list(s)
	>>> transformation_is_canonical([L,1],[l,g],['L','G'],['l','g']) # doctest: +IGNORE_EXCEPTION_DETAIL
	Traceback (most recent call last):
	   ...
	TypeError: types in input lists are not homogeneous
	>>> transformation_is_canonical([L,G],[l,g],['L','G'],['l',1]) # doctest: +IGNORE_EXCEPTION_DETAIL
	Traceback (most recent call last):
	   ...
	TypeError: p_list and q_list must be lists of strings
	>>> transformation_is_canonical(['a'],['b'],['c'],['d']) # doctest: +IGNORE_EXCEPTION_DETAIL
	Traceback (most recent call last):
	   ...
	TypeError: invalid argument type(s)
	
	"""
	from ._core import _transformation_is_canonical
	if not isinstance(new_p,list) or not isinstance(new_q,list):
		raise TypeError("non-list input type")
	if not all([isinstance(_,str) for _ in p_list + q_list]):
		raise TypeError("p_list and q_list must be lists of strings")
	types_set = list(set([type(_) for _ in new_p + new_q]))
	if len(types_set) == 0:
		raise ValueError("empty input list(s)")
	if len(types_set) != 1:
		raise TypeError("types in input lists are not homogeneous")
	try:
		inst = types_set[0]()
	except:
		raise TypeError("cannot construct instance of input type")
	return _cpp_type_catcher(_transformation_is_canonical,new_p,new_q,p_list,q_list,types_set[0]())
