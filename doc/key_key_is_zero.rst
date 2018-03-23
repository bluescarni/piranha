.. _key_key_is_zero:

Detect zero keys
================

.. todo::

   Document key specialisations.

*#include <piranha/key/key_is_zero.hpp>*

.. cpp:function:: template <piranha::KeyIsZeroType T> bool piranha::key_is_zero(T &&x, const piranha::symbol_fset &s)

   This function returns ``true`` if the input key *x* is equal to zero, ``false`` otherwise.

   The implementation is delegated to the call operator of the :cpp:class:`piranha::key_is_zero_impl` function object.
   The body of this function is equivalent to:

   .. code-block:: c++

      return piranha::key_is_zero_impl<Tp>{}(x, s);

   where ``Tp`` is ``T`` after the removal of reference and cv-qualifiers,
   and *x* is perfectly forwarded to the call operator of :cpp:class:`piranha::key_is_zero_impl`.

   Piranha provides specialisations of :cpp:class:`piranha::key_is_zero_impl` for all the available key types.
   See the :ref:`implementation <key_key_is_zero_impls>` section below for more details about the available
   specialisations.

   :param x: the input key.
   :param s: the symbol set associated to *x*.

   :return: ``true`` if *x* is equal to zero, ``false`` otherwise.

   :exception unspecified: any exception thrown by the call operator of :cpp:class:`piranha::key_is_zero_impl`.

Concepts
--------

.. cpp:concept:: template <typename T> piranha::KeyIsZeroType

   This concept is satisfied if :cpp:func:`piranha::key_is_zero()` can be called
   with an argument of type ``T``. Specifically, this concept will be satisfied if

   .. code-block:: c++

      piranha::key_is_zero_impl<Tp>{}(std::declval<T>(), std::declval<const piranha::symbol_fset &>())

   (where ``Tp`` is ``T`` after the removal of reference and cv-qualifiers) is a valid expression whose
   type is :cpp:concept:`convertible <piranha::Convertible>` to ``bool``.

.. _key_key_is_zero_impls:

Implementations
---------------

.. cpp:class:: template <typename T> piranha::key_is_zero_impl

   Unspecialised version of the function object implementing :cpp:func:`piranha::key_is_zero()`.

   This default implementation returns unconditionally ``false``, regardless of the input arguments.
   In other words, keys by default are never considered to be equal to zero.
