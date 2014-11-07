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
`GMP library <http://www.gmplib.org/>`__. As an extra optimisation, for small numbers ``integer`` avoids
using dynamic memory allocation and can thus have better performance than a straightforward ``mpz_t``
wrapper.

``integer`` behaves much like a standard C++ integral type:

* it can be constructed from all the fundamental C++ types (construction from floating-point types
  results in the truncation of the original value),
* it can be converted to all the the fundamental C++ types (if the conversion to a C++ integral
  type overflows the target type, an error is thrown),
* the division operator performs truncated division,
* in mixed-mode operations, the rank of ``integer`` is higher than any other C++ integral type but still
  lower than floating-point types (e.g., ``integer + long`` results in an ``integer``, ``integer + float``
  results in a ``float``).

The following C++ code illustrates some features of the ``integer`` type:

.. literalinclude:: ../../tutorial/integer.cpp
   :language: c++
   :linenos:

Line 38 highlights a couple of handy C++11 features. The snippet ``42_z`` is a *user-defined literal*. That is,
it is equivalent to constructing an ``integer`` object with the string ``"42"``. The constructed temporary object
is then assigned to a variable ``n`` in the statement ``auto n = 42_z``. The type of ``n`` is automatically
deduced via the keyword ``auto`` from the right-hand side of the assignment, thus ``n`` is an ``integer`` object.
