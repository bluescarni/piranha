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

# Use absolute imports to avoid issues with the main math module.
from __future__ import absolute_import as _ai

from ._common import _cpp_type_catcher


def __check_names_argument(names):
    # This is used in a few functions below.
    if not names is None and (not isinstance(names, list) or not all([isinstance(_, str) for _ in names])):
        raise TypeError(
            'the optional \'names\' argument must be a list of strings')


def __check_eval_dict(d):
    # Helper to check that d is a dictionary suitable for use in evaluation.
    # Type checks.
    if not isinstance(d, dict):
        raise TypeError('evaluation dictionary must be a dict object')
    if len(d) == 0:
        raise ValueError('evaluation dictionary cannot be empty')
    if not all([isinstance(k, str) for k in d]):
        raise TypeError(
            'all keys in the evaluation dictionary must be string objects')
    t_set = set([type(d[k]) for k in d])
    if not len(t_set) == 1:
        raise TypeError(
            'all values in the evaluation dictionary must be of the same type')


def cos(arg):
    """Cosine.

    The supported types are ``int``, ``float``, ``Fraction``, ``mpf`` and any symbolic type that supports
    the operation.

    :param arg: cosine argument
    :type arg: ``int``, ``float``, ``Fraction``, ``mpf``, or a supported symbolic type.
    :returns: cosine of *arg*
    :raises: :exc:`TypeError` if the type of *arg* is not supported, or any other exception raised by the invoked
            low-level function

    >>> cos(0)
    1
    >>> cos(2) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    ValueError: cannot compute the cosine of a non-zero integer
    >>> cos(2.) # doctest: +ELLIPSIS
    -0.4161468...
    >>> from pyranha.types import poisson_series, polynomial, rational, int16, monomial
    >>> t = poisson_series[polynomial[rational,monomial[int16]]]()
    >>> cos(2 * t('x'))
    cos(2*x)
    >>> cos('hello') # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: invalid argument type(s)

    """
    from ._core import _cos
    return _cpp_type_catcher(_cos, arg)


def sin(arg):
    """Sine.

    The supported types are ``int``, ``float``, ``Fraction``, ``mpf`` and any symbolic type that supports
    the operation.

    :param arg: sine argument
    :type arg: ``int``, ``float``, ``Fraction``, ``mpf``, or a supported symbolic type.
    :returns: sine of *arg*
    :raises: :exc:`TypeError` if the type of *arg* is not supported, or any other exception raised by the invoked
            low-level function

    >>> sin(0)
    0
    >>> sin(2) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    ValueError: cannot compute the cosine of a non-zero integer
    >>> sin(2.) # doctest: +ELLIPSIS
    0.9092974...
    >>> from pyranha.types import poisson_series, polynomial, rational, int16, monomial
    >>> t = poisson_series[polynomial[rational,monomial[int16]]]()
    >>> sin(2 * t('x'))
    sin(2*x)
    >>> sin('hello') # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: invalid argument type(s)

    """
    from ._core import _sin
    return _cpp_type_catcher(_sin, arg)


def binomial(x, y):
    """Binomial coefficient.

    This function is a wrapper around a lower level function. It will calculate the generalised binomial coefficient :math:`{x \choose y}`,
    supporting various combinations of integral, rational, floating-point and arbitrary-precision floating-point types
    as input.

    :param x: top argument for the binomial coefficient
    :type x: ``int``, ``float``, ``Fraction``, ``mpf``
    :param y: bottom argument for the binomial coefficient
    :type y: ``int``, ``float``, ``Fraction``, ``mpf``
    :returns: *x* choose *y*
    :raises: :exc:`TypeError` if the types of *x* and/or *y* are not supported
    :raises: any exception raised by the invoked low-level function

    >>> binomial(3,2)
    3
    >>> binomial(-6,2)
    21
    >>> from fractions import Fraction as F
    >>> binomial(F(-4,5),2)
    Fraction(18, 25)
    >>> binomial(10,2.4) # doctest: +ELLIPSIS
    70.3995282...
    >>> binomial('hello world',2.4) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: invalid argument type(s)

    """
    from ._core import _binomial
    return _cpp_type_catcher(_binomial, x, y)


