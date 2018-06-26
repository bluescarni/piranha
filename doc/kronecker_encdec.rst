Kronecker encoding/decoding
===========================

*#include <piranha/utils/kronecker_encdec.hpp>*

Classes
-------

.. cpp:class:: template <piranha::UncvCppSignedIntegral T> piranha::k_encoder

   Streaming Kronecker encoder.

   This class can be used to iteratively Kronecker-encode a sequence of signed integrals of type ``T``.
   The typical use is as follows:

   .. code-block:: c++

      k_encoder<int> k(3); // Prepare a k_encoder to encode a sequence of 3 int values.
      k << 1 << 2 << 3;    // Push to k the sequence of values using the '<<' operator.
      auto code = k.get(); // Fetch the encoded value via the get() member function.

   Note that a sequence of size zero is always encoded as zero.

   .. cpp:function:: explicit k_encoder(std::size_t size)

      Constructor from sequence size.

      This constructor will create a :cpp:class:`~piranha::k_encoder` that will be able to encode
      a sequence of exactly *size* signed integrals of type ``T``.

      :param size: the sequence size.

      :exception std\:\:overflow_error: if *size* is larger than an implementation-defined limit.

   .. cpp:function:: k_encoder &operator<<(T n)

      Push a value to the encoder.

      :param n: the value that will be pushed to the encoder

      :return: a reference to *this*.

      :exception std\:\:out_of_range: if *size* values have already been pushed to the encoder (where *size* is
         the parameter used for the construction of the :cpp:class:`~piranha::k_encoder` object).
      :exception std\:\:overflow_error: if *n* is outside an implementation-defined range.

   .. cpp:function:: T get() const

      Get the encoded value.

      :return: the Kronecker-encoded value corresponding to the sequence of signed integrals pushed to the encoder
         via :cpp:func:`~piranha::k_encoder::operator<<()`. Note that if this encoder was constructed with a *size*
         of zero, zero will be returned by this function.

      :exception std\:\:out_of_range: if fewer than *size* values have been pushed to the encoder (where *size* is
         the parameter used for the construction of the :cpp:class:`~piranha::k_encoder` object).

Concepts
--------

.. cpp:concept:: template <typename T> piranha::UncvCppSignedIntegral

   This concept is satisfied if ``T`` is a signed :cpp:concept:`piranha::CppIntegral`
   without cv qualifications.

.. cpp:concept:: template <typename It, typename T> piranha::KEncodableIterator

   This concept is satisfied if ``It`` is an iterator whose value type
   can be Kronecker-encoded to the signed integral type ``T``.

   Specifically, this concept is satisfied if the following conditions hold:

   * ``It`` is a :cpp:concept:`piranha::InputIterator`,
   * ``T`` satisfies the :cpp:concept:`piranha::UncvCppSignedIntegral` concept,
   * the reference type of ``It`` is :cpp:concept:`safely castable <piranha::SafelyCastable>` to ``T``,
   * the difference type of ``It`` is :cpp:concept:`safely castable <piranha::SafelyCastable>`
     to ``std::size_t``.

.. cpp:concept:: template <typename It, typename T> piranha::KEncodableForwardIterator

   This concept is satisfied if ``It`` and ``T`` satisfy :cpp:concept:`piranha::KEncodableIterator` and, additionally,
   ``It`` is a :cpp:concept:`piranha::ForwardIterator`.

.. cpp:concept:: template <typename R, typename T> piranha::KEncodableForwardRange

   This concept is satisfied if ``R`` is a :cpp:concept:`piranha::ForwardRange` whose iterator type
   satisfies the :cpp:concept:`piranha::KEncodableForwardIterator` concept for the signed integral type ``T``.
