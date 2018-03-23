.. _key_key_ldegree:

Key low degree
==============

.. todo::

   Document key specialisations.

*#include <piranha/key/key_ldegree.hpp>*

.. cpp:function:: template <piranha::KeyLdegreeType T> auto piranha::key_ldegree(T &&x, const piranha::symbol_fset &s)
.. cpp:function:: template <piranha::KeyLdegreeType T> auto piranha::key_ldegree(T &&x, const piranha::symbol_idx_fset &idx, const piranha::symbol_fset &s)

   These functions return respectively the total and partial low degree of the input key *x*.

   What is meant exactly by *low degree* depends on the type of of *x*, but, generally speaking,
   this function is intended to return the *polynomial low degree* of a symbolic expression.
   For instance, for a monomial such as

   .. math::

      3xy^2z

   the total low degree should be 4. The *partial* low degree is the low degree when only a subset of all
   symbols in an expression are considered. For the expression above, the partial
   low degree when only the symbols :math:`x` and :math:`y` are considered will be 3. The input
   argument *idx* indicates the indices in *s* of the symbols that need to be taken into account
   when computing the partial low degree.

   The implementations are delegated to the call operators of the :cpp:class:`piranha::key_ldegree_impl`
   function object. The bodies of these functions are equivalent respectively to

   .. code-block:: c++

      return piranha::key_ldegree_impl<Tp>{}(x, s);

   and

   .. code-block:: c++

      return piranha::key_ldegree_impl<Tp>{}(x, idx, s);

   where ``Tp`` is ``T`` after the removal of reference and cv-qualifiers,
   and *x* is perfectly forwarded to the call operators of :cpp:class:`piranha::key_ldegree_impl`.

   Piranha provides specialisations of :cpp:class:`piranha::key_ldegree_impl` for some of the available key types.
   See the :ref:`implementation <key_key_ldegree_impls>` section below for more details about the available
   specialisations.

   :param x: the input key.
   :param idx: the indices in *s* of the symbols that will be considered in the computation of the partial low degree.
   :param s: the symbol set associated to *x*.

   :return: the total or partial low degree of *x*.

   :exception unspecified: any exception thrown by the call operator of :cpp:class:`piranha::key_ldegree_impl`.

Concepts
--------

.. cpp:concept:: template <typename T> piranha::KeyLdegreeType

   This concept is satisfied if both overloads of :cpp:func:`piranha::key_ldegree()` can be called
   with a first argument of type ``T``. Specifically, this concept will be satisfied if both

   .. code-block:: c++

      piranha::key_ldegree_impl<Tp>{}(std::declval<T>(), std::declval<const piranha::symbol_fset &>())

   and

   .. code-block:: c++

      piranha::key_ldegree_impl<Tp>{}(std::declval<T>(), std::declval<const piranha::symbol_idx_fset &>(),
                                      std::declval<const piranha::symbol_fset &>())

   (where ``Tp`` is ``T`` after the removal of reference and cv-qualifiers) are valid expressions
   whose types are :cpp:concept:`returnable <piranha::Returnable>`.

.. _key_key_ldegree_impls:

Implementations
---------------

.. cpp:class:: template <typename T> piranha::key_ldegree_impl

   Unspecialised version of the function object implementing :cpp:func:`piranha::key_ldegree()`.

   This default implementation does not define any call operator, and thus no default implementation
   of :cpp:func:`piranha::key_ldegree()` is available.