def gcd(x, y):
    """Greatest common divisor.

    This function is a wrapper around a lower level function. It will calculate the GCD of *x* and *y*,
    supporting ``int`` and various symbolic types as input.

    :param x: first argument
    :type x: ``int`` or a supported symbolic type
    :param y: second argument
    :type y: ``int`` or a supported symbolic type
    :returns: the GCD of *x* and *y*
    :raises: :exc:`TypeError` if the types of *x* and/or *y* are not supported
    :raises: any exception raised by the invoked low-level function

    >>> gcd(12,9)
    3
    >>> from pyranha.types import polynomial, integer, k_monomial
    >>> pt = polynomial[integer,k_monomial]()
    >>> x,y = pt('x'), pt('y')
    >>> gcd((x**2-y**2)*(x**3+1),(x+1)*(x+2*y)) # doctest: +SKIP
    x+1
    >>> gcd(x**-1,y) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    ValueError: negative exponents are not allowed
    >>> gcd(x,2) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: invalid argument type(s)

    """
    from ._core import _gcd
    return _cpp_type_catcher(_gcd, x, y)


def partial(arg, name):
    """Partial derivative.

    Compute the partial derivative of *arg* with respect to the variable *name*. *arg* must be a symbolic type and
    *name* a string.

    :param arg: argument for the partial derivative
    :type arg: a symbolic type
    :param name: name of the variable with respect to which the derivative will be calculated
    :type name: string
    :returns: partial derivative of *arg* with respect to *name*
    :raises: :exc:`TypeError` if the types of *arg* and/or *name* are not supported, or any other exception raised by the invoked
            low-level function

    >>> from pyranha.types import polynomial, integer, int16, monomial
    >>> pt = polynomial[integer,monomial[int16]]()
    >>> x,y = pt('x'), pt('y')
    >>> partial(x + 2*x*y,'y')
    2*x
    >>> partial(x + 2*x*y,1) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: invalid argument type(s)

    """
    from ._core import _partial
    return _cpp_type_catcher(_partial, arg, name)


def integrate(arg, name):
    """Integration.

    Compute the antiderivative of *arg* with respect to the variable *name*. *arg* must be a symbolic type and
    *name* a string. The success of the operation is not guaranteed, and depends both on the type and value of
    *arg*.

    :param arg: argument for the integration
    :type arg: a symbolic type
    :param name: name of the variable with respect to which the integration will be calculated
    :type name: string
    :returns: antiderivative of *arg* with respect to *name*
    :raises: :exc:`TypeError` if the types of *arg* and/or *name* are not supported, or any other exception raised by the invoked
            low-level function

    >>> from pyranha.types import polynomial, rational, k_monomial
    >>> pt = polynomial[rational,k_monomial]()
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
    return _cpp_type_catcher(_integrate, arg, name)


def factorial(n):
    """Factorial.

    Will compute the factorial of *n*, which must be a non-negative instance of ``int``.

    :param n: argument for the factorial
    :type n: ``int``
    :returns: factorial of *n*
    :raises: :exc:`TypeError` if *n* is not an ``int``
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
    from ._core import _factorial
    if not isinstance(n, int):
        raise TypeError('factorial argument must be an integer')
    try:
        return _factorial(n)
    except ValueError:
        raise ValueError('invalid argument value')


