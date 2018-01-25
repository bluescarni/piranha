.. _math_pow:

Exponentiation
==============

*#include <piranha/pow.hpp>*

.. doxygenfunction:: piranha::math::pow()

.. cpp:namespace:: piranha

.. cpp:class:: template <typename T, Exponentiable<T> U> math::pow_impl<T, U>

   Batuba!

   .. cpp:function:: std::size_t size() const
      
      dsadsa

      :exception TypeError: if the message_body is not a basestring

Concepts
--------

.. cpp:concept:: template <typename T, typename U> Exponentiable

   This concept is satisfied if :cpp:func:`math::pow()` can be called
   with a base of type ``T`` and an exponent of type ``U``. Specifically,
   this concept will be satisfied if 

Implementations
---------------

.. doxygengroup:: math_pow_impl
   :content-only:
   :members:
