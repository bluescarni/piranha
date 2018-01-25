.. _math_pow:

Exponentiation
==============

*#include <piranha/pow.hpp>*

.. doxygenfunction:: piranha::math::pow()

.. cpp:ns:: mppp

   DSDDAS

Concepts
--------

.. cpp:concept:: template <typename T, typename U> piranha::Exponentiable

   This concept is satisfied if :cpp:func:`piranha::math::pow()` can be called
   with a base of type ``T`` and an exponent of type ``U``. Specifically,
   this concept will be satisfied if 

Implementations
---------------

.. cpp:class:: template <typename T, typename U> piranha::math::pow_impl

.. cpp:class:: template <typename T, piranha::Exponentiable<T> U> piranha::math::pow_impl<T, U>

   Batuba!

