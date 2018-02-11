.. _math_pow:

Exponentiation
==============

*#include <piranha/math/pow.hpp>*

.. cpp:function:: template <typename T, typename U> auto piranha::pow(T &&x, U &&y)

   This function computes :math:`x^y`.

   The implementation is delegated to the call operator of the :cpp:class:`piranha::pow_impl` function object.
   The body of this function is equivalent to:

   .. code-block:: c++

      return piranha::pow_impl<Tp, Up>{}(x, y);

   where ``Tp`` and ``Up`` are ``T`` and ``U`` after the removal of reference and cv-qualifiers,
   and *x* and *y* are perfectly forwarded to the call operator of :cpp:class:`piranha::pow_impl`.
   If the expression above is invalid, or if it returns a type which does not satisfy the
   :cpp:concept:`piranha::Returnable` concept,
   then this function will be disabled (i.e., it will not participate in overload resolution).

   Piranha provides specialisations of :cpp:class:`piranha::pow_impl` for the following types:

   * all of C++'s arithmetic types,
   * all :cpp:class:`mppp::integer <mppp::integer>` types (including :cpp:type:`piranha::integer`),
   * all :cpp:class:`mppp::rational <mppp::rational>` types (including :cpp:type:`piranha::rational`),
   * :cpp:class:`mppp::real <mppp::real>`.

   See the :ref:`implementation <math_pow_impls>` section below for more details about the available
   specialisations.

   :param x: the base.
   :param y: the exponent.

   :return: :math:`x^y`.

   :exception unspecified: any exception thrown by the call operator of :cpp:class:`piranha::pow_impl`.

Concepts
--------

.. cpp:concept:: template <typename T, typename U> piranha::Exponentiable

   This concept is satisfied if :cpp:func:`piranha::pow()` can be called
   with a base of type ``T`` and an exponent of type ``U``. Specifically,
   this concept will be satisfied if

   .. code-block:: c++

      piranha::pow(x, y)

   is a valid expression, where ``x`` and ``y`` are references to const ``T`` and ``U`` respectively.

.. _math_pow_impls:

Implementations
---------------

.. cpp:class:: template <typename T, typename U, typename Enable = void> piranha::pow_impl

   Unspecialised version of the function object implementing :cpp:func:`piranha::pow()`.

   This default implementation does not define any call operator, and thus no default implementation
   of :cpp:func:`piranha::pow()` is available.

.. cpp:class:: template <piranha::CppArithmetic T, piranha::CppArithmetic U> piranha::pow_impl<T, U>

   Specialisation of the function object implementing :cpp:func:`piranha::pow()` for C++'s arithmetic types.

   If at least one of ``T`` and ``U`` is a floating-point type, the result of the exponentiation will be calculated
   via one of the overloads of ``std::pow()``, and the result will be a C++ floating-point type.

   Otherwise, the base will be converted to :cpp:type:`piranha::integer`
   before being passed, together with the exponent, to one of mp++'s :ref:`integer exponentiation <mppp:integer_exponentiation>`
   overloads. The type of the result will be :cpp:type:`piranha::integer`.

   :exception unspecified: any exception thrown by the invoked :ref:`integer exponentiation <mppp:integer_exponentiation>` overload.

.. cpp:class:: template <typename U, mppp::IntegerOpTypes<U> T> piranha::pow_impl<T, U>

   Specialisation of the function object implementing :cpp:func:`piranha::pow()` for :cpp:class:`mppp::integer <mppp::integer>`.

   This implementation will invoke one of mp++'s :ref:`integer exponentiation <mppp:integer_exponentiation>` overloads.

   :exception unspecified: any exception thrown by the invoked :ref:`integer exponentiation <mppp:integer_exponentiation>` overload.

.. cpp:class:: template <typename U, mppp::RationalOpTypes<U> T> piranha::pow_impl<T, U>

   *#include <piranha/rational.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::pow()` for :cpp:class:`mppp::rational <mppp::rational>`.

   This implementation will invoke one of mp++'s :ref:`rational exponentiation <mppp:rational_exponentiation>` overloads.

   :exception unspecified: any exception thrown by the invoked :ref:`rational exponentiation <mppp:rational_exponentiation>` overload.

.. cpp:class:: template <typename U, mppp::RealOpTypes<U> T> piranha::pow_impl<T, U>

   .. note::

      This specialisation is available only if mp++ was configured with the ``MPPP_WITH_MPFR`` option enabled
      (see the :ref:`mp++ installation instructions <mppp:installation>`).

   *#include <piranha/real.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::pow()` for :cpp:class:`mppp::real <mppp::real>`.

   This implementation will invoke one of mp++'s :ref:`real exponentiation <mppp:real_exponentiation>` overloads.

   :exception unspecified: any exception thrown by the invoked :ref:`real exponentiation <mppp:real_exponentiation>` overload.
