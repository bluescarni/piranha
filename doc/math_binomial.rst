.. _math_binomial:

Binomial coefficient
====================

*#include <piranha/math/binomial.hpp>*

.. cpp:function:: template <typename T> auto piranha::binomial(piranha::BinomialTypes<T> &&x, T &&y)

   This function computes the binomial coefficient :math:`{x \choose y}`.

   The implementation is delegated to the call operator of the :cpp:class:`piranha::binomial_impl` function object.
   The body of this function is equivalent to:

   .. code-block:: c++

      return piranha::binomial_impl<Tp, Up>{}(x, y);

   where ``Tp`` and ``Up`` are the types of *x* and *y* after the removal of reference and cv-qualifiers,
   and *x* and *y* are perfectly forwarded to the call operator of :cpp:class:`piranha::binomial_impl`.

   Piranha provides specialisations of :cpp:class:`piranha::binomial_impl` for the following types:

   * all of C++'s integral types,
   * all :cpp:class:`mppp::integer <mppp::integer>` types (including :cpp:type:`piranha::integer`),
   * all :cpp:class:`mppp::rational <mppp::rational>` types (including :cpp:type:`piranha::rational`).

   See the :ref:`implementation <math_binomial_impls>` section below for more details about the available
   specialisations.

   :param x: the top argument.
   :param y: the bottom argument.

   :return: :math:`{x \choose y}`.

   :exception unspecified: any exception thrown by the call operator of :cpp:class:`piranha::binomial_impl`.

Concepts
--------

.. cpp:concept:: template <typename T, typename U = T> piranha::BinomialTypes

   This concept is satisfied if :cpp:func:`piranha::binomial()` can be called
   with a top argument of type ``T`` and a bottom argument of type ``U``. Specifically,
   this concept will be satisfied if

   .. code-block:: c++

      piranha::binomial_impl<Tp, Up>{}(std::declval<T>(), std::declval<U>())

   (where ``Tp`` and ``Up`` are ``T`` and ``U`` after the removal of reference and cv-qualifiers) is a valid expression whose
   type is :cpp:concept:`returnable <piranha::Returnable>`.

.. _math_binomial_impls:

Implementations
---------------

.. cpp:class:: template <typename T, typename U> piranha::binomial_impl

   Unspecialised version of the function object implementing :cpp:func:`piranha::binomial()`.

   This default implementation does not define any call operator, and thus no default implementation
   of :cpp:func:`piranha::binomial()` is available.

.. cpp:class:: template <piranha::CppIntegral T, piranha::CppIntegral U> piranha::binomial_impl<T, U>

   Specialisation of the function object implementing :cpp:func:`piranha::binomial()` for C++'s integral types.

   The top argument will be converted to :cpp:type:`piranha::integer`
   before being passed, together with the bottom argument, to one of mp++'s :ref:`integer binomial <mppp:integer_ntheory>`
   overloads. The type of the result will be :cpp:type:`piranha::integer`.

   :exception unspecified: any exception thrown by the invoked :ref:`integer binomial <mppp:integer_ntheory>` overload.

.. cpp:class:: template <typename U, mppp::IntegerIntegralOpTypes<U> T> piranha::binomial_impl<T, U>

   Specialisation of the function object implementing :cpp:func:`piranha::binomial()` for :cpp:class:`mppp::integer <mppp::integer>`.

   This implementation will invoke one of mp++'s :ref:`integer binomial <mppp:integer_ntheory>` overloads.

   :exception unspecified: any exception thrown by the invoked :ref:`integer binomial <mppp:integer_ntheory>` overload.

.. cpp:class:: template <std::size_t SSize, mppp::RationalIntegralInteroperable<SSize> T> piranha::binomial_impl<mppp::rational<SSize>, T>

   *#include <piranha/rational.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::binomial()` for :cpp:class:`mppp::rational <mppp::rational>`
   top arguments and integral bottom arguments.

   This implementation will invoke one of mp++'s :ref:`rational binomial <mppp:rational_ntheory>` overloads.

   :exception unspecified: any exception thrown by the invoked :ref:`rational binomial <mppp:rational_ntheory>` overload.
