.. _math_gcd:

Greatest common divisor
=======================

*#include <piranha/math/gcd.hpp>*

.. cpp:function:: template <typename T, typename U> auto piranha::gcd(T &&x, U &&y)

   This function computes the greatest common divisor (GCD) of *x* and *y*.

   The implementation is delegated to the call operator of the :cpp:class:`piranha::gcd_impl` function object.
   The body of this function is equivalent to:

   .. code-block:: c++

      return piranha::gcd_impl<Tp, Up>{}(x, y);

   where ``Tp`` and ``Up`` are ``T`` and ``U`` after the removal of reference and cv-qualifiers,
   and *x* and *y* are perfectly forwarded to the call operator of :cpp:class:`piranha::gcd_impl`.
   If the expression above is invalid, or if it returns a type which does not satisfy the
   :cpp:concept:`piranha::Returnable` concept,
   then this function will be disabled (i.e., it will not participate in overload resolution).

   Piranha provides specialisations of :cpp:class:`piranha::gcd_impl` for the following types:

   * all of C++'s integral types,
   * all :cpp:class:`mppp::integer <mppp::integer>` types (including :cpp:type:`piranha::integer`).

   See the :ref:`implementation <math_gcd_impls>` section below for more details about the available
   specialisations.

   :param x: the first argument.
   :param y: the second argument.

   :return: the GCD of *x* and *y*.

   :exception unspecified: any exception thrown by the call operator of :cpp:class:`piranha::gcd_impl`.

Concepts
--------

.. cpp:concept:: template <typename T, typename U = T> piranha::GcdTypes

   This concept is satisfied if :cpp:func:`piranha::gcd()` can be called
   with arguments of type ``T`` and ``U``. Specifically, this concept will be satisfied if

   .. code-block:: c++

      piranha::gcd(x, y)

   is a valid expression, where ``x`` and ``y`` are references to const ``T`` and ``U`` respectively.

.. _math_gcd_impls:

Implementations
---------------

.. cpp:class:: template <typename T, typename U> piranha::gcd_impl

   Unspecialised version of the function object implementing :cpp:func:`piranha::gcd()`.

   This default implementation does not define any call operator, and thus no default implementation
   of :cpp:func:`piranha::gcd()` is available.

.. cpp:class:: template <CppIntegral T, CppIntegral U> piranha::gcd_impl<T, U>

   Specialisation of the function object implementing :cpp:func:`piranha::gcd()` for C++'s integral types.

   This implementation always returns non-negative values. The type of the result is the common type
   of ``T`` and ``U``.

   .. warning::
   
      It is the user's responsibility to ensure that the absolute values of the input arguments are representable
      by the common type of ``T`` and ``U``. If this is not the case, the behaviour is undefined.

.. cpp:class:: template <typename U, mppp::IntegerIntegralOpTypes<U> T> piranha::gcd_impl<T, U>

   Specialisation of the function object implementing :cpp:func:`piranha::gcd()` for :cpp:class:`mppp::integer <mppp::integer>`.

   This implementation will invoke one of mp++'s :ref:`integer gcd <mppp:integer_ntheory>` overloads.

   :exception unspecified: any exception thrown by the invoked :ref:`integer gcd <mppp:integer_ntheory>` overload.