def pbracket(f, g, p_list, q_list):
    """Poisson bracket.

    Compute the Poisson bracket of *f* and *g* with respect to the momenta with names in *p_list*
    and coordinates with names in *q_list*. *f* and *g* must be symbolic objects of the same type, and
    *p_list* and *q_list* lists of strings with the same size and no duplicate entries.

    :param f: first argument
    :type f: a symbolic type
    :param g: second argument
    :type g: a symbolic type
    :param p_list: list of momenta names
    :type p_list: list of strings
    :param q_list: list of coordinates names
    :type q_list: list of strings
    :returns: Poisson bracket of *f* and *g* with respect to momenta *p_list* and coordinates *q_list*
    :raises: :exc:`ValueError` if *p_list* and *q_list* have different sizes or duplicate entries
    :raises: :exc:`TypeError` if the types of the arguments are invalid
    :raises: any exception raised by the invoked low-level function

    >>> from pyranha.types import polynomial, rational, int16, monomial
    >>> pt = polynomial[rational,monomial[int16]]()
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
    return _cpp_type_catcher(_pbracket, f, g, p_list, q_list)


def transformation_is_canonical(new_p, new_q, p_list, q_list):
    """Test if transformation is canonical.

    This function will check if a transformation of Hamiltonian momenta and coordinates is canonical using the Poisson bracket test.
    The transformation is expressed as two separate list of objects, *new_p* and *new_q*, representing the new momenta
    and coordinates as functions of the old momenta *p_list* and *q_list*.

    The function requires *new_p* and *new_q* to be lists of symbolic objects of the same type, and
    *p_list* and *q_list* lists of strings with the same size and no duplicate entries.

    :param new_p: list of objects representing the new momenta
    :type new_p: list of symbolic instances
    :param new_q: list of objects representing the new coordinates
    :type new_q: list of symbolic instances
    :param p_list: list of momenta names
    :type p_list: list of strings
    :param q_list: list of coordinates names
    :type q_list: list of strings
    :returns: ``True`` if the transformation defined by *new_p* and *new_q* is canonical, ``False`` otherwise
    :raises: :exc:`ValueError` if the size of all input lists is not the same
    :raises: :exc:`TypeError` if the types of the arguments are invalid
    :raises: any exception raised by the invoked low-level function

    >>> from pyranha.types import polynomial, rational, k_monomial
    >>> pt = polynomial[rational,k_monomial]()
    >>> L,G,H,l,g,h = [pt(_) for _ in 'LGHlgh']
    >>> transformation_is_canonical([-l],[L],['L'],['l'])
    True
    >>> transformation_is_canonical([l],[L],['L'],['l'])
    False
    >>> transformation_is_canonical([2*L+3*G+2*H,4*L+2*G+3*H,9*L+6*G+7*H],
    ... [-4*l-g+6*h,-9*l-4*g+15*h,5*l+2*g-8*h],['L','G','H'],['l','g','h'])
    True
    >>> transformation_is_canonical([2*L+3*G+2*H,4*L+2*G+3*H,9*L+6*G+7*H],
    ... [-4*l-g+6*h,-9*l-4*g+15*h,5*l+2*g-7*h],['L','G','H'],['l','g','h'])
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
    if not isinstance(new_p, list) or not isinstance(new_q, list):
        raise TypeError('non-list input type')
    if not all([isinstance(_, str) for _ in p_list + q_list]):
        raise TypeError('p_list and q_list must be lists of strings')
    types_set = list(set([type(_) for _ in new_p + new_q]))
    if len(types_set) == 0:
        raise ValueError('empty input list(s)')
    if len(types_set) != 1:
        raise TypeError('types in input lists are not homogeneous')
    try:
        inst = types_set[0]()
    except:
        raise TypeError('cannot construct instance of input type')
    return _cpp_type_catcher(_transformation_is_canonical, new_p, new_q, p_list, q_list, types_set[0]())


def truncate_degree(arg, max_degree, names=None):
    """Degree-based truncation.

    This function will eliminate from *arg* parts whose degree is greater than *max_degree*. The truncation
    operates on symbolic types both by eliminating entire terms and by truncating recursively the coefficients,
    if possible.

    If *names* is ``None``, then the total degree is considered for the truncation. Otherwise, *names* must be
    a list of strings enumerating the variables to be considered for the computation of the degree.

    :param arg: argument for the truncation
    :type arg: a symbolic type
    :param max_degree: maximum degree that will be kept in *arg*
    :type max_degree: a type comparable to the type representing the degree of *arg*
    :param names: list of the names of the variables to be considered in the computation of the degree
    :type names: ``None`` or a list of strings
    :returns: the truncated counterpart of *arg*
    :raises: :exc:`TypeError` if *names* is neither ``None`` nor a list of strings
    :raises: any exception raised by the invoked low-level function

    >>> from pyranha.types import polynomial, rational, k_monomial, poisson_series
    >>> pt = polynomial[rational,k_monomial]()
    >>> x,y,z = pt('x'), pt('y'), pt('z')
    >>> truncate_degree(x**2*y+x*y+z,2) # doctest: +SKIP
    x*y+z
    >>> truncate_degree(x**2*y+x*y+z,0,['x'])
    z
    >>> pt = poisson_series[polynomial[rational,k_monomial]]()
    >>> x,y,z,a,b = pt('x'), pt('y'), pt('z'), pt('a'), pt('b')
    >>> truncate_degree((x+y*x+x**2*z)*cos(a+b)+(y-y*z+x**4)*sin(2*a+b),3) # doctest: +SKIP
    (x+x*y+x**2*z)*cos(a+b)+(-y*z+y)*sin(2*a+b)
    >>> truncate_degree((x+y*x+x**2*z)*cos(a+b)+(y-y*z+x**4)*sin(2*a+b),1,['x','y']) # doctest: +SKIP
    x*cos(a+b)+(-y*z+y)*sin(2*a+b)
    >>> truncate_degree((x+y*x+x**2*z)*cos(a+b)+(y-y*z+x**4)*sin(2*a+b),1,'x') # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: the optional 'names' argument must be a list of strings
    >>> truncate_degree((x+y*x+x**2*z)*cos(a+b)+(y-y*z+x**4)*sin(2*a+b),1,['x',1]) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: the optional 'names' argument must be a list of strings

    """
    from ._core import _truncate_degree
    __check_names_argument(names)
    if names is None:
        return _cpp_type_catcher(_truncate_degree, arg, max_degree)
    else:
        return _cpp_type_catcher(_truncate_degree, arg, max_degree, names)


