Kronecker encoding/decoding
===========================

*#include <piranha/utils/kronecker_encdec.hpp>*

Kronecker encoding is a technique to map a vector of integers to a single integer scalar. Specifically, given
a vector :math:`\boldsymbol{n} = \left(n_0, n_1, n_2, \ldots \right)` of values :math:`n_i \in \mathbb{Z}`, Kronecker
encoding produces a value :math:`N \in \mathbb{Z}` via the scalar product

.. math::

   N = \boldsymbol{n} \cdot \boldsymbol{c},

where :math:`\boldsymbol{c}` is an *encoding vector* of positive integrals. Kronecker decoding is the inverse process
of determining :math:`\boldsymbol{n}` given a scalar :math:`N` and an encoding vector :math:`\boldsymbol{c}`.

Kronecker encoding can be seen as a generalisation of bit packing for signed values.

Classes
-------

.. cpp:class:: template <piranha::UncvCppSignedIntegral T> piranha::k_encoder

   Streaming Kronecker encoder.

   This class can be used to iteratively Kronecker-encode a sequence of signed integrals of type ``T``.

   Example:

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

.. cpp:class:: template <piranha::UncvCppSignedIntegral T> piranha::k_decoder

   Streaming Kronecker decoder.

   This class can be used to iteratively Kronecker-decode a signed integral of type ``T``.

   Example:

   .. code-block:: c++

      k_decoder<int> k(123, 3); // Prepare a k_decoder to decode the int 123 into a sequence of 3 values.
      int a, b, c;              // Prepare storage for the 3 decoded values.
      k >> a >> b >> c;         // Decode and write out the individual values via the '>>' operator.

   .. cpp:function:: explicit k_decoder(T n, std::size_t size)

      Constructor from value and sequence size.

      This constructor will create a :cpp:class:`~piranha::k_decoder` that can be used to decode the input value *n*
      into a sequence of a given *size*.

      :param n: the value to be decoded.
      :param size: the sequence size.

      :exception std\:\:overflow_error: if *size* is larger than an implementation-defined limit, or if *n* is outside
         an implementation-defined range.
      :exception std\:\:invalid_argument: if *size* is zero and *n* isn't (that is, only a value of zero can be decoded
         into a sequence of size zero).

   .. cpp:function:: k_decoder &operator>>(T &out)

      Decode and write to *out* the next value of the sequence.

      :param out: the object that will hold the decoded value.

      :return: a reference to *this*.

      :exception std\:\:out_of_range: if *size* values have already been decoded (where *size* is
         the second parameter used for the construction of this :cpp:class:`~piranha::k_decoder` object).

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

   :exception unspecified: any exception thrown by :cpp:func:`piranha::safe_cast()`, the public interface
      of :cpp:class:`piranha::k_encoder` or the public interface of ``It``.

.. cpp:function:: template <typename T, piranha::KEncodableForwardIterator<T> It> T piranha::k_encode(It begin, It end)

   Kronecker-encode a half-open iterator range.

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

   Kronecker-encode a range.

   Example:

   .. code-block:: c++

      std::vector<long> v{1, 2, 3};
      auto code = k_encode<long>(v);

   :param r: the input range.

   :return: the result of the codification of the input range.

   :exception unspecified: any exception thrown by :cpp:func:`piranha::safe_cast()`, ``std::distance()``,
      the public interface of :cpp:class:`piranha::k_encoder`, or the public interface of the iterator type of ``R``.

.. cpp:function:: template <piranha::UncvCppSignedIntegral T, piranha::OutputIterator<T> It> void piranha::k_decode(T n, It begin, std::size_t size)

   Decode a Kronecker code into a sequence of values of length ``size`` starting at ``begin``.

   Example:

   .. code-block:: c++

      int vec[3];
      k_decode(42, vec, 3);

   .. note::

      Due to the way output iterators are modelled, this function cannot safely cast the result of the decodification
      before writing to the iterator. It is thus the caller's responsibility to ensure that the decoded values can be written to the output
      iterators without overflow, loss of precision, etc.

   :param n: the code to be decoded.
   :param begin: an iterator to the start of the sequence that will hold the result of the decodification.
   :param size: the size of the sequence of values into which *n* will be decoded.

   :exception unspecified: any exception thrown by the public interface
      of :cpp:class:`piranha::k_decoder` or the public interface of ``It``.

.. cpp:function:: template <typename T, piranha::KDecodableForwardIterator<T> It> void piranha::k_decode(T n, It begin, It end)

   Decode a Kronecker code into a half-open iterator range.

   Example:

   .. code-block:: c++

      int vec[3];
      k_decode(42, vec, vec + 3);

   :param n: the code to be decoded.
   :param begin: an iterator to the start of the sequence that will hold the result of the decodification.
   :param end: an iterator to the end of the sequence that will hold the result of the decodification.

   :exception unspecified: any exception thrown by :cpp:func:`piranha::safe_cast()`, ``std::distance()``,
      the public interface of :cpp:class:`piranha::k_decoder`, or the public interface of ``It``.

.. cpp:function:: template <typename T, piranha::KDecodableForwardRange<T> R> void piranha::k_decode(T n, R &&r)

   Decode a Kronecker code into a range.

   Example:

   .. code-block:: c++

      std::vector<int> vec(3);
      k_decode(42, vec);

   :param n: the code to be decoded.
   :param range: the range that will hold the result of the decodification.

   :exception unspecified: any exception thrown by :cpp:func:`piranha::safe_cast()`, ``std::distance()``,
      the public interface of :cpp:class:`piranha::k_decoder`, or the public interface of the iterator type of ``R``.

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

.. cpp:concept:: template <typename It, typename T> piranha::KDecodableForwardIterator

   This concept is satisfied if ``It`` is an iterator that can be used as an output in the
   decodification of a Kronecker code of type ``T``.

   Specifically, this concept is satisfied if the following conditions hold:

   * ``It`` is a :cpp:concept:`piranha::MutableForwardIterator`,
   * ``T`` satisfies the :cpp:concept:`piranha::UncvCppSignedIntegral` concept,
   * ``T`` is :cpp:concept:`safely castable <piranha::SafelyCastable>` to the value type of ``It``,
   * the value type of ``It`` is move-assignable,
   * the difference type of ``It`` is :cpp:concept:`safely castable <piranha::SafelyCastable>`
     to ``std::size_t``.

.. cpp:concept:: template <typename R, typename T> piranha::KDecodableForwardRange

   This concept is satisfied if ``R`` is a :cpp:concept:`piranha::MutableForwardRange` whose iterator type
   satisfies the :cpp:concept:`piranha::KDecodableForwardIterator` concept for the signed integral type ``T``.
