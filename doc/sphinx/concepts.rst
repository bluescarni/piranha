.. _concepts:

General concepts and type traits
================================

*#include <piranha/type_traits.hpp>*

Type categories
---------------

.. cpp:concept:: template <typename T> piranha::CppArithmetic

   This concept is satisfied it ``T`` is a C++ arithmetic type, or a cv-qualified version thereof,
   as established by the standard ``std::is_arithmetic`` type trait.

.. cpp:concept:: template <typename T> piranha::CppIntegral

   This concept is satisfied it ``T`` is a C++ integral type, or a cv-qualified version thereof,
   as established by the standard ``std::is_integral`` type trait.

.. cpp:concept:: template <typename T> piranha::CppFloatingPoint

   This concept is satisfied it ``T`` is a C++ floating-point type, or a cv-qualified version thereof,
   as established by the standard ``std::is_floating_point`` type trait.

.. cpp:concept:: template <typename T> piranha::CppComplex

   This concept is satisfied it ``T`` is a complex C++ floating-point type, or a cv-qualified version thereof.

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
