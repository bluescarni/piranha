.. _math_gcd3:

Greatest common divisor (ternary version)
=========================================

*#include <piranha/math/gcd3.hpp>*

.. cpp:function:: template <piranha::NonConst T, typename U, typename V> void piranha::gcd3(T &x, U &&y, V &&z)

   This function writes into *x* the greatest common divisor (GCD) of *y* and *z*.

   The implementation is delegated to the call operator of the :cpp:class:`piranha::gcd3_impl` function object.
   The body of this function is equivalent to:

   .. code-block:: c++

      piranha::gcd3_impl<T, Up, Vp>{}(x, y, z);

   where ``Up`` and ``Vp`` are ``U`` and ``V`` after the removal of reference and cv-qualifiers,
   and *y* and *z* are perfectly forwarded to the call operator of :cpp:class:`piranha::gcd3_impl`.
   If the expression above is invalid, then this function will be disabled (i.e., it will not
   participate in overload resolution).

   Piranha provides specialisations of :cpp:class:`piranha::gcd3_impl` for the following types:

   * any type implementing :cpp:func:`piranha::gcd()` (the default implementation),
   * all :cpp:class:`mppp::integer <mppp::integer>` types (including :cpp:type:`piranha::integer`).

   See the :ref:`implementation <math_gcd3_impls>` section below for more details about the available
   specialisations.

   :param x: the return value.
   :param y: the first argument.
   :param z: the second argument.

   :exception unspecified: any exception thrown by the call operator of :cpp:class:`piranha::gcd3_impl`.

Concepts
--------

.. cpp:concept:: template <typename T, typename U = T, typename V = T> piranha::Gcd3Types

   This concept is satisfied if :cpp:func:`piranha::gcd3()` can be called
   with arguments of type ``T``, ``U`` and ``V``. Specifically, this concept will be satisfied if

   .. code-block:: c++

      piranha::gcd3(x, y, z)

   is a valid expression, where ``x`` is an lvalue reference to ``T`` and ``y`` and ``z`` are
   references to const ``U`` and ``V`` respectively. If ``T`` is an rvalue reference, this concept
   is never satisfied.

.. _math_gcd3_impls:

Implementations
---------------

.. cpp:class:: template <typename T, typename U, typename V> piranha::gcd3_impl

   Unspecialised version of the function object implementing :cpp:func:`piranha::gcd3()`.

   This default implementation defines a call operator whose body is equivalent to:

   .. code-block:: c++

      x = piranha::gcd(y, z);
   
   That is, the input arguments *y* and *z* are perfectly forwarded to :cpp:func:`piranha::gcd()`,
   and the result is then assigned to *x*. If the above expression is not valid, the call operator
   is disabled (i.e., it will not participate in overload resolution), and no default implementation
   of :cpp:func:`piranha::gcd3()` is available.

.. cpp:class:: template <std::size_t SSize> piranha::gcd3_impl<mppp::integer<SSize>, mppp::integer<SSize>, mppp::integer<SSize>>

   Specialisation of the function object implementing :cpp:func:`piranha::gcd3()` for
   :cpp:class:`mppp::integer <mppp::integer>`.

   This implementation will invoke mp++'s :ref:`ternary integer gcd <mppp:integer_ntheory>` overload.

   :exception unspecified: any exception thrown by mp++'s :ref:`ternary integer gcd <mppp:integer_ntheory>` overload.
