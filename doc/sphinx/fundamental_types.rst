.. _fundamental_types:

Fundamental numerical types in Piranha
======================================

Any computer algebra system needs to be able to represent fundamental numerical types such as
integers, rationals, (approximations of) reals, etc. C++ and Python provide basic numerical types
that can be used with Piranha's generic algorithms and data structures. However, especially in
C++, the numerical types provided by the language - while having the advantage of providing
very high performance - can be unsuitable for certain applications (e.g., all the standard integral
types provided by C++ have a finite range).

On the C++ side Piranha provides a set of additional fundamental numerical types that can interoperate
with the C++ fundamental types and that can be used with Piranha's generic algorithms and data structures
(e.g., as coefficient types in a polynomial). On the Python side, Piranha's fundamental types are
automatically translated to/from corresponding Python types.

The ``integer`` type
--------------------

Piranha's ``integer`` type is used to represent signed integers of arbitrary size (that is, the range
of the type is limited only by the available memory). The type is based on the ``mpz_t`` type from the
`GMP library <http://www.gmplib.org/>`__. As an optimisation, for small numbers ``integer`` avoids
using dynamic memory allocation and can thus have better performance than a straightforward ``mpz_t``
wrapper.

``integer`` behaves much like a standard C++ integral type:

* a default-constructed ``integer`` object is initialised to zero;
* an ``integer`` object can be constructed from all the fundamental C++ types (construction from floating-point
  types results in the truncation of the original value);
* an ``integer`` object can be converted to all the the fundamental C++ types (if the conversion to a C++ integral
  type overflows the target type, an error is thrown);
* the division operator performs truncated division;
* in mixed-mode operations, the rank of ``integer`` is higher than any other C++ integral type but still
  lower than floating-point types (e.g., ``integer + long`` results in an ``integer``, ``integer + float``
  results in a ``float``).

The following C++ code illustrates some features of the ``integer`` type:

.. literalinclude:: ../../tutorial/integer.cpp
   :language: c++
   :linenos:

In the first lines, a few possible ways of constructing ``integer`` objects are shown, including a constructor
from string that allows to initialise an ``integer`` with arbitrarily large values. In the following lines,
some examples of arithmetics with basic C++ types are illustrated.

On line 28, we can see how the ``math::pow()`` function can be invoked in order to compute the exponentiation of
an ``integer``. C++ lacks a dedicated exponentiation operator and thus the functionality is provided by a function
instead. The ``math::pow()`` function is part of Piranha's mathematical library.

Line 38 highlights a couple of handy C++11 features. The snippet ``42_z`` is a *user-defined literal*: it results
in the construction of an ``integer`` object via the string ``"42"``. The constructed temporary object
is then assigned to a variable ``n`` in the statement ``auto n = 42_z``. The type of ``n`` is automatically
deduced via the keyword ``auto`` from the right-hand side of the assignment, thus ``n`` is an ``integer`` object.
This is arguably the C++ syntax that most closely matches Python's syntax.

On line 42, we use the ``math::binomial()`` function from the math library to compute the binomial coefficient
:math:`{42 \choose 21}`, passing the two arguments as ``integer`` objects created via the user-defined literal ``_z``.

As expected, on the Python side things look simpler:

.. _integer_py:

.. literalinclude:: ../../pyranha/tutorial/integer.py
   :language: python
   :linenos:

Python indeed provides multiprecision integers as basic types. Some notable differences exist between Python 2.x and
Python 3.x in this regard:

* Python 2.x has two different integral types, ``int`` and ``long``, representing small and large integers
  respectively. Python 3.x instead has a single integral type named ``int``. Piranha is able to deal with both
  cases;
* in Python 2.x, integer division is truncated and returns an integral; in Python 3.x, integral division is a true division
  returning a ``float``.

In the first lines of the :ref:`Python code <integer_py>`, a few ways of constructing an integral object are shown.
Contrary to C++, big integers can be constructed directly without passing through a string constructor. Additionally,
Python has a dedicated exponentiation operator called ``**``, thus it is not necessary to resort to a separate
function for this operation.

On the last line, we see the first real example of using a Piranha function from Python. The :py:func:`pyranha.math.binomial`
function will take Python integers as arguments, convert them to C++ ``integer`` objects and pass them to the C++ function
``math::binomial()``. The output value of the C++ function will be converted back to a Python integer which will then
be returned by the :py:func:`pyranha.math.binomial` function.

The ``rational`` type
---------------------

A second fundamental type provided by Piranha is the ``rational`` class, which represents arbitrary-precision
rational numbers as ``integer`` numerator-denominator pairs. The ``rational`` type in Piranha extends smoothly
the standard C++ numerical hierarchy, and it obeys the following basic rules:

* a default-constructed ``rational`` object is initialised to zero;
* a ``rational`` object is always kept in the usual canonical form consisting of coprime numerator and denominator, with
  the denominator always strictly positive. Zero is always represented as ``0/1``;
* a ``rational`` object can be converted to/from all the basic C++ numerical types and ``integer`` (the conversion to an integral
  type computes the truncated division of numerator and denominator);
* in mixed-mode operations, the rank of ``rational`` is higher than that of ``integer`` but lower than that of floating-point
  types.

The following C++ code showcases a few features of the ``rational`` class:

.. literalinclude:: ../../tutorial/rational.cpp
   :language: c++
   :linenos:

In the first code block, a few ways of constructing a ``rational`` are shown. In addition to the constructors from interoperable
types, a ``rational`` can also be constructed from an integral numerator-denominator pair. The string representation for a ``rational``
consists, intuitively, of the representation of numerator and denominator, separated by a ``/`` sign. The denominator can be
omitted if it is unitary.

In the second code block, some examples of arithmetic and logical operations involving ``rational`` and interoperable types are
displayed.

On lines 33-34, we use the ``math::pow()`` function to compute integral powers of a rational. ``math::pow()`` is able to accept
many different combinations of argument types. In this specific case, with a ``rational`` base and an integral exponent, the result
will be exact and its type will be ``rational``.

The ``real`` type
-----------------

Potential pitfalls
------------------

TLDR
----
