.. _math_sin:

Sine
====

*#include <piranha/math/sin.hpp>*

.. cpp:function:: template <typename T> auto piranha::math::sin(T &&x)

   This function computes :math:`\sin\left( x \right)`.

   The implementation is delegated to the call operator of the :cpp:class:`piranha::math::sin_impl` function object.
   The body of this function is equivalent to:

   .. code-block:: c++

      return piranha::math::sin_impl<Tp>{}(x);

   where ``Tp`` is ``T`` after the removal of reference and cv-qualifiers,
   and *x* is perfectly forwarded to the call operator of :cpp:class:`piranha::math::sin_impl`.
   If the expression above is invalid, or if it returns a type which does not satisfy the
   :cpp:concept:`piranha::Returnable` concept,
   then this function will be disabled (i.e., it will not participate in overload resolution).

   Piranha provides specialisations of :cpp:class:`piranha::math::sin_impl` for the following types:

   * all of C++'s arithmetic types,
   * all :cpp:class:`mppp::integer <mppp::integer>` types (including :cpp:type:`piranha::integer`),
   * all :cpp:class:`mppp::rational <mppp::rational>` types (including :cpp:type:`piranha::rational`),
   * :cpp:class:`mppp::real <mppp::real>`.

   See the :ref:`implementation <math_sin_impls>` section below for more details about the available
   specialisations.

   :param x: the argument for the sine function.

   :return: :math:`\sin\left( x \right)`.

   :exception unspecified: any exception thrown by the call operator of :cpp:class:`piranha::math::sin_impl`.

Concepts
--------

.. cpp:concept:: template <typename T> piranha::SineType

   This concept is satisfied if :cpp:func:`piranha::math::sin()` can be called
   with an argument of type ``T``. Specifically,
   this concept will be satisfied if

   .. code-block:: c++

      piranha::math::sin(x)

   is a valid expression, where ``x`` is a ``const`` reference to ``T``.

.. _math_sin_impls:

Implementations
---------------

.. cpp:class:: template <typename T, typename Enable = void> piranha::math::sin_impl

   Unspecialised version of the function object implementing :cpp:func:`piranha::math::sin()`.

   This default implementation does not define any call operator, and thus no default implementation
   of :cpp:func:`piranha::math::sin()` is available.

.. cpp:class:: template <piranha::CppFloatingPoint T> piranha::math::sin_impl<T>

   Specialisation of the function object implementing :cpp:func:`piranha::math::sin()` for C++'s floating-point types.

   The result of the operation, computed via ``std::sin()``, will be of type ``T``.

.. cpp:class:: template <piranha::CppIntegral T> piranha::math::sin_impl<T>

   Specialisation of the function object implementing :cpp:func:`piranha::math::sin()` for C++'s integral types.

   The operation is successful only if the input argument is zero, in which case the result will be a zero of type ``T``.

   :exception std\:\:invalid_argument: if the input argument is not zero.

.. cpp:class:: template <std::size_t SSize> piranha::math::sin_impl<mppp::integer<SSize>>

   *#include <piranha/integer.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::math::sin()` for :cpp:class:`mppp::integer <mppp::integer>`.

   The operation is successful only if the input argument is zero, in which case the result will be a zero of type
   :cpp:class:`mppp::integer\<SSize\> <mppp::integer>`.

.. cpp:class:: template <typename U, mppp::RealOpTypes<U> T> piranha::math::pow_impl<T, U>

   *#include <piranha/real.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::math::pow()` for :cpp:class:`mppp::real <mppp::real>`.

   This implementation will invoke one of mp++'s :ref:`real exponentiation <mppp:real_exponentiation>` overloads.

   :exception unspecified: any exception thrown by the invoked :ref:`real exponentiation <mppp:real_exponentiation>` overload.

   .. note::

      This specialisation is available only if mp++ was configured with the ``MPPP_WITH_MPFR`` option enabled
      (see the :ref:`mp++ installation instructions <mppp:installation>`).
