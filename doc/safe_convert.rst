.. _safe_convert:

Safe conversion
===============

*#include <piranha/safe_convert.hpp>*

.. cpp:function:: template <typename To, piranha::SafelyConvertible<To> From> bool piranha::safe_convert(To &&x, From &&y)

   This function attempts to set *x* to the exact value of *y*, possibly performing a type conversion. If the value of *y*
   can be represented exactly by the type of *x*, then this function will write the value of *y* into *x* and return ``true``. Otherwise,
   ``false`` will be returned and the value of *x* will be unspecified.

   The implementation is delegated to the call operator of the :cpp:class:`piranha::safe_convert_impl` function object.
   The body of this function is equivalent to:

   .. code-block:: c++

      return piranha::safe_convert_impl<Tp, Up>{}(x, y);

   where ``Tp`` and ``Up`` are the types of *x* and *y* after the removal of reference and cv-qualifiers,
   and *x* and *y* are perfectly forwarded to the call operator of :cpp:class:`piranha::safe_convert_impl`.

   Piranha provides specialisations of :cpp:class:`piranha::safe_convert_impl` for the following types:

   * all assignable types, if ``Tp == Up`` (the default implementation),
   * various combinations of :cpp:class:`mppp::integer <mppp::integer>`, :cpp:class:`mppp::rational <mppp::rational>`,
     :cpp:class:`mppp::real <mppp::real>` and C++ arithmetic types.

   See the :ref:`implementation <safe_convert_impls>` section below for more details about the available
   specialisations.

   :param x: the return value.
   :param y: the value to be converted.

   :return: ``true`` if the conversion was successful, ``false`` otherwise.

   :exception unspecified: any exception thrown by the call operator of :cpp:class:`piranha::safe_convert_impl`.

Concepts
--------

.. cpp:concept:: template <typename From, typename To> piranha::SafelyConvertible

   This concept is satisfied if :cpp:func:`piranha::safe_convert()` can be used to convert
   an instance of type ``From`` to the type ``To``. Specifically, this concept will be satisfied if

   .. code-block:: c++

      piranha::safe_convert_impl<Tp, Up>{}(std::declval<To>(), std::declval<From>())

   (where ``Tp`` and ``Up`` are ``To`` and ``From`` after the removal of reference and cv-qualifiers) is a
   valid expression whose type is :cpp:concept:`convertible <piranha::Convertible>` to ``bool``.

.. _safe_convert_impls:

Implementations
---------------

.. cpp:class:: template <typename To, typename From> piranha::safe_convert_impl

   Unspecialised version of the function object implementing :cpp:func:`piranha::safe_convert()`.

   This default implementation defines a call operator whose body is equivalent to:

   .. code-block:: c++

      x = y;
      return true;

   where *x* and *y* are perfectly forwarded. If *x* and *y* are not the same type,
   after the removal of reference and cv-qualifiers, or if the above expression is not valid,
   then the call operator is disabled (i.e., it will not participate in overload resolution),
   and no default implementation of :cpp:func:`piranha::safe_convert()` is available.

   In other words, the default implementation of :cpp:func:`piranha::safe_convert()`
   is available if the input arguments are of the same type (after the removal of reference
   and cv-qualifiers) and if *y* can be assigned to *x*.

   :exception unspecified: any exception thrown by the invoked assignment operator.

.. cpp:class:: template <piranha::CppIntegral To, piranha::CppIntegral From> piranha::safe_convert_impl<To, From>

   Specialisation of the function object implementing :cpp:func:`piranha::safe_convert()` for
   C++ integrals.

   This implementation will employ the ``std::numeric_limits`` facilities to check if ``To`` can represent exactly
   the value of *y*. The call operator's signature is:

   .. code-block:: c++

      bool operator()(To &, From) const;

.. cpp:class:: template <piranha::CppIntegral To, piranha::CppFloatingPoint From> piranha::safe_convert_impl<To, From>

   Specialisation of the function object implementing :cpp:func:`piranha::safe_convert()` for
   C++ floating-point to C++ integral conversions.

   The conversion will be successful if all these conditions hold:

   * *y* is finite,
   * *y* represents an integral value,
   * the value of *y* does not overflow the range of ``To``.

   The call operator's signature is:

   .. code-block:: c++

      bool operator()(To &, From) const;

