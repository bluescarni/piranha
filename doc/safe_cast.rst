.. _safe_cast:

Safe cast
=========

*#include <piranha/safe_cast.hpp>*

.. cpp:function:: template <typename To> To piranha::safe_cast(piranha::SafelyCastable<To> &&x)

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
   This class inherits all the public members of ``std::invalid_argument`` (including its constructors).

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
