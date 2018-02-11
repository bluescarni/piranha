.. _numerical_types:

Fundamental numerical types
===========================

Integers
--------

*#include <piranha/integer.hpp>*

.. cpp:type:: piranha::integer = mppp::integer<1>

   This type is the main multiprecision integer type used throughout piranha.
   It is an :cpp:class:`mppp::integer <mppp::integer>` with a static size of 1 limb.

.. cpp:function:: piranha::integer piranha::literals::operator"" _z(const char *s)

   This user-defined literal creates a :cpp:type:`piranha::integer` from the input string *s*,
   which will be interpreted as the base-10 representation of an integral value.

   :param s: the input string.

   :return: a :cpp:type:`piranha::integer` constructed from *s*.

   :exception unspecified: any exception thrown by the constructor of :cpp:type:`piranha::integer`
     from string.

Rationals
---------

*#include <piranha/rational.hpp>*

.. cpp:type:: piranha::rational = mppp::rational<1>

   This type is the main multiprecision rational type used throughout piranha.
   It is an :cpp:class:`mppp::rational <mppp::rational>` with a static size of 1 limb.

.. cpp:function:: piranha::rational piranha::literals::operator"" _q(const char *s)

   This user-defined literal creates a :cpp:type:`piranha::rational` from the input string *s*,
   which will be interpreted as the base-10 representation of a rational value. The accepted
   input formats are described in the documentation of the constructor of :cpp:type:`piranha::rational`
   from string.

   :param s: the input string.

   :return: a :cpp:type:`piranha::rational` constructed from *s*.

   :exception unspecified: any exception thrown by the constructor of :cpp:type:`piranha::rational`
     from string.

Multiprecision floats
---------------------

.. note::

   Multiprecision floats are available only if mp++ was configured with the ``MPPP_WITH_MPFR`` option enabled
   (see the :ref:`mp++ installation instructions <mppp:installation>`).

*#include <piranha/real.hpp>*

.. cpp:type:: piranha::real = mppp::real

   This type can be used to represent multiprecision floating-point values. It is an alias
   for :cpp:class:`mppp::real <mppp::real>`.
