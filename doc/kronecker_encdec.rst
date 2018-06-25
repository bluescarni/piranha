Kronecker encoding/decoding
===========================

Concepts
--------

.. cpp:concept:: template <typename T> piranha::UncvCppSignedIntegral

   This concept is satisfied if ``T`` is a signed :cpp:concept:`piranha::CppIntegral`
   without cv qualifications.

.. cpp:concept:: template <typename It, typename T> piranha::KEncodableIterator

   This concept is satisfied if ``It`` is an iterator whose value type
   can be Kronecker-encoded to the signed integral type ``T``.

   Specifically, this concept is satisfied if the following conditions hold:

   * ``It`` is a :cpp:concept:`piranha::ForwardIterator`,
   * ``T`` satisfies the :cpp:concept:`piranha::UncvCppSignedIntegral` concept,
   * the reference type of ``It`` is :cpp:concept:`safely castable <piranha::SafelyCastable>` to ``T``,
   * the difference type of ``It`` is :cpp:concept:`safely castable <piranha::SafelyCastable>`
     to ``std::size_t``.

.. cpp:concept:: template <typename R, typename T> piranha::KEncodableRange

   This concept is satisfied if ``R`` is a :cpp:concept:`piranha::ForwardRange` whose iterator type
   satisfies the :cpp:concept:`piranha::KEncodableIterator` concept for the signed integral type ``T``.
