.. _math_is_zero:

Detect zero
===========

*#include <piranha/math/is_zero.hpp>*

.. cpp:function:: template <typename T> bool piranha::math::is_zero(const T &x)

   This function returns ``true`` if ``x`` is equal to zero, ``false`` otherwise.

   The implementation is delegated to the call operator of the :cpp:class:`piranha::math::is_zero_impl` function object.
   The body of this function is equivalent to:

   .. code-block:: c++

      return piranha::math::is_zero_impl<T>{}(x);

   If the expression above is invalid, or if it returns a type which is not
   :cpp:concept:`convertible <piranha::Convertible>` to ``bool``,
   then this function will be disabled (i.e., it will not participate in overload resolution).

   Piranha provides specialisations of :cpp:class:`piranha::math::is_zero_impl` for the following types:

   * any type for which the expression ``x == T(0)`` is valid,
   * all C++ complex types,
   * all :cpp:class:`mppp::integer <mppp::integer>` types (including :cpp:type:`piranha::integer`),
   * all :cpp:class:`mppp::rational <mppp::rational>` types (including :cpp:type:`piranha::rational`),
   * :cpp:class:`mppp::real <mppp::real>`.

   See the :ref:`implementation <math_is_zero_impls>` section below for more details about the available
   specialisations.

   :param x: the input argument.

   :return: ``true`` if ``x`` is zero, ``false`` otherwise.

   :exception unspecified: any exception thrown by the call operator of :cpp:class:`piranha::math::is_zero_impl`.

Concepts
--------

.. cpp:concept:: template <typename T> piranha::IsZeroType

   This concept is satisfied if :cpp:func:`piranha::math::is_zero()` can be called
   with an argument of type ``T``. Specifically, this concept will be satisfied if

   .. code-block:: c++

      piranha::math::is_zero(x)

   is a valid expression, where ``x`` is a reference to const ``T``.

.. _math_is_zero_impls:

Implementations
---------------

.. cpp:class:: template <typename T, typename Enable = void> piranha::math::is_zero_impl

   Unspecialised version of the function object implementing :cpp:func:`piranha::math::is_zero()`.

   This default implementation defines a call operator whose body is equivalent to:

   .. code-block:: c++

      return x == T(0);
   
   The call operator is enabled (i.e., it participates in overload resolution) only if ``T``
   is :cpp:concept:`constructible <piranha::Constructible>` from ``const int &`` and
   :cpp:concept:`equality-comparable <piranha::EqualityComparable>`. In other words, this default
   implementation is activated only if the expression ``x == T(0)`` is valid.

   :exception unspecified: any exception thrown by the expression ``x == T(0)``.

.. cpp:class:: template <piranha::CppComplex T> piranha::math::is_zero_impl<T>

   Specialisation of the function object implementing :cpp:func:`piranha::math::is_zero()` for
   C++ complex types.

   This specialisation will return ``true`` if both the real and imaginary parts of the input argument
   are equal to zero, ``false`` otherwise.

.. cpp:class:: template <std::size_t SSize> piranha::math::is_zero_impl<mppp::integer<SSize>>

   *#include <piranha/integer.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::math::is_zero()` for :cpp:class:`mppp::integer <mppp::integer>`.

   This specialisation will return the output of :cpp:func:`mppp::integer::is_zero() <mppp::integer::is_zero()>`
   called on the input argument.

   :exception unspecified: any exception thrown by :cpp:func:`mppp::integer::is_zero() <mppp::integer::is_zero()>`.

.. cpp:class:: template <std::size_t SSize> piranha::math::is_zero_impl<mppp::rational<SSize>>

   *#include <piranha/rational.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::math::is_zero()` for :cpp:class:`mppp::rational <mppp::rational>`.

   This specialisation will return the output of :cpp:func:`mppp::rational::is_zero() <mppp::rational::is_zero()>`
   called on the input argument.

   :exception unspecified: any exception thrown by :cpp:func:`mppp::rational::is_zero() <mppp::rational::is_zero()>`.

.. cpp:class:: template <> piranha::math::is_zero_impl<mppp::real>

   *#include <piranha/real.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::math::is_zero()` for :cpp:class:`mppp::real <mppp::real>`.

   This specialisation will return the output of :cpp:func:`mppp::real::zero_p() <mppp::real::zero_p()>`
   called on the input argument.

   :exception unspecified: any exception thrown by :cpp:func:`mppp::real::zero_p() <mppp::real::zero_p()>`.

   .. note::

      This specialisation is available only if mp++ was configured with the ``MPPP_WITH_MPFR`` option enabled
      (see the :ref:`mp++ installation instructions <mppp:installation>`).
