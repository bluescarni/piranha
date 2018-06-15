.. _safe_cast:

Safe cast
=========

*#include <piranha/safe_cast.hpp>*

.. cpp:function:: template <typename To, piranha::SafelyCastable<To> From> To piranha::safe_cast(From &&x)

   This function attempts to convert *x* to the type ``To`` via :cpp:func:`piranha::safe_convert()`.
   If the conversion succeeds (i.e., :cpp:func:`piranha::safe_convert()` returns ``true``), an instance
   of ``To`` representing exactly the value of *x* will be returned.
   Otherwise, an exception of type :cpp:class:`piranha::safe_cast_failure` will be raised.

   In other words, the body of this function is equivalent to:

   .. code-block:: c++

    To retval;
    if (piranha::safe_convert(retval, x)) {
        return retval;
    }
    throw piranha::safe_cast_failure(...);

   (where *x* is perfectly forwarded to :cpp:func:`piranha::safe_convert()`).

   :param x: the object to be converted.

   :return: *x* converted to the type ``To``.

   :exception piranha\:\:safe_cast_failure: if the call to :cpp:func:`piranha::safe_convert()` returned ``false``.
   :exception unspecified: any exception thrown by :cpp:func:`piranha::safe_convert()`, or by the construction
      of the return value.

Types
-----

.. cpp:class:: piranha::safe_cast_failure final : public std::invalid_argument

   This exception is raised by :cpp:func:`piranha::safe_cast()` upon a conversion failure.
   This class inherits all the public members of ``std::invalid_argument``, constructors
   included.

Concepts
--------

.. cpp:concept:: template <typename From, typename To> piranha::SafelyCastable

   This concept is satisfied if :cpp:func:`piranha::safe_cast()` can be used to convert
   an instance of type ``From`` to the type ``To``. Specifically, this concept will be
   satisfied if:

   * ``To`` is :cpp:concept:`default-constructible <piranha::DefaultConstructible>` and
     :cpp:concept:`returnable <piranha::Returnable>`, and
   * ``From`` is :cpp:concept:`safely-convertible <piranha::SafelyConvertible>` to the
     type ``To &``.

.. cpp:concept:: template <typename T, typename To> piranha::SafelyCastableInputIterator

   This concept is satisfied if ``T`` is an :cpp:concept:`input iterator <piranha::InputIterator>`
   whose reference type is :cpp:concept:`safely castable <piranha::SafelyCastable>` to ``To``.

   Specifically, given an input iterator lvalue ``it``, this concept is satisfied if the expression

   .. code-block:: c++

      piranha::safe_cast<To>(*it)

   is well-defined.

.. cpp:concept:: template <typename T, typename To> piranha::SafelyCastableForwardIterator

   This concept is satisfied if ``T`` is a :cpp:concept:`forward iterator <piranha::ForwardIterator>`
   whose reference type is :cpp:concept:`safely castable <piranha::SafelyCastable>` to ``To``.

   Specifically, given a forward iterator lvalue ``it``, this concept is satisfied if the expression

   .. code-block:: c++

      piranha::safe_cast<To>(*it)

   is well-defined.

.. cpp:concept:: template <typename T, typename To> piranha::SafelyCastableMutableForwardIterator

   This concept is satisfied if ``T`` is a :cpp:concept:`mutable forward iterator <piranha::MutableForwardIterator>`
   whose reference type is :cpp:concept:`safely castable <piranha::SafelyCastable>` to ``To``.

   Specifically, given a mutable forward iterator lvalue ``it``, this concept is satisfied if the expression

   .. code-block:: c++

      piranha::safe_cast<To>(*it)

   is well-defined.

.. cpp:concept:: template <typename T, typename To> piranha::SafelyCastableInputRange

   This concept is satisfied if ``T`` is an :cpp:concept:`input range <piranha::InputRange>`
   whose iterator type satisfies the :cpp:concept:`piranha::SafelyCastableInputIterator` concept.

.. cpp:concept:: template <typename T, typename To> piranha::SafelyCastableForwardRange

   This concept is satisfied if ``T`` is a :cpp:concept:`forward range <piranha::ForwardRange>`
   whose iterator type satisfies the :cpp:concept:`piranha::SafelyCastableForwardIterator` concept.

.. cpp:concept:: template <typename T, typename To> piranha::SafelyCastableMutableForwardRange

   This concept is satisfied if ``T`` is a :cpp:concept:`mutable forward range <piranha::MutableForwardRange>`
   whose iterator type satisfies the :cpp:concept:`piranha::SafelyCastableMutableForwardIterator` concept.
