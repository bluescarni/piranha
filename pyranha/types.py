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

#: This type generator represents the standard C++ type ``double``.
double = _t.double

#: This type generator represents a signed integral C++ type whose width is at least 16 bits (it will
#: be exactly 16 bits on most platforms).
int16 = _t.int16

#: This type generator represents the arbitrary-precision integer type provided by the piranha C++ library.
integer = _t.integer

#: This type generator represents the arbitrary-precision rational type provided by the piranha C++ library.
rational = _t.rational

#: This type generator represents the multiprecision floating-point type provided by the piranha C++ library.
real = _t.real

#: This type generator represents the Kronecker monomial type (that is, a monomial "compressed" into a single
#: signed integral value).
k_monomial = _t.k_monomial

#: Type generator template for polynomials.
polynomial = _t.polynomial

#: Type generator template for monomials.
monomial = _t.monomial

#: Type generator template for Poisson series.
poisson_series = _t.poisson_series

#: Type generator template for divisors.
divisor = _t.divisor

#: Type generator template for divisor series.
divisor_series = _t.divisor_series
