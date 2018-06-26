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

      :param n: the value that will be pushed to the encoder.

      :return: a reference to *this*.

      :exception std\:\:out_of_range: if *size* values have already been pushed to the encoder (where *size* is
         the parameter used for the construction of this :cpp:class:`~piranha::k_encoder` object).
      :exception std\:\:overflow_error: if *n* is outside an implementation-defined range.

   .. cpp:function:: T get() const

      Get the encoded value.

      :return: the Kronecker-encoded value corresponding to the sequence of signed integrals pushed to the encoder
         via :cpp:func:`~piranha::k_encoder::operator<<()`. Note that if this encoder was constructed with a *size*
         of zero, zero will be returned by this function.

      :exception std\:\:out_of_range: if fewer than *size* values have been pushed to the encoder (where *size* is
         the parameter used for the construction of this :cpp:class:`~piranha::k_encoder` object).

Functions
---------

.. cpp:function:: template <typename T, piranha::KEncodableIterator<T> It> T piranha::k_encode(It begin, std::size_t size)

   Kronecker-encode a sequence of values of length ``size`` starting at ``begin``.

   Note that this overload requires ``It`` to be only an :cpp:concept:`input iterator <piranha::InputIterator>` (whereas
   the other overloads require a :cpp:concept:`forward iterator <piranha::ForwardIterator>` instead).

   Example:

   .. code-block:: c++

      int v[] = {7, 8, 9};
      auto code = k_encode<int>(v, 3);

   :param begin: an iterator to the start of the sequence to be encoded.
   :param size: the size of the sequence to be encoded.

   :return: the result of the codification of the input sequence.

   :exception unspecified: any exception thrown by :cpp:func:`piranha::safe_cast()` or by the public interface
      of :cpp:class:`piranha::k_encoder`.

.. cpp:function:: template <typename T, piranha::KEncodableForwardIterator<T> It> T piranha::k_encode(It begin, It end)

   Kronecker-encode a half-open forward iterator range.

   Example:

   .. code-block:: c++

      int v[] = {7, 8, 9};
      auto code = k_encode<int>(v, v + 3);

   :param begin: an iterator to the start of the sequence to be encoded.
   :param end: an iterator to the end of the sequence to be encoded.

   :return: the result of the codification of the input sequence.

   :exception unspecified: any exception thrown by :cpp:func:`piranha::safe_cast()`, ``std::distance()``,
      the public interface of :cpp:class:`piranha::k_encoder`, or the public interface of ``It``.

.. cpp:function:: template <typename T, piranha::KEncodableForwardRange<T> R> T piranha::k_encode(R &&r)

   Kronecker-encode a forward range.

   Example:

   .. code-block:: c++

      std::vector<long> v{1, 2, 3};
      auto code = k_encode<long>(v);

   :param r: the input range.

   :return: the result of the codification of the input range.

   :exception unspecified: any exception thrown by :cpp:func:`piranha::safe_cast()`, ``std::distance()``,
      the public interface of :cpp:class:`piranha::k_encoder`, or the public interface of the iterator type of ``R``.

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
