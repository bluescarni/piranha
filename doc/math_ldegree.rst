.. _math_ldegree:

Low degree
==========

.. todo::

   Document series specialisations.

*#include <piranha/math/ldegree.hpp>*

.. cpp:function:: template <piranha::LdegreeType T> auto piranha::ldegree(T &&x)
.. cpp:function:: template <piranha::LdegreeType T> auto piranha::ldegree(T &&x, const piranha::symbol_fset &s)

   These functions return respectively the total and partial low degree of *x*.

   What is meant exactly by *low degree* depends on the type of of *x*, but, generally speaking,
   this function is intended to return the *polynomial low degree* of a symbolic expression.
   For instance, for an expression such as

   .. math::

      3xy^2z - 2xyz + xz

   the total low degree should be 2 (i.e., the degree of the term with the lowest degree).
   The *partial* low degree is the low degree when only a subset of all
   symbols in an expression are considered. For the expression above, the partial
   low degree when only the symbols :math:`x` and :math:`y` are considered will be 1. The input
   argument *s* indicates which symbols need to be taken into account when computing the
   partial low degree.

   The implementations are delegated to the call operators of the :cpp:class:`piranha::ldegree_impl`
   function object. The bodies of these functions are equivalent respectively to

   .. code-block:: c++

      return piranha::ldegree_impl<Tp>{}(x);

   and

   .. code-block:: c++

      return piranha::ldegree_impl<Tp>{}(x, s);

   where ``Tp`` is ``T`` after the removal of reference and cv-qualifiers,
   and *x* is perfectly forwarded to the call operators of :cpp:class:`piranha::ldegree_impl`.

   See the :ref:`implementation <math_ldegree_impls>` section below for more details about the available
   specialisations.

   :param x: the input argument.
   :param s: the list of symbols that will be considered in the computation of the partial low degree.

   :return: the total or partial low degree of *x*.

   :exception unspecified: any exception thrown by the call operator of :cpp:class:`piranha::ldegree_impl`.

Concepts
--------

.. cpp:concept:: template <typename T> piranha::LdegreeType

   This concept is satisfied if both overloads of :cpp:func:`piranha::ldegree()` can be called
   with a first argument of type ``T``. Specifically, this concept will be satisfied if both

   .. code-block:: c++

      piranha::ldegree_impl<Tp>{}(std::declval<T>())

   and

   .. code-block:: c++

      piranha::ldegree_impl<Tp>{}(std::declval<T>(), std::declval<const piranha::symbol_fset &>())

   (where ``Tp`` is ``T`` after the removal of reference and cv-qualifiers) are valid expressions
   whose types are :cpp:concept:`returnable <piranha::Returnable>`.

.. _math_ldegree_impls:

Implementations
---------------

.. cpp:class:: template <typename T> piranha::ldegree_impl

   Unspecialised version of the function object implementing :cpp:func:`piranha::ldegree()`.

   This default implementation does not define any call operator, and thus no default implementation
   of :cpp:func:`piranha::ldegree()` is available.
