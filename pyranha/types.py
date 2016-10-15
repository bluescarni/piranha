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

from ._core import types as _t

#: This generator represents the standard C++ type ``float``.
float = _t.float

#: This generator represents the standard C++ type ``double``.
double = _t.double

#: This generator represents the standard C++ type ``long double``.
long_double = _t.long_double

try:
    #: This generator represents the 128-bit floating-point type from GCC's ``libquadmath``.
    float128 = _t.float128
except AttributeError:
    pass

#: This generator represents the standard C++ type ``signed char`` (a signed integer type
#: whose width is usually 8 bits).
signed_char = _t.signed_char

#: This generator represents the standard C++ type ``short`` (a signed integer type
#: whose width is usually 16 bits).
short = _t.short

#: This generator represents the arbitrary-precision integer type provided by the piranha C++ library.
integer = _t.integer

#: This generator represents the arbitrary-precision rational type provided by the piranha C++ library.
rational = _t.rational

#: This generator represents the multiprecision floating-point type provided by the piranha C++ library.
real = _t.real

#: This generator represents the Kronecker monomial type (that is, a monomial "compressed" into a single
#: signed integral value).
k_monomial = _t.k_monomial

#: Generic type generator for polynomials.
polynomial = _t.polynomial

#: Generic type generator for monomials.
monomial = _t.monomial

#: Generic type generator for Poisson series.
poisson_series = _t.poisson_series

#: Generic type generator for divisors.
divisor = _t.divisor

#: Generic type generator for divisor series.
divisor_series = _t.divisor_series

#: Generic type generator for rational functions.
rational_function = _t.rational_function