.. cpp:class:: template <std::size_t SSize, mppp::CppInteroperable From> piranha::safe_convert_impl<mppp::integer<SSize>, From>

   *#include <piranha/integer.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::safe_convert()` for
   C++ arithmetic types to :cpp:class:`mppp::integer <mppp::integer>` conversions.

   The conversion is always successful if ``From`` is a C++ integral type. If ``From`` is a C++ floating-point
   type, then the conversion is successful only if *y* is a finite integral value.

   The call operator's signature is:

   .. code-block:: c++

      bool operator()(mppp::integer<SSize> &, From) const;

.. cpp:class:: template <mppp::CppIntegralInteroperable To, std::size_t SSize> piranha::safe_convert_impl<To, mppp::integer<SSize>>

   *#include <piranha/integer.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::safe_convert()` for
   :cpp:class:`mppp::integer <mppp::integer>` to C++ integrals conversions.

   The conversion is successful only if *y* does not overflow the range of ``To``.

   The call operator's signature is:

   .. code-block:: c++

      bool operator()(To &, const mppp::integer<SSize> &) const;

.. cpp:class:: template <std::size_t SSize, mppp::RationalIntegralInteroperable<SSize> From> piranha::safe_convert_impl<mppp::rational<SSize>, From>

   *#include <piranha/rational.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::safe_convert()` for
   integrals to :cpp:class:`mppp::rational <mppp::rational>` conversions. The conversion is always successful.

   The call operator's signature is:

   .. code-block:: c++

      bool operator()(mppp::rational<SSize> &, const From &) const;

.. cpp:class:: template <std::size_t SSize, mppp::CppFloatingPointInteroperable From> piranha::safe_convert_impl<mppp::rational<SSize>, From>

   *#include <piranha/rational.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::safe_convert()` for
   C++ floating-point to :cpp:class:`mppp::rational <mppp::rational>` conversions.

   The conversion is successful if *y* represents a finite value.

   The call operator's signature is:

   .. code-block:: c++

      bool operator()(mppp::rational<SSize> &, From) const;

.. cpp:class:: template <std::size_t SSize, mppp::RationalIntegralInteroperable<SSize> To> piranha::safe_convert_impl<To, mppp::rational<SSize>>

   *#include <piranha/rational.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::safe_convert()` for
   :cpp:class:`mppp::rational <mppp::rational>` to integrals conversions.

   The conversion is successful if *y* is an integral value which is representable by ``To``.

   The call operator's signature is:

   .. code-block:: c++

      bool operator()(To &, const mppp::rational<SSize> &) const;

.. cpp:class:: template <mppp::CppIntegralInteroperable To> piranha::safe_convert_impl<To, piranha::real>

   .. note::

      This specialisation is available only if mp++ was configured with the ``MPPP_WITH_MPFR`` option enabled
      (see the :ref:`mp++ installation instructions <mppp:installation>`).

   *#include <piranha/real.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::safe_convert()` for
   :cpp:class:`mppp::real <mppp::real>` to C++ integrals conversions.

   The conversion is successful if *y* is a finite integral value which is representable by ``To``.

   The call operator's signature is:

   .. code-block:: c++

      bool operator()(To &, const piranha::real &) const;

.. cpp:class:: template <std::size_t SSize> piranha::safe_convert_impl<mppp::integer<SSize>, piranha::real>

   .. note::

      This specialisation is available only if mp++ was configured with the ``MPPP_WITH_MPFR`` option enabled
      (see the :ref:`mp++ installation instructions <mppp:installation>`).

   *#include <piranha/real.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::safe_convert()` for
   :cpp:class:`mppp::real <mppp::real>` to :cpp:class:`mppp::integer <mppp::integer>` conversions.

   The conversion is successful if *y* is a finite integral value.

   The call operator's signature is:

   .. code-block:: c++

      bool operator()(mppp::integer<SSize> &, const piranha::real &) const;

.. cpp:class:: template <std::size_t SSize> piranha::safe_convert_impl<mppp::rational<SSize>, piranha::real>

   .. note::

      This specialisation is available only if mp++ was configured with the ``MPPP_WITH_MPFR`` option enabled
      (see the :ref:`mp++ installation instructions <mppp:installation>`).

   *#include <piranha/real.hpp>*

   Specialisation of the function object implementing :cpp:func:`piranha::safe_convert()` for
   :cpp:class:`mppp::real <mppp::real>` to :cpp:class:`mppp::rational <mppp::rational>` conversions.

   The conversion is successful if *y* is a finite value.

   The call operator's signature is:

   .. code-block:: c++

      bool operator()(mppp::rational<SSize> &, const piranha::real &) const;