def evaluate(arg, eval_dict):
    """Evaluation.

    This function will evaluate *arg* according to the evaluation dictionary *eval_dict*. Evaluation
    is the replacement of all symbolic quantities with numerical values.

    *eval_dict* must be a dictionary of ``(name,value)`` pairs, where the ``name`` is a string referring to the symbol
    to be replaced and ``value`` is the value with which ``name`` will be replaced. All values must be of the same type,
    and this type needs to support the operations needed to compute the evaluation.

    :param arg: argument for the evaluation
    :type arg: a symbolic type
    :param eval_dict: evaluation dictionary
    :type eval_dict: a dictionary mapping strings to values, with all values of the same type
    :returns: the evaluation of *arg* according to *eval_dict*
    :raises: :exc:`TypeError` if *eval_dict* does not satisfy the requirements outlined above
    :raises: :exc:`ValueError` if *eval_dict* is empty
    :raises: any exception raised by the invoked low-level function

    >>> from pyranha.types import polynomial, rational, k_monomial
    >>> pt = polynomial[rational,k_monomial]()
    >>> x,y,z = pt('x'), pt('y'), pt('z')
    >>> evaluate(x*y+4*(y/4)**2*z,{'x':3,'y':-3,'z':5})
    Fraction(9, 4)
    >>> evaluate(x*y+4*(y/4)**2*z,{'x':3.01,'y':-3.32,'z':5.34}) # doctest: +ELLIPSIS
    4.7217...
    >>> evaluate(x*y+4*(y/4)**2*z,{'x':3.1,'y':-3.2,'z':5}) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: all values in the evaluation dictionary must be of the same type
    >>> evaluate(x*y+4*(y/4)**2*z,{}) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    ValueError: evaluation dictionary cannot be empty
    >>> evaluate(x*y+4*(y/4)**2*z,{'x':3,'y':-3,5:5}) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: all keys in the evaluation dictionary must be string objects
    >>> evaluate(x*y+4*(y/4)**2*z,[1,2,3]) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: evaluation dictionary must be a dict object
    >>> evaluate(x*y+4*(y/4)**2*z,{'x':3,'y':-3}) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    ValueError: invalid positions map for evaluation
    >>> evaluate(x*y+4*(y/4)**2*z,{'x':'a','y':'b', 'z':'c'}) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: invalid argument type(s)

    """
    from ._core import _evaluate
    # Check input dict.
    __check_eval_dict(eval_dict)
    return _cpp_type_catcher(_evaluate, arg, eval_dict, eval_dict[list(eval_dict.keys())[0]])


def subs(arg, name, x):
    """Substitution.

    This function will replace, in *arg*, the symbol called *name* with the generic object *x*.
    Internally, the functionality is implemented via a call to a lower-level C++ routine that
    supports various combinations for the types of *arg* and *x*.

    :param arg: argument for the substitution
    :type arg: a symbolic type
    :param name: name of the symbol to be substituted
    :type name: a string
    :param x: the quantity that will be substituted for *name*
    :type x: any type supported by the low-level C++ routine
    :returns: *arg* after the substitution of the symbol called *name* with *x*
    :raises: :exc:`TypeError` in case the input types are not supported or invalid
    :raises: any exception raised by the invoked low-level function

    >>> from pyranha.types import polynomial, rational, k_monomial
    >>> pt = polynomial[rational,k_monomial]()
    >>> x,y = pt('x'), pt('y')
    >>> subs(x/2+1,'x',3)
    5/2
    >>> subs(x/2+1,'x',y*2) == y + 1
    True
    >>> subs(x/2+1,3,y*2)  # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: invalid argument type(s)

    """
    from ._core import _subs
    return _cpp_type_catcher(_subs, arg, name, x)


