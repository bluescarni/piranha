.. _safe_convert:

Safe conversion
===============

*#include <piranha/safe_convert.hpp>*

.. cpp:function:: template <typename To> bool piranha::safe_convert(To &&x, piranha::SafelyConvertible<To> &&y)

   This function attempts to set *x* to the exact value of *y*, possibly performing a type conversion. If the value of *y*
   can be represented exactly by the type of *x*, then this function will write the value of *y* into *x* and return ``true``. Otherwise,
   ``false`` will be returned and the value of *x* will be unspecified.

   The implementation is delegated to the call operator of the :cpp:class:`piranha::safe_convert_impl` function object.
   The body of this function is equivalent to:

   .. code-block:: c++

      return piranha::safe_convert_impl<Tp, Up>{}(x, y);

   where ``Tp`` and ``Up`` are the types of *x* and *y* after the removal of reference and cv-qualifiers,
   and *x* and *y* are perfectly forwarded to the call operator of :cpp:class:`piranha::safe_convert_impl`.

Concepts
--------

.. cpp:concept:: template <typename From, typename To> piranha::SafelyConvertible

   This concept is satisfied if :cpp:func:`piranha::safe_convert()` can be used to convert
   an instance of type ``From`` to the type ``To``.
