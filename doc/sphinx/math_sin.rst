.. _math_sin:

Sine
====

*#include <piranha/math/sin.hpp>*

.. cpp:function:: template <typename T> auto piranha::sin(T &&x)

   This function computes :math:`\sin\left( x \right)`.

   The implementation is delegated to the call operator of the :cpp:class:`piranha::sin_impl` function object.
   The body of this function is equivalent to:

   .. code-block:: c++

      return piranha::sin_impl<Tp>{}(x);

   where ``Tp`` is ``T`` after the removal of reference and cv-qualifiers,
   and *x* is perfectly forwarded to the call operator of :cpp:class:`piranha::sin_impl`.
   If the expression above is invalid, or if it returns a type which does not satisfy the
   :cpp:concept:`piranha::Returnable` concept,
   then this function will be disabled (i.e., it will not participate in overload resolution).

   Piranha provides specialisations of :cpp:class:`piranha::sin_impl` for the following types:

   * all of C++'s arithmetic types,
   * all :cpp:class:`mppp::integer <mppp::integer>` types (including :cpp:type:`piranha::integer`),
   * all :cpp:class:`mppp::rational <mppp::rational>` types (including :cpp:type:`piranha::rational`),
   * :cpp:class:`mppp::real <mppp::real>`.

   See the :ref:`implementation <math_sin_impls>` section below for more details about the available
   specialisations.

   :param x: the argument for the sine function.

   :return: :math:`\sin\left( x \right)`.

   :exception unspecified: any exception thrown by the call operator of :cpp:class:`piranha::sin_impl`.

Concepts
--------

.. cpp:concept:: template <typename T> piranha::SineType

   This concept is satisfied if :cpp:func:`piranha::sin()` can be called
   with an argument of type ``T``. Specifically,
   this concept will be satisfied if

   .. code-block:: c++

      piranha::sin(x)

   is a valid expression, where ``x`` is a reference to const ``T``.

.. _math_sin_impls:

Implementations
---------------

.. cpp:class:: template <typename T, typename Enable = void> piranha::sin_impl

   Unspecialised version of the function object implementing :cpp:func:`piranha::sin()`.

   This default implementation does not define any call operator, and thus no default implementation
   of :cpp:func:`piranha::sin()` is available.

.. cpp:class:: template <piranha::CppArithmetic T> piranha::sin_impl<T>

   Specialisation of the function object implementing :cpp:func:`piranha::sin()` for C++ arithmetic types.

   If ``T`` is a floating-point type, the result of the operation, computed via ``std::sin()``,
   will be of type ``T``.

   Otherwise, the operation is successful only if the input argument is zero, in which case the result will be an
   instance of ``T`` constructed from zero.

   :exception std\:\:domain_error: if ``T`` is an integral type and the input argument is not zero.

.. cpp:class:: template <std::size_t SSize> piranha::sin_impl<mppp::integer<SSize>>

   *#include <piranha/integer.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::sin()` for :cpp:class:`mppp::integer <mppp::integer>`.

   The operation is successful only if the input argument is zero, in which case the result will be an instance of
   :cpp:class:`mppp::integer\<SSize\> <mppp::integer>` constructed from zero.

   :exception std\:\:domain_error: if the input argument is not zero.

.. cpp:class:: template <std::size_t SSize> piranha::sin_impl<mppp::rational<SSize>>

   *#include <piranha/rational.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::sin()` for :cpp:class:`mppp::rational <mppp::rational>`.

   The operation is successful only if the input argument is zero, in which case the result will be an instance of
   :cpp:class:`mppp::rational\<SSize\> <mppp::rational>` constructed from zero.

   :exception std\:\:domain_error: if the input argument is not zero.

.. cpp:class:: template <> piranha::sin_impl<mppp::real>

   .. note::

      This specialisation is available only if mp++ was configured with the ``MPPP_WITH_MPFR`` option enabled
      (see the :ref:`mp++ installation instructions <mppp:installation>`).

   *#include <piranha/real.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::sin()` for :cpp:class:`mppp::real <mppp::real>`.

   This implementation will invoke one of mp++'s :ref:`real sine <mppp:real_trig>` overloads.

   :exception unspecified: any exception thrown by the invoked :ref:`real sine <mppp:real_trig>` overload.