def t_subs(arg, name, x, y):
    """Trigonometric substitution.

    This function will replace, in *arg*, the cosine and sine of the symbol called *name* with
    the generic objects *x* and *y*. Internally, the functionality is implemented via a call to
    a lower-level C++ routine that supports various combinations for the types of *arg*, *x* and *y*.

    :param arg: argument for the substitution
    :type arg: a symbolic type
    :param name: name of the symbol whose cosine and sine will be substituted
    :type name: a string
    :param x: the quantity that will be substituted for the cosine of *name*
    :type x: any type supported by the low-level C++ routine
    :param y: the quantity that will be substituted for the sine of *name*
    :type y: any type supported by the low-level C++ routine
    :returns: *arg* after the substitution of the cosine and sine of the symbol called *name* with *x* and *y*
    :raises: :exc:`TypeError` in case the input types are not supported or invalid
    :raises: any exception raised by the invoked low-level function

    >>> from pyranha.types import poisson_series, rational, polynomial, int16, monomial
    >>> pt = poisson_series[polynomial[rational,monomial[int16]]]()
    >>> x,y = pt('x'), pt('y')
    >>> t_subs(cos(x+y),'x',1,0)
    cos(y)
    >>> t_subs(sin(2*x+y),'x',0,1)
    -sin(y)
    >>> t_subs(sin(2*x+y),0,0,1)  # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: invalid argument type(s)

    """
    from ._core import _t_subs
    return _cpp_type_catcher(_t_subs, arg, name, x, y)


def ipow_subs(arg, name, n, x):
    """Integral power substitution.

    This function will replace, in *arg*, the *n*-th power of the symbol called *name* with
    the generic object *x*. Internally, the functionality is implemented via a call to
    a lower-level C++ routine that supports various combinations for the types of *arg* and *x*.

    :param arg: argument for the substitution
    :type arg: a symbolic type
    :param name: name of the symbol whose power will be substituted
    :type name: a string
    :param n: power of *name* that will be substituted
    :type n: an integer
    :param x: the quantity that will be substituted for the power of *name*
    :type x: any type supported by the low-level C++ routine
    :returns: *arg* after the substitution of *n*-th power of the symbol called *name* with *x*
    :raises: :exc:`TypeError` in case the input types are not supported or invalid
    :raises: any exception raised by the invoked low-level function

    >>> from pyranha.types import rational, polynomial, int16, monomial
    >>> pt = polynomial[rational,monomial[int16]]()
    >>> x,y,z = pt('x'), pt('y'), pt('z')
    >>> ipow_subs(x**5*y,'x',2,z)
    x*y*z**2
    >>> ipow_subs(x**6*y,'x',2,z**-1)
    y*z**-3
    >>> ipow_subs(x**6*y,0,2,z**-1)  # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: invalid argument type(s)
    >>> ipow_subs(x**6*y,'x',2.1,z**-1)  # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: invalid argument type(s)

    """
    from ._core import _ipow_subs
    return _cpp_type_catcher(_ipow_subs, arg, name, n, x)


def invert(arg):
    """Inverse.

    Compute the multiplicative inverse of the argument.
    The supported types are ``int``, ``float``, ``Fraction``, ``mpf`` and any symbolic type that supports
    the operation.

    :param arg: inversion argument
    :type arg: ``int``, ``float``, ``Fraction``, ``mpf``, or a supported symbolic type.
    :returns: inverse of *arg*
    :raises: :exc:`TypeError` if the type of *arg* is not supported, or any other exception raised by the invoked
            low-level function

    >>> from fractions import Fraction as F
    >>> invert(F(1,2))
    Fraction(2, 1)
    >>> invert(1.23) # doctest: +ELLIPSIS
    0.8130081...
    >>> invert(0) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    ZeroDivisionError: division by zero
    >>> from pyranha.types import polynomial, rational, int16, monomial, divisor, divisor_series
    >>> t = polynomial[rational,monomial[int16]]()
    >>> invert(t('x'))
    x**-1
    >>> t = divisor_series[polynomial[rational,monomial[int16]],divisor[int16]]()
    >>> invert(-2*t('x')+8*t('y'))
    -1/2*1/[(x-4*y)]
    >>> invert(t('x')+1) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    ValueError: invalid argument for series exponentiation: negative integral value
    >>> invert('hello') # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: invalid argument type(s)

    """
    from ._core import _invert
    return _cpp_type_catcher(_invert, arg)


def degree(arg, names=None):
    """Degree.

    This function will return the degree of the input argument. Various symbolic types are supported as input
    (e.g., polynomials, Poisson series, etc.). If *names* is ``None``, then the total degree is
    returned. Otherwise, *names* must be a list of strings enumerating the variables to be considered for the
    computation of the partial degree.

    :param arg: argument whose degree will be returned
    :type arg: a supported symbolic type.
    :param names: list of the names of the variables to be considered in the computation of the degree
    :type names: ``None`` or a list of strings
    :returns: the degree of *arg*
    :raises: :exc:`TypeError` if the type of *arg* is not supported, or any other exception raised by the invoked
            low-level function

    >>> from pyranha.types import polynomial, rational, k_monomial
    >>> t = polynomial[rational,k_monomial]()
    >>> x,y,z = t('x'),t('y'),t('z')
    >>> degree(x**3+y*z) == 3
    True
    >>> degree(x**3+y*z,['z']) == 1
    True
    >>> from fractions import Fraction as F
    >>> degree(F(1,2)) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: invalid argument type(s)

    """
    from ._core import _degree
    __check_names_argument(names)
    if names is None:
        return _cpp_type_catcher(_degree, arg)
    else:
        return _cpp_type_catcher(_degree, arg, names)


def ldegree(arg, names=None):
    """Low degree.

    This function will return the low degree of the input argument. Various symbolic types are supported as input
    (e.g., polynomials, Poisson series, etc.). If *names* is ``None``, then the total low degree is
    returned. Otherwise, *names* must be a list of strings enumerating the variables to be considered for the
    computation of the partial low degree.

    :param arg: argument whose low degree will be returned
    :type arg: a supported symbolic type.
    :param names: list of the names of the variables to be considered in the computation of the low degree
    :type names: ``None`` or a list of strings
    :returns: the low degree of *arg*
    :raises: :exc:`TypeError` if the type of *arg* is not supported, or any other exception raised by the invoked
            low-level function

    >>> from pyranha.types import polynomial, rational, k_monomial
    >>> t = polynomial[rational,k_monomial]()
    >>> x,y,z = t('x'),t('y'),t('z')
    >>> ldegree(x**3+y*z) == 2
    True
    >>> ldegree(x**3+y*x,['x']) == 1
    True
    >>> from fractions import Fraction as F
    >>> ldegree(F(1,2)) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: invalid argument type(s)

    """
    from ._core import _ldegree
    __check_names_argument(names)
    if names is None:
        return _cpp_type_catcher(_ldegree, arg)
    else:
        return _cpp_type_catcher(_ldegree, arg, names)


def t_degree(arg, names=None):
    """Trigonometric degree.

    This function will return the trigonometric degree of the input argument. Various symbolic types are supported as
    input. If *names* is ``None``, then the total degree is returned. Otherwise, *names* must be a list of strings
    enumerating the variables to be considered for the computation of the partial degree.

    :param arg: argument whose trigonometric degree will be returned
    :type arg: a supported symbolic type.
    :param names: list of the names of the variables to be considered in the computation of the degree
    :type names: ``None`` or a list of strings
    :returns: the trigonometric degree of *arg*
    :raises: :exc:`TypeError` if the type of *arg* is not supported, or any other exception raised by the invoked
            low-level function

    >>> from pyranha.types import poisson_series, polynomial, rational, k_monomial
    >>> from pyranha.math import cos
    >>> t = poisson_series[polynomial[rational,k_monomial]]()
    >>> x,y,z = t('x'),t('y'),t('z')
    >>> t_degree(cos(3*x+y-z)+cos(2*x)) == 3
    True
    >>> t_degree(cos(3*x+y+z)+cos(x),['z']) == 1
    True
    >>> from fractions import Fraction as F
    >>> t_degree(F(1,2)) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: invalid argument type(s)

    """
    from ._core import _t_degree
    __check_names_argument(names)
    if names is None:
        return _cpp_type_catcher(_t_degree, arg)
    else:
        return _cpp_type_catcher(_t_degree, arg, names)


