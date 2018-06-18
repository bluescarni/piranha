Kronecker encoding/decoding
===========================

Concepts
--------

.. cpp:concept:: template <typename T> piranha::UncvCppSignedIntegral

   This concept is satisfied if ``T`` is a signed :cpp:concept:`piranha::CppIntegral`
   without cv qualifications.

.. cpp:concept:: template <typename It, typename T> piranha::KEncodableIterator

   This concept is satisfied if ``It`` is a :cpp:concept:`piranha::ForwardIterator` such that:
   
   * given an lvalue *it* of type ``It``, the result of the expression *\*it*
     is :cpp:concept:`safely castable <piranha::SafelyCastable>` to ``T``, and
   * an rvalue of the difference type of ``It`` is :cpp:concept:`safely castable <piranha::SafelyCastable>`
     to ``std::size_t``.

.. cpp:concept:: template <typename R, typename T> piranha::KEncodableRange

   This concept is satisfied if ``R`` is a :cpp:concept:`piranha::ForwardRange` whose iterator type
   satisfies the :cpp:concept:`piranha::KEncodableIterator` concept for the type ``T``.
