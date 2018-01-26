.. _numerical_types:

Fundamental numerical types
===========================

.. cpp:type:: piranha::integer = mppp::integer<1>

   *#include <piranha/integer.hpp>*

   This type is the main multiprecision integer type used throughout piranha.
   It is an :cpp:class:`mppp::integer <mppp::integer>` with a static size of 1 limb.

.. cpp:type:: piranha::rational = mppp::rational<1>

   *#include <piranha/rational.hpp>*

   This type is the main multiprecision rational type used throughout piranha.
   It is an :cpp:class:`mppp::rational <mppp::rational>` with a static size of 1 limb.

.. cpp:type:: piranha::real = mppp::real

   *#include <piranha/real.hpp>*

   This type can be used to represent multiprecision floating-point values. It is an alias
   for :cpp:class:`mppp::real <mppp::real>`.

   .. note::

      This type is available only if mp++ was configured with the ``MPPP_WITH_MPFR`` option enabled
      (see the :ref:`mp++ installation instructions <mppp:installation>`).