def t_ldegree(arg, names=None):
    """Trigonometric low degree.

    This function will return the trigonometric low degree of the input argument. Various symbolic types are supported as
    input. If *names* is ``None``, then the total low degree is returned. Otherwise, *names* must be a list of strings
    enumerating the variables to be considered for the computation of the partial low degree.

    :param arg: argument whose trigonometric low degree will be returned
    :type arg: a supported symbolic type.
    :param names: list of the names of the variables to be considered in the computation of the low degree
    :type names: ``None`` or a list of strings
    :returns: the trigonometric low degree of *arg*
    :raises: :exc:`TypeError` if the type of *arg* is not supported, or any other exception raised by the invoked
            low-level function

    >>> from pyranha.types import poisson_series, polynomial, rational, k_monomial
    >>> from pyranha.math import cos
    >>> t = poisson_series[polynomial[rational,k_monomial]]()
    >>> x,y,z = t('x'),t('y'),t('z')
    >>> t_ldegree(cos(3*x+y-z)+cos(2*x)) == 2
    True
    >>> t_ldegree(cos(3*x+y-z)+cos(2*y+z),['y']) == 1
    True
    >>> from fractions import Fraction as F
    >>> t_ldegree(F(1,2)) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: invalid argument type(s)

    """
    from ._core import _t_ldegree
    __check_names_argument(names)
    if names is None:
        return _cpp_type_catcher(_t_ldegree, arg)
    else:
        return _cpp_type_catcher(_t_ldegree, arg, names)


def t_order(arg, names=None):
    """Trigonometric order.

    This function will return the trigonometric order of the input argument. Various symbolic types are supported as
    input. If *names* is ``None``, then the total order is returned. Otherwise, *names* must be a list of strings
    enumerating the variables to be considered for the computation of the partial order.

    :param arg: argument whose trigonometric order will be returned
    :type arg: a supported symbolic type.
    :param names: list of the names of the variables to be considered in the computation of the order
    :type names: ``None`` or a list of strings
    :returns: the order of *arg*
    :raises: :exc:`TypeError` if the type of *arg* is not supported, or any other exception raised by the invoked
            low-level function

    >>> from pyranha.types import poisson_series, polynomial, rational, k_monomial
    >>> from pyranha.math import cos
    >>> t = poisson_series[polynomial[rational,k_monomial]]()
    >>> x,y,z = t('x'),t('y'),t('z')
    >>> t_order(cos(3*x+y-z)+cos(x)) == 5
    True
    >>> t_order(cos(3*x+y-z)-sin(y),['z']) == 1
    True
    >>> from fractions import Fraction as F
    >>> t_order(F(1,2)) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: invalid argument type(s)

    """
    from ._core import _t_order
    __check_names_argument(names)
    if names is None:
        return _cpp_type_catcher(_t_order, arg)
    else:
        return _cpp_type_catcher(_t_order, arg, names)


def t_lorder(arg, names=None):
    """Trigonometric low order.

    This function will return the trigonometric low order of the input argument. Various symbolic types are supported as
    input. If *names* is ``None``, then the total low order is returned. Otherwise, *names* must be a list of strings
    enumerating the variables to be considered for the computation of the partial low order.

    :param arg: argument whose trigonometric low order will be returned
    :type arg: a supported symbolic type.
    :param names: list of the names of the variables to be considered in the computation of the low order
    :type names: ``None`` or a list of strings
    :returns: the trigonometric low order of *arg*
    :raises: :exc:`TypeError` if the type of *arg* is not supported, or any other exception raised by the invoked
            low-level function

    >>> from pyranha.types import poisson_series, polynomial, rational, k_monomial
    >>> from pyranha.math import cos
    >>> t = poisson_series[polynomial[rational,k_monomial]]()
    >>> x,y,z = t('x'),t('y'),t('z')
    >>> t_lorder(cos(4*x+y-z)+cos(2*x)) == 2
    True
    >>> t_lorder(cos(3*x+y-z)+cos(2*y+z),['y']) == 1
    True
    >>> from fractions import Fraction as F
    >>> t_lorder(F(1,2)) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: invalid argument type(s)

    """
    from ._core import _t_lorder
    __check_names_argument(names)
    if names is None:
        return _cpp_type_catcher(_t_lorder, arg)
    else:
        return _cpp_type_catcher(_t_lorder, arg, names)


