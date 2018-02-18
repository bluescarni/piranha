.. _math_is_one:

Detect one
==========

*#include <piranha/math/is_one.hpp>*

.. cpp:function:: template <typename T> bool piranha::is_one(const T &x)

   This function returns ``true`` if *x* is equal to one, ``false`` otherwise.

   The implementation is delegated to the call operator of the :cpp:class:`piranha::is_one_impl` function object.
   The body of this function is equivalent to:

   .. code-block:: c++

      return piranha::is_one_impl<T>{}(x);

   If the expression above is invalid, or if it returns a type which is not
   :cpp:concept:`~piranha::Convertible` to ``bool``, then this function will
   be disabled (i.e., it will not participate in overload resolution).

   Piranha provides specialisations of :cpp:class:`piranha::is_one_impl` for the following types:

   * any type for which the expression ``x == T(1)`` is valid (the default implementation),
   * all C++ complex types,
   * all :cpp:class:`mppp::integer <mppp::integer>` types (including :cpp:type:`piranha::integer`),
   * all :cpp:class:`mppp::rational <mppp::rational>` types (including :cpp:type:`piranha::rational`),
   * :cpp:class:`mppp::real <mppp::real>`.

   See the :ref:`implementation <math_is_one_impls>` section below for more details about the available
   specialisations.

   :param x: the input argument.

   :return: ``true`` if *x* is equal to one, ``false`` otherwise.

   :exception unspecified: any exception thrown by the call operator of :cpp:class:`piranha::is_one_impl`.

Concepts
--------

.. cpp:concept:: template <typename T> piranha::IsOneType

   This concept is satisfied if :cpp:func:`piranha::is_one()` can be called
   with an argument of type ``T``. Specifically, this concept will be satisfied if

   .. code-block:: c++

      piranha::is_one(x)

   is a valid expression, where ``x`` is a reference to const ``T``.

.. _math_is_one_impls:

Implementations
---------------

.. cpp:class:: template <typename T> piranha::is_one_impl

   Unspecialised version of the function object implementing :cpp:func:`piranha::is_one()`.

   This default implementation defines a call operator whose body is equivalent to:

   .. code-block:: c++

      return x == T(1);
   
   The call operator is enabled (i.e., it participates in overload resolution) only if ``T``
   is :cpp:concept:`constructible <piranha::Constructible>` from ``const int &`` and
   :cpp:concept:`equality-comparable <piranha::EqualityComparable>`. In other words, this default
   implementation is activated only if the expression ``x == T(1)`` is valid.

   :exception unspecified: any exception thrown by the expression ``x == T(1)``.

.. cpp:class:: template <piranha::CppComplex T> piranha::is_one_impl<T>

   Specialisation of the function object implementing :cpp:func:`piranha::is_one()` for
   C++ complex types.

   This specialisation will return ``true`` if the real part of the input argument is equal to one and the imaginary
   part is equal to zero, ``false`` otherwise.

.. cpp:class:: template <std::size_t SSize> piranha::is_one_impl<mppp::integer<SSize>>

   *#include <piranha/integer.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::is_one()` for :cpp:class:`mppp::integer <mppp::integer>`.

   This specialisation will return the output of :cpp:func:`mppp::integer::is_one() <mppp::integer::is_one()>`
   called on the input argument.

   :exception unspecified: any exception thrown by :cpp:func:`mppp::integer::is_one() <mppp::integer::is_one()>`.

.. cpp:class:: template <std::size_t SSize> piranha::is_one_impl<mppp::rational<SSize>>

   *#include <piranha/rational.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::is_one()` for :cpp:class:`mppp::rational <mppp::rational>`.

   This specialisation will return the output of :cpp:func:`mppp::rational::is_one() <mppp::rational::is_one()>`
   called on the input argument.

   :exception unspecified: any exception thrown by :cpp:func:`mppp::rational::is_one() <mppp::rational::is_one()>`.

.. cpp:class:: template <> piranha::is_one_impl<mppp::real>

   .. note::

      This specialisation is available only if mp++ was configured with the ``MPPP_WITH_MPFR`` option enabled
      (see the :ref:`mp++ installation instructions <mppp:installation>`).

   *#include <piranha/real.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::is_one()` for :cpp:class:`mppp::real <mppp::real>`.

   This specialisation will return the output of :cpp:func:`mppp::real::is_one() <mppp::real::is_one()>`
   called on the input argument.

   :exception unspecified: any exception thrown by :cpp:func:`mppp::real::is_one() <mppp::real::is_one()>`.
