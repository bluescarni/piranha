.. _concepts:

General concepts and type traits
================================

*#include <piranha/type_traits.hpp>*

.. cpp:concept:: template <typename T> piranha::Returnable

   This concept is satisfied if instances of type ``T`` can be returned from a function.
   Specifically, this concept is satisfied if ``T`` is either ``void`` or destructible
   and copy/move constructible.