def lambdify(t, x, names, extra_map={}):
    """Turn a symbolic object into a function.

    This function is a wrapper around :func:`~pyranha.math.evaluate()` which returns a callable object that can be used
    to evaluate the input argument *x*. The call operator of the returned object takes as input a collection of length
    ``len(names)`` of objects of type *t* which, for the purpose of evaluation, are associated to the list of
    symbols *names* at the corresponding positions.

    The optional argument *extra_map* is a dictionary that maps symbol names to callables, and it used to specify
    what values to assign to specific symbols based on the values of the symbols in *names*. For instance, suppose
    that :math:`z` depends on :math:`x` and :math:`y` via :math:`z\\left(x,y\\right)=\\sqrt{3x+y}`. We can then evaluate
    the expression :math:`x+y+z` as follows:

    >>> from pyranha.types import polynomial, rational, k_monomial
    >>> pt = polynomial[rational,k_monomial]()
    >>> x,y,z = x,y,z = pt('x'),pt('y'),pt('z')
    >>> l = lambdify(float,x+y+z,['x','y'],{'z': lambda a: (3.*a[0] + a[1])**.5})
    >>> l([1.,2.]) # doctest: +ELLIPSIS
    5.236067977...

    The output value is :math:`1+2+\\sqrt{5}`.

    :param t: the type that will be used for the evaluation of *x*
    :type t: a supported evaluation type
    :param x: symbolic object that will be evaluated
    :type x: a supported symbolic type
    :param names: the symbols that will be used for evaluation
    :type names: a list of strings
    :param extra_map: a dictionary mapping symbol names to custom evaluation functions
    :type extra_map: a dictionary in which the keys are strings and the values are callables
    :returns: a callable object that can be used to evaluate *x* with objects of type *t*
    :rtype: the type returned by the invoked low-level function
    :raises: :exc:`TypeError` if *t* is not a type, or if *names* is not a list of strings, or if *extra_map* is not
            a dictionary of the specified type
    :raises: :exc:`ValueError` if *names* contains duplicates or if *extra_map* contains strings already present in *names*
    :raises: :exc:`unspecified` any exception raised by the invoked low-level function or by the invoked
            callables in *extra_map*

    >>> from pyranha.types import polynomial, rational, k_monomial
    >>> pt = polynomial[rational,k_monomial]()
    >>> x,y,z = x,y,z = pt('x'),pt('y'),pt('z')
    >>> l = lambdify(int,2*x-y+3*z,['z','y','x'])
    >>> l([1,2,3])
    Fraction(7, 1)
    >>> l([1,2,-3])
    Fraction(-5, 1)
    >>> l = lambdify(float,x+y+z,['x','y'],{'z': lambda a: (3.*a[0] + a[1])**.5})
    >>> l([1.,2.]) # doctest: +ELLIPSIS
    5.236067977...
    >>> l([1]) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    ValueError: invalid size of the evaluation vector
    >>> l = lambdify(3,2*x-y+3*z,['z','y','x']) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: the 't' argument must be a type
    >>> l = lambdify(int,2*x-y+3*z,[1,2,3]) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    TypeError: the 'names' argument must be a list of strings
    >>> l = lambdify(int,2*x-y+3*z,['z','y','x','z']) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    ValueError: the 'names' argument contains duplicates
    >>> l = lambdify(float,x+y+z,['x','y'],{'z': 123})
    Traceback (most recent call last):
       ...
    TypeError: all the values in the 'extra_map' argument must be callables

    """
    from ._core import _lambdify
    if not isinstance(t, type):
        raise TypeError('the \'t\' argument must be a type')
    if not isinstance(names, list) or not all([isinstance(_, str) for _ in names]):
        raise TypeError('the \'names\' argument must be a list of strings')
    if not all([isinstance(_, str) for _ in extra_map]):
        raise TypeError(
            'the \'extra_map\' argument must be a dictionary in which the keys are strings')
    if not all([callable(extra_map[_]) for _ in extra_map]):
        raise TypeError(
            'all the values in the \'extra_map\' argument must be callables')
    return _cpp_type_catcher(_lambdify, x, names, extra_map, t())
