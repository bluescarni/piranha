Kronecker monomial
==================

*#include <piranha/kronecker_monomial.hpp>*

.. cpp:class:: template <piranha::UncvCppSignedIntegral T> piranha::kronecker_monomial

   .. cpp:type:: value_type = T

   .. cpp:function:: kronecker_monomial()

      Default constructor.

      The default constructor initialises a monomial with an internal value of zero.

   .. cpp:function:: template <piranha::KEncodableIterator<T> It> explicit kronecker_monomial(It begin, std::size_t size)
   .. cpp:function:: template <piranha::KEncodableForwardIterator<T> It> explicit kronecker_monomial(It begin, It end)
   .. cpp:function:: template <piranha::KEncodableForwardRange<T> R> explicit kronecker_monomial(R &&r)
   .. cpp:function:: template <piranha::SafelyConvertible<T> U> explicit kronecker_monomial(std::initializer_list<U> list)

      Constructors from ranges.

      These constructors will initialise a :cpp:class:`~piranha::kronecker_monomial` whose internal value will be
      computed from the Kronecker codification, via one of the overloads of :cpp:func:`piranha::k_encode()`, of the values
      pointed to by the input ranges.

      :exception unspecified: any exception thrown by the invoked :cpp:func:`piranha::k_encode()` overload.

   .. cpp:function:: explicit kronecker_monomial(const piranha::symbol_fset &)
