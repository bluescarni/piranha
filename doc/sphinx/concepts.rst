.. _concepts:

General concepts and type traits
================================

*#include <piranha/type_traits.hpp>*

.. cpp:concept:: template <typename T> piranha::CppArithmetic

   This concept is satisfied it ``T`` is a C++ arithmetic type, or a cv-qualified version thereof.

.. cpp:concept:: template <typename T> piranha::CppIntegral

   This concept is satisfied it ``T`` is a C++ integral type, or a cv-qualified version thereof.

.. cpp:concept:: template <typename T> piranha::CppFloatingPoint

   This concept is satisfied it ``T`` is a C++ floating-point type, or a cv-qualified version thereof.

.. cpp:concept:: template <typename T> piranha::Returnable

   This concept is satisfied if instances of type ``T`` can be returned from a function.
   Specifically, this concept is satisfied if ``T`` is either ``void`` or destructible
   and copy or move constructible.
