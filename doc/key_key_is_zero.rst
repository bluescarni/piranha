.. _key_key_is_zero:

Detect zero keys
================

*#include <piranha/key/key_is_zero.hpp>*

.. cpp:function:: template <typename T> bool piranha::key_is_zero(const T &x, const piranha::symbol_fset &s)

   This function returns ``true`` if the input key *x* is equal to zero, ``false`` otherwise.

   The implementation is delegated to the call operator of the :cpp:class:`piranha::key_is_zero_impl` function object.
   The body of this function is equivalent to:

   .. code-block:: c++

      return piranha::key_is_zero_impl<T>{}(x, s);

   If the expression above is invalid, or if it returns a type which is not
   :cpp:concept:`~piranha::Convertible` to ``bool``,
   then this function will be disabled (i.e., it will not participate in overload resolution).

   Piranha provides specialisations of :cpp:class:`piranha::key_is_zero_impl` for all the available key types.
   See the :ref:`implementation <key_key_is_zero_impls>` section below for more details about the available
   specialisations.

   :param x: the input key.
   :param s: the :cpp:type:`piranha::symbol_fset` associated to *x*.

   :return: ``true`` if *x* is equal to zero, ``false`` otherwise.

   :exception unspecified: any exception thrown by the call operator of :cpp:class:`piranha::key_is_zero_impl`.

Concepts
--------

.. cpp:concept:: template <typename T> piranha::KeyIsZeroType

   This concept is satisfied if :cpp:func:`piranha::key_is_zero()` can be called
   with an argument of type ``T``. Specifically, this concept will be satisfied if

   .. code-block:: c++

      piranha::key_is_zero(x, s)

   is a valid expression, where ``x`` is a reference to const ``T`` and ``s`` a reference to const
   :cpp:type:`piranha::symbol_fset`.

.. _key_key_is_zero_impls:

Implementations
---------------

.. cpp:class:: template <typename T> piranha::key_is_zero_impl

   Unspecialised version of the function object implementing :cpp:func:`piranha::key_is_zero()`.

   This default implementation returns unconditionally ``false``, regardless of the input arguments.
   In other words, keys by default are never considered to be equal to zero.
