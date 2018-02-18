.. _concepts:

General concepts and type traits
================================

.. note::

   Generic functions and classes in piranha support `concepts <https://en.wikipedia.org/wiki/Concepts_(C%2B%2B)>`__
   to constrain the types with which they can be used. C++ concepts are not (yet) part of the standard, and they are
   currently available only in GCC 6 and later (with the ``-fconcepts`` compilation flag). When used with compilers which do not
   support concepts natively, piranha will employ a concept emulation layer in order to provide the same functionality as native
   C++ concepts.

   Since the syntax of native C++ concepts is clearer than that of the concept emulation layer, the piranha documentation describes
   and refers to concepts in their native C++ form. As long as concepts are not part of the C++ standard, piranha's concepts
   will be subject to breaking changes, and they should not be regarded as part of the public piranha API.

*#include <piranha/type_traits.hpp>*

Type categories
---------------

.. cpp:concept:: template <typename T> piranha::CppArithmetic

   This concept is satisfied if ``T`` is a C++ arithmetic type, or a cv-qualified version thereof,
   as established by the standard ``std::is_arithmetic`` type trait.

.. cpp:concept:: template <typename T> piranha::CppIntegral

   This concept is satisfied if ``T`` is a C++ integral type, or a cv-qualified version thereof,
   as established by the standard ``std::is_integral`` type trait.

.. cpp:concept:: template <typename T> piranha::CppFloatingPoint

   This concept is satisfied if ``T`` is a C++ floating-point type, or a cv-qualified version thereof,
   as established by the standard ``std::is_floating_point`` type trait.

.. cpp:concept:: template <typename T> piranha::CppComplex

   This concept is satisfied if ``T`` is a C++ complex floating-point type, or a cv-qualified version thereof.

.. cpp:concept:: template <typename T> piranha::NonConst

   This concept is satisfied if ``T`` is *not* a const-qualified type, as established by the standard ``std::is_const``
   type trait.

Type properties
---------------

.. cpp:concept:: template <typename T, typename... Args> piranha::Constructible

   This concept is satisfied if an instance of type ``T`` can be constructed from a set of arguments
   of types ``Args...``, as established by the standard ``std::is_constructible`` type trait.

.. cpp:concept:: template <typename From, typename To> piranha::Convertible

   This concept is satisfied if an instance of type ``From`` can be converted to the type ``To``,
   as established by the standard ``std::is_convertible`` type trait.

.. cpp:concept:: template <typename T> piranha::Returnable

   This concept is satisfied if instances of type ``T`` can be returned from a function.
   Specifically, this concept is satisfied if ``T`` is either:

   * ``void``, or
   * destructible and copy or move constructible.

Arithmetic and logical operators
--------------------------------

.. cpp:concept:: template <typename T, typename U = T> piranha::EqualityComparable

   This concept is satisfied if instances of type ``T`` can be compared to instances of type
   ``U`` via the equality and inequality operators.

   Specifically, this concept is satisfied if the expressions ``a == b`` and ``a != b``,
   where ``a`` and ``b`` are references to const ``T`` and ``U`` respectively, are both valid
   and returning a type convertible to ``bool``.
