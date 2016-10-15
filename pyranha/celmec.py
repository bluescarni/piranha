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


def orbitalR(angles):
    """Orbital rotation matrix.

    The returned value is the composite rotation matrix from the orbital reference system to the reference system
    with respect to which the parameters of the orbit are defined. The rotation angles are the keplerian orbital elements
    :math:`\omega`, :math:`i` and :math:`\Omega`, and they must be passed as elements of a list-like container.

    If the input list contains 3 elements, they will be interpreted as the angles :math:`\omega`, :math:`i` and
    :math:`\Omega`. If the input list contains 6 elements, they will be interpreted as :math:`\cos\omega`, :math:`\sin\omega`,
    :math:`\cos i`, :math:`\sin i`, :math:`\cos\Omega` and 	:math:`\sin\Omega`. The functions from the :py:mod:`.math` module
    are used to compute sines and cosines as needed.

    The returned rotation matrix is:

    .. math:: \\begin{bmatrix}
       \cos\Omega\cos\omega-\sin\Omega\cos i \sin\omega &
       -\cos\Omega\sin\omega-\sin\Omega\cos i \cos\omega &
       \sin\Omega\sin i\\\\
       \sin\Omega\cos\omega+\cos\Omega\cos i \sin\omega &
       -\sin\Omega\sin\omega+\cos\Omega\cos i \cos\omega &
       -\cos\Omega\sin i\\\\
       \sin i \sin\omega &
       \sin i \cos\omega &
       \cos i
       \end{bmatrix}

    Note that this function requires the availability of the NumPy library: the rotation matrix will be returned as a
    NumPy array.

    :param angles: list of rotation angles
    :returns: orbital rotation matrix
    :raises: :exc:`ValueError` if the length of the input list is different from 3 or 6
    :raises: any exception raised by the invoked mathematical functions

    >>> orbitalR([.1,.2,.3]) # doctest: +SKIP
    array([[ 0.92164909, -0.38355704,  0.0587108 ],
           [ 0.3875172 ,  0.902113  , -0.18979606],
           [ 0.01983384,  0.19767681,  0.98006658]])
    >>> orbitalR([0,1,0,1,0,1]) # doctest: +SKIP
    array([[ 0,  0,  1],
           [ 0, -1,  0],
           [ 1,  0,  0]])
    >>> orbitalR([.1,.2,.3,.4]) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    ValueError: input list must contain either 3 or 6 elements

    """
    from numpy import array
    from .math import cos, sin
    l = len(angles)
    if l != 3 and l != 6:
        raise ValueError('input list must contain either 3 or 6 elements')
    if l == 3:
        omega, i, Omega = angles
        cos_omega, sin_omega = cos(omega), sin(omega)
        cos_i, sin_i = cos(i), sin(i)
        cos_Omega, sin_Omega = cos(Omega), sin(Omega)
    else:
        cos_omega, sin_omega, cos_i, sin_i, cos_Omega, sin_Omega = angles
    return array([
        [cos_Omega * cos_omega - sin_Omega * cos_i * sin_omega,
         -cos_Omega * sin_omega - sin_Omega * cos_i * cos_omega,
         sin_Omega * sin_i],
        [sin_Omega * cos_omega + cos_Omega * cos_i * sin_omega,
         -sin_Omega * sin_omega + cos_Omega * cos_i * cos_omega,
         -cos_Omega * sin_i],
        [sin_i * sin_omega,
         sin_i * cos_omega,
         cos_i]
    ])
